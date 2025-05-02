#pragma once

#include "Notification.hpp"
#include <xf/isr/isr.hpp>

namespace xf::task::isr {

struct BinaryNotification : Notification {
    xf::isr::HigherPriorityTaskWoken set();
};

}
