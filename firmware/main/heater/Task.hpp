#pragma once

#include <etk/io/Pin.hpp>
#include <xf/task/task.hpp>

#include "command.hpp"

namespace heater {

struct Idling {
};

struct Heating {
    int target_celsius;
    xf::time::Tick start;
    xf::time::Tick finish;

    float counter { 0.0f };
};

class Task : public xf::task::StaticTask<Task, 3192> {
    XF_TASK;

    void run_impl();

public:
    Task(command::Queue&);

private:
    void handle_command(HeaterControl);

    command::Queue& m_command_queue;

    std::variant<Idling, Heating> m_state;
};

}
