#pragma once

#include "zoneconf.h"
#include "event.h"

#include <library/cpp/containers/str_hash/str_hash.h>

#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>

class TZoneCounter: private TNonCopyable {
public:
    TZoneCounter(IZoneAttrConf* zac, IParsedDocProperties* docProp, bool markNoindex = false);
    ~TZoneCounter();

    const HashSet& GetOpenZones() const;

    bool CheckEvent(const THtmlChunk& e, TZoneEntry* zone);

private:
    void Start(const THtmlChunk& e, TZoneEntry& zone);
    void End(const THtmlChunk& e, TZoneEntry& zone);

private:
    const bool MarkNoindex;
    TVector<std::pair<const char*, unsigned>> Zones;
    // consider zone names are persistent
    IZoneAttrConf* ZoneAttrConf;
    IParsedDocProperties* DocProperties;
    HashSet OpenZones;
    bool InNoindexZone;
};
