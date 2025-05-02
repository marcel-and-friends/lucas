#pragma once

#include <vector>

#include <driver/i2c_master.h>

#include <etk/error/error.hpp>

namespace etk::i2c {

class Master {
public:
    static error::Expected<Master> make(i2c_master_bus_config_t);

    Master(Master&&) noexcept;

    Master& operator=(Master&&) noexcept;

    ~Master();

    Master(const Master&) = delete;
    Master& operator=(const Master&) = delete;

    void register_device(i2c_master_dev_handle_t);

    i2c_master_bus_handle_t bus_handle() const;

private:
    Master(i2c_master_bus_handle_t);

    error::Expected<void> destroy();

    i2c_master_bus_handle_t m_bus_handle;

    std::vector<i2c_master_dev_handle_t> m_device_handles;
};

}
