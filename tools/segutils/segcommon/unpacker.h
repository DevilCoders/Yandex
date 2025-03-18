#pragma once

#include <kernel/tarc/disk/unpacker.h>
#include <kernel/tarc/markup_zones/unpackers.h>
#include <util/string/cast.h>
#include <util/generic/hash.h>

#include <library/cpp/deprecated/dater_old/dater_stats.h>

namespace NDater {
using NSegm::TArchiveSents;

struct TDateSpan: TDaterDate, TArchiveZoneSpan {
public:
    TDateSpan(const TDaterDate& d = TDaterDate(), const TArchiveZoneSpan& s = TArchiveZoneSpan()) :
        TDaterDate(d), TArchiveZoneSpan(s) {
    }
};

typedef TVector<TDateSpan> TDateSpans;
typedef THashMap<TString, TString> TAttrs;
typedef TVector<TArchiveZoneSpan> TArchiveZoneSpans;

TDaterDate GetBestDateFromArchive(const TAttrs& attrs);
TDaterStats& GetDaterStatsFromArchive(const TAttrs& attrs, TDaterStats&);
TDateSpans GetDateSpansFromArchive(const TArchiveZoneSpans& arczone,
        const TArchiveSents& arcsents);
}
