#pragma once

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
    Controller(const Constants& constants)
        : m_constants(constants) { }

    float calculate_output(float error) {
        float proportional = error;
        float derivative = m_previous_error ? (error - *m_previous_error) / m_constants.Dt : 0.0f;
        m_integral += error * m_constants.Dt;

        m_previous_error = error;

        return m_constants.Kp * proportional + m_constants.Ki * m_integral + m_constants.Kd * derivative;
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
