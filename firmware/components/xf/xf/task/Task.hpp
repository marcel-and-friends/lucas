#pragma once

#include <functional>
#include <utility>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "Notification.hpp"
#include <xf/fn.hpp>
#include <xf/time/time.hpp>

namespace xf::task {

using Handle = TaskHandle_t;

template<std::derived_from<Notification>... Notifications>
class Task {
public:
    static_assert(sizeof...(Notifications) <= configTASK_NOTIFICATION_ARRAY_ENTRIES, "The number of notifications for a task must be less than or equal to \"configTASK_NOTIFICATION_ARRAY_ENTRIES\"");

    Task(size_t notification_index_do_not_override_default_value = 0);

    Task(Task&&) noexcept;

    Task& operator=(Task&&) noexcept;

    virtual ~Task();

    // There is no mechanism in FreeRTOS to copy a task
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    [[nodiscard]] bool create(const char* name, uint32_t stack_depth, UBaseType_t priority);

    [[nodiscard]] bool create(uint32_t stack_depth, UBaseType_t priority);

#if ESP_PLATFORM

    [[nodiscard]] bool create_pinned_to_core(const char* name, uint32_t stack_depth, UBaseType_t priority, BaseType_t core_id);

    [[nodiscard]] bool create_pinned_to_core(uint32_t stack_depth, UBaseType_t priority, BaseType_t core_id);

#endif

    void destroy();

    void suspend();

    void resume();

    [[nodiscard]] UBaseType_t stack_high_mark() const;

    [[nodiscard]] UBaseType_t priority() const;

    void set_priority(UBaseType_t);

    template<size_t I = tskDEFAULT_INDEX_TO_NOTIFY>
    [[nodiscard]] auto& notification();

    template<typename Notification>
    [[nodiscard]] auto& notification();

    [[nodiscard]] Handle raw_handle() const;

protected:
    virtual void setup() { }

    virtual void run() = 0;

    template<typename Rep, typename Period>
    void delay(std::chrono::duration<Rep, Period> duration);

    template<typename Rep, typename Period>
    [[nodiscard]] time::Tick delay_until(time::Tick previous_wake_time, std::chrono::duration<Rep, Period> increment);

    template<typename Rep, typename Period>
    void every(std::chrono::duration<Rep, Period> period, ControlFlowFn auto&& callback);

    static void task(void* raw_self);

protected:
    Handle m_handle { nullptr };

private:
    std::tuple<Notifications...> m_notifications;
};

template<std::derived_from<Notification>... Notifications>
Task<Notifications...>::Task(size_t notification_index)
    : m_notifications {
        Notifications { m_handle, notification_index++ }...
    } { }

template<std::derived_from<Notification>... Notifications>
Task<Notifications...>::Task(Task&& other) noexcept
    : m_handle(std::exchange(other.m_handle, nullptr))
    , m_notifications(std::move(other.m_notifications)) {
}

template<std::derived_from<Notification>... Notifications>
Task<Notifications...>& Task<Notifications...>::operator=(Task&& other) noexcept {
    if (this != &other) {
        if (m_handle)
            destroy();
        m_handle = std::exchange(other.m_handle, nullptr);
        m_notifications = std::move(other.m_notifications);
    }
    return *this;
}

template<std::derived_from<Notification>... Notifications>
Task<Notifications...>::~Task() {
    if (m_handle)
        destroy();
}

template<std::derived_from<Notification>... Notifications>
bool Task<Notifications...>::create(const char* name, uint32_t stack_depth, UBaseType_t priority) {
    configASSERT(m_handle == nullptr);
    bool success = xTaskCreate(task, name, stack_depth, this, priority, &m_handle) == pdPASS;
    if (success) {
    }
    return success;
}

template<std::derived_from<Notification>... Notifications>
bool Task<Notifications...>::create(uint32_t stack_depth, UBaseType_t priority) {
    return create(nullptr, stack_depth, priority);
}

#if ESP_PLATFORM

template<std::derived_from<Notification>... Notifications>
bool Task<Notifications...>::create_pinned_to_core(const char* name, uint32_t stack_depth, UBaseType_t priority, BaseType_t core_id) {
    configASSERT(m_handle == nullptr);
    return xTaskCreatePinnedToCore(task, name, stack_depth, this, priority, &m_handle, core_id) == pdPASS;
}

template<std::derived_from<Notification>... Notifications>
bool Task<Notifications...>::create_pinned_to_core(uint32_t stack_depth, UBaseType_t priority, BaseType_t core_id) {
    return create_pinned_to_core(nullptr, stack_depth, priority, core_id);
}

#endif

template<std::derived_from<Notification>... Notifications>
void Task<Notifications...>::destroy() {
    configASSERT(m_handle);
    vTaskDelete(std::exchange(m_handle, nullptr));
}

template<std::derived_from<Notification>... Notifications>
void Task<Notifications...>::suspend() {
    vTaskSuspend(m_handle);
}

template<std::derived_from<Notification>... Notifications>
void Task<Notifications...>::resume() {
    vTaskResume(m_handle);
}

template<std::derived_from<Notification>... Notifications>
UBaseType_t Task<Notifications...>::stack_high_mark() const {
    return uxTaskGetStackHighWaterMark(m_handle);
}

template<std::derived_from<Notification>... Notifications>
UBaseType_t Task<Notifications...>::priority() const {
    return uxTaskPriorityGet(m_handle);
}

template<std::derived_from<Notification>... Notifications>
void Task<Notifications...>::set_priority(UBaseType_t p) {
    vTaskPrioritySet(m_handle, p);
}

template<std::derived_from<Notification>... Notifications>
template<size_t I>
auto& Task<Notifications...>::notification() {
    return std::get<I>(m_notifications);
}

template<std::derived_from<Notification>... Notifications>
template<typename Notification>
auto& Task<Notifications...>::notification() {
    return std::get<Notification>(m_notifications);
}

template<std::derived_from<Notification>... Notifications>
Handle Task<Notifications...>::raw_handle() const {
    return m_handle;
}

template<std::derived_from<Notification>... Notifications>
template<typename Rep, typename Period>
void Task<Notifications...>::delay(std::chrono::duration<Rep, Period> duration) {
    vTaskDelay(time::to_raw_tick(duration));
}

template<std::derived_from<Notification>... Notifications>
template<typename Rep, typename Period>
time::Tick Task<Notifications...>::delay_until(time::Tick previous_wake_time, std::chrono::duration<Rep, Period> increment) {
    auto raw = previous_wake_time.time_since_epoch().count();
    vTaskDelayUntil(&raw, time::to_raw_tick(increment));
    return time::Tick { time::Duration { raw } };
}

template<std::derived_from<Notification>... Notifications>
template<typename Rep, typename Period>
void Task<Notifications...>::every(std::chrono::duration<Rep, Period> period, ControlFlowFn auto&& callback) {
    auto time = time::now();
    while (true) {
        time = delay_until(time, period);
        if (std::invoke(callback) == ControlFlow::Break)
            break;
    }
}

template<std::derived_from<Notification>... Notifications>
void Task<Notifications...>::task(void* raw_self) {
    auto& self = *static_cast<Task*>(raw_self);

    self.setup();

    self.run();

    self.destroy();
}

}
