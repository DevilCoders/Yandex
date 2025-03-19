#pragma once

#include "default_hit_range.h"
#include "generic_hit_layers_range_accessor.h"

namespace NDoom {


template <class Position>
using TDefaultHitRangeAccessor = TGenericHitLayersRangeAccessor<TDefaultHitRange<Position>>;


} // namespace NDoom
