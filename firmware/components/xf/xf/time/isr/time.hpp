#pragma once

#include <xf/time/time.hpp>

namespace xf::time::isr {

inline Tick now() {
    return Tick { Duration { xTaskGetTickCountFromISR() } };
}

}
