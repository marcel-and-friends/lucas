#pragma once

#include <utility>

namespace wxs {

template<typename T>
constexpr T flip(T& value) {
    return std::exchange(value, not value);
}

}
