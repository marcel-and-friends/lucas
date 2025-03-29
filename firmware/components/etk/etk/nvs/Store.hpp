#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include <nvs.h>
#include <nvs_flash.h>

namespace etk::nvs {

namespace {
template<typename>
constexpr std::false_type always_false {};
}

class Store {
public:
    Store(const char* namespace_name) {
        auto err = nvs_open(namespace_name, NVS_READWRITE, &m_handle);
        assert(err == ESP_OK);
    }

    ~Store() {
        nvs_close(m_handle);
    }

    template<typename T>
    void set(const char* key, const T& value) {
        if constexpr (std::is_same_v<std::string_view, T>) {
            auto error = nvs_set_blob(m_handle, key, value.data(), value.size());
            assert(error == ESP_OK);
        } else if constexpr (std::is_trivially_copyable_v<T>) {
            auto error = nvs_set_blob(m_handle, key, &value, sizeof(value));
            assert(error == ESP_OK);
        } else {
            static_assert(always_false<T>, "Unsupported storage type");
        }

        auto error = nvs_commit(m_handle);
        assert(error == ESP_OK);
    }

    template<typename T>
    T get_or_create(const char* key, const T& default_value) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            alignas(T) std::byte storage[sizeof(T)];
            auto length = sizeof(storage);

            switch (nvs_get_blob(m_handle, key, &storage, &length)) {
            case ESP_OK:
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                set(key, default_value);
                return default_value;
            default:
                std::exit(1);
            }

            return *std::launder(reinterpret_cast<T*>(&storage));
        } else {
            static_assert(always_false<T>, "Unsupported storage type");
        }
    }

    template<typename T>
    std::optional<T> get(const char* key) {
        if constexpr (std::is_same_v<std::string, T>) {
            size_t required_size;
            auto error = nvs_get_blob(m_handle, key, nullptr, &required_size);
            if (error != ESP_OK)
                return std::nullopt;

            std::string value(required_size, '\0');
            error = nvs_get_blob(m_handle, key, value.data(), &required_size);
            if (error != ESP_OK)
                return std::nullopt;

            return value;
        } else if constexpr (std::is_trivially_copyable_v<T>) {
            alignas(T) std::byte storage[sizeof(T)];
            auto length = sizeof(storage);

            auto error = nvs_get_blob(m_handle, key, &storage, &length);
            if (error != ESP_OK)
                return std::nullopt;

            return *std::launder(reinterpret_cast<T*>(&storage));
        } else {
            static_assert(always_false<T>, "Unsupported storage type");
        }
    }

    bool erase(const char* key) {
        if (nvs_erase_key(m_handle, key) != ESP_OK)
            return false;

        auto error = nvs_commit(m_handle);
        assert(error == ESP_OK);
        return true;
    }

private:
    nvs_handle_t m_handle;
};

}
