#pragma once

#include <util/generic/hash_set.h>
#include <util/generic/vector.h>

#include <algorithm>
#include <iterator>

template <typename T>
static TVector<T> MakeVector() {
    TVector<T> r;
    return r;
}

template <typename T>
static TVector<T> MakeVector(const T& t0) {
    TVector<T> r;
    r.push_back(t0);
    return r;
}

template <typename T>
static TVector<T> MakeVector(const T& t0, const T& t1) {
    TVector<T> r;
    r.push_back(t0);
    r.push_back(t1);
    return r;
}

template <typename T>
static TVector<T> MakeVector(const T& t0, const T& t1, const T& t2) {
    TVector<T> r;
    r.push_back(t0);
    r.push_back(t1);
    r.push_back(t2);
    return r;
}

template <typename T>
static TVector<T> MakeVector(const T& t0, const T& t1, const T& t2, const T& t3) {
    TVector<T> r;
    r.push_back(t0);
    r.push_back(t1);
    r.push_back(t2);
    r.push_back(t3);
    return r;
}

template <typename T>
static TVector<T> MakeVector(const T& t0, const T& t1, const T& t2, const T& t3, const T& t4) {
    TVector<T> r;
    r.push_back(t0);
    r.push_back(t1);
    r.push_back(t2);
    r.push_back(t3);
    r.push_back(t4);
    return r;
}

template <typename T>
static TVector<T> MakeVector(const T& t0, const T& t1, const T& t2, const T& t3, const T& t4, const T& t5) {
    TVector<T> r;
    r.push_back(t0);
    r.push_back(t1);
    r.push_back(t2);
    r.push_back(t3);
    r.push_back(t4);
    r.push_back(t5);
    return r;
}

template <typename T>
static TVector<T> Concat(const TVector<T>& v0, const TVector<T>& v1) {
    TVector<T> r;
    for (typename TVector<T>::const_iterator i = v0.begin(); i != v0.end(); ++i) {
        r.push_back(*i);
    }
    for (typename TVector<T>::const_iterator i = v1.begin(); i != v1.end(); ++i) {
        r.push_back(*i);
    }
    return r;
}

template <typename T>
static void PushBackAll(TVector<T>& v, const TVector<T>& addenum) {
    std::copy(addenum.begin(), addenum.end(), std::back_inserter(v));
}

template <typename T>
static TVector<T> StableUnique(const TVector<T>& v) {
    TVector<T> r;
    THashSet<T> set;
    for (typename TVector<T>::const_iterator i = v.begin(); i != v.end(); ++i) {
        if (set.insert(*i).second) {
            r.push_back(*i);
        }
    }
    return r;
}

template <typename T>
static TVector<T> Sorted(const TVector<T>& v) {
    TVector<T> r = v;
    std::sort(r.begin(), r.end());
    return r;
}

template <typename T>
static bool Contains(const TVector<T>& v, const T& e) {
    return std::find(v.begin(), v.end(), e) != v.end();
}
