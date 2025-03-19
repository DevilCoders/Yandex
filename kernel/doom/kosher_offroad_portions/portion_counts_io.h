#pragma once

#include <kernel/doom/offroad/counts_hit_adaptors.h>

#include "kosher_portion_io.h"

namespace NDoom {
    using TPortionCountsOffroadIo = TKosherPortionIo<
        TCountsHit, TCountsHitVectorizer, TCountsHitSubtractor,
        PortionCountsKeyIoModelV1, PortionCountsHitIoModelV1>;

} // namespace NDoom
