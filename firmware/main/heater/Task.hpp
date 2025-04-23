#pragma once

#include <etk/io/Pin.hpp>
#include <xf/task/task.hpp>

#include "command.hpp"
#include "pid.hpp"

namespace heater {

namespace state {

struct Idling {
};

struct PreHeating {
    float initial_temperature;
    xf::time::Duration heating_duration;

    int target_celsius;
    xf::time::Tick start;
    xf::time::Tick deadline;

    xf::time::Tick last_report {};
};

struct Heating {
    int target_celsius;
    xf::time::Tick start;
    xf::time::Tick deadline;

    float counter { 0.0f };
};

}

class Task : public xf::task::StaticTask<Task, 3192> {
    XF_TASK;

    void run_impl();

public:
    Task(command::Queue&);

private:
    void handle_command(const HeaterControl&);

    command::Queue& m_command_queue;

    std::variant<state::Idling, state::PreHeating, state::Heating> m_state;

    pid::Constants m_pid_constants;
};

}
