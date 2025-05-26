#pragma once

#include <etk/i2c/Master.hpp>
#include <xf/task/StaticTask.hpp>

#include "TemperatureSensor.hpp"
#include "command.hpp"
#include "pid.hpp"
#include "relays.hpp"

namespace heater {

namespace state {

struct Idling { };

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
    };

    struct HeatingStage : Stage {
        pid::Controller pid;
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

using State = std::variant<state::Idling, state::Heating>;

}

class Heater final : public xf::task::StaticTask<8192> {
    void setup() override;

    void run() override;

public:
    Heater(command::Queue&, etk::i2c::Master&);

private:
    void handle_command(const HeaterControl&);

    void control_heater(state::Heating&, state::Heating::Stage&);

    enum class DelayWaterRelayDisable : uint8_t {
        Yes,
        No
    };
    void disable_relays(DelayWaterRelayDisable);

    float initial_integral(float temperature);

    command::Queue& m_command_queue;

    state::State m_state;

    TemperatureSensor m_temperature_sensor;

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
