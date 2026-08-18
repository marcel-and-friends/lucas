#pragma once

#include <etk/i2c/Master.hpp>
#include <xf/task/StaticTask.hpp>

#include <proto/firmware/Alarm.pb.h>

#include "TemperatureSensor.hpp"
#include "command.hpp"
#include "pid.hpp"
#include "relays.hpp"

namespace heater {

namespace state {

struct Idling { };

// Holds the element at the dry-safe ceiling between pours so the preheat all but disappears.
struct Standby {
    xf::time::Tick last_report {};
    bool element_on { false };
};

struct Heating {
    struct Stage {
        xf::time::Tick start;
        xf::time::Tick deadline;

        relays::Controller relays_controller {};
        xf::time::Tick last_phase_group_state_change {};
        bool first_loop { true };
    };

    struct PreHeatingStage : Stage {
        relays::ControlData control_data;

        // Tracked to project where the body temperature is headed — a cold element at full power
        // climbs tens of degrees per second, so decisions based on the instantaneous reading
        // always come too late.
        float previous_temperature { -1.0f };
        float slope_per_second { 0.0f };
    };

    struct HeatingStage : Stage {
        pid::Controller pid;

        // Lightly low-passed reading fed to the PID so it stops chasing flow-wave noise; raw
        // readings still drive the safety checks and reports.
        float filtered_temperature { -1.0f };

        // Last commanded wattage, for slew limiting.
        int previous_watts { -1 };
    };

    std::variant<PreHeatingStage, HeatingStage> stage;

    xf::time::Tick start;

    HeaterControl_Start parameters;

    struct ControlInfo {
        relays::ControlData control_data;
        std::optional<float> pid;
    } active_control_info {};

    xf::time::Tick last_report {};
};

using State = std::variant<state::Idling, state::Standby, state::Heating>;

}

class Heater final : public xf::task::StaticTask<8192> {
    void setup() override;

    void run() override;

public:
    Heater(command::Queue&, etk::i2c::Master&);

private:
    void handle_command(const HeaterControl&);

    void control_heater(state::Heating&, state::Heating::Stage&);

    void run_standby(state::Standby&);

    enum class DelayWaterRelayDisable : uint8_t {
        Yes,
        No
    };
    void disable_relays(DelayWaterRelayDisable);

    void raise_alarm(AlarmCode);

    float initial_integral(float temperature);

    command::Queue& m_command_queue;

    state::State m_state;

    TemperatureSensor m_temperature_sensor;

    int m_consecutive_sensor_failures { 0 };

    struct LastHeatingInfo {
        float ending_temperature { 0.0f };
        float ending_integral { 0.0f };

        float reuse_integral(float temperature) {
            float delta = ending_temperature - temperature;
            if (delta >= 40.0f)
                return 0.0f;
            float ratio = (temperature / ending_temperature);
            return ending_integral * ratio;
        }
    };

    std::optional<LastHeatingInfo> m_last_heating_info;
};

}
