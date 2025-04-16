#include <wxs/match.hpp>

#include "Task.hpp"
#include <maestro/Maestro.hpp>
#include <util/literals.hpp>

namespace heater {

Task::Task(command::Queue& command_queue)
    : m_command_queue(command_queue) {
}

void Task::run_impl() {
    while (true) {
        wxs::match(
            m_state,
            [&](Idling) {
                auto command = m_command_queue.await_receive();
                handle_command(command);
            },
            [&](Heating& heating) {
                maestro::send_event({
                    .which_tag = FirmwareEvent_temperature_report_tag,
                    .temperature_report = {
                        .celsius = heating.counter += 5.0f,
                    },
                });

                if (auto command = m_command_queue.receive(1s)) {
                    handle_command(*command);
                    return;
                }
            });
    }
}

void Task::handle_command(HeaterControl command) {
    switch (command.which_tag) {
    case HeaterControl_start_tag:
        m_state = Heating {
            .target_celsius = command.start.target_celsius,
        };
        break;
    case HeaterControl_stop_tag:
        m_state = Idling {};
        break;
    };
}
}
