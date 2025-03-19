#pragma once

#include "offroad_io.h"
#include "panther_hit_adaptors.h"

namespace NDoom {

using TOffroadPantherIo = TOffroadIo<OffroadPantherIndexFormat, TPantherHit, NOffroad::TOffsetKeyData, TPantherHitVectorizer, TPantherHitSubtractor, DefaultPantherKeyIoModel, DefaultPantherHitIoModel>;

} // namespace NDoom
