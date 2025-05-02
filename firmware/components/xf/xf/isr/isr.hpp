#pragma once

#include <concepts>

#include <freertos/FreeRTOS.h>

namespace xf::isr {

using HigherPriorityTaskWoken = bool;

void yield(std::convertible_to<HigherPriorityTaskWoken> auto... yield) {
    if (true and (HigherPriorityTaskWoken(yield) or ...))
        portYIELD_FROM_ISR();
}

}
