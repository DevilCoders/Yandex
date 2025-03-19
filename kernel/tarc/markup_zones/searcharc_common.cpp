#include "searcharc_common.h"

#include "arcreader.h"
#include "text_markup.h"

#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/tarcface.h>

#include <util/memory/blob.h>

int FindDocCommon(TBlob b, TBlob doctext, TVector<int>& breaks, TDocArchive& da) {
    try {
        da.Blobsize = (int)b.Size();
        memcpy(da.GetBlob(), b.Data(), b.Size());
        da.Exists = true;

        if (doctext.Empty()) {
            da.NoText = 1;
            return 0;
        }
        TArchiveTextHeader* hdr = (TArchiveTextHeader*)doctext.AsUnsignedCharPtr();
        if (hdr->BlockCount == 0) {
            da.NoText = 1;
            return 0;
        }

        TArchiveMarkupZones mZones;
        GetArchiveMarkupZones(doctext.AsUnsignedCharPtr(), &mZones);

        const TArchiveZoneSpan* tSpan = nullptr;
        TArchiveZone& zTitle = mZones.GetZone(AZ_TITLE);
        if (!zTitle.Spans.empty())
            tSpan = &zTitle.Spans[0];

        TVector<int> extbreaks;
        if (tSpan) {
            extbreaks.reserve(breaks.size() + tSpan->SentEnd);
            for (ui16 i = tSpan->SentBeg; i <= tSpan->SentEnd; i++)
                extbreaks.push_back(i);
            TVector<int>::const_iterator sBeg = breaks.begin();
            TVector<int>::const_iterator sEnd = breaks.end();
            while (sBeg != sEnd && *sBeg <= tSpan->SentEnd)
                ++sBeg;
            if (sBeg != sEnd)
                extbreaks.insert(extbreaks.end(), sBeg, sEnd);
        }

        TVector<TArchiveSent> outSents;
        TArchiveWeightZones wZones;
        GetSentencesByNumbers(doctext.AsUnsignedCharPtr(), tSpan ? extbreaks : breaks, &outSents, &wZones);
        for (size_t i = 0; i < outSents.size(); i++) {
            const TArchiveSent& s = outSents[i];
            if (tSpan && s.Number >= tSpan->SentBeg && s.Number <= tSpan->SentEnd)
                da.TitleSentences.push_back(s.OnlyText);
            else
                da.AddPassage(s.Number, s.OnlyText);
        }
    } catch (...) {
        da.Clear();
        return -1;
    }
    return 0;
}
