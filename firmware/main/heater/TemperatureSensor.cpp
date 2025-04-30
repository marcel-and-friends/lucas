#include <cmath>
#include <expected>

#include <driver/i2c_master.h>

#include <ads1115/ads1115.hpp>
#include <wxs/match.hpp>

#include "TemperatureSensor.hpp"
#include <util/log.hpp>

namespace heater {

static constexpr float steinhart_algorithm(int millivolts) {
    // Constants for the thermistor
    constexpr float R0 = 50000.0f;  // 50kΩ @ 25°C
    constexpr float BETA = 3976.0f; // β25/80
    constexpr float T0 = 298.15f;   // 25°C in Kelvin

    // Voltage divider setup
    constexpr float VIN = 3.3f;    // Supply voltage in volts
    constexpr float RS = 10000.0f; // Series resistor in ohms

    float volts = millivolts / 1000.0f;
    assert(volts >= 0.0 and volts <= VIN);

    float rth = RS * volts / (VIN - volts);

    // Beta equation: T = 1 / (1/T0 + (1/β) * ln(R/R0))
    float inverse = (1.0f / T0) + (1.0f / BETA) * std::log(rth / R0);
    float temperature_kelvin = 1.0f / inverse;

    return temperature_kelvin - 273.15f;
}

TemperatureSensor::TemperatureSensor(etk::i2c::Master& i2c_master)
    : m_dev_handle(TRY_OR_THROW(ads1115::init(i2c_master.bus_handle(), ads1115::AddrLine::GND, 400'000))) {
    i2c_master.register_device(m_dev_handle);

    using namespace ads1115::reg;

    TRY_OR_THROW(ads1115::write(
        m_dev_handle,
        Config {
            .dr = Config::DataRate::_16SPS,
            .mode = Config::Mode::ContinuousConversion,
            .pga = Config::PGA::FSR_4_096V,
            .mux = Config::Mux::AINP_AIN0_AINN_GND,
        }));

    TRY_OR_THROW(ads1115::write(
        m_dev_handle,
        AddressPointer {
            .p = AddressPointer::Register::Conversion,
        }));
}

float TemperatureSensor::read_temperature() const {
    auto conversion = MUST(ads1115::read<ads1115::reg::Conversion>(m_dev_handle)).d;
    assert(conversion >= 0);
    if (conversion == 0)
        return 0.0f;

    float millivolts = 4096.0f * (float(conversion) / INT16_MAX);

    LOGI("Sensor", "millivolts={}", millivolts);

    return steinhart_algorithm(millivolts);
}

}
