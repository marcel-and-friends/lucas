#pragma once

#include <array>
#include <cstddef>
#include <numeric>

#include <xf/time/time.hpp>

#include <util/literals.hpp>
#include <util/time.hpp>

namespace heater::relays {

enum ActiveRelays {
    Neither = 1 << 0,
    Weak = 1 << 1,
    Strong = 1 << 2
};

struct State {
    int watts;
    ActiveRelays active_relays;
};

constexpr int TOTAL_WATTAGE = 3400;
constexpr int WEAK_RELAY_WATTAGE = TOTAL_WATTAGE * 0.3f;
constexpr int STRONG_RELAY_WATTAGE = TOTAL_WATTAGE * 0.7f;

constexpr auto STATE_PERMUTATIONS = std::array {
    State { 0, ActiveRelays::Neither },
    State { WEAK_RELAY_WATTAGE, ActiveRelays::Weak },
    State { STRONG_RELAY_WATTAGE, ActiveRelays::Strong },
    State { WEAK_RELAY_WATTAGE + STRONG_RELAY_WATTAGE, static_cast<ActiveRelays>(ActiveRelays::Weak | ActiveRelays::Strong) },
};

constexpr auto PHASE_GROUP_SIZE = 5;

struct PhaseGroup {
    int average_watts;
    std::array<State, PHASE_GROUP_SIZE> states;
};

constexpr auto PHASE_GROUP_TABLE_SIZE = PHASE_GROUP_SIZE * (STATE_PERMUTATIONS.size() - 1) + 1;
using PhaseGroupTable = std::array<PhaseGroup, PHASE_GROUP_TABLE_SIZE>;

consteval PhaseGroupTable generate_phase_group_table() {
    PhaseGroupTable table {};

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
constexpr auto PHASE_GROUP_TABLE = generate_phase_group_table();

constexpr const PhaseGroup& find_best_phase_group_for_watts(int watts) {
    if (watts >= PHASE_GROUP_TABLE.back().average_watts)
        return PHASE_GROUP_TABLE.back();

    if (watts <= PHASE_GROUP_TABLE.front().average_watts)
        return PHASE_GROUP_TABLE.front();

    auto found = std::lower_bound(PHASE_GROUP_TABLE.begin(), PHASE_GROUP_TABLE.end(), watts, [](const auto& row, auto v) {
        return row.average_watts < v;
    });

    auto previous = found - 1;

    auto previous_delta = std::abs(previous->average_watts - watts);
    auto found_delta = std::abs(found->average_watts - watts);

    return previous_delta < found_delta ? *previous : *found;
}

class Controller {
public:
    bool needs_new_phase_group() const {
        return m_current_state == 0;
    }

    void set_phase_group(const PhaseGroup& phase_group) {
        m_phase_group = &phase_group;
    }

    State next_phase_group_state() {
        auto state = m_phase_group->states[m_current_state];
        m_current_state = (m_current_state + 1) % PHASE_GROUP_SIZE;
        return state;
    }

private:
    size_t m_current_state { 0 };
    const PhaseGroup* m_phase_group { nullptr };
};

}
