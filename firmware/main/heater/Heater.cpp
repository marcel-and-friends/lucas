#include <cmath>

#include <esp_adc/adc_continuous.h>

#include <etk/gpio/Pin.hpp>
#include <wxs/match.hpp>

#include "Heater.hpp"
#include <maestro/Maestro.hpp>
#include <util/literals.hpp>
#include <util/log.hpp>

namespace heater {

static auto RELAYS = std::array {
    // Onboard LED connection on the devkit
    etk::gpio::Output { GPIO_NUM_2 },
};

static constexpr int percent_to_watts(float percent) {
    return std::lround(std::clamp(percent, 0.0f, 100.0f) * relays::TOTAL_WATTAGE / 100.0f);
}

Heater::Heater(command::Queue& command_queue, etk::i2c::Master& i2c_master)
    : m_command_queue(command_queue)
    , m_pid_constants({
          .Kp = 1.0f,
          .Ki = 0.1f,
          .Kd = 0.1f,
          .Dt = relays::PID_DELTA_TIME.count(),
      })
    , m_temperature_sensor(i2c_master) {
}

void Heater::run() {
    static constexpr xf::time::Duration REPORT_INTERVAL = 250ms;

    while (true) {
        wxs::match(
            m_state,
            [&](state::Idling) {
                auto command = m_command_queue.await_receive();
                handle_command(command);
            },
            [&](state::Heating& state) {
                auto start = xf::time::now();
                bool finished_stage = start >= state.deadline;
                bool finished_heating = finished_stage and std::holds_alternative<state::Heating::HeatingStage>(state.stage);

                if ((start - state.last_report >= REPORT_INTERVAL) or finished_heating) {
                    state.last_report = start;

                    float seconds_elapsed = std::chrono::duration_cast<FloatSeconds>(start - state.start).count();
                    maestro::send_event({
                        .which_tag = FirmwareEvent_heating_report_tag,
                        .heating_report {
                            .temperature = read_temperature(),
                            .seconds_elapsed = seconds_elapsed,
                            .finished = finished_heating,
                        },
                    });
                }

                if (finished_stage) {
                    bool back_to_main_loop = wxs::match(
                        state.stage,
                        [&](state::Heating::PreHeatingStage& stage) {
                            LOGI("Heater", "Preheat ended");

                            state.stage = state::Heating::HeatingStage {
                                .pid = pid::Controller(m_pid_constants),
                            };
                            state.deadline = start + state.duration;
                            // Reset the relay controller so that we immediately fall into the `needs_new_phase_group` branch and start using the PID controller
                            state.relays_controller.reset();
                            return false;
                        },
                        [&](state::Heating::HeatingStage& stage) {
                            LOGI("Heater", "Heating finished (min={}, max={})", state.min_temp, state.max_temp);
                            m_state = state::Idling {};
                            return true;
                        });

                    if (back_to_main_loop)
                        return;
                }

                if (state.relays_controller.needs_new_phase_group()) {
                    const auto& phase_group = wxs::match(
                        state.stage,
                        [](state::Heating::PreHeatingStage& stage) {
                            return stage.phase_group;
                        },
                        [&](state::Heating::HeatingStage& stage) {
                            float output = stage.pid.calculate_output(state.target_temperature - read_temperature());
                            int watts = percent_to_watts(output);
                            return relays::find_best_phase_group_for_watts(watts);
                        });

                    state.relays_controller.set_phase_group(phase_group);

                    float temperature = m_temperature_sensor.read_temperature();
                    if (temperature > state.max_temp)
                        state.max_temp = temperature;

                    if (temperature < state.min_temp)
                        state.min_temp = temperature;
                }

                if (start - state.last_phase_group_state_change >= relays::AC_PHASE_CYCLE) {
                    state.last_phase_group_state_change = start;

                    auto relays_state = state.relays_controller.next_phase_group_state();
                    RELAYS[0].set(relays_state.active_relays & relays::Weak);
                }

                auto end = xf::time::now();

                auto time_until_deadline = util::saturating_sub(state.deadline, end);
                auto time_until_next_phase_cycle = util::saturating_sub(relays::AC_PHASE_CYCLE, end - state.last_phase_group_state_change);
                auto time_until_next_report = util::saturating_sub(REPORT_INTERVAL, end - state.last_report);

                if (auto command = m_command_queue.receive(std::min({ time_until_deadline, time_until_next_phase_cycle, time_until_next_report })))
                    handle_command(*command);
            });
    }
}

void Heater::handle_command(const HeaterControl& command) {
    static constexpr xf::time::Duration PREHEAT_DURATION = 1s;
    static constexpr float PREHEAT_MULTIPLIER = 2.0f;

    switch (command.which_tag) {
    case HeaterControl_start_tag: {
        auto start = xf::time::now();

        float delta = command.start.target_temperature - read_temperature();
        float scaled_delta = delta * PREHEAT_MULTIPLIER;
        int watts = percent_to_watts(scaled_delta);

        m_state = state::Heating {
            .stage = state::Heating::PreHeatingStage {
                .phase_group = relays::find_best_phase_group_for_watts(watts),
            },
            .target_temperature = command.start.target_temperature,
            .duration = xf::time::Duration { command.start.duration_ms },
            .start = start,
            .deadline = start + PREHEAT_DURATION,
        };
    } break;
    case HeaterControl_stop_tag:
        m_state = state::Idling {};
        LOGI("Heater", "Stopped heating");
        break;
    };
}

float Heater::read_temperature() const {
    return 25.0f;
}
}
