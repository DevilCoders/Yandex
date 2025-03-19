#include "dtcreator.h"
#include "dthandler.h"

#include <kernel/indexer/direct_text/dt.h>

#include <util/datetime/base.h>
#include <util/system/maxlen.h>

#ifndef BEST_RELEV_SEQUENCE_LIMIT
#  define BEST_RELEV_SEQUENCE_LIMIT 16
#endif
#ifndef HIGH_RELEV_SEQUENCE_LIMIT
#  define HIGH_RELEV_SEQUENCE_LIMIT 25
#endif

namespace NIndexerCore {

TDirectTextHandler::TDirectTextHandler(TDirectTextCreator& creator)
    : Creator(creator)
    , CurRelevLevel(MID_RELEV) // WEIGHT_NORMAL
    , RelevCnt(0)
{
}

void TDirectTextHandler::OnTokenStart(const TWideToken& wtok, const TNumerStat& stat) {
    if (stat.IsOverflow())
        return;

    if (stat.CurWeight != (TEXT_WEIGHT)CurRelevLevel) {
        RelevCnt = 0;
        CurRelevLevel = (RelevLevel)stat.CurWeight;
    }

    RelevLevel normalRelevLevel = CurRelevLevel;

    ++RelevCnt;

    //-- spam fight: cut relevance of long titles and headings
    TPosting pos = stat.TokenPos.DocLength();
    if (CurRelevLevel == BEST_RELEV && RelevCnt >= BEST_RELEV_SEQUENCE_LIMIT)
        normalRelevLevel = MID_RELEV;
    else if (CurRelevLevel == HIGH_RELEV && RelevCnt >= HIGH_RELEV_SEQUENCE_LIMIT)
        normalRelevLevel = MID_RELEV;

    SetPostingRelevLevel(pos, normalRelevLevel);

    Y_ASSERT(*wtok.Token);
    Creator.StoreForm(wtok, stat.InputPosition, pos);
}

void TDirectTextHandler::OnSpaces(TBreakType type, const wchar16* token, unsigned len, const TNumerStat& stat) {
    if (stat.IsOverflow())
        return;
    Creator.StoreSpaces(token, len, type);
}

static int type2type[] = {
    DTAttrSearchLiteral|DTAttrText,  // ATTR_LITERAL
    DTAttrText,                      // ATTR_URL
    DTAttrSearchDate|DTAttrText,     // ATTR_DATE
    DTAttrSearchInteger|DTAttrText,  // ATTR_INTEGER
    DTAttrSearchLiteral|DTAttrText,  // ATTR_BOOLEAN
    DTAttrSearchUrl,                 // ATTR_UNICODE_URL
    DTAttrText,                      // ATTR_NOFOLLOW_URL
};

void TDirectTextHandler::OnMoveInput(const THtmlChunk&, const TZoneEntry* zone, const TNumerStat& stat) {
    if (!zone || zone->NoOpeningTag)
        return;
    TPosting pos = stat.TokenPos.DocLength();
    if (!zone->OnlyAttrs) {
        if (stat.IsOverflow()) {
            if (zone->IsOpen) {
                Zones.insert(zone->Name);
                return;
            }
            if (zone->IsClose) {
                THashSet<TString>::iterator i = Zones.find(zone->Name);
                if (i != Zones.end()) {
                    Zones.erase(i);
                    return;
                }
            }
        }
        if (zone->IsOpen)
            Creator.StoreZone(DTZoneSearch | DTZoneText, zone->Name, stat.TokenPos.DocLength(), true);
    }
    if (!stat.IsOverflow())
        for (size_t i = 0; i < zone->Attrs.size(); ++i) {
            const TAttrEntry& attr = zone->Attrs[i];
            ui8 dttype = (ui8)type2type[attr.Type];
            Creator.StoreZoneAttr(dttype, ~attr.Name, attr.DecodedValue.data(), attr.DecodedValue.size(), (attr.Pos == APOS_DOCUMENT) ? 0 : pos, 
                                  zone->IsNofollow(), zone->IsSponsored(), zone->IsUgc());
        }
    if (!zone->OnlyAttrs && zone->IsClose) {
        Creator.StoreZone(DTZoneSearch | DTZoneText, zone->Name, stat.TokenPos.DocLength(), false);
    }
}

} // namespace NIndexerCore
