#pragma once

#include "offroad_io.h"
#include "counts_hit_adaptors.h"

namespace NDoom {

using TOffroadCountsIo = TOffroadIo<OffroadCountsIndexFormat, TCountsHit, NOffroad::THitCountKeyData, TCountsHitVectorizer, TCountsHitSubtractor, DefaultCountsKeyIoModel, DefaultCountsHitIoModel>;

} // namespace NDoom
