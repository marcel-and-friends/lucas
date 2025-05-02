#pragma once

#include "Task.hpp"

namespace xf::task {

template<size_t STACK_DEPTH, std::derived_from<Notification>... Notifications>
class StaticTask : public Task<Notifications...> {
public:
    static_assert(STACK_DEPTH * sizeof(StackType_t) >= configMINIMAL_STACK_SIZE);

    void create(const char* name, UBaseType_t priority);

    void create(UBaseType_t priority);

#if ESP_PLATFORM

    void create_pinned_to_core(const char* name, UBaseType_t priority, BaseType_t core_id);

    void create_pinned_to_core(UBaseType_t priority, BaseType_t core_id);

#endif

private:
    // Hide the visibility of the `create` function from the base class since that has a stack size parameter, which we take as a template argument, and replace them with our overloads.
    using Task<Notifications...>::create;
    using Task<Notifications...>::create_pinned_to_core;

    StaticTask_t m_task_buffer;
    std::array<StackType_t, STACK_DEPTH> m_stack_buffer;
};

template<size_t STACK_DEPTH, std::derived_from<Notification>... Notifications>
void StaticTask<STACK_DEPTH, Notifications...>::create(const char* name, UBaseType_t priority) {
    configASSERT(this->m_handle == nullptr);
    this->m_handle = xTaskCreateStatic(Task<Notifications...>::task, name, STACK_DEPTH, this, priority, m_stack_buffer.data(), &m_task_buffer);
}

template<size_t STACK_DEPTH, std::derived_from<Notification>... Notifications>
void StaticTask<STACK_DEPTH, Notifications...>::create(UBaseType_t priority) {
    create(nullptr, priority);
}

#if ESP_PLATFORM

template<size_t STACK_DEPTH, std::derived_from<Notification>... Notifications>
void StaticTask<STACK_DEPTH, Notifications...>::create_pinned_to_core(const char* name, UBaseType_t priority, BaseType_t core_id) {
    configASSERT(this->m_handle == nullptr);
    this->m_handle = xTaskCreateStaticPinnedToCore(Task<Notifications...>::task, name, STACK_DEPTH, this, priority, m_stack_buffer.data(), &m_task_buffer, core_id);
}

template<size_t STACK_DEPTH, std::derived_from<Notification>... Notifications>
void StaticTask<STACK_DEPTH, Notifications...>::create_pinned_to_core(UBaseType_t priority, BaseType_t core_id) {
    create_pinned_to_core(nullptr, priority, core_id);
}

#endif

}
