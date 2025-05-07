#include <cmath>

#include <etk/gpio/Pin.hpp>
#include <wxs/match.hpp>
#include <wxs/visit.hpp>

#include "Heater.hpp"
#include <maestro/Maestro.hpp>
#include <util/literals.hpp>
#include <util/log.hpp>

namespace heater {

static auto RELAYS = std::array {
    // Onboard LED connection on the devkit
    etk::gpio::Output { GPIO_NUM_26 },
    etk::gpio::Output { GPIO_NUM_27 },
};

static auto WATER_RELAY = etk::gpio::Output { GPIO_NUM_16 };

static constexpr int percent_to_watts(float percent) {
    return std::lround(std::clamp(percent, 0.0f, 100.0f) * relays::TOTAL_WATTAGE / 100.0f);
}

static constexpr bool check_interval(xf::time::Tick& last_tick, xf::time::Duration interval, xf::time::Tick start) {
    if (auto delta = start - last_tick; delta >= interval) {
        last_tick = start - (delta != start.time_since_epoch() ? delta - interval : xf::time::Duration {});
        return true;
    }
    return false;
}

Heater::Heater(command::Queue& command_queue, etk::i2c::Master& i2c_master)
    : m_command_queue(command_queue)
    , m_temperature_sensor(i2c_master) {
}

void Heater::setup() {
    for (auto& relay : RELAYS)
        relay.setup();

    WATER_RELAY.setup();

    disable_relays();
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
            [&](state::Heating& heating) {
                wxs::visit(heating.stage, [&](state::Heating::Stage& stage) {
                    auto start = std::exchange(stage.first_loop, false) ? stage.start : xf::time::now();

                    bool finished_stage = start >= stage.deadline;
                    bool is_preheating = std::holds_alternative<state::Heating::PreHeatingStage>(heating.stage);
                    bool finished_heating = finished_stage and not is_preheating;

                    auto temperature = [&, temperature = std::optional<float> {}] mutable {
                        if (!temperature)
                            temperature = m_temperature_sensor.read_temperature();
                        return *temperature;
                    };

                    if (check_interval(heating.last_report, REPORT_INTERVAL, start) or finished_heating) {
                        float seconds_elapsed = chrono::duration_cast<FloatSeconds>(start - heating.start).count();
                        maestro::send_event({
                            .which_tag = FirmwareEvent_heating_report_tag,
                            .heating_report = {
                                .temperature = temperature(),
                                .seconds_elapsed = seconds_elapsed,
                                .finished = finished_heating,
                                .stage = is_preheating ? HeatingStage_PreHeating : HeatingStage_Heating,
                            },
                        });
                    }

                    if (finished_stage) {
                        wxs::match(
                            heating.stage,
                            [&](state::Heating::PreHeatingStage&) {
                                heating.stage = state::Heating::HeatingStage {
                                    {
                                        .start = start,
                                        .deadline = start + xf::time::Duration { heating.parameters.duration_ms },
                                    },
                                    pid::Controller(pid::Constants {
                                        .Kp = heating.parameters.p,
                                        .Ki = heating.parameters.i,
                                        .Kd = heating.parameters.d,
                                        .Dt = relays::PID_DELTA_TIME.count(),
                                    }),
                                };

                                WATER_RELAY.enable();
                            },
                            [&](state::Heating::HeatingStage& stage) {
                                LOGI("Heater", "Heating finished (min={}, max={})", stage.min_temp, stage.max_temp);
                                m_state = state::Idling {};
                                disable_relays();
                            });

                        return;
                    }

                    if (stage.relays_controller.needs_new_phase_group()) {
                        const auto& phase_group = wxs::match(
                            heating.stage,
                            [](state::Heating::PreHeatingStage& stage) {
                                return stage.phase_group;
                            },
                            [&](state::Heating::HeatingStage& stage) {
                                float output = stage.pid.calculate_output(heating.parameters.target_temperature - temperature());
                                int watts = percent_to_watts(output);

                                if (temperature() > stage.max_temp)
                                    stage.max_temp = temperature();

                                if (temperature() < stage.min_temp)
                                    stage.min_temp = temperature();

                                return relays::find_best_phase_group_for_watts(watts);
                            });

                        stage.relays_controller.set_phase_group(phase_group);
                    }

                    if (check_interval(stage.last_phase_group_state_change, relays::AC_PHASE_INTERVAL, start)) {
                        auto relays_state = stage.relays_controller.next_phase_group_state();
                        RELAYS[0].set(relays_state.active_relays & relays::Weak);
                        RELAYS[1].set(relays_state.active_relays & relays::Strong);
                    }

                    auto end = xf::time::now();

                    auto time_until_deadline = util::saturating_sub(stage.deadline, end);
                    auto time_until_next_phase_cycle = util::saturating_sub(relays::AC_PHASE_INTERVAL, end - stage.last_phase_group_state_change);
                    auto time_until_next_report = util::saturating_sub(REPORT_INTERVAL, end - heating.last_report);

                    if (auto command = m_command_queue.receive(std::min({ time_until_deadline, time_until_next_phase_cycle, time_until_next_report })))
                        handle_command(*command);
                });
            });
    }
}

void Heater::handle_command(const HeaterControl& command) {
    switch (command.which_tag) {
    case HeaterControl_start_tag: {
        float delta = command.start.target_temperature - m_temperature_sensor.read_temperature();
        float scaled_delta = delta * command.start.preheat_multiplier;
        int watts = percent_to_watts(scaled_delta);

        auto start = xf::time::now();

        m_state = state::Heating {
            .stage = state::Heating::PreHeatingStage {
                {
                    .start = start,
                    .deadline = start + xf::time::Duration { command.start.preheat_duration_ms },
                },
                relays::find_best_phase_group_for_watts(watts),
            },
            .start = start,
            .parameters = command.start,
        };

        LOGI("Heater", "Heating (target={})", command.start.target_temperature);
    } break;
    case HeaterControl_stop_tag:
        m_state = state::Idling {};

        disable_relays();

        LOGI("Heater", "Heating stopped");
        break;
    };
}

void Heater::disable_relays() {
    WATER_RELAY.disable();
    for (auto& relay : RELAYS)
        relay.disable();
}

}
