#pragma once

#include <kernel/tarc/iface/tarcface.h>

#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

namespace NSnippets {
    void SortZones(TVector<TArchiveZoneSpan>& zones);

    //! Class that checking sentence or word on their belonging to zone
    class TForwardInZoneChecker
    {
    private:
        TVector<TArchiveZoneSpan> ZoneSpans;
        int CurrentZoneId;

    public:
        //! default ctor
        TForwardInZoneChecker();

        //! ctor
        //! @param zoneSpans - target zone spans
        TForwardInZoneChecker(const TVector<TArchiveZoneSpan>& zoneSpans, bool needCutAndSort = false);

        //! Seek to target sentence
        bool SeekToSent(int archiveSentenceNumber);

        //! Seek to target word
        //! @param wordOffsetEnd - word end offset in symbols from sentence beginning
        void SeekToWord(int archiveSentenceNumber, int wordOffsetEnd);

        //! Checking sentence
        bool IsSentInCurrentZone(int archiveSentenceNumber) const;

        //! Checking word
        //! @param wordOffsetBegin - word start offset in symbols from sentence beginning
        //! @param WordOffsetEnd - word end offset in symbols from sentence beginning
        //! @param fuzzy - check using word/zone intersection instead of strict bounds checking
        bool IsWordInCurrentZone(int archiveSentenceNumber, int wordOffsetBegin, int WordOffsetEnd, bool fuzzy = false) const;

        //! Checking that class has already checked all possible spans
        bool Empty() const;

        //! Get current span
        //! @return pointer to the last active span if class isn't empty, NULL otherwise
        const TArchiveZoneSpan* GetCurrentSpan() const;

        void SkipCurrentSpan();

        //! Get current zone id
        //! @return span zone id, unique for each zone span, added in class ctor
        int GetCurrentZoneId() const;
    };

    class TArchiveSent;

    /*
    punctHack: take +1 symbol after span end. Zone markup works per-word, so for tasks like cutting zone or checking that zone covers whole sent
    it's more accurate to extend it to include trailing punctuation. It's not always 1 symbol and also may be before first word too, but still
    +1 is more accurate than +0 until a better markup or another solution is implemented.
    */
    TWtringBuf IntersectZoneAndSentText(const TArchiveZoneSpan& span, const TArchiveSent& sent, bool punctHack);

    class TSentPart {
    public:
        TWtringBuf Part;
        bool InZone = false;

        TSentPart() {
        }

        TSentPart(const TWtringBuf& part, bool inZone)
            : Part(part)
            , InZone(inZone)
        {
        }
    };
    using TSentParts = TVector<TSentPart>;

    TSentParts IntersectZonesAndSent(TForwardInZoneChecker& zoneIter, const TArchiveSent& sent, bool punctHack);
}
