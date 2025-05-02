#pragma once

#include <optional>

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

#include <xf/isr/isr.hpp>
#include <xf/time/time.hpp>

namespace xf::timer::isr {

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
class Timer {
public:
    using Handle = TimerHandle_t;

    explicit Timer(Handle);

    [[nodiscard]] std::optional<xf::isr::HigherPriorityTaskWoken> start();

    [[nodiscard]] std::optional<xf::isr::HigherPriorityTaskWoken> stop();

    template<typename Rep, typename Period>
    [[nodiscard]] std::optional<xf::isr::HigherPriorityTaskWoken> change_period(std::chrono::duration<Rep, Period> period);

    [[nodiscard]] std::optional<xf::isr::HigherPriorityTaskWoken> reset();

private:
    Handle m_handle { nullptr };
};

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
Timer<Ctx...>::Timer(Timer::Handle handle)
    : m_handle(handle) {
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
std::optional<xf::isr::HigherPriorityTaskWoken> Timer<Ctx...>::start() {
    BaseType_t higher_priority_task_woken = false;
    if (xTimerStartFromISR(m_handle, &higher_priority_task_woken) == pdFAIL)
        return std::nullopt;

    return higher_priority_task_woken;
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
std::optional<xf::isr::HigherPriorityTaskWoken> Timer<Ctx...>::stop() {
    BaseType_t higher_priority_task_woken = false;
    if (xTimerStopFromISR(m_handle, &higher_priority_task_woken) == pdFAIL)
        return std::nullopt;

    return higher_priority_task_woken;
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
template<typename Rep, typename Period>
std::optional<xf::isr::HigherPriorityTaskWoken> Timer<Ctx...>::change_period(std::chrono::duration<Rep, Period> period) {
    BaseType_t higher_priority_task_woken = false;
    if (xTimerChangePeriodFromISR(m_handle, time::to_raw_tick(period), time::to_raw_tick(period)) == pdFAIL)
        return std::nullopt;

    return higher_priority_task_woken;
}

template<typename... Ctx>
requires(!std::is_reference_v<Ctx> && ...)
std::optional<xf::isr::HigherPriorityTaskWoken> Timer<Ctx...>::reset() {
    BaseType_t higher_priority_task_woken = false;
    if (xTimerResetFromISR(m_handle, &higher_priority_task_woken) == pdFAIL)
        return std::nullopt;

    return higher_priority_task_woken;
}

}
