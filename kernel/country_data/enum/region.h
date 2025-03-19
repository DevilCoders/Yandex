#pragma once

#include <kernel/country_data/enum/protos/country.pb.h>

#include <kernel/search_types/search_types.h>

namespace NCountry {
    TCateg ToRegionId(const ECountry country);
    ECountry FromRegionId(const TCateg regionId);
    bool TryFromRegionId(const TCateg regionId, ECountry& country);
} // namespace NCountry

