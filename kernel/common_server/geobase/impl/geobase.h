#pragma once

#include <kernel/common_server/geobase/geobase.h>
#include <kernel/common_server/geobase/config/config.h>

#include <geobase/include/lookup.hpp>

class TFakeGeobaseProvider: public IGeobaseProvider {
public:
    explicit TFakeGeobaseProvider(const NCS::TGeobaseConfig& geobaseConfig);

    TTimeZoneHelper GetTimeZone(const TMaybe<i32> id) const override;

    i32 GetRegionIdByIp(const TString& ip) const override;
    NGeobase::NImpl::TIpTraits GetTraitsByIp(const TString& ip) const override;
    i32 GetParentIdWithType(const i32 regionId, i32 searchType) const override;
    TMaybe<NGeobase::NImpl::TRegion> GetRegionById(const i32 regionId) const override;

private:
    const NCS::TGeobaseConfig& GeobaseConfig;
};

class TGeobaseProvider: public TFakeGeobaseProvider {
public:
    explicit TGeobaseProvider(THolder<NGeobase::NImpl::TLookup> geobase, const NCS::TGeobaseConfig& geobaseConfig);

    TTimeZoneHelper GetTimeZone(const TMaybe<i32> id) const override;

    i32 GetRegionIdByIp(const TString& ip) const override;
    NGeobase::NImpl::TIpTraits GetTraitsByIp(const TString& ip) const override;
    i32 GetParentIdWithType(const i32 regionId, i32 searchType) const override;
    TMaybe<NGeobase::NImpl::TRegion> GetRegionById(const i32 regionId) const override;

private:
    const THolder<NGeobase::NImpl::TLookup> Geobase;
};
