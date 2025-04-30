#pragma once

#include <expected>

#include <etk/i2c/Master.hpp>

#include <driver/i2c_master.h>
#include <esp_adc/adc_oneshot.h>

namespace heater {

class TemperatureSensor {
public:
    TemperatureSensor(etk::i2c::Master&);

    float read_temperature() const;

private:
    i2c_master_dev_handle_t m_dev_handle;
};

}
