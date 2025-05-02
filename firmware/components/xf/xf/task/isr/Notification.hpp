#pragma once

#include <freertos/FreeRTOS.h>

namespace xf::task::isr {

struct Notification {
    TaskHandle_t _handle;
    size_t _index;
};

}
