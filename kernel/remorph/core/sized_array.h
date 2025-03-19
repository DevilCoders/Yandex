#pragma once

#include <util/generic/ptr.h>
#include <util/generic/utility.h>
#include <util/system/defaults.h>
#include <util/ysaveload.h>

namespace NRemorph {

template<class T>
class TSizedArrayHolder {
private:
    size_t Size_;
    TMallocHolder<T> Array;
private:
    Y_FORCE_INLINE size_t SizeBytes() const {
        return sizeof(Array[0]) * Size_;
    }
    Y_FORCE_INLINE void CopyFrom(const TSizedArrayHolder& r) {
        memcpy(Array.Get(), r.Array.Get(), r.SizeBytes());
    }
public:
    Y_FORCE_INLINE TSizedArrayHolder()
        : Size_(0)
    {
    }
    Y_FORCE_INLINE TSizedArrayHolder(size_t size) {
        Reset(size);
    }
    TSizedArrayHolder(const TSizedArrayHolder& r)
        : Size_(r.Size_)
        , Array((T*)::malloc(SizeBytes()))
    {
        CopyFrom(r);
    }

    inline void Reset(size_t size) {
        Size_ = size;
        Array.Reset((T*)::malloc(SizeBytes()));
        Zero();
    }
    inline TSizedArrayHolder& operator=(const TSizedArrayHolder& r) {
        if (&r != this) {
            if (r.Size() == Size()) {
                CopyFrom(r);
            } else {
                TSizedArrayHolder<T> tmp(r);
                Swap(tmp);
            }
        }
        return *this;
    }
    Y_FORCE_INLINE void Swap(TSizedArrayHolder& r) {
        DoSwap(Size_, r.Size_);
        DoSwap(Array, r.Array);
    }
    Y_FORCE_INLINE void Zero() {
        memset(Array.Get(), 0, SizeBytes());
    }
    Y_FORCE_INLINE size_t Size() const {
        return Size_;
    }
    Y_FORCE_INLINE bool operator==(const TSizedArrayHolder& r) const {
        return (Size() == r.Size()) && (0 == memcmp(Array.Get(), r.Array.Get(), SizeBytes()));
    }
    Y_FORCE_INLINE T& operator[](size_t i) {
        Y_ASSERT(i < Size_);
        return Array[i];
    }
    Y_FORCE_INLINE const T& operator[](size_t i) const {
        Y_ASSERT(i < Size_);
        return Array[i];
    }
    Y_FORCE_INLINE T* Get() {
        return Array.Get();
    }
    Y_FORCE_INLINE const T* Get() const {
        return Array.Get();
    }
    inline void Save(IOutputStream* out) const {
        ::SaveSize(out, Size_);
        if (Size_)
            ::SaveArray(out, Get(), Size_);
    }
    inline void Load(IInputStream* in) {
        size_t size = ::LoadSize(in);
        if (size != Size_) {
            Size_ = size;
            Array.Reset((T*)::malloc(SizeBytes()));
        }
        if (Size_)
            ::LoadArray(in, Get(), Size_);
    }
};

} // NRemorph
