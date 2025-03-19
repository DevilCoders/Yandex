#include "main_header_impl.h"
#include <kernel/segmentator/structs/merge.h>

#include <cstdlib>

namespace NSegm {
namespace NPrivate {

void THeaderFeatures::Calculate(const TDocContext& ctx, const THeaderSpans&, const TSegmentSpans&) {
    if (!ctx.Words)
        return;
}

TMainHeaderSpans FindMainHeaderSpans(const TDocContext& ctx, THeaderSpans& hs, const TSegmentSpans&,
                                     const TMainContentSpans& mains, const TArticleSpans& arts) {
    Y_UNUSED(ctx);

    if (mains.empty() || hs.empty())
        return TMainHeaderSpans();

    TArticleSpans::const_iterator it = arts.begin();

    TMainContentSpan mc(mains.front().Begin, mains.back().End);

    while (it != arts.end() && !mc.Contains(it->Begin))
        ++it;

    if (it != arts.end() && 2 * (ui32)abs(mc.Begin.Sent() - it->Begin.Sent()) < mc.Sentences()) {
        for (THeaderSpans::const_iterator hit = hs.begin(); hit != hs.end(); ++hit) {
            if (it->Contains(hit->Begin))
                return TMainHeaderSpans(1, *hit);
        }
    }

    THeaderSpans::const_iterator hit = hs.begin();

    while(hit != hs.end() && hit->Begin.Sent() < mc.Begin.Sent())
        ++hit;

    if (hit != hs.begin())
        --hit;

    if (2 * (ui32)abs(mc.Begin.Sent() - hit->Begin.Sent()) > mc.Sentences())
        return TMainHeaderSpans();

    return TMainHeaderSpans(1, *hit);
}

TArticleSpans FindArticles(const THeaderSpans& hs, const TSegmentSpans& sp) {
    if (hs.empty())
        return TArticleSpans();

    TSegmentSpans spans = MakeCoarseSpans<false>(sp);
    TArticleSpans arts;

    THeaderSpans::const_iterator it = hs.begin();
    for (TSegmentSpans::const_iterator mit = spans.begin(); mit != spans.end(); ++mit) {
        if (!In(mit->Type, STP_CONTENT, STP_REFERAT, STP_LINKS))
            continue;

        while (it != hs.end() && mit->Begin.Sent() > it->Begin.Sent())
            ++it;

        if (it == hs.end())
            break;

        if (mit->Contains(it->Begin) && 2 * (it->Begin.Sent() - mit->Begin.Sent()) < mit->End.Sent() - it->End.Sent())
            arts.push_back(*mit);
    }

    return arts;
}

}
}
