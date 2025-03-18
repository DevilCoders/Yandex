#pragma once

#include <util/generic/hash_set.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/string/cast.h>

template <typename T>
TString CollectionToString(const T& v) {
    TStringStream r;
    r << "[";
    for (typename T::const_iterator i = v.begin(); i != v.end(); ++i) {
        if (i != v.begin()) {
            r << ", ";
        }
        // very inefficient, thanks to draconian limitations of util
        r << ToString(*i);
    }
    r << "]";
    return r.Str();
}


template <typename T>
TString ToString(const TVector<T> v) {
    return CollectionToString(v);
}

template <typename T>
TString ToString(const THashSet<T> v) {
    return CollectionToString(v);
}

template <typename T>
TString ToString(const TSet<T> v) {
    return CollectionToString(v);
}

