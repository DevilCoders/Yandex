#include <algorithm>

#include <util/generic/utility.h>

#include "hitread.h"
#include "reqserializer.h"
#include "hits_raw.h"
#include "hits_saver.h"

THitsForWrite::THitsForWrite()
{
    Capacity = 0;
    Length = 0;
    Count = 0;
    SetHitFormat(HIT_FMT_UNKNOWN);
}

THitsForWrite::THitsForWrite(EHitFormat format)
{
    Capacity = 0;
    Length = 0;
    Count = 0;
    SetHitFormat(format);
}

THitsForWrite::~THitsForWrite()
{
}

#define ROUND_UP(x,N)   (((x)+=(N-1))/=N)*=N
#define HITS_SIZE_MIN   512

void THitsForWrite::Resize(size_t newSz)
{
    if (newSz == 0) {
        Flush();
        return;
    }

    size_t sz = Capacity ? Capacity : HITS_SIZE_MIN;

    /// input: Capacity, newSz
    /// if Capacity > newSz -- decrease memory (by 1.5)
    /// if newSz > Capacity -- increase memory (by 1.5)
    if (newSz < Capacity)
        while (newSz < sz/3*2)
            (sz /= 3) *= 2;
    else if (Capacity < newSz)
        do {
            assert(sz/2<UINT_MAX/3);
            (sz /= 2) *= 3;
        } while (sz < newSz);
    else
        return;
    ///
    assert(newSz <= sz);
    assert(sz);

    // round up
    ROUND_UP(sz, HITS_SIZE_MIN);

    // last paranoya check
    if (sz == Capacity)
        return;

    TArrayHolder<char> tempBuf(new char[sz]);
    if (!!Data)
        memcpy(tempBuf.Get(), Data.Get(), Length);
    Data.Swap(tempBuf);
    Capacity = sz;
}

void THitsForWrite::Flush()
{
    Capacity = 0;
    Length = 0;
    Count = 0;
    Data.Destroy();
    HitCoder.Reset();
    SetHitFormat(HIT_FMT_UNKNOWN);
}

void THitsForWrite::Save(IOutputStream* rh) const
{
    SavePodType(rh, Count);
    SavePodType(rh, Length);
    SavePodArray(rh, Data.Get(), Length);
}

void THitsForWrite::Swap(THitsForWrite& rhs)
{
    DoSwap(Capacity, rhs.Capacity);
    DoSwap(Length, rhs.Length);
    DoSwap(Count, rhs.Count);
    std::swap(Format, rhs.Format);
    HitCoder.Swap(rhs.HitCoder);
    Data.Swap(rhs.Data);
}

THitsForRead* CreateHitsForRead(THitsForWrite* src)
{
    return new THitsForRead(src->Release());
}

class THitsDataHolderForBlob : public TBlob::TBase,
                               public TSimpleRefCount<THitsDataHolderForBlob>
{
private:
    typedef TSimpleRefCount<THitsDataHolderForBlob> RefBase;
public:
    inline THitsDataHolderForBlob(char* data) noexcept
      : Data(data)
    {
    }
    ~THitsDataHolderForBlob() override {
    }
    void Ref() noexcept override {
        RefBase::Ref();
    }
    void UnRef() noexcept override {
        RefBase::UnRef();
    }
private:
    TArrayHolder<char> Data;
};

TBlob THitsForWrite::Release()
{
    THolder<THitsDataHolderForBlob> base(new THitsDataHolderForBlob(Data.Get()));
    TBlob ret((void*)Data.Get(), Length, base.Get());
    Y_UNUSED(base.Release());
    Capacity = 0;
    Length = 0;
    Count = 0;
    Y_UNUSED(Data.Release());
    HitCoder.Reset();
    SetHitFormat(HIT_FMT_UNKNOWN);
    return ret;
}

//////////////////////////////////////////////////////////////////////////
void THitsWriteBuffer::DumpHits(TRawHits *dst)
{
    if (!Sorted)
        std::sort(Buf.begin(), Buf.end());
    for (size_t i = 1; i < Buf.size(); ++i)
        dst->PushBack(Buf[i]);
}

void THitsWriteBuffer::DumpHits(THitsForRead *dst)
{
    if (!Sorted)
        std::sort(Buf.begin(), Buf.end());
    THitsForWrite hits;
    hits.SetHitFormat(HIT_FMT_RAW_I64);
    for (size_t i = 1; i < Buf.size(); ++i)
        hits.Add(Buf[i]);
    dst->SetData(hits.Release());
}

void THitsWriteBuffer::DumpHits(TVector<SUPERLONG> *dst)
{
    if (!Sorted)
        std::sort(Buf.begin(), Buf.end());
    for (size_t i = 1; i < Buf.size(); ++i)
        dst->push_back(Buf[i]);
}
