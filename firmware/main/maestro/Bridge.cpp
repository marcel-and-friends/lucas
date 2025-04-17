#include <host/ble_hs.h>
#include <host/util/util.h>
#include <nimble/ble.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <services/gap/ble_svc_gap.h>
#include <services/gatt/ble_svc_gatt.h>

#include <pb_decode.h>
#include <pb_encode.h>

#include "Bridge.hpp"
#include <util/literals.hpp>
#include <util/log.hpp>

// Where does this come from? Every ESP-IDF bluetooth example has it but I can't find it...
extern "C" void ble_store_config_init();

namespace maestro {

static Bridge* s_bridge = nullptr;

struct CharacteristicData {
    ble_uuid128_t uuid;
    uint16_t value_handle;
};

static CharacteristicData g_spp_characteristic {
    .uuid = BLE_UUID128_INIT(0x49, 0xc3, 0x73, 0x34, 0xd4, 0x8d, 0x44, 0x9a, 0xbe, 0x95, 0xf5, 0xe7, 0x43, 0xaa, 0x19, 0x50),
    .value_handle = 0,
};

static constexpr ble_uuid16_t SERVICE_UUID = BLE_UUID16_INIT(0xABF0);

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic push

ble_gatt_svc_def Bridge::ble_gatt[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &SERVICE_UUID.u,
        .characteristics = (ble_gatt_chr_def[]) {

            {
                .uuid = &g_spp_characteristic.uuid.u,
                .access_cb = Bridge::spp_gatt_event_handler,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &g_spp_characteristic.value_handle,
            },
            {
                NULL, // NOTE: End of characteristics sentinel.
            },

        },
    },
    {
        BLE_GATT_SVC_TYPE_END, // NOTE: End of services sentinel.
    },
};

#pragma GCC diagnostic pop

Bridge::Bridge(command::Queue& command_queue, size_t device_id)
    : m_maestro_command_queue(command_queue) {
    // There should only be a single instance of Bridge
    assert(s_bridge == nullptr);

    s_bridge = this;

    auto name = std::format("PROTO-{}", device_id);
    ESP_ERROR_CHECK(ble_svc_gap_device_name_set(name.c_str()));

    ESP_ERROR_CHECK(nimble_port_init());

    ble_hs_cfg.reset_cb = reset_cb;
    ble_hs_cfg.sync_cb = sync_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    ble_hs_cfg.sm_io_cap = 3;
    ble_hs_cfg.sm_sc = 0;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    ESP_ERROR_CHECK(ble_gatts_count_cfg(ble_gatt));
    ESP_ERROR_CHECK(ble_gatts_add_svcs(ble_gatt));

    nimble_port_freertos_init(host_task);
}

Bridge& Bridge::the() {
    return *s_bridge;
}

void Bridge::send_event(const FirmwareEvent& event) {
    uint8_t buffer[FirmwareEvent_size];

    auto stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    if (!pb_encode(&stream, FirmwareEvent_fields, &event)) {
        LOGE("Bridge", "Encoding failed (error={})", PB_GET_ERROR(&stream));
        return;
    }

    auto* mbuf = ble_hs_mbuf_from_flat(&buffer, stream.bytes_written);
    if (int rc = ble_gatts_notify_custom(0, g_spp_characteristic.value_handle, mbuf))
        LOGE("Bridge", "Notification failed (rc={})", rc);
}

int Bridge::spp_gatt_event_handler(uint16_t, uint16_t, ble_gatt_access_ctxt* ctx, void*) {
    switch (ctx->op) {
    case BLE_GATT_ACCESS_OP_WRITE_CHR: {
        uint8_t buffer[AppCommand_size];
        uint16_t len;

        if (ble_hs_mbuf_to_flat(ctx->om, buffer, sizeof(buffer), &len)) {
            LOGE("Bridge", "Buffer is not big enough for payload (len={})", os_mbuf_len(ctx->om));
            break;
        }

        auto stream = pb_istream_from_buffer(buffer, len);

        AppCommand message;
        if (not pb_decode(&stream, &AppCommand_msg, &message)) {
            LOGE("Bridge", "Decoding failed (error={})", PB_GET_ERROR(&stream));
            break;
        }

        if (not the().m_maestro_command_queue.send(message, 0ms))
            LOGE("Bridge", "Event queue send failed (tag={})", message.which_tag);
    } break;
    default:
        LOGE("Bridge", "Invalid operation for SPP characteristic");
        break;
    }

    return 0;
}

int Bridge::gap_event_handler(ble_gap_event* event, void*) {
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        LOGI("Bridge", "Connection {} (status={})", event->connect.status == 0 ? "established" : "failed", event->connect.status);
        // Resume advertising when the client is not able to connect.
        if (event->connect.status != 0)
            begin_advertising();
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        LOGW("Bridge", "Disconnected (reason={})", event->disconnect.reason);
        begin_advertising();
        break;
    case BLE_GAP_EVENT_CONN_UPDATE:
        LOGI("Bridge", "Connection updated (status={})", event->conn_update.status);
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        LOGW("Bridge", "Advertisement complete? This shouldn't ever happen (reason={})", event->adv_complete.reason);
        begin_advertising();
        break;
    case BLE_GAP_EVENT_MTU:
        LOGW("Bridge", "MTU updated (cid={}, mtu={})", event->mtu.channel_id, event->mtu.value);
        break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        LOGI("Bridge", "Subscribe event (attr_handle={}, notifying={})", event->subscribe.attr_handle, static_cast<uint8_t>(event->subscribe.cur_notify));
        break;
    default:
        break;
    }

    return 0;
}

void Bridge::begin_advertising() {
    auto uuids = std::array {
        SERVICE_UUID,
    };

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic push

    // Reuse the device name for advertising
    const char* name = ble_svc_gap_device_name();
    ble_hs_adv_fields fields {
        .flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP,

        .uuids16 = uuids.data(),
        .num_uuids16 = uuids.size(),
        .uuids16_is_complete = 1,

        .name = reinterpret_cast<const uint8_t*>(name),
        .name_len = static_cast<uint8_t>(strlen(name)),
        .name_is_complete = 1,

        .tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO,
        .tx_pwr_lvl_is_present = 1,
    };

    ESP_ERROR_CHECK(ble_gap_adv_set_fields(&fields));

    ble_gap_adv_params adv_params {
        .conn_mode = BLE_GAP_CONN_MODE_UND,
        .disc_mode = BLE_GAP_DISC_MODE_GEN,
    };

#pragma GCC diagnostic pop

    uint8_t own_addr_type;
    ESP_ERROR_CHECK(ble_hs_id_infer_auto(0, &own_addr_type));

    ESP_ERROR_CHECK(ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, gap_event_handler, NULL));

    LOGI("Bridge", "Started advertising");
}

void Bridge::sync_cb() {
    LOGI("Bridge", "Syncing");

    ESP_ERROR_CHECK(ble_hs_util_ensure_addr(false));

    begin_advertising();
}

void Bridge::reset_cb(int reason) {
    LOGE("Bridge", "Resetting server (reason={})", reason);
}

void Bridge::host_task(void*) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

}
