#include <wxs/match.hpp>

#include "Task.hpp"
#include <maestro/Maestro.hpp>
#include <util/literals.hpp>
#include <util/log.hpp>

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
            [&](Heating& state) {
                static constexpr xf::time::Duration REPORT_INTERVAL = 500ms;

                auto report_start = xf::time::now();
                auto timestamp = report_start.time_since_epoch().count();

                maestro::send_event({
                    .which_tag = FirmwareEvent_heating_report_tag,
                    .heating_report = {
                        .which_tag = HeatingReport_progress_tag,
                        .progress = {
                            .celsius = state.counter,
                            .timestamp = timestamp,
                        },
                    },
                });

                if (report_start >= state.finish) {
                    maestro::send_event({
                        .which_tag = FirmwareEvent_heating_report_tag,
                        .heating_report = {
                            .which_tag = HeatingReport_finished_tag,
                            .finished = {
                                .celsius = state.counter,
                                .timestamp = timestamp,
                            },
                        },
                    });

                    m_state = Idling {};
                    return;
                }

                if (state.counter < state.target_celsius)
                    state.counter += 5.f;

                auto report_end = xf::time::now();
                auto report_duration = report_end - report_start;
                auto time_until_finish = state.finish <= report_end ? xf::time::Duration {} : state.finish - report_end;

                if (auto command = m_command_queue.receive(std::min(REPORT_INTERVAL - report_duration, time_until_finish))) {
                    handle_command(*command);
                    return;
                }
            });
    }
}

void Task::handle_command(HeaterControl command) {
    switch (command.which_tag) {
    case HeaterControl_start_tag: {
        auto start = xf::time::now();

        m_state = Heating {
            .target_celsius = command.start.target_celsius,
            .start = start,
            .finish = start + xf::time::Duration { command.start.duration },
        };
    } break;
    case HeaterControl_stop_tag:
        m_state = Idling {};
        LOGI("Heater", "Stopped heating");
        break;
    };
}
}
