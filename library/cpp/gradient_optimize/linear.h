#pragma once

#include <util/generic/vector.h>
#include <utility>

#include <util/stream/output.h>

template <class T>
struct TVectorType : TVector<T> {
    explicit TVectorType(int size = 0, T val = T())
        : TVector<T>(size, val)
    {
    }

    template <template <class...> class V, class S, class... R>
    explicit TVectorType(const V<S, R...>& v) {
        auto& self = *this;
        self.yresize(v.ysize());
        for (int i = 0; i < self.ysize(); i++) {
            self[i] = T(v[i]);
        }
    }
};

template <class T, class S>
TVectorType<T> operator*(S c, TVectorType<T>&& v) {
    for (int i = 0; i < v.ysize(); i++) {
        v[i] = c * v[i];
    }
    return v;
}

template <class T, class S>
TVectorType<T> operator*(S c, const TVectorType<T>& v0) {
    TVectorType<T> v = v0;
    return c * std::move(v);
}

template <class T, class S>
TVectorType<T>& operator*=(TVectorType<T>& v, S c) {
    for (int i = 0; i < v.ysize(); i++) {
        v[i] *= c;
    }
    return v;
}

template <class T>
TVectorType<T> operator+(TVectorType<T>&& v1, const TVectorType<T>& v2) {
    for (int i = 0; i < v1.ysize(); i++) {
        v1[i] += v2[i];
    }
    return v1;
}

template <class T>
TVectorType<T> operator+(const TVectorType<T>& v1, TVectorType<T>&& v2) {
    for (int i = 0; i < v1.ysize(); i++) {
        v2[i] += v1[i];
    }
    return v2;
}

template <class T>
TVectorType<T> operator+(TVectorType<T>&& v1, TVectorType<T>&& v2) {
    return std::move(v1) + v2;
}

template <class T>
TVectorType<T> operator+(const TVectorType<T>& v1, const TVectorType<T>& v2) {
    TVectorType<T> v = v1;
    return std::move(v) + v2;
}

template <class T>
TVectorType<T>& operator+=(TVectorType<T>& v1, const TVectorType<T>& v2) {
    for (int i = 0; i < v1.ysize(); i++) {
        v1[i] += v2[i];
    }
    return v1;
}

template <class T>
TVectorType<T> operator-(TVectorType<T>&& v1, const TVectorType<T>& v2) {
    for (int i = 0; i < v1.ysize(); i++) {
        v1[i] -= v2[i];
    }
    return v1;
}

template <class T>
TVectorType<T> operator-(TVectorType<T>&& v1, TVectorType<T>&& v2) {
    return std::move(v1) - v2;
}

template <class T>
TVectorType<T> operator-(const TVectorType<T>& v1, TVectorType<T>&& v2) {
    for (int i = 0; i < v1.ysize(); i++) {
        v2[i] = v1[i] - v2[i];
    }
    return v2;
}

template <class T>
TVectorType<T> operator-(const TVectorType<T>& v1, const TVectorType<T>& v2) {
    TVectorType<T> v = v1;
    return std::move(v) - v2;
}

template <class T>
TVectorType<T>& operator-=(TVectorType<T>& v1, const TVectorType<T>& v2) {
    for (int i = 0; i < v1.ysize(); i++) {
        v1[i] -= v2[i];
    }
    return v1;
}

template <class T>
T operator%(const TVectorType<T>& v1, const TVectorType<T>& v2) {
    T r = 0;
    for (int i = 0; i < v1.ysize(); i++) {
        r += v1[i] * v2[i];
    }
    return r;
}

template <class T>
IOutputStream& operator<<(IOutputStream& os, const TVectorType<T>& v) {
    for (int i = 0; i < v.ysize(); i++) {
        os << v[i] << " ";
    }
    return os;
}
