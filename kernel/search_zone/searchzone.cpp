#include "searchzone.h"
#include <kernel/search_zone/protos/searchzone.pb.h>       // for NRTYServer::TZone
#include <util/charset/wide.h>                      // for UTF8ToWide

void MakeSearchZones(const NRTYServer::TZone& externalSearchZones, TVector<TUtf16String>* storage, NIndexerCore::TExtraTextZones* extraTextZones) {
    typedef ::google::protobuf::RepeatedPtrField<NRTYServer::TZone> TAdditionalZones;
    const TAdditionalZones& addZones = externalSearchZones.GetChildren();

    for (int i = 0; i < addZones.size(); ++i) {
        const NRTYServer::TZone& zone = addZones.Get(i);
        storage->push_back(UTF8ToWide(zone.GetText()));
        const TUtf16String& lastZoneText = storage->back();

        NIndexerCore::TExtraText extraText;
        extraText.Zone = zone.GetName().data();
        extraText.Text = lastZoneText.data();
        extraText.TextLen = lastZoneText.length();
        extraTextZones->push_back(extraText);
    }
}
