#pragma once

#include <concepts>
#include <memory>
#include <utility>

#include <freertos/FreeRTOS.h>

namespace xf::mem {

template<typename T>
T* allocate() {
    return static_cast<T*>(pvPortMalloc(sizeof(T)));
}

inline void deallocate(void* ptr) {
    vPortFree(ptr);
}

template<typename T, typename... Args>
requires std::constructible_from<T, Args...>
T* create(Args&&... args) {
    auto* storage = allocate<T>();
    if (storage == nullptr)
        return nullptr;
    return std::construct_at(storage, std::forward<Args>(args)...);
}

template<typename T>
void destroy(T* ptr) {
    configASSERT(ptr);
    ptr->~T();
    deallocate(ptr);
}

}
