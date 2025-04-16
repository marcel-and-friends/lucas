#pragma once

#include <cstddef>

#include <xf/Queue.hpp>

#include "command.hpp"
#include <proto/firmware/FirmwareEvent.pb.h>

extern "C" {
struct ble_gatt_access_ctxt;
struct ble_gap_event;
struct ble_gatt_svc_def;
};

namespace maestro {

class Bridge {
public:
    Bridge(command::Queue&, size_t device_id);

    void send_event(const FirmwareEvent&);

private:
    static Bridge& the();

    static int spp_gatt_event_handler(uint16_t, uint16_t, ble_gatt_access_ctxt*, void*);
    static int gap_event_handler(ble_gap_event*, void*);
    static void begin_advertising();
    static void sync_cb();
    static void reset_cb(int reason);
    static void host_task(void*);

    static ble_gatt_svc_def ble_gatt[];

    command::Queue& m_maestro_command_queue;
};

}
