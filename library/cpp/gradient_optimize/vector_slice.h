#pragma once

#include "linear.h"
#include <util/system/types.h>

template <class TTFloat>
struct TVectorSlicer {
    int Offset = 0;
    TVector<TTFloat>& Data;

    TVectorSlicer() = default;

    TVectorSlicer(TVector<TTFloat>& data)
        : Data(data)
    {
    }

    int Slice(int size) {
        int offset = Offset;
        Offset += size;
        return offset;
    }
};

template <class TTFloat>
struct TVectorSlice {
    TVector<TTFloat>& Data;
    int Offset;
    int Size;

    TVectorSlice() = default;

    TVectorSlice(TVector<TTFloat>& data, int offset, int size)
        : Data(data)
        , Offset(offset)
        , Size(size)
    {
    }

    TVectorSlice(TVectorSlicer<TTFloat>& slicer, int size)
        : Data(slicer.Data)
        , Offset(slicer.Slice(size))
        , Size(size)
    {
    }

    int ysize() const {
        return Size;
    }

    TTFloat& operator[](int idx) {
        return Data[Offset + idx];
    }

    const TTFloat& operator[](int idx) const {
        return Data[Offset + idx];
    }

    operator TVectorType<TTFloat>() const {
        auto& self = *this;
        TVectorType<TTFloat> v(Size);
        for (int i = 0; i < Size; i++) {
            v[i] = self[i];
        }
        return v;
    }

    // operator= from TVector or TVectorSlice
    template <template <class...> class TTVector, class... R>
    TVectorSlice<TTFloat>& operator=(const TTVector<TTFloat, R...>& v) {
        auto& self = *this;
        Y_VERIFY(v.ysize() == Size, "size mismatch in vector slice assignment: %" PRISZT " != %d", v.size(), Size);
        for (int i = 0; i < Size; i++) {
            self[i] = v[i];
        }
        return self;
    }

    typedef const TTFloat* const_iterator;
    const_iterator begin() const {
        return &Data[Offset];
    }
    const_iterator end() const {
        return begin() + Size;
    }
};

template <class T>
auto VectorMax(const T& v) -> decltype((v[0])) {
    auto x = &v[0];
    for (int i = 1; i < Size(v); i++) {
        if (*x < v[i]) {
            x = &v[i];
        }
    }
    return *x;
}

template <class TTFloat>
struct TMatrixSlice {
    TVector<TTFloat>& Data;
    int Offset;
    int Size[2];

    TMatrixSlice() = default;

    TMatrixSlice(TVectorSlicer<TTFloat>& slicer, int size0, int size1)
        : Data(slicer.Data)
        , Offset(slicer.Slice(size0 * size1))
    {
        Size[0] = size0;
        Size[1] = size1;
    }

    TMatrixSlice<TTFloat>& operator=(const TVectorType<TVectorType<TTFloat>>& v) {
        auto& self = *this;
        Y_VERIFY(v.ysize() == Size[0], "size mismatch in matrix slice assignment: %d != %d", v.ysize(), Size[0]);
        for (int i = 0; i < Size[0]; i++) {
            self[i] = v[i]; // vector slice assignment
        }
        return self;
    }

    const TVectorSlice<TTFloat> operator[](int idx) const {
        return TVectorSlice<TTFloat>(Data, Offset + idx * Size[1], Size[1]);
    }

    TVectorSlice<TTFloat> operator[](int idx) {
        return TVectorSlice<TTFloat>(Data, Offset + idx * Size[1], Size[1]);
    }

    int ysize() const {
        return Size[0];
    }

    struct const_iterator {
        TMatrixSlice<TTFloat>& Matrix;
        int Idx;
        bool operator!=(const_iterator it) const {
            return Idx != it.Idx;
        }
        void operator++() {
            Idx++;
        }
        const TVectorSlice<TTFloat> operator*() const {
            return Matrix[Idx];
        }
    };
    const_iterator begin() const {
        return {*this, 0};
    }
    const_iterator end() const {
        return {*this, Size[0]};
    }
};

template <class T>
IOutputStream& operator<<(IOutputStream& os, const TVectorSlice<T>& v) {
    for (int i = 0; i < v.Size; i++) {
        if (i)
            os << "\t";
        os << v[i];
    }
    return os;
}
