
#pragma once

#include <chrono>
#include <cstring>

#include "Notification.hpp"
#include "isr/StateNotification.hpp"
#include <xf/time/time.hpp>

namespace xf::task {

template<typename T>
struct StateNotification : Notification {
public:
    static_assert(sizeof(T) <= sizeof(uint32_t) and std::is_trivially_copyable_v<T>,
        "Type can't be stored in a FreeRTOS notification");

    void set(T state);

    [[nodiscard]] T await_get();

    template<typename Rep, typename Period>
    [[nodiscard]] std::optional<T> get(std::chrono::duration<Rep, Period> timeout);

    [[nodiscard]] isr::StateNotification<T> to_isr();
};

template<typename T>
void StateNotification<T>::set(T state) {
    uint32_t raw_value = 0;
    std::memcpy(&raw_value, &state, sizeof(T));
    xTaskNotifyIndexed(_handle, _index, raw_value, eSetValueWithOverwrite);
}

template<typename T>
[[nodiscard]] T StateNotification<T>::await_get() {
    return get(time::FOREVER).value();
}

template<typename T>
template<typename Rep, typename Period>
[[nodiscard]] std::optional<T> StateNotification<T>::get(std::chrono::duration<Rep, Period> timeout) {
    uint32_t raw_value = 0;
    const auto received = xTaskNotifyWaitIndexed(_index, 0, UINT32_MAX, &raw_value, time::to_raw_tick(timeout));
    if (received == pdFALSE)
        return std::nullopt;

    alignas(T) std::byte buffer[sizeof(T)];
    std::memcpy(buffer, &raw_value, sizeof(T));
    return *std::launder(reinterpret_cast<T*>(&buffer));
}

template<typename T>
[[nodiscard]] isr::StateNotification<T> StateNotification<T>::to_isr() {
    return { _handle, _index };
}

}
