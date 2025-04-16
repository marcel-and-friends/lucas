#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <nvs.h>
#include <nvs_flash.h>

namespace etk::nvs {

class Store {
public:
    Store(const char* namespace_name) {
        ESP_ERROR_CHECK(nvs_open(namespace_name, NVS_READWRITE, &m_handle));
    }

    ~Store() {
        nvs_close(m_handle);
    }

    template<typename T>
    requires std::is_trivially_copyable_v<T>
    void set(const char* key, const T& value) {
        ESP_ERROR_CHECK(nvs_set_blob(m_handle, key, &value, sizeof(value)));
        ESP_ERROR_CHECK(nvs_commit(m_handle));
    }

    template<typename T>
    requires std::is_trivially_copyable_v<T>
    void set(const char* key, std::span<const T> value) {
        ESP_ERROR_CHECK(nvs_set_blob(m_handle, key, value.data(), value.size()));
        ESP_ERROR_CHECK(nvs_commit(m_handle));
    }

    void set(const char* key, std::string_view value) {
        ESP_ERROR_CHECK(nvs_set_blob(m_handle, key, value.data(), value.size()));
        ESP_ERROR_CHECK(nvs_commit(m_handle));
    }

    template<typename T>
    requires std::is_trivially_copyable_v<T>
    T get_or_create(const char* key, T default_value) {
        alignas(T) std::byte storage[sizeof(T)];
        auto length = sizeof(storage);

        auto error = nvs_get_blob(m_handle, key, &storage, &length);
        if (error == ESP_ERR_NVS_NOT_FOUND) {
            set(key, default_value);
            return default_value;
        }

        ESP_ERROR_CHECK(error);

        return *std::launder(reinterpret_cast<T*>(&storage));
    }

    template<typename T>
    requires std::is_trivially_copyable_v<T>
    std::vector<T> get_or_create(const char* key, std::span<T const> default_value) {
        size_t required_size;

        auto error = nvs_get_blob(m_handle, key, nullptr, &required_size);
        if (error == ESP_ERR_NVS_NOT_FOUND) {
            set(key, default_value);
            return default_value;
        }

        ESP_ERROR_CHECK(error);

        assert(required_size % sizeof(T) == 0);

        std::vector<T> value(required_size / sizeof(T), T {});
        ESP_ERROR_CHECK(nvs_get_blob(m_handle, key, value.data(), &required_size));

        return value;
    }

    std::string get_or_create(const char* key, std::string_view default_value) {
        size_t required_size;

        auto error = nvs_get_blob(m_handle, key, nullptr, &required_size);
        if (error == ESP_ERR_NVS_NOT_FOUND) {
            set(key, default_value);
            return std::string(default_value);
        }

        ESP_ERROR_CHECK(error);

        std::string value(required_size, '\0');
        ESP_ERROR_CHECK(nvs_get_blob(m_handle, key, value.data(), &required_size));

        return value;
    }

    template<typename T>
    requires std::is_trivially_copyable_v<T>
    std::optional<T> get(const char* key) {
        alignas(T) std::byte storage[sizeof(T)];
        auto length = sizeof(storage);

        auto error = nvs_get_blob(m_handle, key, &storage, &length);
        if (error == ESP_ERR_NVS_NOT_FOUND)
            return std::nullopt;

        ESP_ERROR_CHECK(error);

        return *std::launder(reinterpret_cast<T*>(&storage));
    }

    template<typename T>
    requires std::is_trivially_copyable_v<T>
    std::vector<T> get(const char* key) {
        size_t required_size;

        auto error = nvs_get_blob(m_handle, key, nullptr, &required_size);
        if (error == ESP_ERR_NVS_NOT_FOUND)
            return std::nullopt;

        ESP_ERROR_CHECK(error);

        assert(required_size % sizeof(T) == 0);

        std::vector<T> value(required_size / sizeof(T), T {});
        ESP_ERROR_CHECK(nvs_get_blob(m_handle, key, value.data(), &required_size));

        return value;
    }

    std::optional<std::string> get(const char* key) {
        size_t required_size;
        auto error = nvs_get_blob(m_handle, key, nullptr, &required_size);
        if (error == ESP_ERR_NVS_NOT_FOUND)
            return std::nullopt;

        ESP_ERROR_CHECK(error);

        std::string value(required_size, '\0');
        ESP_ERROR_CHECK(nvs_get_blob(m_handle, key, value.data(), &required_size));

        return value;
    }

    void erase(const char* key) {
        ESP_ERROR_CHECK(nvs_erase_key(m_handle, key));
        ESP_ERROR_CHECK(nvs_commit(m_handle));
    }

private:
    nvs_handle_t m_handle;
};

}
