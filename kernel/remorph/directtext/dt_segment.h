#pragma once

#include <kernel/indexer/direct_text/dt.h>

#include <library/cpp/langmask/langmask.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <utility>

namespace NDT {

class TDTSegmenter;
typedef TIntrusivePtr<TDTSegmenter> TDTSegmenterPtr;

struct ISegmentCallback {
    virtual void OnSegment(const NIndexerCore::TDirectTextEntry2* entries, size_t from, size_t to, const TLangMask& langs) = 0;
};


class TDTSegmenter: public TAtomicRefCount<TDTSegmenter> {
private:
    TDTSegmenterPtr Next;

protected:
    TDTSegmenter() {
    }

    TDTSegmenter(const TDTSegmenterPtr& next)
        : Next(next)
    {
    }

    virtual void DoSegmentation(ISegmentCallback& cb, const NIndexerCore::TDirectTextEntry2* entries,
        size_t from, size_t to, const TLangMask& langs) const = 0;

public:
    virtual ~TDTSegmenter() {
    }

    void AddNext(const TDTSegmenterPtr& next) {
        Y_ASSERT(next);
        if (!Next) {
            Next = next;
        } else {
            Next->AddNext(next);
        }
    }

    void Segment(ISegmentCallback& cb, const NIndexerCore::TDirectTextEntry2* entries,
        size_t from, size_t to, const TLangMask& langs) const;
};

}
