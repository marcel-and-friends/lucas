#include <cmath>
#include <numeric>

#include "TemperatureSensor.hpp"
#include <util/log.hpp>

namespace heater {

constexpr auto ADC_UNIT = ADC_UNIT_1;
constexpr auto ADC_ATTENUATION = ADC_ATTEN_DB_12;
constexpr auto ADC_BITWIDTH = ADC_BITWIDTH_DEFAULT;

static constexpr float steinhart_algorithm(int millivolts) {
    // Constants for the thermistor
    constexpr float R0 = 50000.0f;  // 50kΩ @ 25°C
    constexpr float BETA = 3976.0f; // β25/80
    constexpr float T0 = 298.15f;   // 25°C in Kelvin

    // Voltage divider setup
    constexpr float VIN = 3.3f;    // Supply voltage in volts
    constexpr float RS = 10000.0f; // Series resistor in ohms

    float volts = millivolts / 1000.0f;
    assert(volts >= 0.0 && volts <= VIN);

    float rth = RS * volts / (VIN - volts);

    // Beta equation: T = 1 / (1/T0 + (1/β) * ln(R/R0))
    float inverse = (1.0f / T0) + (1.0f / BETA) * std::log(rth / R0);
    float temperature_kelvin = 1.0f / inverse;

    return temperature_kelvin - 273.15f;
}

TemperatureSensor::TemperatureSensor(adc_channel_t adc_channel)
    : m_channel(adc_channel) {
    adc_oneshot_unit_init_cfg_t adc_unit_config {};
    adc_unit_config.unit_id = ADC_UNIT;

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_unit_config, &m_adc_unit_handle));

    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTENUATION,
        .bitwidth = ADC_BITWIDTH,
        .default_vref = ADC_CALI_LINE_FITTING_EFUSE_VAL_EFUSE_VREF,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_config, &m_calibration_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTENUATION,
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(m_adc_unit_handle, m_channel, &config));
}

TemperatureSensor::~TemperatureSensor() {
    adc_oneshot_del_unit(m_adc_unit_handle);
    adc_cali_delete_scheme_line_fitting(m_calibration_handle);
}

float TemperatureSensor::read_temperature() const {
    constexpr size_t NUM_SAMPLES = 20;
    constexpr size_t INVALID_EXTREMES = 20 * 0.2f;

    std::array<int, NUM_SAMPLES> samples;
    for (auto& sample : samples) {
        int raw;
        ESP_ERROR_CHECK(adc_oneshot_read(m_adc_unit_handle, m_channel, &raw));

        int voltage;
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(m_calibration_handle, raw, &voltage));

        sample = voltage;
    }

    std::sort(samples.begin(), samples.end());

    float average_millivolts = std::accumulate(samples.begin() + (INVALID_EXTREMES / 2), samples.end() - (INVALID_EXTREMES / 2), 0) / float(samples.size() - INVALID_EXTREMES);

    return steinhart_algorithm(std::lround(average_millivolts));
}

}
