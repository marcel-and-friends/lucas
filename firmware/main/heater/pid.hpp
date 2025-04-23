#pragma once

namespace heater::pid {

struct Constants {
    float Kp;
    float Ki;
    float Kd;
    float Dt;
};

class Data {
    Data(Constants& constants)
        : m_constants(constants) { }

    float calculate_output() {
        float proportional = m_error;
        float derivative = (m_error - m_previous_error) / m_constants.Dt;
        m_integral += m_error * m_constants.Dt;

        m_previous_error = m_error;

        return m_constants.Kp * proportional + m_constants.Ki * m_integral + m_constants.Kd * derivative;
    }

private:
    float m_integral;
    float m_error;
    float m_previous_error;

    const Constants& m_constants;
};

}
