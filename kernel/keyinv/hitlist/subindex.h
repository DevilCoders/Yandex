#pragma once

#include <cstddef>

#include <util/system/defaults.h>
#include <util/generic/utility.h>

#define DEFAULT_SUBINDEX_STEP 128
#define DEFAULT_PERST_SIZE 16

struct TSubIndexInfo
{
    ui32 nSubIndexStep;
    ui32 nPerstSize;
    ui32 nMinNeedSize;
    bool hasSubIndex;
    TSubIndexInfo()
      : nSubIndexStep(DEFAULT_SUBINDEX_STEP)
      , nPerstSize(DEFAULT_PERST_SIZE)
      , nMinNeedSize(2*DEFAULT_PERST_SIZE*DEFAULT_SUBINDEX_STEP)
      , hasSubIndex(false)
    {}
    TSubIndexInfo(bool _hasSubIndex, ui32 _nStep, ui32 _nPerstSize, ui32 _nMinNeedSize)
      : nSubIndexStep(_nStep)
      , nPerstSize(_nPerstSize)
      , nMinNeedSize(_nMinNeedSize)
      , hasSubIndex(_hasSubIndex)
    {}

    ui32 GetMaxPerstDataSize() const {
        return Max(nSubIndexStep, nMinNeedSize) * sizeof(SUPERLONG);
    }

    static TSubIndexInfo NullSubIndex;
};

#define PERST_JUNK 0
#pragma pack(1)
struct YxPerst {
    SUPERLONG Sum;
    ui32 Off;
#if defined _must_align8_
    ui32 Junk; // justification for 64-bit processors (?)
#endif
    static YxPerst NullSubIndex;
};
#pragma pack()

inline YxPerst GetPerst(SUPERLONG sum, ui32 off) {
    const YxPerst perst = { sum, ui32(off)
#if defined _must_align8_
        , PERST_JUNK
#endif
    };
    return perst;
}

inline const YxPerst *Advance(const YxPerst *p, ptrdiff_t nDelta, int nPerstSize) {
    return (YxPerst*)(((char*)p) + nDelta * nPerstSize);
}

inline const YxPerst *Advance(const YxPerst *p, int nPerstSize) {
    return (YxPerst*)(((char*)p) + nPerstSize);
}

#if defined _must_align8_
#define PERST_SIZE (sizeof(YxPerst))
    inline ui32 perstAlign(SUPERLONG pos) {
        return (ui32)(((pos + PERST_SIZE - 1) & ~(PERST_SIZE - 1)) - pos);
    }
#else
    inline ui32 perstAlign(SUPERLONG) {
        return ((ui32)0);
    }
#endif

inline const YxPerst *subIndexFirstPerst(const char *mappedHits, SUPERLONG hitsOffset, size_t hitsLength) {
        return (YxPerst *)(mappedHits + hitsLength + perstAlign(hitsOffset + hitsLength));
}


inline const YxPerst* subIndexLastPerst(const char *mappedHits, SUPERLONG hitsOffset, ui32 hitsLength, i64 hitsCount, const TSubIndexInfo &subIndexInfo)
{
    return Advance(subIndexFirstPerst(mappedHits, hitsOffset, hitsLength), hitsCount / subIndexInfo.nSubIndexStep, subIndexInfo.nPerstSize);
}

inline const YxPerst *subIndexOffs(const char *p, SUPERLONG offs, size_t len) {
    return subIndexFirstPerst(p, offs, len);
}

inline bool needSubIndex(i64 count, const TSubIndexInfo &si) {
   return si.hasSubIndex && count > si.nMinNeedSize;
}

inline ui32 subIndexSize(SUPERLONG offs, ui32 len, i64 count, const TSubIndexInfo &si)  {
    if (needSubIndex(count, si)) {
       return  perstAlign(offs + len) + (count/si.nSubIndexStep+1)*si.nPerstSize;
    } else {
       return 0;
    }
}

inline ui32 hitsSize(SUPERLONG offs, ui32 len, i64 count, const TSubIndexInfo &si)  {
    return len + subIndexSize(offs, len, count, si);
}
