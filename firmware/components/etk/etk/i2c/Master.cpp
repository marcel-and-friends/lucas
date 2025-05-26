#include <utility>

#include "Master.hpp"

namespace etk::i2c {

error::Expected<Master> Master::make(i2c_master_bus_config_t bus_config) {
    i2c_master_bus_handle_t bus_handle;
    TRY_RAW(i2c_new_master_bus(&bus_config, &bus_handle));
    return Master(bus_handle);
}

Master::~Master() {
    MUST(destroy());
}

Master::Master(Master&& other) noexcept
    : m_bus_handle(std::exchange(other.m_bus_handle, nullptr))
    , m_device_handles(std::move(other.m_device_handles)) { }

Master& Master::operator=(Master&& other) noexcept {
    if (this != &other) {
        MUST(destroy());
        m_bus_handle = std::exchange(other.m_bus_handle, nullptr);
        m_device_handles = std::move(other.m_device_handles);
    }
    return *this;
}

void Master::register_device(i2c_master_dev_handle_t device_handle) {
    m_device_handles.push_back(device_handle);
}

i2c_master_bus_handle_t Master::bus_handle() const {
    return m_bus_handle;
}

Master::Master(i2c_master_bus_handle_t bus_handle)
    : m_bus_handle(bus_handle) {
}

error::Expected<void> Master::destroy() {
    for (auto device_handle : m_device_handles)
        TRY_RAW(i2c_master_bus_rm_device(device_handle));

    if (m_bus_handle)
        TRY_RAW(i2c_del_master_bus(m_bus_handle));

    return {};
}

}
