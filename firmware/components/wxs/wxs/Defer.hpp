#pragma once

#include <functional>
#include <type_traits>
#include <utility>

namespace wxs {

template<std::invocable FN>
class Defer {
public:
    constexpr explicit Defer(FN&& fn)
        : m_fn(std::move(fn)) {
    }

    constexpr explicit Defer(const FN& fn)
        : m_fn(fn) {
    }

    constexpr ~Defer() noexcept(std::is_nothrow_invocable_v<FN>) {
        std::invoke(m_fn);
    }

    Defer(const Defer&) = delete;
    Defer& operator=(const Defer&) = delete;
    Defer(Defer&&) = delete;
    Defer& operator=(Defer&&) = delete;

private:
    FN m_fn;
};

}
