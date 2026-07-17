#include <cmath>

#include <driver/i2c_master.h>
#include <esp_log.h>

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

static int scan_bus(i2c_master_bus_handle_t bus, const char* label) {
    int found = 0;
    for (uint16_t address = 0x08; address <= 0x77; ++address) {
        if (i2c_master_probe(bus, address, 20) == ESP_OK) {
            LOGW("TemperatureSensor", "I2C scan [{}]: device responding at address 0x{:02X}", label, address);
            ++found;
        }
    }
    LOGW("TemperatureSensor", "I2C scan [{}]: {} device(s) found", label, found);
    return found;
}

// When the ADS doesn't answer, figure out why so the bench log says exactly which wire to fix:
// scan the configured bus (expected: ADS at 0x48 with ADDR->GND; 0x49=VDD, 0x4A=SDA, 0x4B=SCL),
// and if nothing answers, retry with SDA/SCL swapped to detect inverted wires.
static void run_wiring_diagnostics(etk::i2c::Master& i2c_master) {
    // The probe misses would otherwise flood the log with one NACK error per address.
    esp_log_level_set("i2c.master", ESP_LOG_NONE);

    if (scan_bus(i2c_master.bus_handle(), "SDA=27 SCL=26") > 0) {
        LOGE("TemperatureSensor", "The bus works but the ADS didn't answer at 0x48 — check the module's ADDR pin (must go to GND)");
        esp_log_level_set("i2c.master", ESP_LOG_ERROR);
        return;
    }

    i2c_master_bus_config_t swapped_config = {
        .i2c_port = I2C_NUM_1,
        .sda_io_num = GPIO_NUM_26,
        .scl_io_num = GPIO_NUM_27,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = true,
            .allow_pd = false,
        },
    };

    i2c_master_bus_handle_t swapped;
    if (i2c_new_master_bus(&swapped_config, &swapped) == ESP_OK) {
        if (scan_bus(swapped, "SDA=26 SCL=27 (swapped)") > 0)
            LOGE("TemperatureSensor", "Devices answer with SDA/SCL swapped — the wires on GPIO26/GPIO27 are inverted");
        else
            LOGE("TemperatureSensor", "No I2C device answers in either arrangement — check the module's VDD/GND and the wires to GPIO27 (SDA) / GPIO26 (SCL)");
        i2c_del_master_bus(swapped);
    }

    esp_log_level_set("i2c.master", ESP_LOG_ERROR);
}

// A missing or unresponsive ADS must not take the whole firmware down with it: the machine still
// needs to boot, advertise over BLE and be able to refuse to heat with a SENSOR_FAILURE alarm.
TemperatureSensor::TemperatureSensor(etk::i2c::Master& i2c_master) {
    auto device_handle = ads111x::init(i2c_master.bus_handle(), ads111x::AddrSelection::GND, 400'000);
    if (not device_handle) {
        LOGE("TemperatureSensor", "Failed to initialize the ADS (error={:X}), all readings will be invalid", device_handle.error());
        run_wiring_diagnostics(i2c_master);
        return;
    }

    m_device_handle = *device_handle;
    i2c_master.register_device(m_device_handle);

    using namespace ads111x::reg;

    auto configured = ads111x::write(
        m_device_handle,
        Config {
            // 128 SPS gives us a sample every ~7.8ms, which means we'll most likely get a sample within the current AC phase.
            .dr = Config::DataRate::_128SPS,
            .mode = Config::Mode::ContinuousConversion,
            // We want to read the full 0-3.3v range. 0-4.096v is the closest the ADS can give us.
            .pga = Config::PGA::FSR_4_096V,
            // We only care about the value in the AIN0 channel, which should be compared against GND.
            .mux = Config::Mux::AINP_AIN0_AINN_GND,
        });
    if (not configured) {
        LOGE("TemperatureSensor", "Failed to configure the ADS (error={:X}), all readings will be invalid", configured.error());
        run_wiring_diagnostics(i2c_master);
        return;
    }

    auto pointed = ads111x::write(
        m_device_handle,
        AddressPointer {
            .p = AddressPointer::Register::Conversion,
        });
    if (not pointed) {
        LOGE("TemperatureSensor", "Failed to set the ADS address pointer (error={:X}), all readings will be invalid", pointed.error());
        run_wiring_diagnostics(i2c_master);
        return;
    }

    m_initialized = true;
}

// An intact NTC on the heater body can never read outside of this range; values beyond it mean
// the sensor is open, shorted or disconnected.
constexpr float MIN_PLAUSIBLE_TEMPERATURE = -20.0f;
constexpr float MAX_PLAUSIBLE_TEMPERATURE = 300.0f;

TemperatureSensor::Reading TemperatureSensor::read() {
    if (not m_initialized)
        return { 0.0f, false };

    int16_t conversion;
    bool i2c_ok = true;
    if (auto reg = ads111x::read<ads111x::reg::Conversion>(m_device_handle)) {
        conversion = m_last_conversion = reg->d;
    } else {
        LOGE("TemperatureSensor", "Failed to read conversion register, re-using last conversion.");
        conversion = m_last_conversion;
        i2c_ok = false;
    }

    // NOTE: We configure the PGA to give us a range of ~4.096v.
    float volts = std::clamp(4.096f * (static_cast<float>(conversion) / INT16_MAX), 0.0f, VREF);
    float temperature = steinhart_formula(volts);

    bool plausible = temperature > MIN_PLAUSIBLE_TEMPERATURE and temperature < MAX_PLAUSIBLE_TEMPERATURE;

    return { temperature, i2c_ok and plausible };
}

}
