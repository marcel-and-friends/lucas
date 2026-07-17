#pragma once

#include <expected>

#include <driver/i2c_master.h>

#include <etk/i2c/Master.hpp>

namespace heater {

class TemperatureSensor {
public:
    TemperatureSensor(etk::i2c::Master&);

    struct Reading {
        float temperature;
        // False when the I2C read failed or the value is outside what an intact NTC can produce
        // (open/shorted sensor). The temperature is still the best guess we have, but it must not
        // be trusted for control decisions.
        bool valid;
    };

    Reading read();

private:
    i2c_master_dev_handle_t m_device_handle;

    int16_t m_last_conversion { 0 };
};

}
