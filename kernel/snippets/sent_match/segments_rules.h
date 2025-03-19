#pragma once

#include <kernel/segmentator/structs/classification.h>
#include <kernel/segmentator/structs/segment_span.h>

using namespace NSegm;

namespace NSnippets
{
    namespace NSegments
    {
        namespace NRules
        {
            inline bool Footer(const ESegmentType& type)
            {
                return type == STP_FOOTER;
            }
            inline bool Ads(const TSegmentSpan& prevSeg, const TSegmentSpan& curSeg)
            {
                return (prevSeg.AdsCSS || prevSeg.AdsHeader) && !curSeg.AdsHeader && !curSeg.AdsCSS;
            }
            inline bool Inputs(const TSegmentSpan& prevSeg, const TSegmentSpan& curSeg)
            {
                return !!prevSeg.Inputs != !!curSeg.Inputs;
            }
            inline bool Words(const TSegmentSpan& prevSeg, const TSegmentSpan& curSeg)
            {
                return !prevSeg.IsHeader && prevSeg.Words < 10 && 10 <= curSeg.Words;
            }
            inline bool ToHeader(const TSegmentSpan& prevSeg, const TSegmentSpan& curSeg)
            {
                return !prevSeg.IsHeader && curSeg.IsHeader;
            }
            inline bool FromHeader(const TSegmentSpan& prevSeg, const TSegmentSpan& curSeg)
            {
                return prevSeg.IsHeader && !curSeg.IsHeader;
            }
            inline bool TextVsLinks(const TSegmentSpan& prevSeg, const TSegmentSpan& curSeg)
            {
                const float curLWPW = curSeg.AvLinkWordsPerWord();
                const float prevLWPW = prevSeg.AvLinkWordsPerWord();
                const float diffLWPW = curLWPW - prevLWPW;
                return diffLWPW < -0.2f || 0.2f < diffLWPW;
            }
            inline bool LocVsExt(const TSegmentSpan& prevSeg, const TSegmentSpan& curSeg)
            {
                return prevSeg.AvLinkWordsPerWord() > 0.5 && curSeg.AvLinkWordsPerWord() > 0.5 && LinksAreMostlyLocal(prevSeg) && !LinksAreMostlyLocal(curSeg);
            }
            inline bool IsBad(const ESegmentType& type)
            {
                return type == STP_AUX || type == STP_MENU || type == STP_LINKS;
            }
            inline bool IsGood(const ESegmentType& type)
            {
                return type == STP_CONTENT || type == STP_REFERAT /*|| type == STP_HEADER*/;
            }
            inline bool FromBad2Good(const ESegmentType& prevType, const ESegmentType& curType, bool prevHasMatch)
            {
                return prevHasMatch && IsBad(prevType) && IsGood(curType);
            }
            inline bool GoodIsNear(const ESegmentType& prev, const ESegmentType& cur, const ESegmentType& next)
            {
                return IsBad(prev) && IsBad(cur) && IsGood(next);
            }
        }
    }
}
