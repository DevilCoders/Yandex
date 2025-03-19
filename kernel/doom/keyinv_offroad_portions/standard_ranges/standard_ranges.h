#pragma once

#include "range_types.h"

#include <kernel/doom/keyinv_offroad_portions/keyinv_portions_key_ranges.h>

#include <library/cpp/resource/resource.h>

#include <util/string/cast.h>
#include <util/stream/mem.h>

namespace NDoom {

class TStandardKeyInvPortionsKeyRanges {
public:
    static TKeyInvPortionsKeyRanges Ranges(EStandardKeyInvPortionsKeyRangeTypes rangeType) {
        TString serializedRanges = NResource::Find("standard_keyinv_ranges_" + ToString<EStandardKeyInvPortionsKeyRangeTypes>(rangeType));
        TKeyInvPortionsKeyRanges ranges;
        TMemoryInput input(serializedRanges.data(), serializedRanges.size());
        ranges.Load(&input);
        return ranges;
    }
};

} // namespace NDoom
