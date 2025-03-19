#include "unpackers.h"

#include <library/cpp/charset/wide.h>

#include <util/generic/strbuf.h>


namespace NSegm {

ESegmentType GetType(EArchiveZone zone) {
    switch (zone) {
    default:
        return STP_NONE;
    case AZ_SEGAUX:
        return STP_AUX;
    case AZ_SEGCONTENT:
        return STP_CONTENT;
    case AZ_SEGCOPYRIGHT:
        return STP_FOOTER;
    case AZ_SEGHEAD:
        return STP_HEADER;
    case AZ_SEGLINKS:
        return STP_LINKS;
    case AZ_SEGMENU:
        return STP_MENU;
    case AZ_SEGREFERAT:
        return STP_REFERAT;
    }
}


TSegmentSpans GetSegmentsFromArchive(const TArchiveMarkupZones& zones) {
    const TString attrName(NArchiveZoneAttr::NSegm::STATISTICS);

    TSegmentSpans result;

    for (ui32 i = 0; i < ArchiveZonesCount; ++i) {
        EArchiveZone zt = ArchiveZones[i];
        const TArchiveZone& z = zones.GetZone(zt);
        const TArchiveZoneAttrs& za = zones.GetZoneAttrs(zt);

        TSegmentSpans tmp = GetSpansFromZone<TSegmentSpan> (z.Spans.begin(), z.Spans.size());
        result.reserve(result.size() + z.Spans.size());

        for (size_t j = 0; j < tmp.size(); ++j) {
            tmp[j].Type = (ui8)GetType(zt);
            TConstSpanAttributes spanAttributes = za.GetSpanAttrs(z.Spans[j]);
            const THashMap<TString, TUtf16String>* attrs = spanAttributes.AttrsHash;
            const TString* segmentAttrs = spanAttributes.SegmentAttrs;
            TString attrVal;
            if (attrs && attrs->find(attrName) != attrs->end()) {
                attrVal = UnescapeAttribute(WideToChar(attrs->find(attrName)->second, CODES_YANDEX));
            } else if (segmentAttrs) {
                attrVal = *segmentAttrs;
            }
            if (!attrVal.empty()) {
                DecodeSegmentSpan(tmp[j], attrVal.begin(), attrVal.size(), zones.GetSegVersion());
                result.push_back(tmp[j]);
            }
        }
    }

    Sort(result.begin(), result.end());
    return result;
}

THeaderSpans GetHeadersFromArchive(const TArchiveMarkupZones& zones) {
    const TArchiveZone& h = zones.GetZone(AZ_HEADER);
    return GetSpansFromZone<THeaderSpan> (h.Spans.begin(), h.Spans.size());
}

TMainHeaderSpans GetMainHeadersFromArchive(const TArchiveMarkupZones& zones) {
    const TArchiveZone& h = zones.GetZone(AZ_MAIN_HEADER);
    return GetSpansFromZone<TMainHeaderSpan> (h.Spans.begin(), h.Spans.size());
}

TMainContentSpans GetMainContentsFromArchive(const TArchiveMarkupZones& zones) {
    const TArchiveZone& h = zones.GetZone(AZ_MAIN_CONTENT);
    return GetSpansFromZone<TMainContentSpan> (h.Spans.begin(), h.Spans.size());
}

}
