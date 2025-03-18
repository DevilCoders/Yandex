#pragma once

#include <util/generic/vector.h>

template <typename T>
TVector<T> Take(const TVector<T>& v, size_t count) {
    TVector<T> r;
    for (size_t i = 0; i < Min(v.size(), count); ++i) {
        r.push_back(v.at(i));
    }
    return r;
}

template <typename T>
TVector<T> Drop(const TVector<T>& v, size_t count) {
    TVector<T> r;
    for (size_t i = count; i < v.size(); ++i) {
        r.push_back(v.at(i));
    }
    return r;
}
