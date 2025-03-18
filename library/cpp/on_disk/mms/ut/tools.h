#pragma once

#include "align_tools.h"
#include "write_to_blob.h"

#include <library/cpp/on_disk/mms/cast.h>
#include <library/cpp/on_disk/mms/map.h>
#include <library/cpp/on_disk/mms/set.h>
#include <library/cpp/on_disk/mms/string.h>
#include <library/cpp/on_disk/mms/vector.h>
#include <library/cpp/on_disk/mms/writer.h>

#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <utility>

class TMmsObjects {
public:
    template <class K, class V, template <class> class Cmp>
    NMms::TMapType<NMms::TMmapped, K, V, Cmp> MakeMmappedMap(const TMap<K, V, Cmp<K>>& m) {
        NMms::TMapType<NMms::TStandalone, K, V, Cmp> mmsMap(m.begin(), m.end());
        AllBlobs.push_back(UnsafeWriteToBlob(mmsMap));
        return Cast<NMms::TMapType<NMms::TMmapped, K, V, Cmp>>(AllBlobs.back());
    }

    template <class T, template <class> class Cmp>
    NMms::TSetType<NMms::TMmapped, T, Cmp> MakeMmappedSet(const TSet<T, Cmp<T>>& s) {
        NMms::TSetType<NMms::TStandalone, T, Cmp> mmsSet(s.begin(), s.end());
        AllBlobs.push_back(UnsafeWriteToBlob(mmsSet));
        return Cast<NMms::TSetType<NMms::TMmapped, T, Cmp>>(AllBlobs.back());
    }

    template <class T>
    NMms::TVectorType<NMms::TMmapped, T> MakeMmappedVector(const TVector<T>& v) {
        NMms::TVectorType<NMms::TStandalone, T> mmsVector(v.begin(), v.end());
        AllBlobs.push_back(UnsafeWriteToBlob(mmsVector));
        return Cast<NMms::TVectorType<NMms::TMmapped, T>>(AllBlobs.back());
    }

    NMms::TStringType<NMms::TMmapped> MakeMmappedString(const TString& s) {
        NMms::TStringType<NMms::TStandalone> mmsString(s);
        AllBlobs.push_back(UnsafeWriteToBlob(mmsString));
        return Cast<NMms::TStringType<NMms::TMmapped>>(AllBlobs.back());
    }

private:
    TVector<TBlob> AllBlobs;
};

inline size_t AlignedSize(size_t size) {
    if (size == 0)
        return 0;
    return ((size - 1) / sizeof(void*) + 1) * sizeof(void*);
}

template <class T>
struct GenT {
    T operator()(unsigned seed) const {
        return static_cast<T>(seed * 17);
    }
};

template <>
struct GenT<bool> {
    bool operator()(unsigned seed) const {
        return (seed * 17) % 15 >= 7;
    }
};

template <class T>
inline TVector<T> GenVector(unsigned seed, size_t size) {
    TVector<T> result;
    for (size_t i = 0; i < size; ++i) {
        result.push_back(GenT<T>()(seed));
        seed *= 189;
    }
    return result;
}

template <class T>
inline TSet<T> GenSet(unsigned seed, size_t size) {
    TSet<T> result;
    for (size_t i = 0; i < size; ++i) {
        result.insert(GenT<T>()(seed));
        seed *= 189;
    }
    return result;
}

template <class K, class V>
inline TMap<K, V> GenMap(unsigned seed, size_t size) {
    TMap<K, V> result;
    for (size_t i = 0; i < size; ++i) {
        result.insert(std::pair<K, V>(GenT<K>()(seed), GenT<V>()(seed * 19)));
        seed *= 185;
    }
    return result;
}

template <class T>
struct LexicographicalCompare {
    bool operator()(const T& t1, const T& t2) const {
        return std::lexicographical_compare(t1.begin(), t1.end(),
                                            t2.begin(), t2.end());
    }
};
