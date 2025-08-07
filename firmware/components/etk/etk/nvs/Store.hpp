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

class Store;

namespace detail {

template<typename T>
struct GetImpl;

template<typename T, typename U>
struct GetOrCreateImpl;

}

class Store {
public:
    static error::Expected<Store> make(const char* namespace_name, nvs_open_mode_t);

    ~Store();

    Store(Store&&) noexcept;
    Store& operator=(Store&&) noexcept;

    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;

    /// Retrieve the NVS entry for the given key.
    /// The template argument `T` must either be trivially copyable or one of `std::string` and `std::vector<X>` (where `X` is trivially copyable).
    template<typename T>
    error::Expected<T> get(const char* key) const;

    /// Set the NVS entry for the given key.
    template<typename T>
    requires std::is_trivially_copyable_v<T>
    error::Expected<void> set(const char* key, const T& value);

    /// Set the NVS entry for the given key.
    template<typename T>
    requires std::is_trivially_copyable_v<T>
    error::Expected<void> set(const char* key, std::span<const T> value);

    error::Expected<void> set(const char* key, std::string_view value);

    template<typename T, typename U>
    error::Expected<T> get_or_create(const char* key, U fallback_value);

    error::Expected<void> erase(const char* key);

    error::Expected<void> commit();

private:
    Store(nvs_handle_t);

    void destroy();

    nvs_handle_t m_handle;
};

template<typename T>
error::Expected<T> Store::get(const char* key) const {
    return detail::GetImpl<T>::get(m_handle, key);
}

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

template<typename T, typename U>
error::Expected<T> Store::get_or_create(const char* key, U fallback_value) {
    return detail::GetOrCreateImpl<T, U>::get_or_create(*this, key, fallback_value);
}

namespace detail {

template<typename T>
struct GetImpl {
    static error::Expected<T> get(nvs_handle_t handle, const char* key) {
        static_assert(false, "Invalid template argument given to Store::get. Refer to it's documentation to know more about the types it accepts.");
        std::unreachable();
    }
};

template<typename T, typename U>
struct GetOrCreateImpl {
    static error::Expected<T> get_or_create(Store&, const char*, U) {
        static_assert(false, "Invalid arguments given to Store::get_or_create. Refer to it's documentation to know more about the types it accepts.");
        std::unreachable();
    }
};

template<typename T>
requires std::is_trivially_copyable_v<T>
struct GetImpl<T> {
    static error::Expected<T> get(nvs_handle_t handle, const char* key) {
        alignas(T) std::byte storage[sizeof(T)];
        size_t length = sizeof(storage);

        TRY_RAW(nvs_get_blob(handle, key, &storage, &length));

        assert(length == sizeof(storage));

        return std::bit_cast<T>(storage);
    }
};

template<typename T>
requires std::is_trivially_copyable_v<T> and std::is_default_constructible_v<T>
struct GetImpl<std::vector<T>> {
    static error::Expected<std::vector<T>> get(nvs_handle_t handle, const char* key) {
        size_t required_size;

        TRY_RAW(nvs_get_blob(handle, key, nullptr, &required_size));

        assert(required_size % sizeof(T) == 0);

        std::vector<T> value(required_size / sizeof(T), T {});
        TRY_RAW(nvs_get_blob(handle, key, value.data(), &required_size));

        return value;
    }
};

template<>
struct GetImpl<std::string> {
    static error::Expected<std::string> get(nvs_handle_t handle, const char* key) {
        size_t required_size;

        TRY_RAW(nvs_get_blob(handle, key, nullptr, &required_size));

        std::string value(required_size, '\0');
        TRY_RAW(nvs_get_blob(handle, key, value.data(), &required_size));

        return value;
    }
};

template<typename T, std::convertible_to<T> U>
requires std::is_trivially_copyable_v<T>
struct GetOrCreateImpl<T, U> {
    static error::Expected<T> get_or_create(Store& store, const char* key, const U& fallback_value) {
        auto get = store.get<T>(key);

        if (not get.has_value() and get.error() == ESP_ERR_NVS_NOT_FOUND) {
            TRY(store.set(key, fallback_value));
            return fallback_value;
        }

        return get;
    }
};

template<std::convertible_to<std::string_view> U>
struct GetOrCreateImpl<std::string, U> {
    static error::Expected<std::string> get_or_create(Store& store, const char* key, std::string_view fallback_value) {
        auto get = store.get<std::string>(key);

        if (not get.has_value() and get.error() == ESP_ERR_NVS_NOT_FOUND) {
            TRY(store.set(key, fallback_value));
            return std::string(fallback_value);
        }

        return get;
    }
};

template<typename T, std::convertible_to<std::span<const T>> U>
requires std::is_trivially_copyable_v<T> and std::is_default_constructible_v<T>
struct GetOrCreateImpl<std::vector<T>, U> {
    static error::Expected<std::vector<T>> get_or_create(Store& store, const char* key, std::span<const T> fallback_value) {
        auto get = store.get<std::vector<T>>(key);

        if (not get.has_value() and get.error() == ESP_ERR_NVS_NOT_FOUND) {
            TRY(store.set(key, fallback_value));
            return std::vector(fallback_value.begin(), fallback_value.end());
        }

        return get;
    }
};

}

}
