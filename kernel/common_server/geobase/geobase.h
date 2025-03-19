#pragma once

#include <kernel/common_server/util/datetime/datetime.h>

#include <util/generic/maybe.h>

#include <geobase/include/lookup.hpp>

class IGeobaseProvider {
public:
    virtual ~IGeobaseProvider() = default;

    virtual TTimeZoneHelper GetTimeZone(const TMaybe<i32> id) const = 0;

    virtual i32 GetRegionIdByIp(const TString& ip) const = 0;
    virtual NGeobase::NImpl::TIpTraits GetTraitsByIp(const TString& ip) const = 0;
    virtual i32 GetParentIdWithType(const i32 regionId, i32 searchType) const = 0;
    virtual TMaybe<NGeobase::NImpl::TRegion> GetRegionById(const i32 regionId) const = 0;

    TLocalInstant GetLocalInstant(const TInstant instant, const TMaybe<i32> id) const;
};
