#pragma once

#include <array>

template <size_t N, class T>
std::array<T, N> MakeFilledArray(const T& value) {
    std::array<T, N> result;
    result.fill(value);
    return result;
}
