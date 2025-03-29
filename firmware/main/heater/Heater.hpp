#include <array>
#include <atomic>
#include <numeric>

#include <etk/io/Pin.hpp>
#include <xf/task/task.hpp>

namespace heater {
namespace chrono = std::chrono;

using FloatSeconds = chrono::duration<float>;

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

class Executor : public xf::task::StaticTask<Executor, 3192>,
                 public xf::task::Notifiable<xf::task::BinaryNotifier> {
    XF_TASK;

    void run_impl();

public:
    explicit Executor()
        : Notifiable(*this) {
    }

    static void setup_io();

    void stop_executing() {
        m_stop.store(true, std::memory_order::relaxed);
    }

private:
    void run_preheating();

    template<xf::util::Fn<const TableEntry*()> FN>
    void relay_control_loop(FN&& entry_picker) {
        m_stop.store(false, std::memory_order::relaxed);

        const TableEntry* table_entry = nullptr;
        uint8_t phase_group_step = 0;
        every(AC_PHASE_CYCLE, [&, entry_picker = std::forward<FN>(entry_picker)] {
            if (m_stop.load(std::memory_order::relaxed))
                return xf::util::ControlFlow::Break;

            if (phase_group_step == 0) {
                table_entry = entry_picker();
                if (table_entry == nullptr)
                    return xf::util::ControlFlow::Break;
            }

            const auto& phase_info = table_entry->phase_group[phase_group_step];
            RELAYS[0].set(phase_info.relay_states & PhaseInfo::WeakElement);
            RELAYS[1].set(phase_info.relay_states & PhaseInfo::StrongElement);

            phase_group_step = (phase_group_step + 1) % PHASE_GROUP_SIZE;

            return xf::util::ControlFlow::Continue;
        });
    }

    template<xf::util::Fn<const TableEntry*()> FN, typename Rep, typename Ratio>
    void relay_control_loop_for(chrono::duration<Rep, Ratio> duration, FN&& entry_picker) {
        const auto begin = xf::time::now();
        relay_control_loop([&, entry_picker = std::forward<FN>(entry_picker)] -> const TableEntry* {
            if (xf::time::now() - begin >= duration)
                return nullptr;

            return entry_picker();
        });
    }

    static constexpr auto RELAYS = std::array {
        etk::io::Output { GPIO_NUM_0 },
        etk::io::Output { GPIO_NUM_10, etk::io::Invert::Yes }
    };

    float m_previous_integral { 45.0f };

    std::atomic<bool> m_stop { false };
};
}
