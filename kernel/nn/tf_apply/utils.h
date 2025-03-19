#pragma once

#include <util/generic/vector.h>
#include <utility>

namespace NTFModel {
    template <typename T>
    TVector<std::remove_reference_t<T>> WrapInVector(T&& obj) {
        TVector<std::remove_reference_t<T>> wrap;
        wrap.reserve(1);
        wrap.push_back(std::forward<T>(obj));
        return wrap;
    }
}
