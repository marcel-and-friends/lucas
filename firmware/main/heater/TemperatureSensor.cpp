#include <cmath>

#include <driver/i2c_master.h>

#include <ads111x/ads111x.hpp>

#include "TemperatureSensor.hpp"
#include <util/log.hpp>

namespace heater {

static constexpr float steinhart_algorithm(int volts) {
    // Constants for the thermistor
    constexpr float R0 = 50000.0f;  // 50kΩ @ 25°C
    constexpr float BETA = 3976.0f; // β25/80
    constexpr float T0 = 298.15f;   // 25°C in Kelvin

    // Voltage divider setup
    constexpr float VIN = 3.3f;    // Supply voltage in volts
    constexpr float RS = 10000.0f; // Series resistor in ohms

    float rth = RS * volts / (VIN - volts);

    // Beta equation: T = 1 / (1/T0 + (1/β) * ln(R/R0))
    float inverse = (1.0f / T0) + (1.0f / BETA) * std::log(rth / R0);
    float temperature_kelvin = 1.0f / inverse;

    return temperature_kelvin - 273.15f;
}

TemperatureSensor::TemperatureSensor(etk::i2c::Master& i2c_master)
    : m_device_handle(TRY_OR_THROW(ads111x::init(i2c_master.bus_handle(), ads111x::AddrSelection::GND, 400'000))) {
    i2c_master.register_device(m_device_handle);

    using namespace ads111x::reg;

    TRY_OR_THROW(ads111x::write(
        m_device_handle,
        Config {
            // The PID control loop needs a fresh temperature once every 85ms (see `relays::PID_DELTA_TIME`),
            // meaning we need to sample at least ~12 times per second. 16 is the closest the ADS can give us.
            .dr = Config::DataRate::_16SPS,
            .mode = Config::Mode::ContinuousConversion,
            // We want to read the full 0-3.3v range. 0-4.096v is the closest the ADS can give us.
            .pga = Config::PGA::FSR_4_096V,
            // We only care about the value in the AIN0 channel, which should be compared against GND.
            .mux = Config::Mux::AINP_AIN0_AINN_GND,
        }));

    TRY_OR_THROW(ads111x::write(
        m_device_handle,
        AddressPointer {
            .p = AddressPointer::Register::Conversion,
        }));
}

float TemperatureSensor::read_temperature() const {
    int16_t conversion = MUST(ads111x::read<ads111x::reg::Conversion>(m_device_handle)).d;
    assert(conversion >= 0);
    if (conversion == 0)
        return 0.0f;

    // NOTE: We configure the PGA to give us a range of ~4.096v.
    float volts = 4.096f * (float(conversion) / INT16_MAX);

    LOGI("Sensor", "volts={}", volts);

    return steinhart_algorithm(std::lround(volts));
}

}
