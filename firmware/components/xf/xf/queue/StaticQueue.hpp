#pragma once

#include "Queue.hpp"

namespace xf::queue {

template<typename Item, size_t SIZE>
class StaticQueue : public Queue<Item> {
public:
    static_assert(SIZE > 0, "Static queue size must be at least 1");

    void create() {
        configASSERT(this->m_handle == nullptr);
        this->m_handle = xQueueCreateStatic(SIZE, sizeof(typename Queue<Item>::StoredItem), m_static_storage.data(), &m_static_queue);
    }

private:
    using Queue<Item>::create;

    StaticQueue_t m_static_queue;
    std::array<std::uint8_t, SIZE * sizeof(typename Queue<Item>::StoredItem)> m_static_storage;
};

}
