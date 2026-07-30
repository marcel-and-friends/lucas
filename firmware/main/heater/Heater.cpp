#include <cmath>

#include <etk/gpio/Pin.hpp>
#include <wxs/match.hpp>
#include <wxs/visit.hpp>

#include "Heater.hpp"
#include <maestro/Maestro.hpp>
#include <util/literals.hpp>
#include <util/log.hpp>

namespace heater {

static auto HEATER_RELAYS = std::array {
    etk::gpio::Output { GPIO_NUM_18 },
    etk::gpio::Output { GPIO_NUM_19 },
};

static auto PREHEATER_RELAY = etk::gpio::Output { GPIO_NUM_17 };

static auto WATER_RELAY = etk::gpio::Output { GPIO_NUM_21 };

// Safety limits. The NTC measures the FTH's body, not the water, so during normal operation it
// runs well above the water temperature — this ceiling is meant to catch runaway heating, not to
// regulate. Review on the bench before changing.
static constexpr float MAX_ELEMENT_TEMPERATURE = 150.0f;
// The sensor updates every ~7.8ms and the control loop wakes at least every AC phase cycle, so
// this bounds how long we tolerate flying blind to roughly a couple hundred milliseconds.
static constexpr int MAX_CONSECUTIVE_SENSOR_FAILURES = 8;

// The resistive film ahead of the NTC runs far hotter than the reading while at full power, and
// that stored heat keeps pushing the body up after the relays cut (bench 17/07: exiting the dry
// stage at full power overshot by ~20C no matter the exit margin, steaming the first sip). Two
// defenses, both driven by the PROJECTED temperature (reading + slope * LOOKAHEAD) because a
// cold element at full power climbs ~50C/s and instantaneous thresholds trigger too late: slow
// down to soak power once the projection nears the target, and only open the water once the
// projection reaches it.
// Calibrated on the bench: cutting at full power, the body kept climbing ~27C past a 1s
// projection — the stored film heat is worth well over two seconds of the measured slope.
static constexpr float PREHEAT_LOOKAHEAD_SECONDS = 2.5f;
static constexpr float PREHEAT_SOAK_BAND = 6.0f;
static constexpr float PREHEAT_SOAK_POWER_PERCENT = 30.0f;
static constexpr float PREHEAT_EXIT_MARGIN = 4.0f;

// A DRY channel sizzles when first wetted at high temperature no matter how settled the film is
// (bench dose-response: opened at 96 -> sizzled, 91 -> sizzled faintly, <=58 -> silent), while a
// wetted wall held 94C at full power without a hiss. The film also hides ~15-19C from the NTC
// even at low preheat power, so this ceiling is set well below the wet-safe boundary — the body
// lands in the low 80s when the water opens, and the rest of the climb happens under flow.
static constexpr float PREHEAT_MAX_DRY_TEMPERATURE = 65.0f;

// Standby parks the element at the dry-safe ceiling so a pour needs no preheat. Only the weak
// element cycles (small film gradient — the whole point is staying inside the silent zone).
static constexpr float STANDBY_TEMPERATURE = PREHEAT_MAX_DRY_TEMPERATURE;
static constexpr float STANDBY_HYSTERESIS = 1.5f;

// After the relays cut, the film keeps dumping its stored heat into the body for about a second
// (bench: the body peaked 0.9s AFTER the preheat ended). Water arriving during that window
// boils against the film even when the body reading looks fine — the sip steamed with the
// outlet at 83C. Hold the water until the film has settled.
static constexpr auto PREHEAT_SETTLE_TIME = 1200ms;

static constexpr int percentage_to_watts(float percent) {
    return std::lround(percent / 100.0f * relays::MAX_WATTS);
}

static constexpr float watts_to_percentage(int watts) {
    return static_cast<float>(watts) / relays::MAX_WATTS * 100.0f;
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
    for (auto& relay : HEATER_RELAYS) {
        relay.setup();
        relay.disable();
    }

    PREHEATER_RELAY.setup();
    PREHEATER_RELAY.disable();

    WATER_RELAY.setup();
    WATER_RELAY.disable();
}

void Heater::run() {
    while (true) {
        wxs::match(
            m_state,
            [&](state::Idling) {
                auto command = m_command_queue.await_receive();
                handle_command(command);
            },
            [&](state::Standby& standby) {
                run_standby(standby);
            },
            [&](state::Heating& heating) {
                wxs::visit(heating.stage, [&](state::Heating::Stage& stage) {
                    control_heater(heating, stage);
                });
            });
    }
}

void Heater::handle_command(const HeaterControl& command) {
    switch (command.which_tag) {
    case HeaterControl_start_tag: {
        // The first conversion after a long idle can be stale (bench: 52C on a 26C element) and
        // the whole preheat is sized off this one number — discard it and wait for a fresh one.
        m_temperature_sensor.read();
        delay(15ms);

        auto reading = m_temperature_sensor.read();
        if (not reading.valid) {
            raise_alarm(AlarmCode_SENSOR_FAILURE);
            return;
        }

        m_consecutive_sensor_failures = 0;

        float delta = command.start.target_temperature - reading.temperature;

        int preheat_duration = command.start.preheat_duration_multiplier ? std::max(std::lround(delta * command.start.preheat_duration_multiplier), 250l) : 0;
        float preheat_power = command.start.preheat_power_multiplier ? std::max(delta * command.start.preheat_power_multiplier, 30.0f) : 0.0f;

        // HACK: Do proper non-heating pour functionality
        if (command.start.target_temperature == 0) {
            m_last_heating_info = std::nullopt;
            preheat_duration = 0;
            preheat_power = 0.0f;
        }

        auto start = xf::time::now();
        m_state = state::Heating {
            .stage = state::Heating::PreHeatingStage {
                {
                    .start = start,
                    .deadline = start + xf::time::Duration { preheat_duration },
                },
                relays::find_best_control_data_for_watts(percentage_to_watts(preheat_power)),
            },
            .start = start,
            .parameters = command.start,
        };
    } break;
    case HeaterControl_stop_tag:
        m_state = state::Idling {};

        disable_relays(DelayWaterRelayDisable::Yes);

        LOGI("Heater", "Heating stopped");
        break;
    case HeaterControl_standby_tag:
        // Entering from a pour shuts everything down first; the standby loop then takes over.
        disable_relays(DelayWaterRelayDisable::Yes);

        m_state = state::Standby {};

        LOGI("Heater", "Standby engaged");
        break;
    };
}

void Heater::control_heater(state::Heating& heating, state::Heating::Stage& stage) {
    static constexpr xf::time::Duration REPORT_INTERVAL = 250ms;

    auto started_stage = std::exchange(stage.first_loop, false);
    auto start = started_stage ? stage.start : xf::time::now();

    auto reading = m_temperature_sensor.read();
    m_consecutive_sensor_failures = reading.valid ? 0 : m_consecutive_sensor_failures + 1;
    if (m_consecutive_sensor_failures >= MAX_CONSECUTIVE_SENSOR_FAILURES) {
        raise_alarm(AlarmCode_SENSOR_FAILURE);
        return;
    }

    if (reading.valid and reading.temperature >= MAX_ELEMENT_TEMPERATURE) {
        raise_alarm(AlarmCode_OVER_TEMPERATURE);
        return;
    }

    float temperature = reading.temperature;

    bool is_preheating = std::holds_alternative<state::Heating::PreHeatingStage>(heating.stage);

    // The preheat deadline is an upper bound: once the projected body temperature reaches the
    // exit threshold there is nothing left to do dry, and staying dry any longer only sizzles
    // the first water that comes in.
    float preheat_exit_threshold = std::min(
        static_cast<float>(heating.parameters.target_temperature) - PREHEAT_EXIT_MARGIN,
        PREHEAT_MAX_DRY_TEMPERATURE);

    float projected_temperature = temperature;
    if (auto* preheat_stage = std::get_if<state::Heating::PreHeatingStage>(&heating.stage))
        projected_temperature += preheat_stage->slope_per_second * PREHEAT_LOOKAHEAD_SECONDS;

    bool preheat_done_early = is_preheating
        and heating.parameters.target_temperature > 0
        and reading.valid
        and projected_temperature >= preheat_exit_threshold;

    bool finished_stage = start >= stage.deadline or preheat_done_early;
    bool finished_heating = finished_stage and not is_preheating;

    if (stage.relays_controller.needs_new_control_data()) {
        auto control_info = wxs::match(
            heating.stage,
            [&](state::Heating::PreHeatingStage& stage) -> state::Heating::ControlInfo {
                if (reading.valid) {
                    if (stage.previous_temperature >= 0.0f)
                        stage.slope_per_second = (temperature - stage.previous_temperature) / relays::PID_DELTA_TIME.count();
                    stage.previous_temperature = temperature;
                }

                float projected = temperature + stage.slope_per_second * PREHEAT_LOOKAHEAD_SECONDS;
                bool soaking = heating.parameters.target_temperature > 0
                    and reading.valid
                    and projected >= preheat_exit_threshold - PREHEAT_SOAK_BAND;
                if (soaking)
                    return { relays::find_best_control_data_for_watts(percentage_to_watts(PREHEAT_SOAK_POWER_PERCENT)), std::nullopt };

                return { stage.control_data, std::nullopt };
            },
            [&](state::Heating::HeatingStage& stage) -> state::Heating::ControlInfo {
                float pid = stage.pid.calculate_output(heating.parameters.target_temperature - temperature);
                int watts = percentage_to_watts(pid);
                return { relays::find_best_control_data_for_watts(watts), pid };
            });

        heating.active_control_info = control_info;

        stage.relays_controller.set_control_data(control_info.control_data);
    }

    if (check_interval(heating.last_report, REPORT_INTERVAL, start) or finished_stage or started_stage) {
        float seconds_elapsed = chrono::duration_cast<FloatSeconds>(start - heating.start).count();
        maestro::send_event({
            .which_tag = FirmwareEvent_heating_report_tag,
            .heating_report = {
                .temperature = temperature,
                .pid = heating.active_control_info.pid.value_or(-1),
                .power = watts_to_percentage(heating.active_control_info.control_data.average_watts),
                .seconds_elapsed = seconds_elapsed,
                .stage = finished_heating ? HeatingStage_Finished : is_preheating ? HeatingStage_PreHeating
                                                                                  : HeatingStage_Heating,
            },
        });
    }

    if (finished_stage) {
        wxs::match(
            heating.stage,
            [&](state::Heating::PreHeatingStage&) {
                float initial_integral = this->initial_integral(temperature);
                LOGI("Heater", "Reusing last heating info (integral={})", initial_integral);

                // Let the film settle dry before any water arrives — the body peak from the
                // stored heat must happen without water present to boil against it. Skipped for
                // non-heating pours, which have no preheat to settle from.
                if (heating.parameters.target_temperature > 0) {
                    for (auto& relay : HEATER_RELAYS)
                        relay.disable();
                    delay(PREHEAT_SETTLE_TIME);
                }

                auto heating_start = xf::time::now();
                heating.stage = state::Heating::HeatingStage {
                    {
                        .start = heating_start,
                        .deadline = heating_start + xf::time::Duration { heating.parameters.duration_ms },
                    },
                    pid::Controller(
                        pid::Constants {
                            .Kp = heating.parameters.p,
                            .Ki = heating.parameters.i,
                            .Kd = heating.parameters.d,
                            .Dt = relays::PID_DELTA_TIME.count(),
                        },
                        initial_integral),
                };

                PREHEATER_RELAY.enable();
                WATER_RELAY.enable();
            },
            [&](state::Heating::HeatingStage& stage) {
                // HACK: Do proper non-heating pour functionality
                if (heating.parameters.target_temperature)
                    m_last_heating_info = LastHeatingInfo {
                        .ending_temperature = temperature,
                        .ending_integral = stage.pid.integral(),
                    };

                LOGI("Heater", "Saving heating info (integral={})", stage.pid.integral());

                m_state = state::Idling {};

                disable_relays(heating.parameters.target_temperature ? DelayWaterRelayDisable::Yes : DelayWaterRelayDisable::No);
            });

        return;
    }

    if (check_interval(stage.last_phase_group_state_change, relays::AC_PHASE_INTERVAL, start)) {
        auto relays_state = stage.relays_controller.next_control_state();
        HEATER_RELAYS[0].set(relays_state.active_relays & relays::Weak);
        HEATER_RELAYS[1].set(relays_state.active_relays & relays::Strong);
    }

    auto end = xf::time::now();

    auto time_until_deadline = util::saturating_sub(stage.deadline, end);
    auto time_until_next_phase_cycle = util::saturating_sub(relays::AC_PHASE_INTERVAL, end - stage.last_phase_group_state_change);
    auto time_until_next_report = util::saturating_sub(REPORT_INTERVAL, end - heating.last_report);

    if (auto command = m_command_queue.receive(std::min({ time_until_deadline, time_until_next_phase_cycle, time_until_next_report })))
        handle_command(*command);
}

void Heater::run_standby(state::Standby& standby) {
    auto reading = m_temperature_sensor.read();
    m_consecutive_sensor_failures = reading.valid ? 0 : m_consecutive_sensor_failures + 1;
    if (m_consecutive_sensor_failures >= MAX_CONSECUTIVE_SENSOR_FAILURES) {
        raise_alarm(AlarmCode_SENSOR_FAILURE);
        return;
    }

    if (reading.valid and reading.temperature >= MAX_ELEMENT_TEMPERATURE) {
        raise_alarm(AlarmCode_OVER_TEMPERATURE);
        return;
    }

    if (reading.valid) {
        if (standby.element_on) {
            if (reading.temperature >= STANDBY_TEMPERATURE)
                standby.element_on = false;
        } else {
            if (reading.temperature < STANDBY_TEMPERATURE - STANDBY_HYSTERESIS)
                standby.element_on = true;
        }
    } else {
        // Flying blind (briefly, before the failure counter trips) — keep the element off.
        standby.element_on = false;
    }

    HEATER_RELAYS[0].set(standby.element_on);

    if (check_interval(standby.last_report, xf::time::Duration { 2000 }, xf::time::now())) {
        maestro::send_event({
            .which_tag = FirmwareEvent_heating_report_tag,
            .heating_report = {
                .temperature = reading.temperature,
                .pid = -1,
                .power = standby.element_on ? watts_to_percentage(relays::WEAK_RELAY_WATTS) : 0.0f,
                .seconds_elapsed = 0,
                .stage = HeatingStage_Standby,
            },
        });
    }

    if (auto command = m_command_queue.receive(250ms)) {
        HEATER_RELAYS[0].disable();
        handle_command(*command);
    }
}

void Heater::raise_alarm(AlarmCode code) {
    LOGE("Heater", "Alarm raised (code={}), shutting the heater down", static_cast<int>(code));

    m_state = state::Idling {};

    // Flushing water through the element while it powers down is what cools it — especially
    // important on an over-temperature trip.
    disable_relays(DelayWaterRelayDisable::Yes);

    maestro::send_event({
        .which_tag = FirmwareEvent_alarm_tag,
        .alarm = {
            .code = code,
        },
    });
}

void Heater::disable_relays(Heater::DelayWaterRelayDisable delay_water_relay) {
    for (auto& relay : HEATER_RELAYS)
        relay.disable();

    PREHEATER_RELAY.disable();

    if (delay_water_relay == DelayWaterRelayDisable::Yes)
        // This is roughly the minimum amount of time for the heater to not boil the body of residual water when turning it off.
        delay(350ms);

    WATER_RELAY.disable();
}

float Heater::initial_integral(float temperature) {
    return m_last_heating_info.transform([&](auto& info) { return info.reuse_integral(temperature); }).value_or(0.0f);
}

}
