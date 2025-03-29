#pragma once

#include <type_traits>

#include <etk/nvs/Store.hpp>
#include <etk/util/StringLiteral.hpp>

namespace etk::nvs {

template<std::equality_comparable T, util::StringLiteral KEY>
class PersistentValue {
public:
    static_assert(KEY.size() <= NVS_KEY_NAME_MAX_SIZE, "Key is too big");

    PersistentValue(Store& store, T default_value)
        : m_store(store)
        , m_cached_value(m_store.get_or_create(KEY, default_value)) {
    }

    PersistentValue() = delete;

    PersistentValue(const PersistentValue&) = delete;
    PersistentValue(PersistentValue&&) = delete;
    PersistentValue& operator=(const PersistentValue&) = delete;
    PersistentValue& operator=(PersistentValue&&) = delete;

    void store(T value) {
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

}
