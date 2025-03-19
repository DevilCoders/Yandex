#pragma once

#include <geobase/include/lookup.hpp>

#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/generic/string.h>

namespace NGeoDB::NCanonical {

class TGeoData {
public:
    TGeoData();
    static const NGeobase::NImpl::TLookup* Geobase() {
        return Instance().GetGeobase();
    }
private:
    const NGeobase::NImpl::TLookup* GetGeobase() const {
        return GeobaseLookup.Get();
    }
    static const TGeoData& Instance() {
        return *Singleton<TGeoData>();
    }

    THolder<NGeobase::NImpl::TLookup> GeobaseLookup;
    TString RawData;
};
}
