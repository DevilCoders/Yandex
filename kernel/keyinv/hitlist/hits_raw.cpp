#include <util/generic/utility.h>

#include "hits_raw.h"

TRawHits::TRawHits(EHitFormat format)
    : BufferCapacity(0)
    , BufferPos(0)
    , Coder(format)
    , Size(0)
{
}

void TRawHits::Swap(TRawHits& rhs) {
    Coder.Swap(rhs.Coder);
    DoSwap(Size, rhs.Size);
    DoSwap(BufferPos, rhs.BufferPos);
    DoSwap(BufferCapacity, rhs.BufferCapacity);
    Buffer.Swap(rhs.Buffer);
}

void TRawHits::Clear() {
    BufferPos = 0;
    BufferCapacity = 0;
    Buffer.Destroy();
    Size = 0;
    Coder.Reset();
}

size_t TRawHits::GetSize() const {
    return Size;
}

void TRawHits::Write(IOutputStream* out) {
    out->Write(Buffer.Get(), BufferPos);
}

TRawHitsIter::TRawHitsIter()
    : Now(nullptr)
    , End(nullptr)
    , Decoder(FMT_RAW_HITS)
    , IsValid(false)
{
}

void TRawHitsIter::Init(const TRawHits& vct) {
    Now = vct.Buffer.Get();
    End = Now + vct.BufferPos;
    IsValid = Now < End;
    if (IsValid)
        Decoder.Next(Now);
}

void TRawHitsIter::SkipTo(SUPERLONG pos) {
    IsValid = Decoder.SkipTo(Now, End, pos);
}
