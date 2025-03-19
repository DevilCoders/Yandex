#include "geobase.h"

#include <library/cpp/logger/global/global.h>

TFakeGeobaseProvider::TFakeGeobaseProvider(const NCS::TGeobaseConfig& geobaseConfig)
    : GeobaseConfig(geobaseConfig)
{
}

TTimeZoneHelper TFakeGeobaseProvider::GetTimeZone(const TMaybe<i32> /*id*/) const {
    return GeobaseConfig.GetDefaultTimeZone();
}

i32 TFakeGeobaseProvider::GetRegionIdByIp(const TString& /*ip*/) const {
    return GeobaseConfig.GetDefaultRegionId();
}

NGeobase::NImpl::TIpTraits TFakeGeobaseProvider::GetTraitsByIp(const TString& /*ip*/) const {
    return {};
}

i32 TFakeGeobaseProvider::GetParentIdWithType(const i32 /*regionId*/, i32 /*searchType*/) const {
    return GeobaseConfig.GetDefaultRegionId();
}

TMaybe<NGeobase::NImpl::TRegion> TFakeGeobaseProvider::GetRegionById(const i32 /*regionId*/) const {
    return Nothing();
}

TGeobaseProvider::TGeobaseProvider(THolder<NGeobase::NImpl::TLookup> geobase, const NCS::TGeobaseConfig& geobaseConfig)
    : TFakeGeobaseProvider(geobaseConfig)
    , Geobase(std::move(geobase))
{
    CHECK_WITH_LOG(Geobase);
}

TTimeZoneHelper TGeobaseProvider::GetTimeZone(const TMaybe<i32> id) const {
    if (id) {
        const auto tz = Geobase->GetTimezoneById(*id);
        return TTimeZoneHelper::FromSeconds(tz.Offset);
    }
    return TFakeGeobaseProvider::GetTimeZone(id);
}

i32 TGeobaseProvider::GetRegionIdByIp(const TString& ip) const {
    return Geobase->GetRegionIdByIp(ip.Data());
}

NGeobase::NImpl::TIpTraits TGeobaseProvider::GetTraitsByIp(const TString& ip) const {
    return Geobase->GetTraitsByIp(ip.Data());
}

i32 TGeobaseProvider::GetParentIdWithType(const i32 regionId, i32 searchType) const {
    return Geobase->GetParentIdWithType(regionId, searchType);
}

TMaybe<NGeobase::NImpl::TRegion> TGeobaseProvider::GetRegionById(const i32 regionId) const {
    return Geobase->GetRegionById(regionId);
}
