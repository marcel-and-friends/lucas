#pragma once

#include <algorithm>
#include <cmath>

namespace wxs {

template<typename T>
constexpr T normalize(T value, T min, T max) {
    return (value - min) / (max - min);
}

template<typename T>
constexpr T mix(T value, T min, T lerp_min, T max, T lerp_max) {
    const auto clamped = std::clamp(value, min, max);
    const auto normalized = normalize(clamped, min, max);
    return std::lerp(lerp_min, lerp_max, normalized);
}

}
