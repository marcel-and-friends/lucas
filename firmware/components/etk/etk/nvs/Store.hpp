#pragma once

#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <nvs.h>
#include <nvs_flash.h>

#include <etk/error/error.hpp>

namespace etk::nvs {

class Store {
public:
    static error::Expected<Store> make(const char* namespace_name, nvs_open_mode_t);

    Store(Store&&) noexcept;

    Store& operator=(Store&&) noexcept;

    ~Store();

    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;

    template<typename T>
    requires std::is_trivially_copyable_v<T>
    error::Expected<void> set(const char* key, const T& value);

    template<typename T>
    requires std::is_trivially_copyable_v<T>
    error::Expected<void> set(const char* key, std::span<const T> value);

    error::Expected<void> set(const char* key, std::string_view value);

    template<typename T>
    requires std::is_trivially_copyable_v<T>
    error::Expected<T> get_or_create(const char* key, T fallback_value);

    template<typename T>
    requires(std::is_trivially_copyable_v<T> and std::is_default_constructible_v<T>)
    error::Expected<std::vector<T>> get_or_create(const char* key, std::span<const T> fallback_value);

    error::Expected<std::string> get_or_create(const char* key, std::string_view fallback_value);

    template<typename T>
    requires std::is_trivially_copyable_v<T>
    error::Expected<T> get(const char* key);

    template<typename T>
    requires(std::is_trivially_copyable_v<T> and std::is_default_constructible_v<T>)
    error::Expected<std::vector<T>> get(const char* key);

    error::Expected<std::string> get(const char* key);

    error::Expected<void> erase(const char* key);

    error::Expected<void> commit();

private:
    Store(nvs_handle_t);

    void destroy();

    nvs_handle_t m_handle;
};

template<typename T>
requires std::is_trivially_copyable_v<T>
error::Expected<void> Store::set(const char* key, const T& value) {
    return set(key, std::span { &value, 1 });
}

template<typename T>
requires std::is_trivially_copyable_v<T>
error::Expected<void> Store::set(const char* key, std::span<const T> value) {
    TRY_RAW(nvs_set_blob(m_handle, key, value.data(), value.size_bytes()));
    return {};
}

template<typename T>
requires std::is_trivially_copyable_v<T>
error::Expected<T> Store::get_or_create(const char* key, T fallback_value) {
    alignas(T) std::byte storage[sizeof(T)];
    size_t length = sizeof(storage);

    auto error = nvs_get_blob(m_handle, key, &storage, &length);
    if (error == ESP_ERR_NVS_NOT_FOUND) {
        TRY(set(key, fallback_value));
        return fallback_value;
    }

    TRY_RAW(error);

    return *std::launder(reinterpret_cast<T*>(&storage));
}

template<typename T>
requires(std::is_trivially_copyable_v<T> and std::is_default_constructible_v<T>)
error::Expected<std::vector<T>> Store::get_or_create(const char* key, std::span<const T> fallback_value) {
    size_t required_size;

    auto error = nvs_get_blob(m_handle, key, nullptr, &required_size);
    if (error == ESP_ERR_NVS_NOT_FOUND) {
        TRY(set(key, fallback_value));
        return fallback_value;
    }

    TRY_RAW(error);

    assert(required_size % sizeof(T) == 0);

    std::vector<T> value(required_size / sizeof(T), T {});
    TRY_RAW(nvs_get_blob(m_handle, key, value.data(), &required_size));

    return value;
}

template<typename T>
requires std::is_trivially_copyable_v<T>
error::Expected<T> Store::get(const char* key) {
    alignas(T) std::byte storage[sizeof(T)];
    size_t length = sizeof(storage);

    TRY_RAW(nvs_get_blob(m_handle, key, &storage, &length));

    return *std::launder(reinterpret_cast<T*>(&storage));
}

template<typename T>
requires(std::is_trivially_copyable_v<T> and std::is_default_constructible_v<T>)
error::Expected<std::vector<T>> Store::get(const char* key) {
    size_t required_size;

    TRY_RAW(nvs_get_blob(m_handle, key, nullptr, &required_size));

    assert(required_size % sizeof(T) == 0);

    std::vector<T> value(required_size / sizeof(T), T {});
    TRY_RAW(nvs_get_blob(m_handle, key, value.data(), &required_size));

    return value;
}

}
