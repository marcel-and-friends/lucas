#pragma once

#include <etk/nvs/PersistentValue.hpp>
#include <etk/nvs/Store.hpp>
#include <xf/MutexProtected.hpp>
#include <xf/task/task.hpp>

#include "Bridge.hpp"
#include "command.hpp"
#include <heater/Task.hpp>

namespace maestro {

class Maestro : public xf::task::StaticTask<Maestro, 3192> {
    XF_TASK;

    void setup_impl();

    void run_impl();

public:
    explicit Maestro();

    static void begin();

    friend void send_event(const FirmwareEvent&);

private:
    static Maestro& the();

    etk::nvs::Store m_nvs_store;
    etk::nvs::PersistentValue<size_t, "Maestro/id"> m_machine_id;

    command::Queue m_command_queue;

    xf::MutexProtected<Bridge> m_bridge;

    heater::command::Queue m_heater_command_queue;
    heater::Task m_heater;
};

void send_event(const FirmwareEvent&);
}
