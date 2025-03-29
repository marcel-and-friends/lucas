#pragma once

#include <array>
#include <cassert>
#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace wxs {

namespace detail {
template<std::unsigned_integral auto N>
struct SmallestTypeForImpl {
    using type = std::conditional_t<
        N <= UINT8_MAX, std::uint8_t,
        std::conditional_t<
            N <= UINT16_MAX, std::uint16_t,
            std::conditional_t<
                N <= UINT32_MAX, std::uint32_t,
                uint64_t>>>;
};

template<std::unsigned_integral auto N>
using SmallestTypeFor = typename SmallestTypeForImpl<N>::type;
}

template<typename T, size_t N>
class StaticVector {
public:
    static_assert(N > 0, "Vector must hold at least 1 element");

    // Extensions
    using storage_value_type = std::array<std::byte, sizeof(T)>;
    using storage_type = std::array<storage_value_type, N>;
    using storage_interface_type = std::array<T, N>;

    // From the std::array interface
    using value_type = storage_interface_type::value_type;
    using size_type = detail::SmallestTypeFor<N>; // Small-size optimization
    using difference_type = storage_interface_type::difference_type;
    using reference = storage_interface_type::reference;
    using const_reference = storage_interface_type::const_reference;
    using pointer = storage_interface_type::pointer;
    using const_pointer = storage_interface_type::const_pointer;
    using iterator = storage_interface_type::iterator;
    using const_iterator = storage_interface_type::const_iterator;
    using reverse_iterator = storage_interface_type::reverse_iterator;
    using const_reverse_iterator = storage_interface_type::const_reverse_iterator;

    constexpr StaticVector() = default;

    template<std::same_as<T>... U>
    explicit StaticVector(const T& first, const U&... rest) {
        static_assert(sizeof...(rest) + 1 <= N, "Not enough capacity.");
        m_size = 0;
        push_back(first);
        (push_back(rest), ...);
    }

    template<std::same_as<T>... U>
    explicit StaticVector(T&& first, U&&... rest) {
        static_assert(sizeof...(rest) + 1 <= N, "Not enough capacity.");
        m_size = 0;
        push_back(std::move(first));
        (push_back(std::move(rest)), ...);
    }

    static StaticVector resized(size_type n) {
        StaticVector result;
        // Set `m_size` directly without value-initializing `result` so that the storage past `n` remains uninitialized
        result.m_size = 0;
        result.resize(n);
        return result;
    }

    StaticVector(const StaticVector&) noexcept(std::is_nothrow_copy_constructible_v<T>)
    requires(std::is_trivially_copy_constructible_v<T>)
    = default;

    template<size_t OTHER_N>
    StaticVector(const StaticVector<T, OTHER_N>& other) noexcept(std::is_nothrow_copy_constructible_v<T>)
    requires(not std::is_trivially_copy_constructible_v<T> and std::is_copy_constructible_v<T>)
    {
        static_assert(OTHER_N <= N, "Not enough capacity.");
        m_size = 0;
        for (const auto& v : other)
            push_back(v);
    }

    StaticVector& operator=(const StaticVector&) noexcept(std::is_nothrow_copy_assignable_v<T>)
    requires(std::is_trivially_copy_assignable_v<T>)
    = default;

    template<size_t OTHER_N>
    StaticVector& operator=(const StaticVector<T, OTHER_N>& other) noexcept(std::is_nothrow_copy_assignable_v<T> and std::is_nothrow_copy_constructible_v<T> and std::is_nothrow_destructible_v<T>)
    requires(not std::is_trivially_copy_assignable_v<T> and std::is_copy_assignable_v<T> and std::is_copy_constructible_v<T>)
    {
        static_assert(OTHER_N <= N, "Not enough capacity.");
        return assignment_impl(other);
    }

    StaticVector(StaticVector&&)
    requires(std::is_trivially_move_constructible_v<T>)
    = default;

    template<size_t OTHER_N>
    StaticVector(StaticVector<T, OTHER_N>&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
    requires(not std::is_trivially_move_constructible_v<T> and std::is_move_constructible_v<T>)
    {
        static_assert(OTHER_N <= N, "Not enough capacity.");
        m_size = 0;
        for (auto& v : other)
            push_back(std::move(v));
        other.m_size = 0;
    }

    StaticVector& operator=(StaticVector&&) noexcept(std::is_nothrow_move_assignable_v<T>)
    requires(std::is_trivially_move_assignable_v<T>)
    = default;

    template<size_t OTHER_N>
    StaticVector& operator=(StaticVector<T, OTHER_N>&& other) noexcept(std::is_nothrow_move_assignable_v<T> and std::is_nothrow_move_constructible_v<T> and std::is_nothrow_destructible_v<T>)
    requires(not std::is_trivially_move_assignable_v<T> and std::is_move_assignable_v<T> and std::is_move_constructible_v<T>)
    {
        static_assert(OTHER_N <= N, "Not enough capacity");
        auto& self = assignment_impl(other);
        other.m_size = 0;
        return self;
    }

    ~StaticVector() noexcept(std::is_nothrow_destructible_v<T>)
    requires(std::is_trivially_destructible_v<T>)
    = default;

    ~StaticVector() noexcept(std::is_nothrow_destructible_v<T>)
    requires(not std::is_trivially_destructible_v<T>)
    {
        clear();
    }

    void push_back(const T& item) noexcept(std::is_nothrow_copy_constructible_v<T>)
    requires(std::is_copy_constructible_v<T>)
    {
        assert(m_size < max_size());

        std::construct_at(storage_at(m_size), item);
        ++m_size;
    }

    void push_back(T&& item) noexcept(std::is_nothrow_move_constructible_v<T>)
    requires(std::is_move_constructible_v<T>)
    {
        assert(m_size < max_size());

        std::construct_at(storage_at(m_size), std::move(item));
        ++m_size;
    }

    template<typename... Args>
    reference emplace_back(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    requires(std::is_constructible_v<T, Args...>)
    {
        assert(m_size < max_size());

        std::construct_at(storage_at(m_size), std::forward<Args>(args)...);
        ++m_size;

        return value_at(m_size - 1);
    }

    void pop_back() noexcept(std::is_nothrow_destructible_v<T>) {
        assert(m_size > 0);

        value_at(m_size).~T();
        --m_size;
    }

    void resize(size_t n) noexcept(std::is_nothrow_destructible_v<T> and std::is_nothrow_default_constructible_v<T>) {
        assert(n <= max_size());

        const auto size_backup = m_size;
        if (n > size_backup) {
            for (size_type i = 0; i < n - size_backup; ++i)
                emplace_back();
        } else if (n < size_backup) {
            for (size_type i = 0; i < size_backup - n; ++i)
                pop_back();
        }
    }

    reference operator[](size_t i) noexcept {
        return at(i);
    }

    const_reference operator[](size_t i) const noexcept {
        return at(i);
    }

    pointer data() noexcept {
        return storage_interface().data();
    }

    const_pointer data() const noexcept {
        return storage_interface().data();
    }

    reference at(size_t i) {
        assert(i < m_size);
        return value_at(i);
    }

    const_reference at(size_t i) const {
        assert(i < m_size);
        return value_at(i);
    }

    void clear() noexcept(std::is_nothrow_destructible_v<T>) {
        if (m_size == 0)
            return;

        for (size_type i = 0; i < m_size; ++i)
            std::destroy_at(&value_at(i));

        m_size = 0;
    }

    iterator begin() noexcept {
        return storage_interface().begin();
    }

    const_iterator begin() const noexcept {
        return storage_interface().begin();
    }

    const_iterator cbegin() const noexcept {
        return storage_interface().cbegin();
    }

    iterator end() noexcept {
        return begin() + m_size;
    }

    const_iterator end() const noexcept {
        return begin() + m_size;
    }

    const_iterator cend() const noexcept {
        return cbegin() + m_size;
    }

    reverse_iterator rbegin() noexcept {
        return storage_interface().rbegin();
    }

    const_reverse_iterator rbegin() const noexcept {
        return rbegin() + m_size;
    }

    reverse_iterator rend() noexcept {
        return rbegin() + m_size;
    }

    const_reverse_iterator rend() const noexcept {
        return rbegin() + m_size;
    }

    bool empty() const noexcept {
        return m_size == 0;
    }

    size_type size() const noexcept {
        return m_size;
    }

    size_type max_size() const noexcept {
        return N;
    }

    size_type capacity() const noexcept {
        return N;
    }

    void reserve(size_t n) noexcept {
        assert(n <= max_size());
        // nop
        return;
    }

    void shrink_to_fit() const noexcept {
        // nop
        return;
    }

    // These are friend so that they can access `m_size` to have a vector in a valid state while
    // keeping the backing storage uninitialized
    template<typename OTHER_T, size_t OTHER_N>
    friend StaticVector<OTHER_T, OTHER_N> to_static_vector(OTHER_T (&array)[OTHER_N]);

    template<typename OTHER_T, size_t OTHER_N>
    friend StaticVector<OTHER_T, OTHER_N> to_static_vector(OTHER_T (&&array)[OTHER_N]);

private:
    pointer storage_at(size_t i) {
        return reinterpret_cast<pointer>(&m_storage[i]);
    }

    const_pointer storage_at(size_t i) const {
        return reinterpret_cast<const_pointer>(&m_storage[i]);
    }

    reference value_at(size_t i) {
        return *std::launder(storage_at(i));
    }

    const_reference value_at(size_t i) const {
        return *std::launder(storage_at(i));
    }

    storage_interface_type& storage_interface() {
        return *std::launder(reinterpret_cast<storage_interface_type*>(&m_storage));
    }

    const storage_interface_type& storage_interface() const {
        return *std::launder(reinterpret_cast<const storage_interface_type*>(&m_storage));
    }

    StaticVector& assignment_impl(auto&& other) {
        if (this == &other)
            return *this;

        if (other.m_size == 0) {
            clear();
            return *this;
        }

        // Copy the elements that both vectors are guaranteed to have...
        const auto min = std::min(m_size, other.m_size);

        for (size_type i = 0; i < min; ++i)
            value_at(i) = std::forward(other[i]);

        const auto delta = m_size > other.m_size ? m_size - other.m_size : other.m_size - m_size;
        if (m_size < other.m_size) {
            for (size_type i = 0; i < delta; i++)
                // ...now construct (not assign, since they aren't initialized) the remaining.
                push_back(std::forward(other[min + i]));
        } else if (m_size > other.m_size) {
            for (size_type i = 0; i < delta; ++i)
                // ...now destroy the leftovers.
                pop_back();
        }

        return *this;
    }

    alignas(storage_interface_type) storage_type m_storage;
    size_type m_size;

    // Move-assignment/construction requires setting ther other vector's `m_size` variable to 0,
    // and since assigning/constructing a vector from one with a smaller capacity is allowed
    // we need to have every capacity-differing instatiations of `StaticVector<T>` friended,
    // otherwise their `m_size` variable is not accessible.
    // Ideally this should be a partial-specialization, something like:
    // ```
    // template<size_t OTHER_N>
    // friend class StaticVector<T, OTHER_N>;
    // ```
    // but that is unfortunately not allowed.
    template<typename OTHER_T, size_t OTHER_N>
    friend class StaticVector;
};

template<typename T, std::same_as<T>... U>
StaticVector(T, U...) -> StaticVector<T, sizeof...(U) + 1>;

template<typename T, size_t N>
StaticVector<T, N> to_static_vector(T (&array)[N]) {
    StaticVector<std::remove_cv_t<T>, N> result;
    result.m_size = 0;
    [&]<size_t... I>(std::index_sequence<I...>) {
        (result.push_back(array[I]), ...);
    }(std::make_index_sequence<N> {});
    return result;
}

template<typename T, size_t N>
StaticVector<T, N> to_static_vector(T (&&array)[N]) {
    StaticVector<std::remove_cv_t<T>, N> result;
    result.m_size = 0;
    [&]<size_t... I>(std::index_sequence<I...>) {
        (result.push_back(std::move(array[I])), ...);
    }(std::make_index_sequence<N> {});
    return result;
}

}
