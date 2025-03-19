#include "doc_attr_filler.h"

#include <library/cpp/charset/wide.h>
#include <library/cpp/packedtypes/fixed_point.h>

#include <kernel/indexer/face/inserter.h>
#include <kernel/tarc/markup_zones/unpackers.h>
#include <library/cpp/string_utils/base64/base64.h>

namespace {

template<class T>
inline void InsertSpans(const T& hSpans, EArchiveZone zoneId, IDocumentDataInserter& inserter, bool archiveOnly = false) {
    TString zoneName = ToString(zoneId);
    for (typename T::const_iterator it = hSpans.begin(); it != hSpans.end(); ++it) {
        if (!it->NoSents())
            inserter.StoreZone(zoneName.data(), it->Begin.SentAligned(), it->End.SentAligned(), archiveOnly);
    }
}

inline void InsertSpans(const NSegm::TSegmentSpans& hSpans, NSegm::ESegmentType type, EArchiveZone zoneId, IDocumentDataInserter& inserter) {
    TString zoneName = ToString(zoneId);
    for (NSegm::TSegmentSpans::const_iterator it = hSpans.begin(); it != hSpans.end(); ++it) {
        if (!it->NoSents() && (NSegm::ESegmentType)it->Type == type)
            inserter.StoreZone(zoneName.data(), it->Begin.SentAligned(), it->End.SentAligned(), false);
    }
}

inline EArchiveZone ResolveTypedSpanZone(const NSegm::TTypedSpan& s) {
    if (s.Type == NSegm::TTypedSpan::ST_LIST) {
        return Min(static_cast<EArchiveZone>(AZ_LIST0 + s.Depth), AZ_LIST5);
    }
    if (s.Type == NSegm::TTypedSpan::ST_LIST_ITEM) {
        return Min(static_cast<EArchiveZone>(AZ_LIST_ITEM0 + s.Depth), AZ_LIST_ITEM5);
    }
    return AZ_LIST0;
}

inline void InsertSpans(const NSegm::TTypedSpans& spans, IDocumentDataInserter& inserter) {
    for(NSegm::TTypedSpans::const_iterator it = spans.begin(); it != spans.end(); ++it) {
        if (!it->NoSents()) {
            TString zoneName = ToString(ResolveTypedSpanZone(*it));
            inserter.StoreZone(zoneName.data(), it->Begin.SentAligned(), it->End.SentAligned(), true);
        }
    }
}

} // namespace

void Fill(const NSegm::TSegmentSpans& spans, IDocumentDataInserter* inserter) {
    //JIRA ticket BUKI-1109. Corresponding logs:
    //SegmentAuxSpacesInText - segment_8_item_2.dump.normalized.i5000.log
    //SegmentAuxAlphasInText - segment_8_item_4.dump.normalized.i5000.log
    //SegmentContentCommasInText - segment_9_item_3.dump.normalized.i5000.log

    TMap<TString, ui32> m;

    double segmentWordPortionFromMainContent = 0;
    size_t allWordNumber = 0;
    for (NSegm::TSegmentSpans::const_iterator it = spans.begin(); it != spans.end(); ++it) {
        if (!it->NoSents()) {
            if (static_cast<NSegm::ESegmentType>(it->Type) == NSegm::STP_AUX) {
                m["SegmentAuxSpacesInText"] += it->SpacesInText;
                m["SegmentAuxAlphasInText"] += it->AlphasInText;
            }
            if (static_cast<NSegm::ESegmentType>(it->Type) == NSegm::STP_CONTENT) {
                m["SegmentContentCommasInText"] += it->CommasInText;
            }
            if ((float)NPackedFloat::float16<1>(it->MainWeight) >= 0.03869058846) {
                segmentWordPortionFromMainContent += it->Words;
            }
            allWordNumber += it->Words;
        }
    }

    typedef NFixedPoint::TFixedPoint<10000> TFPoint;
    if (m["SegmentAuxSpacesInText"]) {
        float rawValue = m["SegmentAuxSpacesInText"];
        rawValue = rawValue / (rawValue + 50.734);
        const TFPoint value = TFPoint(rawValue);
        TString val = Sprintf("%d.%.4d", (ui32)value.Int(), (ui32)value.Frac());
        inserter->StoreErfDocAttr("SegmentAuxSpacesInText", val);
    }
    if (m["SegmentAuxAlphasInText"]) {
        float rawValue = m["SegmentAuxAlphasInText"];
        rawValue = rawValue / (rawValue + 399.265);
        const TFPoint value = TFPoint(rawValue);
        TString val = Sprintf("%d.%.4d", (ui32)value.Int(), (ui32)value.Frac());
        inserter->StoreErfDocAttr("SegmentAuxAlphasInText", val);
    }
    if (m["SegmentContentCommasInText"]) {
        float rawValue = m["SegmentContentCommasInText"];
        rawValue = rawValue / (rawValue + 285.663);
        const TFPoint value = TFPoint(rawValue);
        TString val = Sprintf("%d.%.4d", (ui32)value.Int(), (ui32)value.Frac());
        inserter->StoreErfDocAttr("SegmentContentCommasInText", val);
    }
    if (allWordNumber != 0) {
        segmentWordPortionFromMainContent /= allWordNumber;
        const TFPoint value = TFPoint(segmentWordPortionFromMainContent);
        TString val = Sprintf("%d.%.4d", (ui32)value.Int(), (ui32)value.Frac());
        inserter->StoreErfDocAttr("SegmentWordPortionFromMainContent", val);
    }
}

void StoreSegmentatorSpans(const NSegm::TSegmentatorHandler<>& handler, IDocumentDataInserter& inserter) {
    const NSegm::TSegmentSpans& spans = handler.GetSegmentSpans();
    {
        const size_t bsz = Base64EncodeBufSize(NSegm::TStoreSegmentSpanData::NBytes);
        TTempBuf tmp(bsz);
        TTempArray<wchar16> wtmp(bsz);

        for (NSegm::TSegmentSpans::const_iterator it = spans.begin(); it != spans.end(); ++it) {
            if (it->NoSents())
                continue;
            NSegm::TStoreSegmentSpanData data = NSegm::CreateStoreSegmentSpanData(*it);
            TStringBuf preparedAttr = Base64Encode(TStringBuf(data.Bytes, (ui32)NSegm::TStoreSegmentSpanData::NBytes), tmp.Data());
            CharToWide(preparedAttr.data(), preparedAttr.size(), wtmp.Data(), csYandex);
            inserter.StoreArchiveZoneAttr(NArchiveZoneAttr::NSegm::STATISTICS, wtmp.Data(), preparedAttr.size(), it->Begin.SentAligned());
        }
    }

    InsertSpans(handler.GetSegmentSpans(), NSegm::STP_AUX, AZ_SEGAUX, inserter);
    InsertSpans(handler.GetSegmentSpans(), NSegm::STP_CONTENT, AZ_SEGCONTENT, inserter);
    InsertSpans(handler.GetSegmentSpans(), NSegm::STP_FOOTER, AZ_SEGCOPYRIGHT, inserter);
    InsertSpans(handler.GetSegmentSpans(), NSegm::STP_HEADER, AZ_SEGHEAD, inserter);
    InsertSpans(handler.GetSegmentSpans(), NSegm::STP_LINKS, AZ_SEGLINKS, inserter);
    InsertSpans(handler.GetSegmentSpans(), NSegm::STP_MENU, AZ_SEGMENU, inserter);
    InsertSpans(handler.GetSegmentSpans(), NSegm::STP_REFERAT, AZ_SEGREFERAT, inserter);
    InsertSpans(handler.GetHeaderSpans(), AZ_HEADER, inserter);
    InsertSpans(handler.GetStrictHeaderSpans(), AZ_STRICT_HEADER, inserter);
    InsertSpans(handler.GetMainContentSpans(), AZ_MAIN_CONTENT, inserter);
    InsertSpans(handler.GetMainHeaderSpans(), AZ_MAIN_HEADER, inserter);
    InsertSpans(handler.GetListSpans(), inserter);
    InsertSpans(handler.GetListItemSpans(), inserter);
    InsertSpans(handler.GetTableSpans(), AZ_TABLE, inserter, true);
    InsertSpans(handler.GetTableRowSpans(), AZ_TABLE_ROW, inserter, true);
    InsertSpans(handler.GetTableCellSpans(), AZ_TABLE_CELL, inserter, true);
}

