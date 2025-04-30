#pragma once

#include <etk/i2c/Master.hpp>
#include <etk/nvs/PersistentValue.hpp>
#include <etk/nvs/Store.hpp>
#include <xf/MutexProtected.hpp>
#include <xf/task/task.hpp>

#include "Bridge.hpp"
#include <heater/Task.hpp>

namespace maestro {

class Maestro : public xf::task::StaticTask<Maestro, 4096> {
    XF_TASK;

    void setup_impl();

    void run_impl();

public:
    static void start();

    friend void send_event(const FirmwareEvent&);

private:
    Maestro();

    static Maestro& the();

    etk::nvs::Store m_nvs_store;
    etk::nvs::PersistentValue<size_t> m_machine_id;
    etk::i2c::Master m_i2c_master;

    command::Queue m_command_queue;
    heater::command::Queue m_heater_command_queue;

    Bridge m_bridge;

    heater::Task m_heater;
};

void send_event(const FirmwareEvent&);

}
