#pragma once

#include <util/system/defaults.h>
#include <util/generic/ptr.h>
#include <util/memory/blob.h>
#include <util/generic/vector.h>

#include "hits_coders.h"
#include <util/generic/noncopyable.h>

class IInputStream;
class IOutputStream;

class THitsForWrite : TNonCopyable
{
private:
    size_t Capacity; // текущий размер буфера
    size_t Length;   // длина записанных сжатых позиций в байтах
    size_t Count;    // число записанных позиций
    EHitFormat Format;
    CHitCoder HitCoder;
    TArrayHolder<char> Data;
private:
    void Resize(size_t newSz);
public:
    THitsForWrite();
    THitsForWrite(EHitFormat format);
    ~THitsForWrite();

    TBlob Release();
    TBlob GetData() const {
        return TBlob::NoCopy((void*)Data.Get(), Length);
    }
    void Flush();
    void Save(IOutputStream* rh) const;
public:
    size_t GetLength() const {
        return Length;
    }

    size_t GetCount() const {
        return Count;
    }

    EHitFormat GetHitFormat() const {
        return Format;
    }

    void SetHitFormat(EHitFormat f) {
        Format = f;
        HitCoder.SetFormat(f);
    }

    void Add(const SUPERLONG& l) {
        if (Length + MAX_PACKED_WORDPOS_SIZE > Capacity)
            Resize(Length + MAX_PACKED_WORDPOS_SIZE);

        SUPERLONG diff = l - HitCoder.GetCurrent();
        Y_ASSERT(TWordPosition::PositionOnly(l) >= TWordPosition::PositionOnly(HitCoder.GetCurrent()));
        if (diff > SUPERLONG(0UL)) {
            Length += HitCoder.Output(l, Data.Get() + Length);
            Count++;
        }
        assert(Length <= Capacity);
    }

    void Swap(THitsForWrite& rhs);
};

class THitsForRead;
THitsForRead* CreateHitsForRead(THitsForWrite* src);

class TRawHits;

class THitsWriteBuffer
{
    TVector<SUPERLONG> Buf;
    bool Sorted;

public:
    THitsWriteBuffer() { Clear(); }
    void Clear()
    {
        Sorted = true;
        if (Buf.size() == 1) {
            Y_ASSERT(Buf[0] == 0LL);
            return;
        }
        Buf.resize(1);
        Buf[0] = 0UL;
    }
    void Add(SUPERLONG x)
    {
        Y_ASSERT(x >= 0LL);
        Sorted &= x >= Buf.back();
        Buf.push_back(x);
    }
    void DumpHits(TRawHits *dst);
    void DumpHits(THitsForRead *dst);
    void DumpHits(TVector<SUPERLONG> *dst);
    bool Empty() const
    {
        return Buf.size() == 1;
    }
};
