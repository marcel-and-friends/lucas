#pragma once

#include <functional>
#include <optional>
#include <utility>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <xf/time/time.hpp>

namespace xf {

template<typename T>
class MutexProtected {
public:
    MutexProtected(const MutexProtected&) = delete;
    MutexProtected(MutexProtected&&) = delete;
    MutexProtected& operator=(const MutexProtected&) = delete;
    MutexProtected& operator=(MutexProtected&&) = delete;

    template<typename... Args>
    requires std::constructible_from<T, Args...>
    explicit MutexProtected(Args&&...);

    ~MutexProtected();

    void create();

    void destroy();

    template<std::invocable<T&> FN, typename R = std::invoke_result_t<FN, T&>>
    R await_access(FN&& fn);

    template<std::invocable<const T&> FN, typename R = std::invoke_result_t<FN, T&>>
    R await_access(FN&& fn) const;

    template<std::invocable<T&> FN, typename Rep, typename Period, typename R = std::invoke_result_t<FN, T&>>
    [[nodiscard]] std::optional<std::conditional_t<std::is_void_v<R>, std::monostate, R>> access(FN&& fn, std::chrono::duration<Rep, Period> timeout);

    template<std::invocable<const T&> FN, typename Rep, typename Period, typename R = std::invoke_result_t<FN, T&>>
    [[nodiscard]] std::optional<std::conditional_t<std::is_void_v<R>, std::monostate, R>> access(FN&& fn, std::chrono::duration<Rep, Period> timeout) const;

private:
    T m_value;

    SemaphoreHandle_t m_handle;
    StaticSemaphore_t m_static_semaphore;
};

template<typename T>
template<typename... Args>
requires std::constructible_from<T, Args...>
MutexProtected<T>::MutexProtected(Args&&... args)
    : m_value(std::forward<Args>(args)...) {
}

template<typename T>
MutexProtected<T>::~MutexProtected() {
    if (m_handle)
        destroy();
}

template<typename T>
void MutexProtected<T>::create() {
    configASSERT(m_handle == nullptr);
    m_handle = xSemaphoreCreateMutexStatic(&m_static_semaphore);
}

template<typename T>
void MutexProtected<T>::destroy() {
    configASSERT(m_handle);
    vSemaphoreDelete(std::exchange(m_handle, nullptr));
}

template<typename T>
template<std::invocable<T&> FN, typename R>
R MutexProtected<T>::await_access(FN&& fn) {
    if constexpr (std::is_void_v<R>) {
        auto ret = access(std::forward<FN>(fn), time::FOREVER);
        configASSERT(ret.has_value());
        return;
    } else {
        return access(std::forward<FN>(fn), time::FOREVER).value();
    }
}

template<typename T>
template<std::invocable<const T&> FN, typename R>
R MutexProtected<T>::await_access(FN&& fn) const {
    if constexpr (std::is_void_v<R>) {
        auto ret = access(std::forward<FN>(fn), time::FOREVER);
        configASSERT(ret.has_value());
        return;
    } else {
        return access(std::forward<FN>(fn), time::FOREVER).value();
    }
}

template<typename T>
template<std::invocable<T&> FN, typename Rep, typename Period, typename R>
std::optional<std::conditional_t<std::is_void_v<R>, std::monostate, R>> MutexProtected<T>::access(FN&& fn, std::chrono::duration<Rep, Period> timeout) {
    if (xSemaphoreTake(m_handle, time::to_raw_tick(timeout)) != pdTRUE)
        return std::nullopt;

    if constexpr (std::is_void_v<R>) {
        std::invoke(std::forward<FN>(fn), m_value);

        auto give = xSemaphoreGive(m_handle);
        configASSERT(give == pdTRUE);

        return std::monostate {};
    } else {
        auto result = std::invoke(std::forward<FN>(fn), m_value);

        auto give = xSemaphoreGive(m_handle);
        configASSERT(give == pdTRUE);

        return result;
    }
}

template<typename T>
template<std::invocable<const T&> FN, typename Rep, typename Period, typename R>
std::optional<std::conditional_t<std::is_void_v<R>, std::monostate, R>> MutexProtected<T>::access(FN&& fn, std::chrono::duration<Rep, Period> timeout) const {
    if (xSemaphoreTake(m_handle, time::to_raw_tick(timeout)) != pdTRUE)
        return std::nullopt;

    if constexpr (std::is_void_v<R>) {
        std::invoke(std::forward<FN>(fn), m_value);

        auto give = xSemaphoreGive(m_handle);
        configASSERT(give == pdTRUE);

        return std::monostate {};
    } else {
        auto result = std::invoke(std::forward<FN>(fn), m_value);

        auto give = xSemaphoreGive(m_handle);
        configASSERT(give == pdTRUE);

        return result;
    }
}

}
