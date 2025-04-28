#pragma once

#include <esp_adc/adc_oneshot.h>

namespace heater {

class TemperatureSensor {
public:
    TemperatureSensor(adc_channel_t);

    ~TemperatureSensor();

    float read_temperature() const;

private:
    adc_channel_t m_channel;
    adc_oneshot_unit_handle_t m_adc_unit_handle;
    adc_cali_handle_t m_calibration_handle;
};

}
