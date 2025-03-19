#pragma once

#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/system/defaults.h>

#include <library/cpp/binsaver/bin_saver.h>

/**
 * "Vector" for writing decompressed data into it
 **/
template<typename T>
class TPODVector {
private:
    TArrayHolder<T> Data_;
    size_t Size_ = 0;
    size_t Capacity_ = 0;

public:
    TPODVector(size_t initialCapacity = 0) {
        if (initialCapacity > 0) {
            Data_.Reset(new T[initialCapacity]);
        }
        Capacity_ = initialCapacity;
    }

    TPODVector(const TPODVector<T>& other) {
        *this = other;
    }

    const TPODVector<T>& operator=(const TPODVector<T>& other) {
        if (this == &other) {
            return *this;
        }
        // basically you don't want to do it
        Data_.Reset(new T[other.Capacity_]);
        if (other.Size_) {
            memcpy(Data_.Get(), other.Data_.Get(), other.Size_ * sizeof(T));
        }
        Capacity_ = other.Capacity_;
        Size_ = other.Size_;
        return *this;
    }

    void Reserve(size_t newCapacity) {
        if (newCapacity <= Capacity_) {
            return;
        }
        TArrayHolder<T> newData(new T[newCapacity]);
        if (Size_) {
            memcpy(newData.Get(), Data_.Get(), Size_ * sizeof(T));
        }
        Data_.Reset(newData.Release());
        Capacity_ = newCapacity;
    }

    inline T& operator[](size_t index) {
        Y_ASSERT(index < Capacity_); // Do not write past end of buffer
        return Data_[index];
    }

    inline const T& operator[](size_t index) const {
        Y_ASSERT(index < Size_); // Do not read from uninitialized memory
        return Data_[index];
    }

    void Append(const T* begin, const T* end) {
        size_t delta = end - begin;
        Reserve(Size_ + delta);
        T* dest = Data_.Get() + Size_;
        for (const T* ptr = begin; ptr != end; ++ptr, ++dest) {
            *dest = *ptr;
        }
        Size_ += delta;
    }

    inline const T* operator~() const {
        return Data_.Get();
    }

    inline size_t Size() const {
        return Size_;
    }
    inline size_t operator+() const {
        return Size();
    }
    /**
     * std::vector werewolfing
     **/
    inline size_t size() const {
        return Size();
    }
    /**
     * TVector werewolfing
     **/
    inline i64 ysize() const {
        return i64(Size());
    }

    /**
     * Kosher resize
     **/
    inline void resize(size_t newSize) {
        size_t oldSize = Size_;
        Reserve(newSize);
        for (size_t i = oldSize; i < newSize; ++i) {
            Data_[i] = T();
        }
        Size_ = newSize;
    }

    inline void push_back(const T& value) {
        if (Size_ == Capacity_) {
            Reserve(Size_ * 3 / 2 + 1);
        }
        Y_ASSERT(Size_ < Capacity_);
        Data_[Size_] = value;
        ++Size_;
    }

    inline T& back() {
        Y_ASSERT(Size_ > 0);
        return Data_[Size_ - 1];
    }

    const T* begin() const {
        return Data_.Get();
    }

    const T* end() const {
        return Data_.Get() + Size_;
    }

    bool empty() const {
        return 0 == Size_;
    }

    inline const T& back() const {
        Y_ASSERT(Size_ > 0);
        return Data_[Size_ - 1];
    }

    inline void SetInitializedSize(size_t size) {
        Y_ASSERT(size <= Capacity_);
        Size_ = size;
    }

    inline void clear() {
        SetInitializedSize(0);
    }

    int operator&(IBinSaver& f) {
        IBinSaver::TStoredSize nSize = 0;
        if (f.IsReading()) {
            f.Add(2, &nSize);
            Reserve(nSize);
        } else {
            nSize = size();
            f.Add(2, &nSize);
        }
        for (IBinSaver::TStoredSize i = 0; i < nSize; i++) {
            f.Add(1, &Data_[i]);
        }
        SetInitializedSize(nSize);
        return 0;
    }

};

using TSentenceLengths = TPODVector<ui8>;

class TSentenceLengthsCoderData {
public:
    static const ui64 MULT = 101;
    static size_t GetNumBytesForHashVersion2();
    static size_t GetBlockCount();
    static void GetBlockVersion2(ui16 index, TSentenceLengths* result);
};

namespace NDataVersion2 {
    struct TOffset {
        ui32 Index;
        ui8 Len;
    };

    extern const size_t LENGTHS_NUMBYTESFORHASH;
    extern const TOffset LENGTHS_OFFSETS[];
    extern const ui8 LENGTHS_SEQUENCES[];

    struct TPackedOffsets {
        TPODVector<ui32> Offsets;

        TPackedOffsets();
    };

    extern TPackedOffsets PackedOffsets;
} // namespace NDataVersion2
