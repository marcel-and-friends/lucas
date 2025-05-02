#pragma once

#include <optional>
#include <utility>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "isr/Queue.hpp"
#include <xf/mem/mem.hpp>
#include <xf/time/time.hpp>

namespace xf::queue {

template<typename Item>
class Queue {
public:
    using Handle = QueueHandle_t;

    // We support non-trivially-copyable items through an indirection backed by a heap allocation
    using StoredItem = std::conditional_t<
        std::is_trivially_copyable_v<Item>,
        Item,
        std::add_pointer_t<Item>>;

    Queue() = default;

    Queue(Queue&&) noexcept;

    Queue& operator=(Queue&&) noexcept;

    ~Queue();

    // There is no mechanism in FreeRTOS to copy a queue
    Queue(const Queue&) = delete;
    Queue operator=(const Queue&) = delete;

    [[nodiscard]] bool create(size_t size);

    void destroy();

    void await_send(const Item&);

    void await_send(Item&&);

    void reset_and_await_send(const Item&);

    void reset_and_await_send(Item&&);

    template<typename Rep, typename Period>
    [[nodiscard]] bool send(const Item&, std::chrono::duration<Rep, Period> timeout);

    template<typename Rep, typename Period>
    [[nodiscard]] bool send(Item&&, std::chrono::duration<Rep, Period> timeout);

    template<typename Rep, typename Period>
    [[nodiscard]] bool send_to_back(const Item&, std::chrono::duration<Rep, Period> timeout);

    template<typename Rep, typename Period>
    [[nodiscard]] bool send_to_back(Item&&, std::chrono::duration<Rep, Period> timeout);

    template<typename Rep, typename Period>
    [[nodiscard]] bool send_to_front(const Item&, std::chrono::duration<Rep, Period> timeout);

    template<typename Rep, typename Period>
    [[nodiscard]] bool send_to_front(Item&&, std::chrono::duration<Rep, Period> timeout);

    [[nodiscard]] bool overwrite(const Item&);

    [[nodiscard]] bool overwrite(Item&&);

    [[nodiscard]] Item await_receive() const;

    template<typename Rep, typename Period>
    [[nodiscard]] std::optional<Item> receive(std::chrono::duration<Rep, Period> timeout) const;

    [[nodiscard]] Item await_peek() const;

    template<typename Rep, typename Period>
    [[nodiscard]] std::optional<Item> peek(std::chrono::duration<Rep, Period> timeout) const;

    void reset();

    [[nodiscard]] Handle raw_handle() const;

    [[nodiscard]] size_t messages_waiting() const;

    [[nodiscard]] size_t spaces_available() const;

    [[nodiscard]] bool is_empty() const;

    [[nodiscard]] bool is_full() const;

    [[nodiscard]] isr::Queue<Item> for_isr();

protected:
    Handle m_handle { nullptr };

private:
    template<typename T, typename Rep, typename Period>
    bool generic_send(T&& item, BaseType_t copy_position, std::chrono::duration<Rep, Period> timeout);
};

template<typename Item>
Queue<Item>::Queue(Queue&& other) noexcept
    : m_handle(std::exchange(other.m_handle, nullptr)) {
}

template<typename Item>
Queue<Item>& Queue<Item>::operator=(Queue&& other) noexcept {
    if (this != &other) {
        if (m_handle)
            destroy();
        m_handle = std::exchange(other.m_handle, nullptr);
    }
    return *this;
}

template<typename Item>
Queue<Item>::~Queue() {
    if (m_handle)
        destroy();
}

template<typename Item>
[[nodiscard]] bool Queue<Item>::create(size_t size) {
    configASSERT(m_handle == nullptr);
    m_handle = xQueueCreate(size, sizeof(StoredItem));
    return m_handle != nullptr;
}

template<typename Item>
void Queue<Item>::destroy() {
    configASSERT(m_handle);
    vQueueDelete(std::exchange(m_handle, nullptr));
}

template<typename Item>
void Queue<Item>::await_send(const Item& item) {
    auto ret = send(item, time::FOREVER);
    configASSERT(ret == true);
}

template<typename Item>
void Queue<Item>::await_send(Item&& item) {
    auto ret = send(std::move(item), time::FOREVER);
    configASSERT(ret == true);
}

template<typename Item>
void Queue<Item>::reset_and_await_send(const Item& item) {
    reset();
    await_send(item);
}

template<typename Item>
void Queue<Item>::reset_and_await_send(Item&& item) {
    reset();
    await_send(std::move(item));
}

template<typename Item>
template<typename Rep, typename Period>
bool Queue<Item>::send(const Item& item, std::chrono::duration<Rep, Period> timeout) {
    return send_to_back(item, timeout);
}

template<typename Item>
template<typename Rep, typename Period>
bool Queue<Item>::send(Item&& item, std::chrono::duration<Rep, Period> timeout) {
    return send_to_back(std::move(item), timeout);
}

template<typename Item>
template<typename Rep, typename Period>
bool Queue<Item>::send_to_back(const Item& item, std::chrono::duration<Rep, Period> timeout) {
    return generic_send(item, queueSEND_TO_BACK, timeout);
}

template<typename Item>
template<typename Rep, typename Period>
bool Queue<Item>::send_to_back(Item&& item, std::chrono::duration<Rep, Period> timeout) {
    return generic_send(std::move(item), queueSEND_TO_BACK, timeout);
}

template<typename Item>
template<typename Rep, typename Period>
bool Queue<Item>::send_to_front(const Item& item, std::chrono::duration<Rep, Period> timeout) {
    return generic_send(item, queueSEND_TO_FRONT, timeout);
}

template<typename Item>
template<typename Rep, typename Period>
bool Queue<Item>::send_to_front(Item&& item, std::chrono::duration<Rep, Period> timeout) {
    return generic_send(std::move(item), queueSEND_TO_FRONT, timeout);
}

template<typename Item>
bool Queue<Item>::overwrite(const Item& item) {
    return generic_send(item, queueOVERWRITE, time::FOREVER);
}

template<typename Item>
bool Queue<Item>::overwrite(Item&& item) {
    return generic_send(std::move(item), queueOVERWRITE, time::FOREVER);
}

template<typename Item>
Item Queue<Item>::await_receive() const {
    return receive(time::FOREVER).value();
}

template<typename Item>
template<typename Rep, typename Period>
std::optional<Item> Queue<Item>::receive(std::chrono::duration<Rep, Period> timeout) const {
    alignas(StoredItem) std::byte buffer[sizeof(StoredItem)];
    if (xQueueReceive(m_handle, &buffer, time::to_raw_tick(timeout)) == pdFALSE)
        return std::nullopt;

    auto& stored_item = *std::launder(reinterpret_cast<StoredItem*>(&buffer));
    if constexpr (std::is_trivially_copyable_v<Item>) {
        return stored_item;
    } else {
        auto result { std::move(*stored_item) };
        mem::destroy(stored_item);
        return result;
    }
}

template<typename Item>
Item Queue<Item>::await_peek() const {
    return peek(time::FOREVER).value();
}

template<typename Item>
template<typename Rep, typename Period>
std::optional<Item> Queue<Item>::peek(std::chrono::duration<Rep, Period> timeout) const {
    alignas(StoredItem) std::byte buffer[sizeof(StoredItem)];
    if (xQueuePeek(m_handle, &buffer, time::to_raw_tick(timeout)) == pdFALSE)
        return std::nullopt;

    auto& stored_item = *std::launder(reinterpret_cast<StoredItem*>(&buffer));
    if constexpr (std::is_trivially_copyable_v<Item>) {
        return stored_item;
    } else {
        return *stored_item;
    }
}

template<typename Item>
void Queue<Item>::reset() {
    xQueueReset(m_handle);
}

template<typename Item>
Queue<Item>::Handle Queue<Item>::raw_handle() const {
    return m_handle;
}

template<typename Item>
size_t Queue<Item>::messages_waiting() const {
    return uxQueueMessagesWaiting(m_handle);
}

template<typename Item>
size_t Queue<Item>::spaces_available() const {
    return uxQueueSpacesAvailable(m_handle);
}

template<typename Item>
bool Queue<Item>::is_empty() const {
    return messages_waiting() == 0;
}

template<typename Item>
bool Queue<Item>::is_full() const {
    return spaces_available() == 0;
}

template<typename Item>
template<typename T, typename Rep, typename Period>
bool Queue<Item>::generic_send(T&& item, BaseType_t copy_position, std::chrono::duration<Rep, Period> timeout) {
    if constexpr (std::is_trivially_copyable_v<Item>) {
        return xQueueGenericSend(m_handle, &item, time::to_raw_tick(timeout), copy_position) == pdTRUE;
    } else {
        auto* new_item = mem::create<Item>(std::forward<T>(item));
        if (new_item == nullptr)
            return false;

        if (xQueueGenericSend(m_handle, &new_item, time::to_raw_tick(timeout), copy_position) == pdTRUE) {
            return true;
        } else {
            // Cleanup the allocation before returning failure
            mem::destroy(new_item);
            return false;
        }
    }
}

template<typename Item>
isr::Queue<Item> Queue<Item>::for_isr() {
    return isr::Queue<Item> { m_handle };
}

}
