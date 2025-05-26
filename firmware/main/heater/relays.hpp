#pragma once

#include <array>
#include <cstddef>
#include <numeric>

#include <xf/time/time.hpp>

#include <util/literals.hpp>
#include <util/time.hpp>

namespace heater::relays {

enum ActiveRelays : uint8_t {
    Neither = 1 << 0,
    Weak = 1 << 1,
    Strong = 1 << 2
};

struct State {
    int16_t watts;
    ActiveRelays active_relays;
};

constexpr int MAX_WATTS = 3400;
constexpr int WEAK_RELAY_WATTS = MAX_WATTS * 0.3f;
constexpr int STRONG_RELAY_WATTS = MAX_WATTS * 0.7f;

constexpr auto STATE_PERMUTATIONS = std::array {
    State { 0, ActiveRelays::Neither },
    State { WEAK_RELAY_WATTS, ActiveRelays::Weak },
    State { STRONG_RELAY_WATTS, ActiveRelays::Strong },
    State { WEAK_RELAY_WATTS + STRONG_RELAY_WATTS, static_cast<ActiveRelays>(ActiveRelays::Weak | ActiveRelays::Strong) },
};

constexpr auto PHASE_GROUP_SIZE = 10;

struct ControlData {
    int average_watts;
    std::array<State, PHASE_GROUP_SIZE> states;
};

constexpr auto PHASE_GROUP_TABLE_SIZE = PHASE_GROUP_SIZE * (STATE_PERMUTATIONS.size() - 1) + 1;
using ControlDataTable = std::array<ControlData, PHASE_GROUP_TABLE_SIZE>;

consteval ControlDataTable generate_control_data_table() {
    ControlDataTable table {};

    for (size_t r = 1; r < table.size(); ++r) {
        const size_t row = r - 1;

        for (size_t c = 0; c <= row % PHASE_GROUP_SIZE; ++c)
            table[r].states[c] = STATE_PERMUTATIONS[(row + PHASE_GROUP_SIZE) / PHASE_GROUP_SIZE];

        for (size_t c = (row % PHASE_GROUP_SIZE) + 1; c < PHASE_GROUP_SIZE; ++c)
            table[r].states[c] = STATE_PERMUTATIONS[row / PHASE_GROUP_SIZE];

        auto total_watts = std::accumulate(table[r].states.begin(), table[r].states.end(), 0, [](auto acc, auto info) {
            return acc + info.watts;
        });

        table[r].average_watts = total_watts / PHASE_GROUP_SIZE;
    }

    return table;
}

constexpr auto AC_PHASE_INTERVAL = chrono::round<xf::time::Duration>(FloatSeconds { 1 } / 60);
constexpr auto PID_DELTA_TIME = chrono::duration_cast<FloatSeconds>(AC_PHASE_INTERVAL) * PHASE_GROUP_SIZE;
constexpr auto CONTROL_DATA_TABLE = generate_control_data_table();

constexpr const ControlData& find_best_control_data_for_watts(int watts) {
    if (watts >= CONTROL_DATA_TABLE.back().average_watts)
        return CONTROL_DATA_TABLE.back();

    if (watts <= CONTROL_DATA_TABLE.front().average_watts)
        return CONTROL_DATA_TABLE.front();

    auto found = std::lower_bound(CONTROL_DATA_TABLE.begin(), CONTROL_DATA_TABLE.end(), watts, [](const auto& row, auto v) {
        return row.average_watts < v;
    });

    auto previous = found - 1;

    auto previous_delta = std::abs(previous->average_watts - watts);
    auto found_delta = std::abs(found->average_watts - watts);

    return previous_delta < found_delta ? *previous : *found;
}

class Controller {
public:
    bool needs_new_control_data() const {
        return m_current_state == 0;
    }

    State next_control_state() {
        auto state = m_control_data->states[m_current_state];
        m_current_state = (m_current_state + 1) % m_control_data->states.size();
        return state;
    }

    void set_control_data(const ControlData& control_data) {
        m_control_data = &control_data;
    }

private:
    size_t m_current_state { 0 };
    const ControlData* m_control_data { nullptr };
};

}
