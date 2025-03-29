#pragma once

#include <algorithm>
#include <cstddef>

namespace etk::util {

template<size_t N>
struct StringLiteral {
    char _data[N];

    consteval StringLiteral(const char (&s)[N]) {
        std::copy_n(s, N, _data);
    }

    constexpr auto size() const {
        return N;
    }

    constexpr operator const char*() const {
        return _data;
    }
};

}
