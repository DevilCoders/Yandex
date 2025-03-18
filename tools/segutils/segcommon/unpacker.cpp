#include "unpacker.h"

#include <library/cpp/deprecated/dater_old/scanner/scanner.h>

namespace NDater {

template<typename TRes>
TRes GetFromArchive(const TAttrs& attrs, const char* marker) {
    TAttrs::const_iterator it = attrs.find(marker);

    if (it != attrs.end())
        return TRes::FromString(it->second);

    return TRes();
}

TDaterDate GetBestDateFromArchive(const TAttrs& attrs) {
    TAttrs::const_iterator it = attrs.find(DATER_DATE_ATTR);

    if (it != attrs.end())
        return TDaterDate::FromString(it->second);

    return TDaterDate();
}

TDaterStats& GetDaterStatsFromArchive(const TAttrs& attrs, TDaterStats& st) {
    TAttrs::const_iterator it = attrs.find(DATER_STATS_ATTR);

    if (it != attrs.end())
        st.FromString(it->second);

    it = attrs.find(DATER_STATS_DM_ATTR);

    if (it != attrs.end())
        st.FromString(it->second);

    return st;
}

TDateSpans GetDateSpansFromArchive(const TArchiveZoneSpans& spans, const TArchiveSents& sents) {
    TDateSpans dspans;
    dspans.reserve(spans.size());

    TArchiveSents::const_iterator sit = sents.begin();

    for (TArchiveZoneSpans::const_iterator it = spans.begin(); it != spans.end() && sit
            != sents.end(); ++it) {
        while (sit != sents.end() && sit->Number < it->SentBeg)
            ++sit;

        if (sit == sents.end() || sit->Number > it->SentEnd)
            continue;

        const TUtf16String& w = sit->OnlyText.substr(it->OffsetBeg, it->OffsetEnd - it->OffsetBeg);
        TDateCoords cs = ScanText(w.data(), w.size());
        FilterOverlappingDates(cs);

        if (cs.empty() || cs.size() > 1)
            continue;

        dspans.push_back(TDateSpan(cs.front(), *it));
    }

    return dspans;
}

}
