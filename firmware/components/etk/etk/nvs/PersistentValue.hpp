#pragma once

#include <concepts>

#include <etk/nvs/Store.hpp>
#include <etk/util/StringLiteral.hpp>

namespace etk::nvs {

template<std::equality_comparable T, util::StringLiteral KEY>
class PersistentValue {
public:
    static_assert(KEY.size() > 0 && KEY.size() <= NVS_KEY_NAME_MAX_SIZE, "NVS key length is invalid.");

    PersistentValue(Store& store, const auto& default_value)
        : m_store(store)
        , m_cached_value(m_store.get_or_create(KEY, default_value)) {
    }

    PersistentValue() = delete;

    PersistentValue(const PersistentValue&) = delete;
    PersistentValue(PersistentValue&&) = delete;
    PersistentValue& operator=(const PersistentValue&) = delete;
    PersistentValue& operator=(PersistentValue&&) = delete;

    const T* operator->() const {
        return &m_cached_value;
    }

    const T& operator*() const {
        return m_cached_value;
    }

    void store(T value) {
        if (m_cached_value == value)
            return;

        m_cached_value = value;
        m_store.set(KEY, value);
    }

    const T& load() const {
        return m_cached_value;
    }

private:
    Store& m_store;
    T m_cached_value;
};

template<std::equality_comparable T, util::StringLiteral KEY>
class PersistentValue<std::optional<T>, KEY> {
public:
    static_assert(KEY.size() > 0 && KEY.size() <= NVS_KEY_NAME_MAX_SIZE, "NVS key length is invalid.");

    PersistentValue(Store& store, const auto& default_value)
        : m_store(store)
        , m_cached_value(m_store.get_or_create(KEY, default_value)) {
    }

    PersistentValue(Store& store)
        : m_store(store)
        , m_cached_value(std::nullopt) {
    }

    PersistentValue() = delete;

    PersistentValue(const PersistentValue&) = delete;
    PersistentValue(PersistentValue&&) = delete;
    PersistentValue& operator=(const PersistentValue&) = delete;
    PersistentValue& operator=(PersistentValue&&) = delete;

    const std::optional<T>* operator->() const {
        return &m_cached_value;
    }

    const std::optional<T>& operator*() const {
        return m_cached_value;
    }

    void store(std::optional<T> value) {
        if (m_cached_value == value)
            return;

        m_cached_value = std::move(value);

        if (m_cached_value.has_value()) {
            m_store.set(KEY, *m_cached_value);
        } else {
            m_store.erase(KEY);
        }
    }

    const std::optional<T>& load() const {
        return m_cached_value;
    }

private:
    Store& m_store;
    std::optional<T> m_cached_value;
};
}
