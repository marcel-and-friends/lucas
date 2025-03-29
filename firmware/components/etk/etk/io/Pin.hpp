#pragma once

#include <cstdint>
#include <driver/gpio.h>

namespace etk::io {
enum class Invert {
    No,
    Yes
};

class Pin {
public:
    constexpr Pin(gpio_num_t pin, gpio_mode_t mode)
        : m_pin(pin)
        , m_mode(mode) { }

    void setup() const {
        gpio_config_t cfg {
            .pin_bit_mask = (1ULL << m_pin),
            .mode = m_mode,
        };
        gpio_config(&cfg);
    }

    [[nodiscard]] gpio_num_t pin() const {
        return m_pin;
    }

    [[nodiscard]] gpio_mode_t mode() const {
        return m_mode;
    }

private:
    gpio_num_t m_pin { GPIO_NUM_0 };
    gpio_mode_t m_mode { GPIO_MODE_OUTPUT };
};

class Output : public Pin {
public:
    constexpr Output(gpio_num_t pin, Invert invert = Invert::No)
        : Pin(pin, GPIO_MODE_OUTPUT)
        , m_invert(invert == Invert::Yes) { }

    void set(bool state) const {
        gpio_set_level(pin(), state ^ m_invert);
    }

    void enable() const {
        set(true);
    }

    void disable() const {
        set(false);
    }

private:
    bool m_invert;
};

class AnalogOutput : public Pin {
public:
    constexpr AnalogOutput(gpio_num_t pin, Invert invert = Invert::No)
        : Pin(pin, GPIO_MODE_OUTPUT)
        , m_invert(invert == Invert::Yes) { }

    static constexpr auto PWM_MAX = 255;

    void write(uint8_t value) const {
        // analogWrite(pin(), m_invert ? PWM_MAX - value : value);
    }

    void write_normalized(float value) const {
        // configASSERT(value >= 0.0f && value <= 1.0f);
        write(static_cast<uint8_t>(value * PWM_MAX));
    }

    void enable() const {
        write(PWM_MAX);
    }

    void disable() const {
        write(0);
    }

private:
    bool m_invert;
};

class Input : public Pin {
public:
    using ISR = void (*)();

    constexpr explicit Input(gpio_num_t pin, Invert invert = Invert::No)
        : Pin(pin, GPIO_MODE_INPUT)
        , m_invert(invert == Invert::Yes) { }

    [[nodiscard]] bool read() const {
        // return digitalRead(pin()) ^ m_invert;
        return false;
    }

    void attach_isr(ISR isr, int mode) const {
        // attachInterrupt(digitalPinToInterrupt(pin()), isr, mode);
    }

    void detach_isr() const {
        // detachInterrupt(digitalPinToInterrupt(pin()));
    }

private:
    bool m_invert;
};

class AnalogInput : public Pin {
public:
    constexpr AnalogInput(gpio_num_t pin)
        : Pin(pin, GPIO_MODE_INPUT) { }

    [[nodiscard]] uint16_t read() const {
        // return analogRead(pin());
        return 0;
    }
};
}
