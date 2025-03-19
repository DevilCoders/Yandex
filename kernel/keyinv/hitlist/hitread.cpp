#include <util/generic/buffer.h>
#include <util/stream/input.h>
#include <util/stream/output.h>

#include "reqserializer.h"
#include "hitread.h"
#include "positerator.h"

static void SaveRelevanceArray(IOutputStream* rh)//, const TRelevanceArray& data)
{
    i32 x = 0;//data.size();
    SavePodType(rh, x);
    i32 y = 0;//x * sizeof(TRelevance); //not used
    SavePodType(rh, y);
    if (x == 0)
        return;
    //SavePodArray(rh, &data[0], x);
}

static void LoadRelevanceArray(IInputStream* rh)//, TRelevanceArray& data)
{
    i32 x;
    LoadPodType(rh, x);
    //data.resize(x); // check allocation?
    i32 y;
    LoadPodType(rh, y);
    //if (data.size() != ui32(y / sizeof(TRelevance)))
    //    throw yexception("bad storage\n");
    if (x == 0)
        return;
    TBuffer tmp(x);
    LoadPodArray(rh, tmp.Begin(), x);//&data[0], x);
}

static const int N_HITS_HEADER_SIZE = sizeof(i32)*2;
static const int N_WEIGHTED_HITS_HEADER_SIZE = N_HITS_HEADER_SIZE + sizeof(i32)*2;

static i32 EmptyHits[] = {0, 4*sizeof(i32), 0, 0, 0, 0};

static void WriteEmptyHits(IOutputStream* rh) {
    rh->Write(EmptyHits, sizeof(EmptyHits));
}



void THitsForRead::SaveHits(IOutputStream* rh) const
{
    if (!(void *)this) {
        WriteEmptyHits(rh);
        return;
    }

    Y_ASSERT(this->HitFormat == HIT_FMT_RAW_I64); // if it has another format it will not be able to be loaded

    const ui32 length = this->Length; // cut to ui32
    if (length != this->Length)
        ythrow yexception() << "writing too large hits (" <<  this->Length << ")";

    i32 sumSkip = (int)HIT_FMT_RAW_I64; //hits.GetHitFormat();
    SavePodType(rh, sumSkip);
    // how many to skip for reading next records
    sumSkip = length + N_WEIGHTED_HITS_HEADER_SIZE;
    SavePodType(rh, sumSkip);
    // data itself
    SavePodType(rh, 0/*this.Count*/); // это поле нигде не используется, AFAIK
    SavePodType(rh, length);
    SavePodArray(rh, this->Data.AsCharPtr(), length);
    SaveRelevanceArray(rh);
}

class TMyBase : public TBlob::TBase,
                public TSimpleRefCount<TMyBase>
{
private:
    typedef TSimpleRefCount<TMyBase> RefBase;
public:
    inline TMyBase(char* data) noexcept
      : Data(data)
    {
    }
    ~TMyBase() override {
    }
    void Ref() noexcept override {
        RefBase::Ref();
    }
    void UnRef() noexcept override {
        RefBase::UnRef();
    }
private:
    THolder<char, TDeleteArray> Data;
};

void THitsForRead::LoadHits(IInputStream* rh, bool skipFirst)
{
    if (skipFirst) { // now it's used only in webutil/printhit
        i32 skipLen;
        LoadPodType(rh, skipLen); // hitFormat
        LoadPodType(rh, skipLen); // THitsForRead record size + N_WEIGHTED_HITS_HEADER_SIZE;
        TArrayHolder<char> data(new char[skipLen]);
        LoadPodArray(rh, data.Get(), skipLen);
    }
    i32 iHitFormat;
    LoadPodType(rh, iHitFormat);
    this->HitFormat = (EHitFormat)iHitFormat;

    i32 dummy;
    LoadPodType(rh, dummy); // THitsForRead record size + N_WEIGHTED_HITS_HEADER_SIZE;
    LoadPodType(rh, dummy); // counter of hits, is 0 now

    this->FirstPerst = &YxPerst::NullSubIndex;

    ui32 length = 0;
    LoadPodType(rh, length);

    this->Length = length; // assigning ui32 to size_t
    this->Count = 0;

    THolder<char, TDeleteArray> data(new char[length]);
    LoadPodArray(rh, data.Get(), length);
    THolder<TMyBase> base(new TMyBase(data.Get()));
    this->Data = TBlob((void*)data.Get(), length, base.Get());
    Y_UNUSED(data.Release());
    Y_UNUSED(base.Release());

    LoadRelevanceArray(rh);
}

struct TIndexAccessForHits {
    i64             Offset;
    ui32            Length;
    i64             Count;
};

void THitsForRead::SaveIndexAccess(IOutputStream* rh) const
{
    Y_ASSERT(Count > 0); // index data is available

    TIndexAccessForHits access;
    access.Length = Length;
    access.Count = Count;
    access.Offset = Offset;

    SavePodType(rh, access);
}

void THitsForRead::RestoreIndexAccess(IInputStream* rh, const IKeysAndPositions& index, READ_HITS_TYPE type)
{
    TIndexAccessForHits access;
    LoadPodType(rh, access);
    ReadHits(index, access.Offset, access.Length, access.Count, type);
}
