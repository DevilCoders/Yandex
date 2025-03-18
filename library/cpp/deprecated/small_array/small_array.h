#pragma once

#include <util/system/defaults.h>
#include <util/ysaveload.h>

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>

#define offsetoffield(f) ((char*)&f - (char*)this)
#pragma pack(1)
template <class T, ui16 N>
class Y_PACKED TSmallArray {
private:
    static const bool NLessThan256 = N < 256;

public:
    using TSize = std::conditional_t<NLessThan256, ui8, ui16>;

private:
    friend class TSerializer<TSmallArray<T, N>>;

    TSize Size;
    T Data[N];

public:
    TSmallArray()
        : Size(0)
    {
    }

    TSmallArray(const T* data, TSize size) {
        assign(data, data + size);
    }

    TSmallArray(const TSmallArray& smallArray) {
        assign(smallArray.Data, smallArray.Data + smallArray.Size);
    }

    TSmallArray& operator=(const TSmallArray& rhs) {
        assign(rhs.Data, rhs.Data + rhs.Size);
        return *this;
    }

    template <class TIter>
    void assign(TIter begin, TIter end) {
        Y_ASSERT(end - begin <= N);
        Copy(begin, end, Data);
        Size = end - begin;
    }

    void assign(TSize size, const T& value) {
        Y_ASSERT(size <= N);
        ::Fill(Data, Data + size, value);
        Size = size;
    }

    const T* data() const {
        return Data;
    }

    TSize size() const {
        return Size;
    }

    static TSize capacity() {
        return N;
    }

    void resize(TSize size) {
        Y_ASSERT(size <= N);
        Size = size;
    }

    void push_back(const T& v) {
        Y_ASSERT(Size < N);
        Data[Size++] = v;
    }

    T& operator[](TSize i) {
        Y_ASSERT(i < Size);
        return Data[i];
    }

    const T& operator[](TSize i) const {
        Y_ASSERT(i < Size);
        return Data[i];
    }

    T& back() {
        Y_ASSERT(Size > 0);
        return Data[Size - 1];
    }

    const T& back() const {
        Y_ASSERT(Size > 0);
        return Data[Size - 1];
    }

    void clear() {
        resize(0);
    }

    bool empty() const {
        return Size == 0;
    }
};
#pragma pack()

template <class T, ui16 N>
const T* begin(const TSmallArray<T, N>& a) {
    return a.data();
}

template <class T, ui16 N>
const T* end(const TSmallArray<T, N>& a) {
    return a.data() + a.size();
}

template <class T, ui16 N>
class TSerializer<TSmallArray<T, N>> {
public:
    static inline void Save(IOutputStream* rh, const TSmallArray<T, N>& v) {
        ::Save(rh, v.Size);
        ::SavePodArray(rh, v.Data, v.Size);
    }

    static inline void Load(IInputStream* rh, TSmallArray<T, N>& v) {
        ::Load(rh, v.Size);
        ::LoadPodArray(rh, v.Data, v.Size);
    }
};

template <class T, class TMerge, class TCmp, class TEq>
size_t MiniReduce(T* begin, T* end, TMerge mf, TCmp cmp, TEq eq) {
    size_t size = end - begin;
    if (size < 2) {
        return size;
    }
    Sort(begin, end, cmp);
    T* it = begin;
    T* res = begin;
    for (++it; it != end; ++it) {
        if (eq(*res, *it)) {
            mf(*res, *it);
        } else {
            *(++res) = *it;
        }
    }
    return res - begin + 1;
}

template <class T, ui16 N, class TMerge, class TCmp, class TEq>
void MergeSmallArrays(const TSmallArray<T, N>& a1, const TSmallArray<T, N>& a2, TSmallArray<T, N>& res,
                      TMerge mf, TCmp cmp, TEq eq) {
    TVector<T> merged;
    merged.insert(merged.end(), a1.data(), a1.data() + a1.size());
    merged.insert(merged.end(), a2.data(), a2.data() + a2.size());
    size_t size = MiniReduce(merged.begin(), merged.end(), mf, cmp, eq);
    Y_ASSERT(size <= N);
    res.assign(merged.begin(), merged.begin() + size);
}
