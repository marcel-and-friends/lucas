#pragma once

#include <chrono>

#include <xf/time/time.hpp>

namespace chrono = std::chrono;

using FloatSeconds = chrono::duration<float>;

namespace util {

constexpr xf::time::Duration saturating_sub(xf::time::Tick a, xf::time::Tick b) {
    return b > a ? xf::time::Duration {} : a - b;
}

constexpr xf::time::Duration saturating_sub(xf::time::Duration a, xf::time::Duration b) {
    return b > a ? xf::time::Duration {} : a - b;
}

}
