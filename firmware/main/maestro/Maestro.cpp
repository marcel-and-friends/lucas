#include <nvs_flash.h>

#include <etk/nvs/PersistentValue.hpp>

#include "Maestro.hpp"
#include <proto/firmware/FirmwareEvent.pb.h>
#include <util/literals.hpp>
#include <util/log.hpp>

namespace maestro {

static Maestro* s_instance = nullptr;

Maestro::Maestro()
    : m_nvs_store(TRY_OR_THROW(etk::nvs::Store::make("Lucas", NVS_READWRITE)))
    , m_machine_id(TRY_OR_THROW(etk::nvs::PersistentValue<size_t>::make(m_nvs_store, "MachineId", 0)))
    , m_i2c_master(TRY_OR_THROW(etk::i2c::Master::make({
          .i2c_port = I2C_NUM_0,
          .sda_io_num = GPIO_NUM_4,
          .scl_io_num = GPIO_NUM_5,
          .clk_source = I2C_CLK_SRC_DEFAULT,
          .glitch_ignore_cnt = 7,
          .intr_priority = 0,
          .trans_queue_depth = 0,
          .flags {
              .enable_internal_pullup = true,
              .allow_pd = false,
          },
      })))
    , m_bridge(m_command_queue, *m_machine_id)
    , m_heater(m_heater_command_queue, m_i2c_master) {
    assert(s_instance == nullptr);
    s_instance = this;
}

void Maestro::setup() {
    m_command_queue.create();
    m_heater_command_queue.create();

    m_heater.create_pinned_to_core(10, APP_CPU_NUM);
}

void Maestro::run() {
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

void Maestro::start() {
    assert(s_instance == nullptr);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES or ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        TRY_RAW_OR_THROW(nvs_flash_erase());
        TRY_RAW_OR_THROW(nvs_flash_init());
    }

    static Maestro s_maestro;

    s_maestro.create_pinned_to_core("MAESTRO", 10, PRO_CPU_NUM);
}

void send_event(const FirmwareEvent& event) {
    Maestro::the().m_bridge.send_event(event);
}

Maestro& Maestro::the() {
    return *s_instance;
}

}
