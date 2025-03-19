#include "geodata.h"

#include <library/cpp/resource/resource.h>

constexpr TStringBuf GEOBASE_RESOURCE_NAME = "canonical_geodb";

namespace NGeoDB::NCanonical {
    TGeoData::TGeoData() {
        RawData = NResource::Find(GEOBASE_RESOURCE_NAME);
        GeobaseLookup.Reset(new NGeobase::NImpl::TLookup(RawData.data(), RawData.size()));
    }
}
