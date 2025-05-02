#pragma once

#include <chrono>
#include <optional>

#include "Notification.hpp"
#include "isr/CountingNotification.hpp"
#include <xf/time/time.hpp>

namespace xf::task {

struct CountingNotification : Notification {
public:
    void give();

    [[nodiscard]] uint32_t await_take();

    template<typename Rep, typename Period>
    [[nodiscard]] std::optional<uint32_t> take(std::chrono::duration<Rep, Period> timeout);

    [[nodiscard]] uint32_t await_fetch();

    template<typename Rep, typename Period>
    [[nodiscard]] std::optional<uint32_t> fetch(std::chrono::duration<Rep, Period> timeout);

    uint32_t current_value() const;

    uint32_t consume_value() const;

    void clear();

    [[nodiscard]] isr::CountingNotification to_isr();
};

template<typename Rep, typename Period>
[[nodiscard]] std::optional<uint32_t> CountingNotification::take(std::chrono::duration<Rep, Period> timeout) {
    BaseType_t value = ulTaskNotifyTakeIndexed(_index, pdTRUE, time::to_raw_tick(timeout));
    if (value == pdFALSE)
        return std::nullopt;

    return value;
}

template<typename Rep, typename Period>
[[nodiscard]] std::optional<uint32_t> CountingNotification::fetch(std::chrono::duration<Rep, Period> timeout) {
    uint32_t result = 0;

    BaseType_t received = xTaskNotifyWaitIndexed(_index, 0, 0, &result, time::to_raw_tick(timeout));
    if (received == pdFALSE)
        return std::nullopt;

    return result;
}
}
