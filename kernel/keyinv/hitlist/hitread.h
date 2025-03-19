#pragma once

#include <util/system/defaults.h>
#include <util/memory/blob.h>
#include <util/system/yassert.h>

#include "subindex.h"
#include "hitformat.h"
#include "invsearch.h"

class IInputStream;
class IOutputStream;

class THitsForRead
{
public:
    THitsForRead()
      : FirstPerst(&YxPerst::NullSubIndex)
      , LastPerst(nullptr)
      , Length(0)
      , Count(0)
      , HitFormat(HIT_FMT_RAW_I64)
      , Offset(0)
    {}

    THitsForRead(const TBlob& r)
      : Data(r)
      , FirstPerst(&YxPerst::NullSubIndex)
      , LastPerst(nullptr)
      , Length(r.Length())
      , Count(0)
      , HitFormat(HIT_FMT_RAW_I64)
      , Offset(0)
    {}

    void Init(const TBlob& r, size_t length, i64 count, EHitFormat hitFormat) {
        Data = r;
        Length = length;
        Count = count;
        HitFormat = hitFormat;
        Offset = 0;
    }

    const Y_FORCE_INLINE YxPerst *GetFirstPerst() const {
        return FirstPerst;
    }

     const Y_FORCE_INLINE YxPerst *GetLastPerst() const {
        return LastPerst;
    }

    size_t GetLength() const {
        return Length;
    }

    i64 GetCount() const {
        return Count;
    }

    const TBlob& GetData() const {
        return Data;
    }

    TBlob GetData(size_t lowBound, size_t highBound) {
        return Data.SubBlob(lowBound, highBound);
    }

    void SetData(const TBlob& d) {
        Data = d;
        Length = d.Length();
    }

    const TSubIndexInfo& GetSubIndexInfo() const {
        return SubIndexInfo;
    }

    EHitFormat GetHitFormat() const {
        return HitFormat;
    }

    void SetHitFormat(EHitFormat hitFormat) {
        HitFormat = hitFormat;
    }

    bool Empty() const {
        return Data.Empty();
    }

    inline void ReadHits(const IKeysAndPositions& index,
                         i64 offset, ui32 length, i64 count, READ_HITS_TYPE type);
    inline void AttachTo(const TIndexInfo& si, TBlob& parent,
                         i64 parent_offset, i64 offset, ui32 length, i64 count);

    void SaveHits(IOutputStream* rh) const;
    void LoadHits(IInputStream* rh, bool skipFirst);

    void SaveIndexAccess(IOutputStream* rh) const;
    void RestoreIndexAccess(IInputStream* rh, const IKeysAndPositions& index, READ_HITS_TYPE type = RH_DEFAULT);

    bool operator<(const THitsForRead& hits) const {
        if (Data.AsCharPtr() != hits.Data.AsCharPtr())
            return Data.AsCharPtr() < hits.Data.AsCharPtr();
        return Length < hits.Length;
    }

    bool operator==(const THitsForRead& hits) const {
        if (Data.AsCharPtr() != hits.Data.AsCharPtr())
            return false;
        return Length == hits.Length;
    }

    bool operator!=(const THitsForRead& hits) const {
        return !(*this == hits);
    }

private:
    const YxPerst* GetLastPerstInt() const {
        return FirstPerst != &YxPerst::NullSubIndex ?
            Advance(FirstPerst, Count / GetSubIndexInfo().nSubIndexStep, GetSubIndexInfo().nPerstSize) :
            &YxPerst::NullSubIndex;
    }
    TBlob          Data;
    const YxPerst *FirstPerst;
    const YxPerst *LastPerst;
    TSubIndexInfo  SubIndexInfo;
    size_t         Length;
    i64            Count;
    EHitFormat     HitFormat;

    i64            Offset; // For index access save/restore
};

inline void THitsForRead::ReadHits(const IKeysAndPositions& index,
                            i64 offset, ui32 length, i64 count, READ_HITS_TYPE type)
{
    Length = length;
    Count = count;
    SubIndexInfo = index.GetSubIndexInfo();

    length += ::subIndexSize(offset, length, count, SubIndexInfo);
    index.GetBlob(Data, offset, length, type);

    if (needSubIndex(count, SubIndexInfo))
        FirstPerst = subIndexFirstPerst(Data.AsCharPtr(), offset, Length);
    else
        FirstPerst = &YxPerst::NullSubIndex;

    LastPerst = GetLastPerstInt();
    HitFormat = DetectHitFormat(index.GetVersion());

    Offset = offset;
}

inline void THitsForRead::AttachTo(const TIndexInfo& ii, TBlob& parent,
                                   i64 parent_offset, i64 offset, ui32 length, i64 count)
{
    Length = length;
    SubIndexInfo = ii.SubIndexInfo;
    Count = count;

    Y_ASSERT(offset >= parent_offset);
    size_t offsetParent = size_t(offset - parent_offset);

    length += subIndexSize(offset, length, count, SubIndexInfo);
    Data = parent.SubBlob(offsetParent, offsetParent + length);

    if (needSubIndex(count, SubIndexInfo))
        FirstPerst = subIndexFirstPerst(Data.AsCharPtr(), offset, Length);
    else
        FirstPerst = &YxPerst::NullSubIndex;

    LastPerst = GetLastPerstInt();
    HitFormat = DetectHitFormat(ii.Version);

    Offset = offset;
}
