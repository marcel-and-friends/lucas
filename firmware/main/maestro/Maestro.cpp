#include <nvs_flash.h>

#include <etk/nvs/PersistentValue.hpp>

#include "Maestro.hpp"
#include <proto/firmware/FirmwareEvent.pb.h>
#include <util/literals.hpp>
#include <util/log.hpp>

namespace maestro {

static Maestro* s_instance = nullptr;

Maestro::Maestro()
    : m_nvs_store("Lucas")
    , m_machine_id(m_nvs_store, 0)
    , m_bridge(m_command_queue, *m_machine_id)
    , m_heater(m_heater_command_queue) {
}

void Maestro::setup_impl() {
    m_bridge.create();

    m_command_queue.create();
    m_heater_command_queue.create();

    m_heater.create(1);
}

void Maestro::run_impl() {
    while (true) {
        auto command = m_command_queue.await_receive();
        LOGI("Maestro", "Received command (tag={})", command.which_tag);
        switch (command.which_tag) {
        case AppCommand_heater_control_tag:
            m_heater_command_queue.await_send(command.heater_control);
            break;
        }
    }
}

void Maestro::begin() {
    assert(s_instance == nullptr);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    static Maestro s_maestro;
    s_instance = &s_maestro;

    s_instance->create("MAESTRO", 10);
}

Maestro& Maestro::the() {
    return *s_instance;
}

void send_event(const FirmwareEvent& event) {
    Maestro::the().m_bridge.await_access([&](Bridge& bridge) {
        bridge.send_event(event);
    });
}

}
