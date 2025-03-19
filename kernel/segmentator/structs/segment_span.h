#pragma once

#include "list_accessors.h"
#include "segment_span_decl.h"

#include <util/generic/list.h>
#include <util/generic/utility.h>

namespace NSegm {

typedef TVector<TSegmentSpan> TSegmentSpans;

typedef TSegmentSpan TMainContentSpan;
typedef TVector<TMainContentSpan> TMainContentSpans;

template<typename tlist>
struct TSegmentSpanAccessor : TAccessor<tlist, TSegmentSpan> {
    static TBlockDist GetBlockDist(TSegmentSpan * first, TSegmentSpan * second) {
        return first && second ? TBlockDist(first->LastBlock, second->FirstBlock) : TBlockDist::Max();
    }
};

typedef TSegmentSpanAccessor<TSegmentSpans> TSegSpanVecAccessor;

enum ESegmentType {
    STP_NONE = 0, // ST_UNDEFINED

    STP_FOOTER, // same value as the former (nonsence) ST_TITLE, very different semantics.
    STP_HEADER, // ST_HEADER
    STP_MENU, // ST_MENU
    STP_REFERAT, // ST_REFERAT
    STP_LINKS, // ST_LINKS
    STP_AUX, // ST_AUX
    STP_CONTENT, // ST_CONTENT

    STP_COUNT
};

const char* GetSegmentName(NSegm::ESegmentType type);
NSegm::ESegmentType GetSegmentTypeByName(TStringBuf name);

bool In(ui8 type, ESegmentType a, ESegmentType b = STP_NONE, ESegmentType c = STP_NONE, ESegmentType d = STP_NONE,
               ESegmentType e = STP_NONE);

template <typename tlist, typename TBigSpans>
void CheckIsIncluded(tlist& segs, const TBigSpans& spans, void (*Action)(TSegmentSpan&, const typename TBigSpans::value_type&)) {
    typename tlist::iterator it = segs.begin();
    for (typename TBigSpans::const_iterator at = spans.begin(); at != spans.end(); ++at) {
        while (it != segs.end() && it->Begin.Sent() < at->Begin.Sent())
            ++it;

        while (it != segs.end() && at->ContainsSent(it->Begin.Sent())) {
            Action(*it, *at);
            ++it;
        }
    }
}

template <typename tlist, typename TSmallSpans>
void CheckIncludes(tlist& segs, const TSmallSpans& spans, void (*Action)(TSegmentSpan&, const typename TSmallSpans::value_type&)) {
    typename TSmallSpans::const_iterator at = spans.begin();
    for (typename tlist::iterator it = segs.begin(); it != segs.end(); ++it) {
        while (at != spans.end() && at->Begin.Sent() < it->Begin.Sent())
            ++at;

        while (at != spans.end() && it->ContainsSent(at->Begin.Sent())) {
            Action(*it, *at);
            ++at;
        }
    }
}

template<typename TSpan>
void SetInArticle(TSegmentSpan& s, const TSpan&) {
    s.InArticle = true;
}

template<typename TSpan>
void SetHasMainHeaderNews(TSegmentSpan& s, const TSpan&) {
    s.HasMainHeaderNews = true;
}

template<typename TSpan>
void SetInReadabilitySpans(TSegmentSpan& s, const TSpan&) {
    s.InReadabilitySpans = true;
}

template<typename TSpan>
void SetInMainContentNews(TSegmentSpan& s, const TSpan&) {
    s.InMainContentNews = true;
}

template<typename TSpan>
void SetHeader(TSegmentSpan& s, const TSpan& p) {
    s.HasHeader = true;
    TSegmentSpan::CheckedAdd(s.HeadersCount, 1);

    if (s.Begin.Sent() == p.Begin.Sent() && s.End.Sent() == p.End.Sent())
        s.IsHeader = s.Words && !s.Inputs && s.Blocks <= 1;
}

}
