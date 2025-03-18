#include "zonecount.h"

#include "noindex.h"

static NHtml5::TNoindexType ParseNoindexType(const THtmlChunk& e) {
    if (e.leng < 7) {
        return NHtml5::TNoindexType();
    }
    return NHtml5::DetectNoindex(TStringBuf(e.text + 4, e.leng - 7));
}

TZoneCounter::TZoneCounter(IZoneAttrConf* zac, IParsedDocProperties* docProp, bool markNoindex)
    : MarkNoindex(markNoindex)
    , ZoneAttrConf(zac)
    , DocProperties(docProp)
    , InNoindexZone(false)
{
}

TZoneCounter::~TZoneCounter() {
}

const HashSet& TZoneCounter::GetOpenZones() const {
    return OpenZones;
}

bool TZoneCounter::CheckEvent(const THtmlChunk& e, TZoneEntry* zone) {
    // maybe create noindex zone
    if (MarkNoindex && (e.Tag && e.Tag->id() == HT_NOINDEX)) {
        if (e.GetLexType() == HTLEX_COMMENT) {
            if (NHtml5::TNoindexType type = ParseNoindexType(e)) {
                if (type.IsClose() && InNoindexZone) {
                    zone->Name = "noindex";
                    zone->IsClose = true;

                    InNoindexZone = false;
                } else if (!InNoindexZone) {
                    zone->Name = "noindex";
                    zone->IsOpen = true;

                    InNoindexZone = true;
                }

                return true;
            }
        }
        return false;
    }

    if (e.flags.type != PARSED_MARKUP || e.flags.markup == MARKUP_IGNORED) {
        return false;
    }

    if (e.GetLexType() == HTLEX_END_TAG) {
        End(e, *zone); // possibly close zone if tree count tells so
    } else {
        if (ZoneAttrConf)
            ZoneAttrConf->CheckEvent(e, DocProperties, zone);
        Start(e, *zone);
    }

    // zone can be opened and closed in one event

    Y_ASSERT(!zone->OnlyAttrs);
    if (zone->IsOpen) {
        Y_ASSERT(zone->Name);
        if (!OpenZones.Has(zone->Name)) {
            OpenZones.Add(zone->Name);
        } else {
            // TODO: remove this flag, use Name != NULL and !Attrs.empty()
            zone->OnlyAttrs = true;
        }
    }

    if (zone->IsClose) {
        Y_ASSERT(zone->Name);
        if (OpenZones.Has(zone->Name)) {
            OpenZones.Detach(zone->Name);
        } else {
            // TODO: remove this flag
            zone->NoOpeningTag = true;
        }
    }

    return true;
}

void TZoneCounter::Start(const THtmlChunk& e, TZoneEntry& zone) {
    if (e.GetLexType() == HTLEX_EMPTY_TAG) {
        if (zone.IsOpen)
            zone.IsClose = true;
    } else {
        Y_ASSERT(e.GetLexType() == HTLEX_START_TAG);
        if (zone.IsOpen) {
            Zones.push_back(std::make_pair(zone.Name, 1));
        } else {
            if (!Zones.empty()) {
                ++Zones.back().second;
            }
        }
    }
}

void TZoneCounter::End(const THtmlChunk& e, TZoneEntry& zone) {
    Y_ASSERT(e.flags.type == PARSED_MARKUP && e.flags.apos == HTLEX_END_TAG);
    if (Zones.empty())
        return;
    if (!--Zones.back().second) {
        // current zone ends here
        zone.Name = Zones.back().first;
        zone.IsClose = true;

        Zones.pop_back();
    }
}
