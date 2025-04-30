#include <utility>

#include "Store.hpp"

namespace etk::nvs {

error::Expected<Store> Store::make(const char* namespace_name, nvs_open_mode_t open_mode) {
    nvs_handle_t handle;
    TRY_RAW(nvs_open(namespace_name, open_mode, &handle));
    return Store(handle);
}

Store::Store(nvs_handle_t handle)
    : m_handle(handle) {
}

Store::Store(Store&& other) noexcept
    : m_handle(std::exchange(other.m_handle, 0)) { }

Store& Store::operator=(Store&& other) noexcept {
    if (this != &other) {
        destroy();
        m_handle = std::exchange(other.m_handle, 0);
    }
    return *this;
}

Store::~Store() {
    destroy();
}

error::Expected<void> Store::set(const char* key, std::string_view value) {
    TRY_RAW(nvs_set_blob(m_handle, key, value.data(), value.size()));
    TRY_RAW(nvs_commit(m_handle));
    return {};
}

error::Expected<std::string> Store::get_or_create(const char* key, std::string_view default_value) {
    size_t required_size;

    auto error = nvs_get_blob(m_handle, key, nullptr, &required_size);
    if (error == ESP_ERR_NVS_NOT_FOUND) {
        TRY(set(key, default_value));
        return std::string(default_value);
    }

    TRY_RAW(error);

    std::string value(required_size, '\0');
    TRY_RAW(nvs_get_blob(m_handle, key, value.data(), &required_size));

    return value;
}

error::Expected<std::string> Store::get(const char* key) {
    size_t required_size;

    TRY_RAW(nvs_get_blob(m_handle, key, nullptr, &required_size));

    std::string value(required_size, '\0');
    TRY_RAW(nvs_get_blob(m_handle, key, value.data(), &required_size));

    return value;
}

error::Expected<void> Store::erase(const char* key) {
    TRY_RAW(nvs_erase_key(m_handle, key));
    TRY_RAW(nvs_commit(m_handle));
    return {};
}

void Store::destroy() {
    if (m_handle)
        nvs_close(m_handle);
}

}
