#pragma once

#include <util/generic/ptr.h>
#include <util/generic/typetraits.h>
#include <util/system/yassert.h>
#include <util/system/defaults.h>

template <typename T>
class TRingBuffer {
private:
    size_t BufferSize;
    size_t FillSize;
    size_t StartPos;

    TMallocHolder<T> BufferHolder;
    T* Buffer;

private:
    void EnsureBufferAlloc() {
        if (Y_UNLIKELY(nullptr == Buffer)) {
            Y_ASSERT(0 == FillSize);
            BufferHolder.Reset((T*)::malloc(sizeof(T) * BufferSize));
            Buffer = BufferHolder.Get();
        }
    }

public:
    TRingBuffer(size_t bufferSize)
        : BufferSize(bufferSize)
        , FillSize(0)
        , StartPos(0)
        , Buffer(nullptr)
    {
        Y_VERIFY(BufferSize > 0, "Buffer size cannot be 0");
    }

    TRingBuffer(T* buffer, size_t bufferSize)
        : BufferSize(bufferSize)
        , FillSize(0)
        , StartPos(0)
        , Buffer(buffer)
    {
        Y_VERIFY(BufferSize > 0, "Buffer size cannot be 0");
    }

    template <size_t N>
    TRingBuffer(T buffer[N])
        : BufferSize(N)
        , FillSize(0)
        , StartPos(0)
        , Buffer(buffer)
    {
        Y_VERIFY(BufferSize > 0, "Buffer size cannot be 0");
    }

    ~TRingBuffer() {
    }

    inline size_t Size() const {
        return FillSize;
    }

    inline bool Empty() const {
        return 0 == FillSize;
    }

    void Put(typename TTypeTraits<T>::TFuncParam e) {
        EnsureBufferAlloc();
        if (FillSize < BufferSize) {
            new (Buffer + FillSize) T(e);
            ++FillSize;
        } else {
            Buffer[StartPos] = e;
            StartPos = (StartPos + 1) % BufferSize;
        }
    }

    inline const T& At(size_t pos) const {
        Y_ASSERT(pos < FillSize);
        return Buffer[(pos + StartPos) % BufferSize];
    }
};
