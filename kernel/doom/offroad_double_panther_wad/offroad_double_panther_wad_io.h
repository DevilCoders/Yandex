#pragma once

#include <kernel/doom/offroad/panther_hit_adaptors.h>
#include <kernel/doom/offroad_inv_wad/offroad_inv_wad_io.h>
#include <kernel/doom/offroad_key_wad/offroad_key_wad_io.h>
#include <kernel/doom/offroad_keyinv_wad/offroad_keyinv_wad_io.h>

#include "double_panther_hit_range_adaptors.h"

namespace NDoom {

using TOffroadDoublePantherKeyIo = TOffroadKeyWadIo<
    PantherIndexType,
    TDoublePantherHitRange,
    TDoublePantherHitRangeVectorizer,
    TDoublePantherHitRangeSubtractor,
    TDoublePantherHitRangeSerializer,
    TDoublePantherHitRangeCombiner,
    DefaultDoublePantherKeyIoModel
>;

using TOffroadDoublePantherHitIo = TOffroadInvWadIo<
    PantherIndexType,
    TPantherHit,
    TPantherHitVectorizer,
    TPantherHitSubtractor,
    NOffroad::TNullVectorizer,
    DefaultDoublePantherHitIoModel
>;

using TOffroadDoublePantherIo = TOffroadKeyInvWadIo<
    TOffroadDoublePantherKeyIo,
    TOffroadDoublePantherHitIo,
    TDoublePantherHitRangeAccessor
>;

} // namespace NDoom
