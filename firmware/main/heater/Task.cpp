#include <numeric>

#include <wxs/match.hpp>

#include "Task.hpp"
#include <maestro/Maestro.hpp>
#include <util/literals.hpp>
#include <util/log.hpp>

namespace heater {

using FloatSeconds = std::chrono::duration<float>;

struct PhaseInfo {
    enum State {
        Off = 1 << 0,
        WeakElement = 1 << 1,
        StrongElement = 1 << 2
    };
    int watts;
    int relay_states;
};

constexpr auto MAX_WATTAGE = 3400;

constexpr auto PHASE_STATES = std::to_array<PhaseInfo>({
    PhaseInfo { 0, PhaseInfo::Off },
    PhaseInfo { int(MAX_WATTAGE * 0.3f), PhaseInfo::WeakElement },
    PhaseInfo { int(MAX_WATTAGE * 0.7f), PhaseInfo::StrongElement },
    PhaseInfo { MAX_WATTAGE, PhaseInfo::WeakElement | PhaseInfo::StrongElement },
});

constexpr auto PHASE_GROUP_SIZE = 5;
using PhaseGroup = std::array<PhaseInfo, PHASE_GROUP_SIZE>;

struct TableEntry {
    int average_watts;
    PhaseGroup phase_group;
};

constexpr auto PHASE_GROUP_TABLE_SIZE = PHASE_GROUP_SIZE * (PHASE_STATES.size() - 1) + 1;
using PhaseGroupTable = std::array<TableEntry, PHASE_GROUP_TABLE_SIZE>;

static consteval PhaseGroupTable generate_phase_group_table() {
    PhaseGroupTable table {};

    for (size_t r = 1; r < table.size(); ++r) {
        const size_t row = r - 1;

        for (size_t c = 0; c <= row % PHASE_GROUP_SIZE; ++c)
            table[r].phase_group[c] = PHASE_STATES[(row + PHASE_GROUP_SIZE) / PHASE_GROUP_SIZE];

        for (size_t c = (row % PHASE_GROUP_SIZE) + 1; c < PHASE_GROUP_SIZE; ++c)
            table[r].phase_group[c] = PHASE_STATES[row / PHASE_GROUP_SIZE];

        auto group_wattage = std::accumulate(table[r].phase_group.begin(), table[r].phase_group.end(), 0, [](auto acc, auto info) {
            return acc + info.watts;
        });

        table[r].average_watts = group_wattage / PHASE_GROUP_SIZE;
    }

    return table;
}

static constexpr auto AC_PHASE_CYCLE = FloatSeconds { 1 } / 60;
static constexpr auto PID_DELTA_TIME = AC_PHASE_CYCLE * PHASE_GROUP_SIZE;
static constexpr auto PHASE_GROUP_TABLE = generate_phase_group_table();

static const TableEntry* find_closest_table_entry(float watts) {
    if (watts >= PHASE_GROUP_TABLE.back().average_watts)
        return &PHASE_GROUP_TABLE.back();

    if (watts <= PHASE_GROUP_TABLE.front().average_watts)
        return &PHASE_GROUP_TABLE.front();

    const auto found = std::lower_bound(PHASE_GROUP_TABLE.begin(), PHASE_GROUP_TABLE.end(), watts, [](const auto& row, auto v) {
        return row.average_watts < v;
    });

    auto previous = found - 1;

    auto previous_delta = std::abs(previous->average_watts - watts);
    auto found_delta = std::abs(found->average_watts - watts);

    return previous_delta < found_delta ? previous : found;
}

Task::Task(command::Queue& command_queue)
    : m_command_queue(command_queue)
    , m_pid_constants({
          .Kp = 1.0f,
          .Ki = 0.1f,
          .Kd = 0.1f,
          .Dt = PID_DELTA_TIME.count(),
      }) {
}

void Task::run_impl() {
    static constexpr xf::time::Duration REPORT_INTERVAL = 250ms;

    while (true) {
        wxs::match(
            m_state,
            [&](state::Idling) {
                auto command = m_command_queue.await_receive();
                handle_command(command);
            },
            [&](state::PreHeating& state) {
                auto start = xf::time::now();
                bool finished = start >= state.deadline;

                if (start - state.last_report >= REPORT_INTERVAL) {
                    state.last_report = start;

                    float seconds_elapsed = std::chrono::duration_cast<FloatSeconds>(start - state.start).count();
                    maestro::send_event({
                        .which_tag = FirmwareEvent_heating_report_tag,
                        .heating_report = {
                            .celsius = 1.0f,
                            .seconds_elapsed = seconds_elapsed,
                            .finished = false,
                        },
                    });
                }

                if (finished) {
                    m_state = state::Heating {
                        .target_celsius = state.target_celsius,
                        .start = start,
                        .deadline = start + state.heating_duration,
                    };
                    return;
                }

                auto end = xf::time::now();
                auto duration = end - start;
                auto time_until_deadline = end >= state.deadline ? xf::time::Duration {} : state.deadline - end;
                auto next_phase_cycle = std::chrono::duration_cast<xf::time::Duration>(AC_PHASE_CYCLE - duration);

                if (auto command = m_command_queue.receive(std::min(next_phase_cycle, time_until_deadline))) {
                    handle_command(*command);
                    return;
                }
            },
            [&](state::Heating& state) {
                auto report_start = xf::time::now();
                float seconds_elapsed = std::chrono::duration_cast<FloatSeconds>(report_start - state.start).count();
                bool finished = report_start >= state.deadline;

                maestro::send_event({
                    .which_tag = FirmwareEvent_heating_report_tag,
                    .heating_report = {
                        .celsius = state.counter,
                        .seconds_elapsed = seconds_elapsed,
                        .finished = finished,
                    },
                });

                if (finished) {
                    m_state = state::Idling {};
                    return;
                }

                if (state.counter < state.target_celsius)
                    state.counter += 5.0f;

                auto report_end = xf::time::now();
                auto report_duration = report_end - report_start;
                auto time_until_finish = report_end >= state.deadline ? xf::time::Duration {} : state.deadline - report_end;

                if (auto command = m_command_queue.receive(std::min(REPORT_INTERVAL - report_duration, time_until_finish))) {
                    handle_command(*command);
                    return;
                }
            });
    }
}

void Task::handle_command(const HeaterControl& command) {
    static constexpr xf::time::Duration REPORT_INTERVAL = 250ms;
    static constexpr xf::time::Duration PREHEAT_DURATION = 1s;

    switch (command.which_tag) {
    case HeaterControl_start_tag: {
        auto start = xf::time::now();

        m_state = state::PreHeating {
            .initial_temperature = 0.0f,
            .heating_duration = xf::time::Duration { command.start.duration_ms },
            .target_celsius = command.start.target_celsius,
            .start = start,
            .deadline = start + PREHEAT_DURATION,
        };
    } break;
    case HeaterControl_stop_tag:
        m_state = state::Idling {};
        LOGI("Heater", "Stopped heating");
        break;
    };
}
}
