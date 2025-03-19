#include "zone_checker.h"
#include <kernel/snippets/iface/archive/sent.h>

#include <util/generic/algorithm.h>

namespace NSnippets
{
    template<class A, class B>
    static bool LessPair(const A& a1, const B& b1, const A& a2, const B& b2) {
        if (a1 != a2)
            return a1 < a2;
        return b1 < b2;
    }

    static bool LessZoneBeg(const TArchiveZoneSpan& a, const TArchiveZoneSpan& b) {
        return LessPair(a.SentBeg, a.OffsetBeg, b.SentBeg, b.OffsetBeg);
    }
    static bool GreaterZoneBeg(const TArchiveZoneSpan& a, const TArchiveZoneSpan& b) {
        return LessPair(b.SentBeg, b.OffsetBeg, a.SentBeg, a.OffsetBeg);
    }
    static bool LessZoneEnd(const TArchiveZoneSpan& a, const TArchiveZoneSpan& b) {
        return LessPair(a.SentEnd, a.OffsetEnd, b.SentEnd, b.OffsetEnd);
    }

    static void MergeZone(TArchiveZoneSpan& a, const TArchiveZoneSpan& b) {
        if (GreaterZoneBeg(a, b)) {
            a.SentBeg = b.SentBeg;
            a.OffsetBeg = b.OffsetBeg;
        }
        if (LessZoneEnd(a, b)) {
            a.SentEnd = b.SentEnd;
            a.OffsetEnd = b.OffsetEnd;
        }
    }

    static bool ZoneInters(const TArchiveZoneSpan& a, const TArchiveZoneSpan& b) {
        if (LessPair(a.SentEnd, a.OffsetEnd, b.SentBeg, b.OffsetBeg))
            return false;
        if (LessPair(b.SentEnd, b.OffsetEnd, a.SentBeg, a.OffsetBeg))
            return false;
        return true;
    }

    void SortZones(TVector<TArchiveZoneSpan>& zones) {
        Sort(zones.begin(), zones.end(), LessZoneBeg);
    }

    static void RevSortAndCatZones(TVector<TArchiveZoneSpan>& zones) {
        if (!zones.size())
            return;
        Sort(zones.begin(),  zones.end(), GreaterZoneBeg);
        size_t n = 1;
        for (size_t i = 1; i < zones.size(); ++i) {
            zones[n++] = zones[i];
            while (n >= 2 && ZoneInters(zones[n - 2], zones[n - 1])) {
                --n;
                MergeZone(zones[n - 1], zones[n]);
            }
        }
        zones.resize(n);
    }

    TForwardInZoneChecker::TForwardInZoneChecker()
        : ZoneSpans()
        , CurrentZoneId(1)
    {
    }

    TForwardInZoneChecker::TForwardInZoneChecker(const TVector<TArchiveZoneSpan>& zoneSpans, bool needCutAndSort)
        : ZoneSpans(zoneSpans)
        , CurrentZoneId(1)
    {
        if (needCutAndSort) {
            RevSortAndCatZones(ZoneSpans);
        } else {
            Sort(ZoneSpans.begin(), ZoneSpans.end(), GreaterZoneBeg);
        }
    }

    bool TForwardInZoneChecker::SeekToSent(int archiveSentenceNumber)
    {
        while (!ZoneSpans.empty() &&
            ZoneSpans.back().SentEnd < archiveSentenceNumber)
        {
            ZoneSpans.pop_back();
            ++CurrentZoneId;
        }
        return IsSentInCurrentZone(archiveSentenceNumber);
    }

    bool TForwardInZoneChecker::IsSentInCurrentZone(int archiveSentenceNumber) const
    {
        if (!ZoneSpans.empty()) {
            return archiveSentenceNumber >= ZoneSpans.back().SentBeg && archiveSentenceNumber <= ZoneSpans.back().SentEnd;
        }

        return false;
    }

    void TForwardInZoneChecker::SeekToWord(int archiveSentenceNumber, int wordOffsetEnd)
    {
        SeekToSent(archiveSentenceNumber);
        while (!ZoneSpans.empty() &&
            ZoneSpans.back().SentEnd == archiveSentenceNumber &&
            ZoneSpans.back().OffsetEnd < wordOffsetEnd)
        {
            ZoneSpans.pop_back();
            ++CurrentZoneId;
        }
    }

    bool TForwardInZoneChecker::IsWordInCurrentZone(int archiveSentenceNumber, int wordOffsetBegin, int wordOffsetEnd, bool fuzzy) const
    {
        if (!ZoneSpans.empty()) {
            const bool greaterThenBeg =
                archiveSentenceNumber > ZoneSpans.back().SentBeg ||
                (archiveSentenceNumber == ZoneSpans.back().SentBeg &&
                (fuzzy ? wordOffsetEnd >= ZoneSpans.back().OffsetBeg : wordOffsetBegin >= ZoneSpans.back().OffsetBeg));
            const bool lowerThenEnd =
                archiveSentenceNumber < ZoneSpans.back().SentEnd ||
                (archiveSentenceNumber == ZoneSpans.back().SentEnd &&
                (fuzzy ? wordOffsetBegin <= ZoneSpans.back().OffsetEnd : wordOffsetEnd <= ZoneSpans.back().OffsetEnd));
            if (greaterThenBeg && lowerThenEnd) {
                return true;
            }
        }

        return false;
    }

    const TArchiveZoneSpan* TForwardInZoneChecker::GetCurrentSpan() const
    {
        if (!Empty()) {
            return &ZoneSpans.back();
        }

        return nullptr;
    }

    void TForwardInZoneChecker::SkipCurrentSpan()
    {
        if (!Empty()) {
            ZoneSpans.pop_back();
            ++CurrentZoneId;
        }
    }

    int TForwardInZoneChecker::GetCurrentZoneId() const
    {
        return CurrentZoneId;
    }

    bool TForwardInZoneChecker::Empty() const
    {
        return ZoneSpans.empty();
    }

    class TZoneInSent {
    public:
        size_t OffsetBeg = 0;
        size_t OffsetEnd = 0;

        size_t GetLength() const {
            return OffsetEnd - OffsetBeg;
        }
    };

    static TZoneInSent IntersectZoneAndSent(const TArchiveZoneSpan& span, const TArchiveSent& sent, bool punctHack) {
        TZoneInSent res;
        if (sent.SentId < span.SentBeg || sent.SentId > span.SentEnd) {
            return res;
        }
        if (span.SentBeg == span.SentEnd && span.OffsetEnd <= span.OffsetBeg) {
            return res;
        }
        const size_t offsetMax = sent.Sent.size();
        res.OffsetBeg = sent.SentId == span.SentBeg ? Min<size_t>(span.OffsetBeg, offsetMax) : 0;
        res.OffsetEnd = sent.SentId == span.SentEnd ? Min<size_t>(span.OffsetEnd, offsetMax) : offsetMax;
        if (punctHack && res.OffsetEnd < offsetMax) {
            ++res.OffsetEnd;
        }
        return res;
    }

    TWtringBuf IntersectZoneAndSentText(const TArchiveZoneSpan& span, const TArchiveSent& sent, bool punctHack) {
        const TZoneInSent res = IntersectZoneAndSent(span, sent, punctHack);
        return TWtringBuf(sent.Sent.data() + res.OffsetBeg, sent.Sent.data() + res.OffsetEnd);
    }

    TSentParts IntersectZonesAndSent(TForwardInZoneChecker& zoneIter, const TArchiveSent& sent, bool punctHack) {
        TSentParts res;
        TWtringBuf rest = sent.Sent;
        for (zoneIter.SeekToSent(sent.SentId); zoneIter.IsSentInCurrentZone(sent.SentId); zoneIter.SkipCurrentSpan()) {
            TWtringBuf part = IntersectZoneAndSentText(*zoneIter.GetCurrentSpan(), sent, punctHack);
            if (!part.size() || part.data() + part.size() <= rest.data()) {
                continue;
            }
            if (part.data() < rest.data()) { //a possibility if punctHack is true and zoneIter was fed with touching spans
                part = TWtringBuf(rest.data(), part.data() + part.size());
            }
            Y_ASSERT(rest.size() && part.data() >= rest.data() && part.data() + part.size() <= rest.data() + rest.size());
            if (part.data() > rest.data()) {
                res.push_back(TSentPart(rest.NextTokAt(part.data() - rest.data()), false));
            }
            res.push_back(TSentPart(rest.NextTokAt(part.size()), true));

            if (zoneIter.IsSentInCurrentZone(sent.SentId + 1)) {
                Y_ASSERT(!rest.size()); //so we're done with this sent anyway, don't skip span needed for next sents
                break;
            }
        }
        if (rest.size()) {
            res.push_back(TSentPart(rest.NextTokAt(rest.size()), false));
        }
        return res;
    }
}
