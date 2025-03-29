#pragma once

#include <cstddef>
#include <proto/firmware/FirmwareEvent.pb.h>
#include <xf/Queue.hpp>

namespace maestro {

class Bridge {
public:
    Bridge(size_t device_id);

    void send_event(FirmwareEvent);
};

}
