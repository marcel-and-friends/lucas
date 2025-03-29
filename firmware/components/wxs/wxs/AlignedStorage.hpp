#pragma once

#include <memory>

namespace wxs {

template<typename T>
class AlignedStorage {
public:
    AlignedStorage() = default;

    AlignedStorage(const AlignedStorage&) = delete;
    AlignedStorage(AlignedStorage&&) = delete;
    AlignedStorage& operator=(const AlignedStorage&) = delete;
    AlignedStorage& operator=(AlignedStorage&&) = delete;

    T* pointer() {
        return reinterpret_cast<T*>(&m_storage);
    }

    const T* pointer() const {
        return reinterpret_cast<const T*>(&m_storage);
    }

    T& get() {
        return *std::launder(pointer());
    }

    const T& get() const {
        return *std::launder(pointer());
    }

    size_t size() const {
        return sizeof(m_storage);
    }

private:
    alignas(T) std::byte m_storage[sizeof(T)];
};

}
