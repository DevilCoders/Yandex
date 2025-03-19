#pragma once

#include "text_markup.h"

#include <kernel/tarc/docdescr/docdescr.h>

#include <kernel/segmentator/structs/structs.h>

#include <util/generic/algorithm.h>


namespace NSegm {

const EArchiveZone ArchiveZones[] = { AZ_SEGAUX, AZ_SEGCONTENT, AZ_SEGCOPYRIGHT, AZ_SEGHEAD,
        AZ_SEGLINKS, AZ_SEGMENU, AZ_SEGREFERAT };
const ui32 ArchiveZonesCount = sizeof(ArchiveZones) / sizeof(EArchiveZone);


ESegmentType GetType(EArchiveZone zone);

typedef TVector<TArchiveSent> TArchiveSents;

template<typename TSpan>
TVector<TSpan> GetSpansFromZone(const TArchiveZoneSpan* zspanbegin, size_t zspancnt) {
    TVector<TSpan> spans;
    spans.reserve(zspancnt);

    for (size_t off = 0; off < zspancnt; ++off) {
        const TArchiveZoneSpan& z = zspanbegin[off];
        if (z.Empty())
            continue;

        TPosting beg = 0;
        TPosting end = 0;
        SetPosting(beg, z.SentBeg, 1);
        SetPosting(end, z.SentEnd + bool(z.OffsetEnd), 1);
        if (beg < end)
            spans.push_back(TSpan(beg, end));
    }

    return spans;
}

ESegmentType GetType(EArchiveZone zone);

TSegmentSpans GetSegmentsFromArchive(const TArchiveMarkupZones& zones);

THeaderSpans GetHeadersFromArchive(const TArchiveMarkupZones& zones);
TMainHeaderSpans GetMainHeadersFromArchive(const TArchiveMarkupZones& zones);
TMainContentSpans GetMainContentsFromArchive(const TArchiveMarkupZones& zones);

}
