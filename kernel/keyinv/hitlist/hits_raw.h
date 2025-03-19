#pragma once

#include <util/generic/vector.h>
#include <util/generic/ptr.h>

#include "longs.h"
#include "hits_coders.h"

class IOutputStream;

const EHitFormat FMT_RAW_HITS = HIT_FMT_RAW_I64;

class TRawHits : TNonCopyable {
private:
    size_t BufferCapacity;
    size_t BufferPos;
    TArrayHolder<char> Buffer;

    CHitCoder Coder;
    size_t Size;

    void Ensure(size_t size) {
        if (BufferCapacity < size) {
            const size_t newBufferCapacity = Max(size, 2*BufferCapacity);
            TArrayHolder<char> newBuffer(new char[newBufferCapacity]);
            memcpy(newBuffer.Get(), Buffer.Get(), BufferCapacity);
            Buffer.Swap(newBuffer);
            BufferCapacity = newBufferCapacity;
        }
    }

public:
    TRawHits(EHitFormat format = FMT_RAW_HITS);

    void PushBack(SUPERLONG value) {
        Ensure(BufferPos + MAX_PACKED_WORDPOS_SIZE);
        BufferPos += Coder.Output(value, Buffer.Get() + BufferPos);
        ++Size;
    }

    void Clear();
    void Swap(TRawHits& rhs);
    size_t GetSize() const;
    void Write(IOutputStream* out);

    friend class TRawHitsIter;
};


class TRawHitsIter {
private:
    const char* Now;
    const char* End;
    CHitDecoder Decoder;
    bool IsValid;

public:
    TRawHitsIter();

    void Init(const TRawHits& hits);

    bool Valid() const {
        return IsValid;
    }

    void SkipTo(SUPERLONG pos);

    ui32 Doc() const {
        return TWordPosition::Doc(Current());
    }

    SUPERLONG Current() const {
        assert(Valid());
        return Decoder.GetCurrent();
    }

    void operator++() {
        Y_ASSERT(Valid());
        if (Now < End)
            Decoder.Next(Now);
        else
            IsValid = false;
    }

    SUPERLONG operator*() const {
        return Current();
    }
};
