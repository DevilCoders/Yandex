#pragma once

#include <kernel/doom/offroad/attributes_hit_adaptors.h>

#include "kosher_portion_io.h"

namespace NDoom {
    using TPortionAttributesOffroadIo = TKosherPortionIo<
        TAttributesHit, TAttributesHitVectorizer, NOffroad::TD1Subtractor,
        PortionAttributesKeyIoModelV1, PortionAttributesHitIoModelV1>;

} // namespace NDoom
