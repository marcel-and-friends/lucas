#pragma once

#include <expected>

#include <driver/i2c_master.h>

#include <etk/i2c/Master.hpp>

namespace heater {

class TemperatureSensor {
public:
    TemperatureSensor(etk::i2c::Master&);

    float read_temperature();

private:
    i2c_master_dev_handle_t m_device_handle;

    int16_t m_last_conversion { 0 };
};

}
