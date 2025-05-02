#pragma once

#include <tuple>
#include <utility>

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

#include "isr/Timer.hpp"
#include <xf/isr/isr.hpp>
#include <xf/time/time.hpp>

namespace xf::timer {

enum class Mode {
    Repeating,
    SingleShot,
    SelfDestructive,
};

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
class Timer {
public:
    using Handle = TimerHandle_t;

    using Callback = void (*)(Ctx&...);

    Timer(Mode, Callback, Ctx&...);

    Timer(Timer&&) noexcept;

    Timer& operator=(Timer&&) noexcept;

    ~Timer();

    // There is no mechanism in FreeRTOS to copy a timer
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    template<typename Rep, typename Period>
    void create(const char* name, std::chrono::duration<Rep, Period> period);

    void await_start();

    template<typename Rep, typename Period>
    [[nodiscard]] bool start(std::chrono::duration<Rep, Period> timeout);

    void await_stop();

    template<typename Rep, typename Period>
    [[nodiscard]] bool stop(std::chrono::duration<Rep, Period> timeout);

    template<typename Rep, typename Period>
    void await_change_period(std::chrono::duration<Rep, Period> period);

    template<typename Rep, typename Period, typename Rep2, typename Period2>
    [[nodiscard]] bool change_period(std::chrono::duration<Rep, Period> period, std::chrono::duration<Rep2, Period2> timeout);

    void await_reset();

    template<typename Rep, typename Period>
    [[nodiscard]] bool reset(std::chrono::duration<Rep, Period> timeout);

    void await_destroy();

    template<typename Rep, typename Period>
    [[nodiscard]] bool destroy(std::chrono::duration<Rep, Period> timeout);

    [[nodiscard]] bool is_active() const;

    [[nodiscard]] bool is_valid() const;

    [[nodiscard]] isr::Timer<Ctx...> for_isr();

private:
    static void callback(TimerHandle_t);

    Handle m_handle { nullptr };
    StaticTimer_t m_static_timer;

    Mode m_mode;

    Callback m_callback;
    std::tuple<Ctx&...> m_ctx;
};

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
Timer<Ctx...>::Timer(Mode mode, Callback callback, Ctx&... ctx)
    : m_mode(mode)
    , m_callback(callback)
    , m_ctx(ctx...) {
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
Timer<Ctx...>::Timer(Timer&& other) noexcept
    : m_handle(std::exchange(other.m_handle, nullptr))
    , m_callback(std::exchange(other.m_callback, nullptr))
    , m_ctx(std::move(other.m_ctx))
    , m_mode(other.m_mode) {
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
Timer<Ctx...>& Timer<Ctx...>::operator=(Timer&& other) noexcept {
    if (this != &other) {
        if (m_handle)
            await_destroy();
        m_handle = std::exchange(other.m_handle, nullptr);
        m_callback = std::exchange(other.m_callback, nullptr);
        m_ctx = std::move(other.m_ctx);
        m_mode = other.m_mode;
    }
    return *this;
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
Timer<Ctx...>::~Timer() {
    if (m_handle)
        await_destroy();
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
template<typename Rep, typename Period>
void Timer<Ctx...>::create(const char* name, std::chrono::duration<Rep, Period> period) {
    configASSERT(m_handle == nullptr);
    m_handle = xTimerCreateStatic(
        name,
        time::to_raw_tick(period),
        m_mode == Mode::Repeating,
        this,
        &Timer::callback,
        &m_static_timer);
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
void Timer<Ctx...>::await_start() {
    (void)start(time::FOREVER);
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
template<typename Rep, typename Period>
bool Timer<Ctx...>::start(std::chrono::duration<Rep, Period> timeout) {
    return xTimerStart(m_handle, time::to_raw_tick(timeout)) == pdTRUE;
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
void Timer<Ctx...>::await_stop() {
    (void)stop(time::FOREVER);
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
template<typename Rep, typename Period>
bool Timer<Ctx...>::stop(std::chrono::duration<Rep, Period> timeout) {
    return xTimerStop(m_handle, time::to_raw_tick(timeout)) == pdTRUE;
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
template<typename Rep, typename Period>
void Timer<Ctx...>::await_change_period(std::chrono::duration<Rep, Period> period) {
    (void)change_period(period, time::FOREVER);
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
template<typename Rep, typename Period, typename Rep2, typename Period2>
bool Timer<Ctx...>::change_period(std::chrono::duration<Rep, Period> period, std::chrono::duration<Rep2, Period2> timeout) {
    return xTimerChangePeriod(m_handle, time::to_raw_tick(period), time::to_raw_tick(timeout)) == pdTRUE;
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
void Timer<Ctx...>::await_reset() {
    (void)reset(time::FOREVER);
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
template<typename Rep, typename Period>
bool Timer<Ctx...>::reset(std::chrono::duration<Rep, Period> timeout) {
    return xTimerReset(m_handle, time::to_raw_tick(timeout)) == pdTRUE;
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
void Timer<Ctx...>::await_destroy() {
    (void)destroy(time::FOREVER);
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
template<typename Rep, typename Period>
bool Timer<Ctx...>::destroy(std::chrono::duration<Rep, Period> timeout) {
    if (xTimerDelete(m_handle, time::to_raw_tick(timeout)) == pdTRUE) {
        m_handle = nullptr;
        return true;
    } else {
        return false;
    }
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
bool Timer<Ctx...>::is_active() const {
    return xTimerIsTimerActive(m_handle) != pdFALSE;
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
bool Timer<Ctx...>::is_valid() const {
    return m_handle != nullptr;
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
void Timer<Ctx...>::callback(TimerHandle_t handle) {
    auto& self = *static_cast<Timer*>(pvTimerGetTimerID(handle));

    std::apply([&](Ctx&&... ctx) { self.m_callback(ctx...); }, self.m_ctx);

    if (self.m_mode == Mode::SelfDestructive)
        self.await_destroy();
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
isr::Timer<Ctx...> Timer<Ctx...>::for_isr() {
    return { m_handle };
}

}
