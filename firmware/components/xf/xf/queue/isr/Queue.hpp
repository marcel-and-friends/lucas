#pragma once

#include <cstddef>
#include <optional>

#include <xf/isr/isr.hpp>

namespace xf::queue::isr {

template<typename Item>
class Queue {
public:
    static_assert(std::is_trivially_copyable_v<Item>, "Items must be trivially copyable so that no allocation happens inside an ISR.");

    using Handle = QueueHandle_t;

    explicit Queue(Handle);

    [[nodiscard]] std::optional<xf::isr::HigherPriorityTaskWoken> send(const Item&) const;

    [[nodiscard]] std::optional<xf::isr::HigherPriorityTaskWoken> send_to_back(const Item&) const;

    [[nodiscard]] std::optional<xf::isr::HigherPriorityTaskWoken> send_to_front(const Item&) const;

    [[nodiscard]] xf::isr::HigherPriorityTaskWoken overwrite(const Item&);

    struct ReceiveData {
        Item item;
        xf::isr::HigherPriorityTaskWoken higher_priority_task_woken;
    };

    [[nodiscard]] std::optional<ReceiveData> receive();

    [[nodiscard]] std::optional<ReceiveData> peek();

    [[nodiscard]] bool is_empty() const;

    [[nodiscard]] bool is_full() const;

    [[nodiscard]] size_t messages_waiting() const;

private:
    std::optional<xf::isr::HigherPriorityTaskWoken> generic_send(const Item&, BaseType_t copy_position) const;

private:
    QueueHandle_t m_handle;
};

template<typename Item>
Queue<Item>::Queue(QueueHandle_t handle)
    : m_handle(handle) {
}

template<typename Item>
std::optional<xf::isr::HigherPriorityTaskWoken> Queue<Item>::send(const Item& item) const {
    return send_to_back(item);
}

template<typename Item>
std::optional<xf::isr::HigherPriorityTaskWoken> Queue<Item>::send_to_back(const Item& item) const {
    return generic_send(item, queueSEND_TO_BACK);
}

template<typename Item>
std::optional<xf::isr::HigherPriorityTaskWoken> Queue<Item>::send_to_front(const Item& item) const {
    return generic_send(item, queueSEND_TO_FRONT);
}

template<typename Item>
xf::isr::HigherPriorityTaskWoken Queue<Item>::overwrite(const Item& item) {
    // Overwrite is infallible
    return generic_send(item, queueOVERWRITE).value();
}

template<typename Item>
std::optional<typename Queue<Item>::ReceiveData> Queue<Item>::receive() {
    BaseType_t higher_priority_task_woken = pdFALSE;
    alignas(Item) std::byte buffer[sizeof(Item)];
    if (xQueueReceiveFromISR(m_handle, &buffer, &higher_priority_task_woken) != pdTRUE)
        return std::nullopt;

    return { *std::launder(reinterpret_cast<Item*>(&buffer)), higher_priority_task_woken };
}

template<typename Item>
std::optional<typename Queue<Item>::ReceiveData> Queue<Item>::peek() {
    BaseType_t higher_priority_task_woken = pdFALSE;
    alignas(Item) std::byte buffer[sizeof(Item)];
    if (xQueuePeekFromISR(m_handle, &buffer) != pdTRUE)
        return std::nullopt;

    return { *std::launder(reinterpret_cast<Item*>(&buffer)), higher_priority_task_woken };
}

template<typename Item>
bool Queue<Item>::is_empty() const {
    return xQueueIsQueueEmptyFromISR(m_handle) == pdTRUE;
}

template<typename Item>
bool Queue<Item>::is_full() const {
    return xQueueIsQueueFullFromISR(m_handle) == pdTRUE;
}

template<typename Item>
size_t Queue<Item>::messages_waiting() const {
    return uxQueueMessagesWaitingFromISR(m_handle);
}

template<typename Item>
std::optional<xf::isr::HigherPriorityTaskWoken> Queue<Item>::generic_send(const Item& item, BaseType_t copy_position) const {
    BaseType_t higher_priority_task_woken = pdFALSE;
    if (xQueueGenericSendFromISR(m_handle, &item, &higher_priority_task_woken, copy_position) != pdTRUE)
        return std::nullopt;
    return higher_priority_task_woken;
}

}
