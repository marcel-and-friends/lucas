#pragma once

#include "Notification.hpp"
#include <xf/isr/isr.hpp>

namespace xf::task::isr {

struct CountingNotification : Notification {
public:
    xf::isr::HigherPriorityTaskWoken give();
};

}
