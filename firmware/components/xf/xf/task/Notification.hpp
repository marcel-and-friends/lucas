#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace xf::task {

struct Notification {
    void clear_state();

    const TaskHandle_t& _handle;
    UBaseType_t _index;
};

}
