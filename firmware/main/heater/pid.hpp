#pragma once

#include <algorithm>
#include <format>
#include <optional>

namespace heater::pid {

struct Constants {
    float Kp;
    float Ki;
    float Kd;
    float Dt;
};

class Controller {
public:
    // The output is a percentage of the heater's power, so anything the PID asks for outside of
    // this range is physically impossible to deliver.
    static constexpr float OUTPUT_MIN = 0.0f;
    static constexpr float OUTPUT_MAX = 100.0f;

    Controller(const Constants& constants, float initial_integral = 0.0f)
        : m_constants(constants)
        , m_integral(initial_integral) { }

    float calculate_output(float error) {
        float proportional = error;
        float derivative = m_previous_error ? (error - *m_previous_error) / m_constants.Dt : 0.0f;
        m_integral += error * m_constants.Dt;

        // Anti-windup: never let the integral term alone demand more than the output range can
        // deliver, otherwise it keeps growing while saturated and causes a large overshoot when
        // the error finally flips sign.
        if (m_constants.Ki > 0.0f)
            m_integral = std::clamp(m_integral, OUTPUT_MIN / m_constants.Ki, OUTPUT_MAX / m_constants.Ki);

        m_previous_error = error;

        float output = m_constants.Kp * proportional + m_constants.Ki * m_integral + m_constants.Kd * derivative;
        return std::clamp(output, OUTPUT_MIN, OUTPUT_MAX);
    }

    float integral() {
        return m_integral;
    }

private:
    Constants m_constants;

    float m_integral { 0.0f };
    std::optional<float> m_previous_error;
};

}

template<>
struct std::formatter<heater::pid::Constants> : std::formatter<std::string> {
    auto format(const heater::pid::Constants& c, format_context& ctx) const {
        return formatter<std::string>::format(std::format("(Kp={}, Ki={}, Kd={}, Dt={})", c.Kp, c.Ki, c.Kd, c.Dt), ctx);
    }
};
