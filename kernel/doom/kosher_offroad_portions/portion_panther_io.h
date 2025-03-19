#pragma once

#include <kernel/doom/offroad/panther_hit_adaptors.h>

#include "kosher_portion_io.h"

namespace NDoom {
    using TPortionPantherOffroadIo = TKosherPortionIo<
        TPantherHit, TPantherHitVectorizer, TPantherHitSubtractor,
        PortionPantherKeyIoModelV1, PortionPantherHitIoModelV1>;

} // namespace NDoom
