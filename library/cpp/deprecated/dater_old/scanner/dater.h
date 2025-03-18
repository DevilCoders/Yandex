#pragma once

#include <util/generic/hash.h>
#include <util/stream/str.h>
#include <kernel/segmentator/structs/structs.h>
#include <kernel/segmentator/structs/spans.h>

#include "scanner.h"

namespace NDater {
    struct TDaterException: public yexception {
    };

    struct TDatePosition : TDaterDate, NSegm::TBaseSpan {
        explicit TDatePosition(const TDaterDate& date = TDaterDate(), const TBaseSpan& span = TBaseSpan())
            : TDaterDate(date)
            , TBaseSpan(span)
        {
        }

        friend bool operator<(const TDatePosition& a, const TDatePosition& b) {
            return (const TDaterDate&)a < (const TDaterDate&)b;
        }
    };

    typedef TVector<TDatePosition> TDatePositions;

    void MakeDatePositions(TDatePositions& dps, const TDateCoords& dates, const NSegm::TPosCoords& coords, ui8 from = TDaterDate::FromUnknown);
    void MakeDatePositions(TDatePositions& dps, const TDateCoords& dates);

    ui8 SegmentTypeToDateFrom(ui8 t);

    void MarkDatePositions(TDatePositions& textDates, const NSegm::TSpan& title, const NSegm::TMainContentSpans&, const NSegm::TSegmentSpans&);

    void FilterOverlappingDates(TDatePositions&);
    void FilterFutureDates(TDatePositions& dates, const TDaterDate& indexed);
    void FilterInsaneInternetDates(TDatePositions& dates);
    TDatePositions FilterYearOnlyDates(TDatePositions& dates);

    void FilterUnreliableDates(TDatePositions& urldates, TDatePositions& textdates,
                               TDatePositions& urlyearonly, TDatePositions& textyearonly,
                               TDaterDate indexed);

    inline void FilterUnreliableDates(TDatePositions& urldates, TDatePositions& textdates, TDaterDate indexed) {
        TDatePositions urlyo, textyo;
        FilterUnreliableDates(urldates, textdates, urlyo, textyo, indexed);
    }

    void FindBestDate(TDatePosition& best,
                      const TDatePositions& urldates, const TDatePositions& textdates, const TDaterDate& indexed,
                      TDatePositions* top = nullptr, ui32 maxtop = 8);
    void FindBestDate(TDateCoord& best,
                      const TDateCoords& urldates, const TDateCoords& textdates, const TDaterDate& indexed,
                      TDateCoords* top = nullptr, ui32 maxtop = 8);

    typedef TVector<TDatePositions> TDatePositionsBySource;

    void SortDatesBySources(TDatePositionsBySource&, const TDatePositions&);
    void FindBestDateFromSorted(TDatePositions&, const TDatePositionsBySource&, const TDaterDate& indexed);

    ui32 GetHostYear(TStringBuf host);

    template <typename TDateSpan>
    TString SaveDatesList(const TVector<TDateSpan>& d) {
        TStringStream s;
        for (ui32 i = 0, sz = d.size(); i < sz; ++i) {
            if (i)
                s << ", ";
            s << d[i].ToString();
        }

        return s.Str();
    }

}
