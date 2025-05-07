#pragma once

#include <variant>

namespace wxs {

template<typename V, typename FN>
decltype(auto) visit(V&& v, FN&& fn) {
    return std::visit(std::forward<FN>(fn), std::forward<V>(v));
}

}
