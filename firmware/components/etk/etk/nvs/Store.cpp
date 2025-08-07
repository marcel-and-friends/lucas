#include <utility>

#include "Store.hpp"

namespace etk::nvs {

error::Expected<Store> Store::make(const char* namespace_name, nvs_open_mode_t open_mode) {
    nvs_handle_t handle;
    TRY_RAW(nvs_open(namespace_name, open_mode, &handle));
    return Store(handle);
}

Store::~Store() {
    destroy();
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

error::Expected<void> Store::set(const char* key, std::string_view value) {
    return set(key, std::span { value.data(), value.size() });
}

error::Expected<void> Store::erase(const char* key) {
    TRY_RAW(nvs_erase_key(m_handle, key));
    return {};
}

error::Expected<void> Store::commit() {
    TRY_RAW(nvs_commit(m_handle));
    return {};
}

Store::Store(nvs_handle_t handle)
    : m_handle(handle) {
}

void Store::destroy() {
    if (m_handle)
        nvs_close(m_handle);
}

}
