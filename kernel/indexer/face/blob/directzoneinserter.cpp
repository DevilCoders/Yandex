#include "directzoneinserter.h"

#include "markup.h"

namespace NIndexerCore {

TDirectZoneInserter::TDirectZoneInserter()
    : Pool_(64 * 1024)
{ }

void TDirectZoneInserter::StoreZone(const char* zoneName, TPosting begin, TPosting end, bool archiveOnly) {
    Zones_.StoreZone(archiveOnly ? NIndexerCore::DTZoneText : NIndexerCore::DTZoneSearch | NIndexerCore::DTZoneText, zoneName, begin, end);
}

void TDirectZoneInserter::StoreArchiveZoneAttr(const char* name, const wchar16* value, size_t length, TPosting pos) {
    wchar16* p = Pool_.append(value, length + 1);
    p[length] = 0;
    Zones_.StoreArchiveZoneAttr(NIndexerCore::DTAttrText, name, p, pos, false, false, false);
}

void  TDirectZoneInserter::PrepareZones() {
    Zones_.Prepare();
}

TBuffer TDirectZoneInserter::SerializeZones() const {
    return NIndexerCore::SerializeMarkup(Zones_.GetAttrs(), Zones_.GetZones());
}

} // namespace NIndexerCore
