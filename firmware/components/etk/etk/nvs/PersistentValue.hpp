#pragma once

#include <optional>

#include "Store.hpp"

namespace etk::nvs {

template<typename T>
class PersistentValue {
public:
    static error::Expected<PersistentValue> make(Store&, const char* key, const auto& default_value);

    PersistentValue(PersistentValue&&);
    PersistentValue& operator=(PersistentValue&&);

    PersistentValue(const PersistentValue&) = delete;
    PersistentValue& operator=(const PersistentValue&) = delete;

    const T* operator->() const;

    const T& operator*() const;

    error::Expected<void> store(T value);

    const T& load() const;

private:
    PersistentValue(Store&, const char* key, T cached_value);

    Store& m_store;

    const char* m_key;

    T m_cached_value;
};

template<typename T>
class PersistentValue<std::optional<T>> {
public:
    static error::Expected<PersistentValue> make(Store&, const char* key, const auto& default_value);

    static error::Expected<PersistentValue> make(Store&, const char* key);

    PersistentValue(PersistentValue&&);
    PersistentValue& operator=(PersistentValue&&);

    PersistentValue(const PersistentValue&) = delete;
    PersistentValue& operator=(const PersistentValue&) = delete;

    const std::optional<T>* operator->() const;

    const std::optional<T>& operator*() const;

    error::Expected<void> store(std::optional<T> value);

    const std::optional<T>& load() const;

private:
    PersistentValue(Store&, const char* key, std::optional<T> cached_value);

    Store& m_store;

    const char* m_key;

    std::optional<T> m_cached_value;
};

template<typename T>
error::Expected<PersistentValue<T>> PersistentValue<T>::make(Store& store, const char* key, const auto& default_value) {
    auto cached_value = TRY(store.get_or_create(key, default_value));
    TRY(store.commit());
    return PersistentValue(store, key, std::move(cached_value));
}

template<typename T>
PersistentValue<T>::PersistentValue(PersistentValue&& other)
    : m_store(other.m_store)
    , m_key(std::exchange(other.m_key, nullptr))
    , m_cached_value(std::move(other.m_cached_value)) {
}

template<typename T>
PersistentValue<T>& PersistentValue<T>::operator=(PersistentValue&& other) {
    if (this != &other) {
        m_store = other.m_store;
        m_key = std::exchange(other.m_key, nullptr);
        m_cached_value = std::move(other.m_cached_value);
    }
    return *this;
}

template<typename T>
PersistentValue<T>::PersistentValue(Store& store, const char* key, T cached_value)
    : m_store(store)
    , m_key(key)
    , m_cached_value(std::move(cached_value)) {
}

template<typename T>
const T* PersistentValue<T>::operator->() const {
    return &m_cached_value;
}

template<typename T>
const T& PersistentValue<T>::operator*() const {
    return load();
}

template<typename T>
error::Expected<void> PersistentValue<T>::store(T value) {
    if (m_cached_value == value)
        return {};

    m_cached_value = std::move(value);

    TRY(m_store.set(m_key, value));
    TRY(m_store.commit());

    return {};
}

template<typename T>
const T& PersistentValue<T>::load() const {
    return m_cached_value;
}

template<typename T>
error::Expected<PersistentValue<std::optional<T>>> PersistentValue<std::optional<T>>::make(Store& store, const char* key, const auto& default_value) {
    auto cached_value = TRY(store.get_or_create(key, default_value));
    TRY(store.commit());
    return PersistentValue(store, key, std::move(cached_value));
}

template<typename T>
error::Expected<PersistentValue<std::optional<T>>> PersistentValue<std::optional<T>>::make(Store& store, const char* key) {
    if (auto cached_value = store.get(key)) {
        return PersistentValue(store, key, *cached_value);
    } else {
        return PersistentValue(store, key, std::nullopt);
    }
}

template<typename T>
PersistentValue<std::optional<T>>::PersistentValue(PersistentValue&& other)
    : m_store(other.m_store)
    , m_key(std::exchange(other.m_key, nullptr))
    , m_cached_value(std::move(other.m_cached_value)) {
}

template<typename T>
PersistentValue<std::optional<T>>& PersistentValue<std::optional<T>>::operator=(PersistentValue&& other) {
    if (this != &other) {
        m_store = other.m_store;
        m_key = std::exchange(other.m_key, nullptr);
        m_cached_value = std::move(other.m_cached_value);
    }
    return *this;
}

template<typename T>
PersistentValue<std::optional<T>>::PersistentValue(Store& store, const char* key, std::optional<T> cached_value)
    : m_store(store)
    , m_key(key)
    , m_cached_value(std::move(cached_value)) {
}

template<typename T>
const std::optional<T>* PersistentValue<std::optional<T>>::operator->() const {
    return &m_cached_value;
}

template<typename T>
const std::optional<T>& PersistentValue<std::optional<T>>::operator*() const {
    return load();
}

template<typename T>
error::Expected<void> PersistentValue<std::optional<T>>::store(std::optional<T> value) {
    if (m_cached_value == value)
        return {};

    m_cached_value = std::move(value);

    if (m_cached_value.has_value()) {
        TRY(m_store.set(m_key, *m_cached_value));
    } else {
        TRY(m_store.erase(m_key));
    }

    TRY(m_store.commit());

    return {};
}

template<typename T>
const std::optional<T>& PersistentValue<std::optional<T>>::load() const {
    return m_cached_value;
}

}
