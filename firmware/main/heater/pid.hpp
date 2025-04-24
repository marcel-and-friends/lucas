#pragma once

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
        float derivative = (error - m_previous_error) / m_constants.Dt;
        m_integral += error * m_constants.Dt;

        m_previous_error = error;

        return m_constants.Kp * proportional + m_constants.Ki * m_integral + m_constants.Kd * derivative;
    }

private:
    float m_integral { 0.0f };
    float m_previous_error { 0.0f };

    Constants m_constants;
};

}
