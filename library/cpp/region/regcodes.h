#pragma once

#include <util/system/defaults.h>
#include <util/generic/strbuf.h>

namespace NRegion {
    //eats any region name (ru, RuS) and returns canonized (e.g. RUS)
    const char* CanonizeRegion(const char* region);
    const char* CanonizeRegion(ui16 numRegionCode);

    ui16 RegionCodeByName(const TStringBuf& name);

    //returns name for unspecified or incorrect region
    const char* UnknownRegion();

}
