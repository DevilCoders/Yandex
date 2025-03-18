#pragma once

#include <util/generic/vector.h>

#include <library/cpp/wordpos/wordpos.h>

class TDocSkip {
public:
    template <class T>
    static Y_FORCE_INLINE bool SkipTo(T& decoder, const char*& cur0, const char* upper, SUPERLONG to) {
        return decoder.SkipToDoc(cur0, upper, to);
    }

    template <class T>
    static Y_FORCE_INLINE bool SmallSkipTo(T& decoder, SUPERLONG to) {
        return decoder.SmallSkipToDoc(i32(to >> DOC_LEVEL_Shift));
    }
};

template <typename X, typename Y>
struct TValuePair {
    X* Pointer;
    Y Value;
    TValuePair() {
        Pointer = nullptr;
        Value = X::MaxValue();
    }
    void Init(X* pointer) {
        Pointer = pointer;
    }
};

template <size_t Size, typename X, typename Y>
struct THoldArrayEx {
    typedef Y value_type;
    TValuePair<X, value_type> Pairs[Size + 1];
    void Resize(size_t size) {
        (void)size;
        assert(size < Size);
    }
};

template <size_t Size, typename X>
struct THoldArray {
    typedef typename X::value_type value_type;
    TValuePair<X, value_type> Pairs[Size + 1];
    void Resize(size_t size) {
        (void)size;
        assert(size < Size);
    }
};

template <typename X>
struct THoldVector {
    typedef typename X::value_type value_type;
    TVector<TValuePair<X, value_type>> Pairs;
    void Resize(size_t size) {
        if (Pairs.size() < size + 1)
            Pairs.resize(size + 1);
    }
};
