#pragma once

#include <cstdio>
#include <util/system/defaults.h>
#include <util/generic/yexception.h>

#include <kernel/segmentator/structs/structs.h>

#ifdef SEGMENTATOR_DEBUG
#   define SEGMENTATOR_BCD0(i, begin, end) printf("\n%u %u - %u :: ", i, TWordPosition::Break(begin), TWordPosition::Break(end))
#   define SEGMENTATOR_BCD1(s) printf(s)
#else
#   define SEGMENTATOR_BCD0(i, begin, end)
#   define SEGMENTATOR_BCD1(s)
#endif

namespace NSegm {

template<typename T> inline bool Equal(T a, T b) {
    return a - b < std::numeric_limits<T>::epsilon() && b - a < std::numeric_limits<T>::epsilon();
}

template<typename T>
inline bool Greater(T a, T b) {
    return a - b >= std::numeric_limits<T>::epsilon();
}

template<typename T>
inline bool GreaterEqual(T a, T b) {
    return Greater(a, b) || Equal(a, b);
}

template<typename T>
inline bool Less(T a, T b) {
    return !GreaterEqual(a, b);
}

template<typename T>
inline bool LessEqual(T a, T b) {
    return !Greater(a, b);
}

inline ESegmentType ChardinFooterRule(const TSegmentSpan& sg) {
    if (4 * sg.Number > 3 * sg.Total && 1 <= sg.Words && sg.Words <= 100
                    && (sg.FooterText || (sg.FirstBlock.Included && sg.FooterCSS))) {
        SEGMENTATOR_BCD1("ChardinFooterRule");
        return STP_FOOTER;
    }
    return STP_NONE;
}

inline ESegmentType AdsRule(const TSegmentSpan& sg) {
    if (sg.FirstBlock.Included && (sg.AdsCSS || (sg.AdsHeader && sg.Words < 100)) && sg.Domains) {
        SEGMENTATOR_BCD1("AdsRule");
        return STP_LINKS;
    }
    return STP_NONE;
}

inline ESegmentType PollRule(const TSegmentSpan& s) {
    if (2 <= s.Words && s.Words < 100 && s.FirstBlock.Included && s.PollCSS) {
        SEGMENTATOR_BCD1("PollRule");
        return STP_AUX;
    }
    return STP_NONE;
}

inline ESegmentType FormRule(const TSegmentSpan& s) {
    if (s.Inputs && s.Words < 75 && Less(s.AvWordsPerInput(), 20.f)) {
        SEGMENTATOR_BCD1("FormRule");
        return STP_AUX;
    }
    return STP_NONE;
}

inline ESegmentType HeaderRule(const TSegmentSpan& sg) {
    if (sg.IsHeader) {
        SEGMENTATOR_BCD1("HeaderRule");
        return STP_HEADER;
    }
    return STP_NONE;
}

inline bool LinksAreMostlyLocal(const TSegmentSpan& s) {
    return Greater(s.AvLocalLinksPerLink(), 0.6f) && s.Domains < 3;
}

inline ESegmentType ChardinLinksRule(const TSegmentSpan& s) {
    if (s.Links > 1 && !LinksAreMostlyLocal(s) && (Less(s.AvWordsPerLink(), 35.f) || Less(
            s.AvWordsPerDomain(), 35.f))) {
        SEGMENTATOR_BCD1("ChardinLinksRule");
        return STP_LINKS;
    }
    return STP_NONE;
}

inline ESegmentType ChardinMenuRule(const TSegmentSpan& s) {
    if (s.Links > 1 && LinksAreMostlyLocal(s) && LessEqual(s.AvWordsPerLink(), 3.5f)) {
        SEGMENTATOR_BCD1("ChardinMenuRule");
        return STP_MENU;
    }
    return STP_NONE;
}

inline ESegmentType ChardinReferatRule(const TSegmentSpan& s) {
    if (s.Links > 1 && LinksAreMostlyLocal(s) && Less(s.AvWordsPerLink(), 35.f)) {
        SEGMENTATOR_BCD1("ChardinReferatRule");
        return STP_REFERAT;
    }
    return STP_NONE;
}

inline ESegmentType ArtemkinReferatRule(const TSegmentSpan& s) {
    if (s.Links && LinksAreMostlyLocal(s) && s.Words > 7 && Greater(s.AvLinkWordsPerWord(), 0.5f)) {
        SEGMENTATOR_BCD1("ArtemkinReferatRule");
        return STP_REFERAT;
    }
    return STP_NONE;
}

inline ESegmentType ArtemkinLinksRule(const TSegmentSpan& s) {
    if (s.Links && !LinksAreMostlyLocal(s) && s.Words > 7 && Greater(s.AvLinkWordsPerWord(), 0.5f)) {
        SEGMENTATOR_BCD1("ArtemkinLinksRule");
        return STP_LINKS;
    }
    return STP_NONE;
}

inline ESegmentType ChardinAuxRule(const TSegmentSpan& s) {
    if (Less(s.AvWordsPerBlock(), 4.f) && s.Blocks >= 3) {
        SEGMENTATOR_BCD1("ChardinAuxRule");
        return STP_AUX;
    }
    return STP_NONE;
}

inline ESegmentType ChardinContentRule(const TSegmentSpan& s) {
    if (10 <= s.Words) {
        SEGMENTATOR_BCD1("ChardinContentRule");
        return STP_CONTENT;
    }
    return STP_NONE;
}

//inline ESegmentType LinksPostRule(const TSegmentSpan * /*sgPrev*/, const TSegmentSpan & sg,
//        ESegmentType type) {
//    if ((STP_NONE == type || STP_AUX == type || STP_MENU == type || STP_CONTENT == type && sg.Words
//            < 100) && (sg.FirstBlock.Included && (sg.AdsCSS || sg.AdsHeader) && sg.Domains)) {
//        SEGMENTATOR_BCD1("LinksPostRule");
//        return STP_LINKS;
//    }
//    return type;
//}

inline ESegmentType AuxPostRule(ESegmentType type) {
    if (STP_NONE == type) {
        SEGMENTATOR_BCD1("AuxPostRule");
        return STP_AUX;
    }
    return type;
}

inline ESegmentType PreClassify(const TSegmentSpan& span) {
    ESegmentType type = STP_NONE;
    SEGMENTATOR_BCD0(i, span.Begin, span.End);

    if ((type = ChardinFooterRule(span)) != 0)
        return type;

    if ((type = ChardinLinksRule(span)) != 0)
        return type;

    if ((type = ArtemkinLinksRule(span)) != 0)
        return type;

    if ((type = AdsRule(span)) != 0)
        return type;

    if ((type = PollRule(span)) != 0)
        return type;

    if ((type = FormRule(span)) != 0)
        return type;

    if ((type = HeaderRule(span)) != 0)
        return type;

    if ((type = ChardinMenuRule(span)) != 0)
        return type;

    if ((type = ChardinReferatRule(span)) != 0)
        return type;

    if ((type = ArtemkinReferatRule(span)) != 0)
        return type;

    if ((type = ChardinAuxRule(span)) != 0)
        return type;

    if ((type = ChardinContentRule(span)) != 0)
        return type;

    return type;
}

inline ESegmentType PostClassifySpan(const TSegmentSpan& /*span*/, ESegmentType type) {
    return AuxPostRule(type);
}

inline ESegmentType ClassifySpan(const TSegmentSpan& span) {
    return PostClassifySpan(span, PreClassify(span));
}

template <typename tlist>
void FixNumbers(tlist& segs) {
    ui32 num = 0;
    ui32 size = segs.size();

    for (typename tlist::iterator it = segs.begin(); it != segs.end(); ++it) {
        it->Total = size;
        it->Number = num++;
    }
}

template<typename TListType>
inline void Classify(TListType& segs) {
    FixNumbers(segs);

    for (typename TListType::iterator it = segs.begin(); it != segs.end(); ++it)
        it->Type = ClassifySpan(*it);
}

}

#undef SEGMENTATOR_BCD0
#undef SEGMENTATOR_BCD1
