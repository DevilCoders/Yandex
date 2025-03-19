#include "extratext.h"
#include "directtokenizer.h"
#include <kernel/indexer/dtcreator/dtcreator.h>

namespace NIndexerCore {

    TPosting GetLastPos(const TDirectTextEntries& e) {
        if (e.GetEntryCount()) {
            for (const TDirectTextEntry2* p = e.GetEntries() + e.GetEntryCount() - 1; p >= e.GetEntries(); --p) {
                const size_t tokenCount = p->LemmatizedTokenCount;
                if (tokenCount) {
                    const ui8 lastTokenOffset = p->LemmatizedToken[tokenCount - 1].FormOffset;
                    return PostingInc(p->Posting, lastTokenOffset);
                }
            }
        }
        return TWordPosition(0, 1, 1).DocLength();
    }

    void AppendExtraText(TDirectTextCreator& creator, const TExtraTextZones& zones) {
        TDirectTokenizer directTokenizer(creator, false, GetLastPos(creator.GetDirectTextEntries()));
        for (size_t i = 0; i < zones.size(); ++i) {
            const TExtraText& t = zones[i];
            if (t.Zone) {
                creator.StoreZone(DTZoneSearch, t.Zone, directTokenizer.GetPosition().DocLength(), true);
                directTokenizer.StoreText(t.Text, t.TextLen, t.Relev);
                creator.StoreZone(DTZoneSearch, t.Zone, directTokenizer.GetPosition().DocLength(), false);
            } else {
                directTokenizer.StoreText(t.Text, t.TextLen, t.Relev);
            }
        }
    }

} // namespace NIndexerCore
