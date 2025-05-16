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
        // This is a pointer instead of reference to not implicitly-delete the std::variant's default copy ctor and allow state transitions
        const relays::PhaseGroup* phase_group;
    };

    struct HeatingStage : Stage {
        pid::Controller pid;
    };

    std::variant<PreHeatingStage, HeatingStage> stage;
    xf::time::Tick start;

    xf::time::Tick last_report {};

    HeaterControl_Start parameters;
};

using State = std::variant<state::Idling, state::Heating>;

}

class Heater final : public xf::task::StaticTask<4096> {
    void setup() override;

    void run() override;

public:
    Heater(command::Queue&, etk::i2c::Master&);

private:
    void handle_command(const HeaterControl&);

    void disable_relays();

    command::Queue& m_command_queue;

    state::State m_state;

    TemperatureSensor m_temperature_sensor;

    struct LastHeatingInfo {
        float last_temperature { 0.0f };
        float last_integral { 0.0f };
    };

    std::optional<LastHeatingInfo> m_last_heating_info;
};

}
