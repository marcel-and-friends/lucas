#include <cmath>

#include <driver/i2c_master.h>

#include <ads111x/ads111x.hpp>

#include "TemperatureSensor.hpp"
#include <util/log.hpp>

namespace heater {

constexpr float VREF = 3.3f;

static constexpr float steinhart_formula(float volts) {
    // Steinhart-Hart constants and circuit taken from ferro techniek's "Ntc temperature readout and NTC Table rev6"
    constexpr double A = 9.66475227118302e-4;
    constexpr double B = 2.00488995949766e-4;
    constexpr double D = 1.67581403147716e-7;
    constexpr double RS = 12000.0;

    double resistance = (RS * volts) / (VREF - volts);
    double log = std::log(resistance);
    return 1.0 / (A + B * log + D * std::pow(log, 3)) - 273.15;
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
    // NOTE: We configure the PGA to give us a range of ~4.096v.
    float volts = std::clamp(4.096f * (float(conversion) / INT16_MAX), 0.0f, VREF);
    return steinhart_formula(volts);
}

}
