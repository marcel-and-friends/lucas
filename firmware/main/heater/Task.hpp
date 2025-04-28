#pragma once

#include <numeric>

#include <etk/io/Pin.hpp>
#include <xf/task/task.hpp>

#include "TemperatureSensor.hpp"
#include "command.hpp"
#include "pid.hpp"
#include "relays.hpp"

namespace heater {

namespace state {

struct Idling {
};

struct PreHeating {
    int target_temperature;
    xf::time::Tick start;
    xf::time::Tick deadline;

    relays::Controller relays_controller {};
    xf::time::Tick last_report {};
};

struct Heating {
    struct PreHeatingStage {
        relays::PhaseGroup phase_group;
    };

    struct HeatingStage {
        pid::Controller pid;
    };

    std::variant<PreHeatingStage, HeatingStage> stage;
    int target_temperature;
    xf::time::Duration duration;
    xf::time::Tick start;
    xf::time::Tick deadline;

    relays::Controller relays_controller {};
    xf::time::Tick last_report {};
    xf::time::Tick last_phase_group_state_change {};
};

}

class Task : public xf::task::StaticTask<Task, 3192> {
    XF_TASK;

    void run_impl();

public:
    Task(command::Queue&);

private:
    void handle_command(const HeaterControl&);

    float read_temperature() const;

    command::Queue& m_command_queue;

    std::variant<state::Idling, state::Heating> m_state;

    pid::Constants m_pid_constants;

    TemperatureSensor m_temperature_sensor;
};

}
