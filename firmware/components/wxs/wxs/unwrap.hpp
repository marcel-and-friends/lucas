#pragma once

#include <optional>
#include <utility>

namespace wxs {

template<typename T>
constexpr T unwrap(std::optional<T>& optional) {
    return std::exchange(optional, std::nullopt).value();
}

}
