#pragma once

#include "dt_segment.h"

#include <kernel/indexer/direct_text/dt.h>

#include <library/cpp/token/nlptypes.h>
#include <library/cpp/wordpos/wordpos.h>

#include <util/system/defaults.h>
#include <library/cpp/containers/sorted_vector/sorted_vector.h>

namespace NDT {

class TDTSplitSegmenter: public TDTSegmenter {
protected:
    // Returns the position of next break
    virtual size_t FindNextBreak(const NIndexerCore::TDirectTextEntry2* entries,
        size_t from, size_t to) const = 0;

protected:
    TDTSplitSegmenter()
        : TDTSegmenter()
    {
    }

    TDTSplitSegmenter(const TDTSegmenterPtr& next)
        : TDTSegmenter(next)
    {
    }

    void DoSegmentation(ISegmentCallback& cb, const NIndexerCore::TDirectTextEntry2* entries,
        size_t from, size_t to, const TLangMask& langs) const override;
};

class TDTSentSegmenter: public TDTSplitSegmenter {
private:
    TBreakType BreakMask;

protected:
    size_t FindNextBreak(const NIndexerCore::TDirectTextEntry2* entries,
        size_t from, size_t to) const override;

public:
    TDTSentSegmenter(TBreakType breaks = ST_SENTBRK, const TDTSegmenterPtr& next = TDTSegmenterPtr())
        : TDTSplitSegmenter(next)
        , BreakMask(breaks)
    {
    }

    void SetBreaks(TBreakType breaks) {
        Y_ASSERT(breaks != 0);
        BreakMask = breaks;
    }
};

class TDTExtBreakSegmenter: public TDTSplitSegmenter {
private:
    const NSorted::TSortedVector<TPosting>* BreakPositions;

private:
    inline TPosting GetBreakAfter(TPosting pos) const {
        NSorted::TSortedVector<TPosting>::const_iterator iBreak = BreakPositions->UpperBound(pos);
        return iBreak != BreakPositions->end() ? *iBreak : Max();
    }

protected:
    size_t FindNextBreak(const NIndexerCore::TDirectTextEntry2* entries,
        size_t from, size_t to) const override;

public:
    TDTExtBreakSegmenter(const NSorted::TSortedVector<TPosting>& brkPos, const TDTSegmenterPtr& next = TDTSegmenterPtr())
        : TDTSplitSegmenter(next)
        , BreakPositions(&brkPos)
    {
    }

    void SetBreakPositions(const NSorted::TSortedVector<TPosting>& brkPos) {
        BreakPositions = &brkPos;
    }
};

class TDTDelimSegmenter: public TDTSplitSegmenter {
private:
    NSorted::TSimpleSet<wchar16> Delims;

protected:
    size_t FindNextBreak(const NIndexerCore::TDirectTextEntry2* entries,
        size_t from, size_t to) const override;

public:
    TDTDelimSegmenter(const TUtf16String& delims, const TDTSegmenterPtr& next = TDTSegmenterPtr())
        : TDTSplitSegmenter(next)
    {
        SetDelims(delims);
    }

    void SetDelims(const TUtf16String& delims);
};

}
