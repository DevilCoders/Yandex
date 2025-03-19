#include "view_sent.h"

#include "arcreader.h" // for TTArcViewSentReader
#include "unpackers.h" // for NSegm

#include <kernel/tarc/docdescr/docdescr.h>  // for TDocDescr

#include <library/cpp/deprecated/dater_old/date_attr_def/date_attr_def.h> // for DATER_DATE_ATTR
#include <library/cpp/charset/wide.h>

TTArcViewSentReader::TTArcViewSentReader(const bool sentAttrs, const bool decodeSentAttrs)
    : TSentReader(sentAttrs)
    , SegStMarker(ASCIIToWide("\t" + TString(NArchiveZoneAttr::NSegm::STATISTICS) + "\t"))
    , DaterDateMarker(ASCIIToWide("\t" DATER_DATE_ATTR "\t"))
    , SentAttrs(sentAttrs)
    , DecodeSentAttrs(decodeSentAttrs)
{
}

void TTArcViewSentReader::DecodeSegStAttribute(TUtf16String& raw, const TString& s, ui8 segVersion) const {
    if(s.empty())
        return;

    NSegm::TSegmentSpan sp;
    NSegm::DecodeSegmentSpan(sp, s, segVersion);
    TUtf16String res = CharToWide(sp.ToString(), csYandex);
    size_t attrbegin = raw.find(SegStMarker);
    Y_ASSERT(TUtf16String::npos != attrbegin);
    attrbegin += SegStMarker.size();
    size_t attrend = raw.find('\t', attrbegin);
    size_t n = TUtf16String::npos;

    if(TUtf16String::npos != attrend)
        n = attrend - attrbegin;

    raw.replace(attrbegin, n, res);
}

TUtf16String TTArcViewSentReader::GetText(const TArchiveSent& sent, ui8 segVersion) const {
    if(!SentAttrs || !DecodeSentAttrs)
        return TSentReader::GetText(sent, segVersion);

    TUtf16String w = TSentReader::GetText(sent, segVersion);
    DecodeSegStAttribute(w, sent.GetSentAttribute(NArchiveZoneAttr::NSegm::STATISTICS), segVersion);

    return w;
}
