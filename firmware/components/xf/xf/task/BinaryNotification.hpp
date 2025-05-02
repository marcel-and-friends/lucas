
#pragma once

#include <chrono>

#include "Notification.hpp"
#include "isr/BinaryNotification.hpp"
#include <xf/time/time.hpp>

namespace xf::task {

struct BinaryNotification : Notification {
public:
    void set();

    void await_get();

    template<typename Rep, typename Period>
    [[nodiscard]] bool get(std::chrono::duration<Rep, Period> timeout);

    [[nodiscard]] bool current_value();

    [[nodiscard]] isr::BinaryNotification to_isr();
};

template<typename Rep, typename Period>
[[nodiscard]] bool BinaryNotification::get(std::chrono::duration<Rep, Period> timeout) {
    return static_cast<bool>(xTaskNotifyWaitIndexed(_index, 0, UINT32_MAX, nullptr, time::to_raw_tick(timeout)));
}

}
