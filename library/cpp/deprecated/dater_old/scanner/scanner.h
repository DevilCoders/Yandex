#pragma once

#include <library/cpp/deprecated/dater_old/structs.h>
#include <kernel/segmentator/structs/spans.h>

namespace NDater {
    using NSegm::TCoord;

    struct TDateCoord : TDaterDate, TCoord {
        explicit TDateCoord(TDaterDate date = TDaterDate(), TCoord coord = TCoord())
            : TDaterDate(date)
            , TCoord(coord)
        {
        }
    };

    template <typename TDateSpan>
    bool LessCoord(const TDateSpan& a, const TDateSpan& b) {
        return a.Begin < b.Begin;
    }

    typedef TVector<TDateCoord> TDateCoords;

    TDateCoords ScanHost(const char* p0, const char* pe);
    TDateCoords ScanUrl(const char* p0, const char* pe);
    TDateCoords ScanText(const wchar16* p0, const wchar16* pe);
    TDateCoords ScanHumanUrl(const wchar16* p0, const wchar16* pe);

    inline TDateCoords ScanUrl(const char* p0, size_t len) {
        return ScanUrl(p0, p0 + len);
    }

    inline TDateCoords ScanText(const wchar16* p0, size_t len) {
        return ScanText(p0, p0 + len);
    }

    inline TDateCoords ScanHumanUrl(const wchar16* p0, size_t len) {
        return ScanHumanUrl(p0, p0 + len);
    }

    void FilterOverlappingDates(TDateCoords&);
    void FilterInsaneInternetDates(TDateCoords&);
    void FilterFutureDates(TDateCoords&, const TDaterDate& indexed);
    TDateCoords FilterYearOnlyDates(TDateCoords&);

}
