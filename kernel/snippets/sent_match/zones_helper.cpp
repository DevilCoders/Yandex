#include "zones_helper.h"
#include "sent_match.h"
#include "tsnip.h"

#include <kernel/snippets/archive/zone_checker/zone_checker.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/iface/archive/sent.h>
#include <kernel/snippets/sent_info/sent_info.h>

#include <kernel/tarc/iface/tarcface.h>

namespace NSnippets {

    static inline bool IsSpanInInterval(const TArchiveZoneSpan& span, int w0, int w1, const TSentsInfo& si) {
        // left border
        bool goodLeft = false;
        int s0 = si.WordId2SentId(w0);
        const TArchiveSent& arcSent0 = si.GetArchiveSent(s0);
        if (arcSent0.SentId < span.SentBeg) {
            goodLeft = true;
        } else if (arcSent0.SentId == span.SentBeg) {
            int begSymbol;
            if (si.IsWordIdFirstInSent(w0)) {
                begSymbol = si.SentVal[s0].ArchiveOrigin.data() - arcSent0.Sent.data();
            } else {
                begSymbol = si.WordVal[w0].Origin.data() - arcSent0.Sent.data();
            }
            goodLeft = (begSymbol <= span.OffsetBeg);
        }
        if (!goodLeft) {
            return false;
        }

        // right border
        bool goodRight = false;
        int s1 = si.WordId2SentId(w1);
        const TArchiveSent& arcSent1 = si.GetArchiveSent(s1);
        if (span.SentEnd < arcSent1.SentId) {
            goodRight = true;
        } else if (span.SentEnd == arcSent1.SentId) {
            int endSymbol;
            if (si.IsWordIdLastInSent(w1)) {
                endSymbol = si.SentVal[s1].ArchiveOrigin.data() + si.SentVal[s1].ArchiveOrigin.size() - arcSent1.Sent.data();
            } else {
                endSymbol = si.WordVal[w1].Origin.data() + si.WordVal[w1].Origin.size() - arcSent1.Sent.data();
            }
            goodRight = span.OffsetEnd <= endSymbol;
        }
        return goodRight;
    }

    static void ExpandZoneEnd(const TForwardInZoneChecker& archiveZoneChecker, const TSentsInfo& si, int& zEnd, bool fuzzyWordCheck) {
        for (int newEnd = zEnd; newEnd < (int)si.WordVal.size(); ++newEnd) {
            const int wordId = newEnd;
            const int sentId = si.WordId2SentId(wordId);
            TWtringBuf origin = si.WordVal[wordId].Origin;
            const TArchiveSent& arcSent = si.GetArchiveSent(sentId);

            if (!origin || arcSent.SourceArc != ARC_TEXT) {
                continue;
            }
            if (archiveZoneChecker.IsWordInCurrentZone(arcSent.SentId,
                origin.data() - arcSent.Sent.data(),
                origin.data() + origin.size() - arcSent.Sent.data(),
                fuzzyWordCheck))
            {
                zEnd = newEnd; // expand the ending of the fragment
            } else {
                break;
            }
        }
    }

    void GetZones(const TSingleSnip& snip, EArchiveZone zName, TVector<TZoneWords>& borders, bool strictlyInnerZones, bool fuzzyWordCheck) {
        borders.clear();

        const TSentsInfo& si = snip.GetSentsMatchInfo()->SentsInfo;

        TForwardInZoneChecker archiveZoneChecker(si.GetTextArcMarkup().GetZone(zName).Spans);

        int w0 = snip.GetFirstWord();
        int w1 = snip.GetLastWord();
        while (w0 <= w1 && (si.GetArchiveSent(si.WordId2SentId(w0)).SourceArc != ARC_TEXT || !si.WordVal[w0].Origin)) {
            ++w0;
        }
        while (w0 <= w1 && (si.GetArchiveSent(si.WordId2SentId(w1)).SourceArc != ARC_TEXT || !si.WordVal[w1].Origin)) {
            --w1;
        }
        if (w0 > w1) {
            return;
        }

        for (int wordId = w0; wordId <= w1 && !archiveZoneChecker.Empty(); ++wordId) {
            const int sentId = si.WordId2SentId(wordId);
            TWtringBuf origin = si.WordVal[wordId].Origin;
            const TArchiveSent& arcSent = si.GetArchiveSent(sentId);

            if (!origin || arcSent.SourceArc != ARC_TEXT) {
                continue;
            }

            archiveZoneChecker.SeekToWord(arcSent.SentId, origin.data() + origin.size() - arcSent.Sent.data());
            if (!archiveZoneChecker.IsWordInCurrentZone(arcSent.SentId,
                origin.data() - arcSent.Sent.data(),
                origin.data() + origin.size() - arcSent.Sent.data(),
                fuzzyWordCheck))
            {
                continue;
            }
            const TArchiveZoneSpan* zSpan = archiveZoneChecker.GetCurrentSpan(); // it's not NULL
            if (strictlyInnerZones && !IsSpanInInterval(*zSpan, w0, w1, si)) { // select spans lying entirely in the fragment
                continue;
            }
            int zBeg = wordId;
            int zEnd = wordId;
            ExpandZoneEnd(archiveZoneChecker, si, zEnd, fuzzyWordCheck);
            zEnd = Min(zEnd, w1);
            borders.push_back(TZoneWords(*zSpan, zBeg, zEnd));
            wordId = zEnd; // worId has changed!
        }
    }
}
