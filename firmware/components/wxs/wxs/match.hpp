// Credits to https://www.reddit.com/r/cpp/comments/1b3pj0g/comment/ksu30rs/?utm_source=share&utm_medium=web3x&utm_name=web3xcss&utm_term=1&utm_content=share_button
#pragma once

#include <type_traits>
#include <variant>

namespace wxs {

#if defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning(disable : 4584)
#else
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Winaccessible-base"
#endif

template<typename... Ts>
struct overload : Ts... {
    using Ts::operator()...;
};

template<typename... Ts>
overload(Ts...) -> overload<Ts...>;

#if defined(_MSC_VER)
#    pragma warning(pop)
#else
#    pragma GCC diagnostic pop
#endif

namespace detail {
template<typename F>
concept generic_overload = !requires { &F::operator(); };

template<typename R, typename C, typename T>
consteval auto param_is_impl(R (C::*)(T) const) -> std::decay_t<T>;

template<typename F, typename T>
concept param_is = requires { { param_is_impl(&F::operator()) } -> std::same_as<T>; };

template<typename F, typename T>
concept overloads_type = true and (param_is<F, std::decay_t<T>> or generic_overload<F>) and (std::invocable<F, T>);

template<typename F, typename... Ts>
consteval auto check_overload_set() {
    static_assert((... or !std::invocable<F, Ts>), "overload [F] matches no alternative [Ts]");
}

template<typename T, typename... Fs>
consteval auto check_alternatives() {
    static_assert((... or overloads_type<Fs, T>), "alternative [T] matches no overload [Fs]");
}

template<typename F, typename... Fs>
using block_fn = overload<overload<F>, Fs...>;

template<typename T, typename U>
using copy_ref = std::conditional_t<std::is_lvalue_reference_v<T>, U&, U&&>;

template<typename... Fs, typename... Ts, typename V>
consteval void match_impl(const std::variant<Ts...>&, V&&) noexcept(noexcept(
    (..., check_overload_set<block_fn<Fs, Fs...>, copy_ref<V, Ts>...>()),
    (..., check_alternatives<copy_ref<V, Ts>, Fs...>())));
}

#define WXS_FWD(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)

constexpr decltype(auto) match(auto&& v, auto&&... fs) noexcept(noexcept((detail::match_impl<std::decay_t<decltype(fs)>...>(v, WXS_FWD(v))))) {
    if constexpr (sizeof...(fs) > 1) {
        return std::visit(overload { WXS_FWD(fs)... }, WXS_FWD(v));
    } else {
        return std::visit(WXS_FWD(fs)..., WXS_FWD(v));
    }
}

#undef WXS_FWD

}
