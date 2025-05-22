#pragma once

#include <array>
#include <cstddef>

#include <xf/time/time.hpp>

#include <util/literals.hpp>
#include <util/time.hpp>

namespace heater::relays {

constexpr size_t PHASE_GROUP_SIZE = 10;

using RelayStateGroup = uint32_t;

struct ControlData {
    RelayStateGroup relays_state_group;
    int average_watts;
};

using ControlTable = std::array<ControlData, 25>;

constexpr ControlTable CONTROL_TABLE = { {
    { 0b00'00'00'00'00'00'00'00'00'00, 0 },
    { 0b10'00'00'00'00'00'00'00'00'00, 102 },
    { 0b10'00'10'00'00'00'00'00'00'00, 204 },
    { 0b10'00'10'00'10'00'00'00'00'00, 306 },
    { 0b10'00'10'00'10'00'10'00'00'00, 408 },
    { 0b10'00'10'00'10'00'10'00'10'00, 510 },
    { 0b10'10'10'00'10'00'10'00'10'00, 612 },
    { 0b10'10'10'10'10'00'10'00'10'00, 714 },
    { 0b10'10'10'10'10'10'10'00'10'00, 816 },
    { 0b10'10'10'10'10'10'10'10'10'00, 918 },
    { 0b10'10'10'10'10'10'10'10'10'10, 1020 },
    { 0b01'00'01'00'01'00'01'00'01'00, 1190 },
    { 0b01'10'01'10'01'10'01'10'01'10, 1700 },
    { 0b11'10'11'10'11'10'11'10'11'10, 2210 },
    { 0b01'01'01'01'01'01'01'01'01'01, 2380 },
    { 0b11'01'01'01'01'01'01'01'01'01, 2482 },
    { 0b11'01'11'01'01'01'01'01'01'01, 2584 },
    { 0b11'01'11'01'11'01'01'01'01'01, 2686 },
    { 0b11'01'11'01'11'01'11'01'01'01, 2788 },
    { 0b11'01'11'01'11'01'11'01'11'01, 2890 },
    { 0b11'11'11'01'11'01'11'01'11'01, 2992 },
    { 0b11'11'11'11'11'01'11'01'11'01, 3094 },
    { 0b11'11'11'11'11'11'11'01'11'01, 3196 },
    { 0b11'11'11'11'11'11'11'11'11'01, 3298 },
    { 0b11'11'11'11'11'11'11'11'11'11, 3400 },
} };

constexpr auto MAX_WATTS = CONTROL_TABLE.back().average_watts;
constexpr auto AC_PHASE_INTERVAL = chrono::round<xf::time::Duration>(FloatSeconds { 1 } / 60);
constexpr auto PID_DELTA_TIME = chrono::duration_cast<FloatSeconds>(AC_PHASE_INTERVAL) * PHASE_GROUP_SIZE;

constexpr ControlData find_best_control_data_for_watts(int watts) {
    if (watts >= CONTROL_TABLE.back().average_watts)
        return CONTROL_TABLE.back();

    if (watts <= CONTROL_TABLE.front().average_watts)
        return CONTROL_TABLE.front();

    auto found = std::lower_bound(CONTROL_TABLE.begin(), CONTROL_TABLE.end(), watts, [](const auto& row, auto v) {
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
        return m_phase_group_index == 0;
    }

    void set_relay_state_group(RelayStateGroup relay_state_group) {
        m_relay_state_group = relay_state_group;
    }

    struct RelayState {
        bool strong;
        bool weak;
    };

    RelayState next_relay_state() {
        bool strong = m_relay_state_group & (1 << m_phase_group_index * 2);
        bool weak = m_relay_state_group & (1 << (m_phase_group_index * 2 + 1));

        RelayState relay_state = {
            .strong = strong,
            .weak = weak,
        };

        m_phase_group_index = (m_phase_group_index + 1) % PHASE_GROUP_SIZE;

        return relay_state;
    }

private:
    size_t m_phase_group_index { 0 };
    RelayStateGroup m_relay_state_group;
};

}
