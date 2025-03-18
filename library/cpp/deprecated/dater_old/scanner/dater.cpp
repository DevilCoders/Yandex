#include "dater.h"

#include <kernel/segmentator/structs/classification.h>
#include <kernel/segmentator/structs/merge.h>
#include <kernel/segmentator/structs/spans.h>

namespace NDater {
    using namespace NDatetime;
    using namespace NSegm;

    ui32 GetHostYear(TStringBuf host) {
        const TDateCoords& c = ScanHost(host.begin(), host.end());
        for (TDateCoords::const_iterator it = c.begin(); it != c.end(); ++it)
            if (it->Year)
                return it->Year;
        return TDaterDate::ErfZeroYear;
    }

    static bool MoreTrust(const TDaterDate& a, const TDaterDate& b) {
        ui32 tra = (a.TrustLevel() << 4) + ((!!a.Day) << 1) + a.WordPattern + a.LongYear;
        ui32 trb = (b.TrustLevel() << 4) + ((!!b.Day) << 1) + b.WordPattern + b.LongYear;
        return tra > trb || tra == trb && a > b;
    }

    template <typename TDateSpan>
    static bool Overlap(const TDateSpan& a, const TDateSpan& b) {
        return a.Begin == b.Begin || a.End == b.End || a.Begin < b.Begin && a.End > b.Begin || b.Begin < a.Begin && b.End > a.Begin;
    }

    template <typename TDateSpan>
    static void FilterOverlappingDateSpans(TVector<TDateSpan>& dates) {
        Sort(dates.begin(), dates.end(), LessCoord<TDateSpan>);

        TVector<TDateSpan> res;
        res.reserve(dates.size());

        for (typename TVector<TDateSpan>::iterator it = dates.begin(); it != dates.end(); ++it) {
            if (res.empty() || !Overlap(res.back(), *it)) {
                res.push_back(*it);
                continue;
            }

            TDaterDate a = res.back();
            TDaterDate b = *it;

            // the dates are in conflict, delete both
            TDaterDate a1 = a < b ? a : b;
            TDaterDate b1 = a < b ? b : a;

            if (a1.Year != b1.Year || a1.Month && a1.Month != b1.Month || a1.Day && a1.Day != b1.Day) {
                res.pop_back();
                continue;
            }

            // the dates don't conflict and the former is better defined
            if (a.Day || a.Month && !b.Day)
                continue;

            res.pop_back();
            res.push_back(*it);
        }

        dates.swap(res);
    }

    void FilterOverlappingDates(TDateCoords& dates) {
        FilterOverlappingDateSpans(dates);
    }

    void FilterOverlappingDates(TDatePositions& dates) {
        FilterOverlappingDateSpans(dates);
    }

    template <typename TDateSpan>
    static void FilterInsaneInternetDateSpans(TVector<TDateSpan>& dates) {
        typedef TVector<TDateSpan> TDateSpans;
        TDateSpans res;
        res.reserve(dates.size());
        for (typename TDateSpans::const_iterator it = dates.begin(); it != dates.end(); ++it)
            if (it->SaneInternetDate())
                res.push_back(*it);
        dates.swap(res);
    }

    void FilterInsaneInternetDates(TDateCoords& dates) {
        FilterInsaneInternetDateSpans(dates);
    }

    void FilterInsaneInternetDates(TDatePositions& dates) {
        FilterInsaneInternetDateSpans(dates);
    }

    template <typename TDateSpan>
    static void FilterFutureDateSpans(TVector<TDateSpan>& dates, const TDaterDate& indexed) {
        TDaterDate tommorrow = TDaterDate::FromSimpleTM(indexed.ToSimpleTM().Add(TSimpleTM::F_DAY, 1));
        TVector<TDateSpan> le;
        le.reserve(dates.size());

        for (typename TVector<TDateSpan>::const_iterator it = dates.begin(); it != dates.end(); ++it)
            if (tommorrow >= *it)
                le.push_back(*it);

        dates.swap(le);
    }

    void FilterFutureDates(TDatePositions& dates, const TDaterDate& indexed) {
        FilterFutureDateSpans(dates, indexed);
    }

    void FilterFutureDates(TDateCoords& dates, const TDaterDate& indexed) {
        FilterFutureDateSpans(dates, indexed);
    }

    template <typename TDateSpan>
    static TVector<TDateSpan> FilterYearOnlyDateSpans(TVector<TDateSpan>& dates) {
        TVector<TDateSpan> nyo;
        TVector<TDateSpan> yo;
        nyo.reserve(dates.size());
        yo.reserve(dates.size());

        for (typename TVector<TDateSpan>::const_iterator it = dates.begin(); it != dates.end(); ++it)
            (it->Month ? nyo : yo).push_back(*it);

        dates.swap(nyo);
        return yo;
    }

    template <typename TDateSpan>
    static TVector<TDateSpan> FilterNoYearDateSpans(TVector<TDateSpan>& dates) {
        TVector<TDateSpan> ny;
        TVector<TDateSpan> y;
        ny.reserve(dates.size());
        y.reserve(dates.size());

        for (typename TVector<TDateSpan>::const_iterator it = dates.begin(); it != dates.end(); ++it)
            (it->Year ? y : ny).push_back(*it);

        dates.swap(y);
        return ny;
    }

    TDateCoords FilterYearOnlyDates(TDateCoords& dates) {
        return FilterYearOnlyDateSpans(dates);
    }

    TDatePositions FilterYearOnlyDates(TDatePositions& dates) {
        return FilterYearOnlyDateSpans(dates);
    }

    TDateCoords FilterNoYearDates(TDateCoords& dates) {
        return FilterNoYearDateSpans(dates);
    }

    TDatePositions FilterNoYearDates(TDatePositions& dates) {
        return FilterNoYearDateSpans(dates);
    }

    template <typename TDateSpan>
    static void FilterUnconfirmedUrlDatesImpl(TVector<TDateSpan>& urldates, TVector<TDateSpan>& textdates) {
        typedef TVector<TDateSpan> TDateSpans;

        if (textdates.empty())
            return;

        TDateSpans confirmed;
        confirmed.reserve(urldates.size());

        for (typename TDateSpans::const_iterator it = urldates.begin(); it != urldates.end(); ++it) {
            TDateSpan toConfirm;

            if (it->LongYear) {
                toConfirm = *it;
                toConfirm.Day = 0;
            }

            for (typename TDateSpans::const_iterator tit = textdates.begin(); tit != textdates.end(); ++tit) {
                // cannot confirm
                if (!SameButDay(*it, *tit))
                    continue;

                // year and month are confirmed
                if (!toConfirm) {
                    toConfirm = *it;
                    toConfirm.Day = 0; // to be confirmed
                }

                if (!it->Day || it->Day == tit->Day) {
                    toConfirm.Day = it->Day;
                    break;
                }
            }

            if (it->Day && !toConfirm.Day) {
                for (typename TDateSpans::const_iterator tit = textdates.begin(); tit != textdates.end(); ++tit) {
                    // cannot confirm
                    if (!SameButDay(toConfirm, *tit))
                        continue;

                    // the day is confirmed
                    i32 dd = it->DiffDays(*tit);
                    if (it->Day && tit->Day && abs(dd) < 2) {
                        toConfirm.Day = tit->Day;
                        break;
                    }
                }
            }

            if (toConfirm)
                confirmed.push_back(toConfirm);
        }

        DoSwap(urldates, confirmed);
    }

    template <typename TDateSpan>
    static void FilterUnreliableDatesImpl(TVector<TDateSpan>& urldates, TVector<TDateSpan>& textdates,
                                          TVector<TDateSpan>& urlyearonly, TVector<TDateSpan>& textyearonly,
                                          TDaterDate indexed) {
        FilterFutureDates(urldates, indexed);
        FilterFutureDates(textdates, indexed);

        FilterInsaneInternetDates(urldates);
        FilterInsaneInternetDates(textdates);

        urlyearonly = FilterYearOnlyDates(urldates);
        textyearonly = FilterYearOnlyDates(textdates);

        FilterNoYearDates(urldates);
        FilterNoYearDates(textdates);
    }

    void FilterUnreliableDates(TDatePositions& urldates, TDatePositions& textdates,
                               TDatePositions& urlyearonly, TDatePositions& textyearonly,
                               TDaterDate indexed) {
        FilterUnreliableDatesImpl(urldates, textdates, urlyearonly, textyearonly, indexed);
    }

    ui8 SegmentTypeToDateFrom(ui8 t) {
        switch (t) {
            default:
                return TDaterDate::FromText;
            case STP_CONTENT:
                return TDaterDate::FromContent;
            case STP_FOOTER:
                return TDaterDate::FromFooter;
        }
    }

    void MakeDatePositions(TDatePositions& dps, const TDateCoords& dates) {
        dps.reserve(dps.size() + dates.size());

        for (TDateCoords::const_iterator it = dates.begin(); it != dates.end(); ++it)
            dps.push_back(TDatePosition(*it));
    }

    void MakeDatePositions(TDatePositions& dps, const TDateCoords& dates, const TPosCoords& coords, ui8 from) {
        if (dates.empty() || coords.empty())
            return;

        dps.reserve(dps.size() + dates.size());
        TDateCoords dcs(dates);
        Sort(dcs.begin(), dcs.end(), TCoordCmp<true>());

        //invariant: there are no gaps in posting coordinates,
        // so we must get a posting for the beginning and for the ending
        TPosCoords::const_iterator bit = coords.begin();
        for (TDateCoords::const_iterator dit = dcs.begin(); dit != dcs.end(); ++dit) {
            while (bit != coords.end() && bit->End <= dit->Begin)
                ++bit;

            Y_VERIFY(bit != coords.end() && bit->Begin <= dit->Begin, " ");

            TPosCoords::const_iterator eit = bit;
            while (eit != coords.end() && eit->Begin <= dit->End)
                ++eit;

            TAlignedPosting bpos = bit->Pos;
            TAlignedPosting epos;

            if (eit != coords.end() && eit->Pos > bpos)
                epos = eit->Pos;
            else
                epos = bpos.NextSent();

            dps.push_back(TDatePosition(*dit, TSpan(bpos, epos)));

            if (from)
                dps.back().From = from;
        }
    }

    void MarkDatePositions(TDatePositions& dps, const TSpan& title, const TMainContentSpans&,
                           const TSegmentSpans& segs) {
        if (dps.empty())
            return;

        TSegmentSpans::const_iterator sit = segs.begin();
        TDatePositions dates;
        dates.reserve(dps.size());

        for (TDatePositions::iterator it = dps.begin(); it != dps.end(); ++it) {
            if (title.Contains(it->Begin)) {
                if (title.ContainsInEnd(it->End)) {
                    it->From = TDaterDate::FromTitle;
                    dates.push_back(*it);
                }

                continue;
            }

            if (segs.empty()) {
                it->From = TDaterDate::FromText;
                dates.push_back(*it);
                continue;
            }

            while (sit != segs.end() && !sit->Contains(it->Begin))
                ++sit;

            if (sit != segs.end() && sit->MainContentFrontAdjoining) {
                it->From = TDaterDate::FromBeforeMainContent;
            } else if (sit != segs.end() && sit->MainContentBackAdjoining) {
                it->From = TDaterDate::FromAfterMainContent;
            } else if (sit != segs.end() && (sit->MainContentFront || sit->HasMainHeaderNews)) {
                it->From = TDaterDate::FromMainContentStart;
            } else if (sit != segs.end() && sit->MainContentBack) {
                it->From = TDaterDate::FromMainContentEnd;
            } else if (sit != segs.end() && sit->InMainContentNews) {
                it->From = TDaterDate::FromMainContent;
            } else {
                if (sit != segs.end()) {
                    it->From = SegmentTypeToDateFrom(sit->Type);
                } else {
                    it->From = SegmentTypeToDateFrom(segs.back().Type);
                }
            }

            dates.push_back(*it);
        }

        dps.swap(dates);
    }

    static bool LessByVal(const TDaterDate& a, const TDaterDate& b) {
        return a.All < b.All;
    }

    static bool EqByVal(const TDaterDate& a, const TDaterDate& b) {
        return a.All == b.All;
    }

    template <typename TDateSpan>
    void FindBestDateImpl(TDateSpan& best, TVector<TDateSpan> urldates, TVector<TDateSpan> textdates, TDaterDate indexed,
                          TVector<TDateSpan>* top, ui32 maxtop) {
        best = TDateSpan();

        TVector<TDateSpan> urlyearonly;
        TVector<TDateSpan> textyearonly;

        FilterUnreliableDatesImpl(urldates, textdates, urlyearonly, textyearonly, indexed);

        TVector<TDateSpan> dates;

        if (urldates.empty() && textdates.empty()) {
            dates.reserve(urlyearonly.size() + textyearonly.size());

            dates.insert(dates.end(), urlyearonly.begin(), urlyearonly.end());
            dates.insert(dates.end(), textyearonly.begin(), textyearonly.end());
        } else {
            dates.reserve(urldates.size() + textdates.size());

            dates.insert(dates.end(), urldates.begin(), urldates.end());
            dates.insert(dates.end(), textdates.begin(), textdates.end());
        }

        if (dates.empty())
            return;

        StableSort(dates.begin(), dates.end(), MoreTrust);
        best = dates[0];

        if (top) {
            top->clear();
            top->insert(top->end(), dates.begin(), dates.end());
            Sort(top->begin(), top->end(), LessByVal);
            top->resize(Unique(top->begin(), top->end(), EqByVal) - top->begin());
            Sort(top->begin(), top->end(), MoreTrust);

            while (top->size() > maxtop || !top->empty() && (top->back().YearOnly() || !top->back().Year))
                top->pop_back();
        }
    }

    void FindBestDate(TDatePosition& best,
                      const TDatePositions& urldates, const TDatePositions& textdates, const TDaterDate& indexed,
                      TDatePositions* top, ui32 maxtop) {
        FindBestDateImpl(best, urldates, textdates, indexed, top, maxtop);
    }

    void FindBestDate(TDateCoord& best,
                      const TDateCoords& urldates, const TDateCoords& textdates, const TDaterDate& indexed,
                      TDateCoords* top, ui32 maxtop) {
        FindBestDateImpl(best, urldates, textdates, indexed, top, maxtop);
    }

    void SortDatesBySources(TDatePositionsBySource& dpss, const TDatePositions& dps) {
        dpss.resize(TDaterDate::FromsCount);
        for (ui32 i = TDaterDate::FromUnknown + 1; i < TDaterDate::FromsCount; ++i)
            for (TDatePositions::const_iterator it = dps.begin(); it != dps.end(); ++it)
                if (it->From == i)
                    dpss[i].push_back(*it);
    }

    void FindBestDateFromSorted(TDatePositions& bdfss, const TDatePositionsBySource& dpss, const TDaterDate& indexed) {
        bdfss.resize(TDaterDate::FromsCount);
        for (ui32 i = TDaterDate::FromUnknown + 1; i < TDaterDate::FromsCount; ++i) {
            FindBestDate(bdfss[i], TDatePositions(), dpss[i], indexed);
        }
    }

}
