#pragma once

#include <util/ysaveload.h>

namespace NOffroad::NMinHash {

struct THeader {
    ui32 NumSlots = 0;
    ui32 NumBuckets = 0;
    ui64 Seed = 0;

    Y_SAVELOAD_DEFINE(NumSlots, NumBuckets, Seed);
};

} // namespace NOffroad::NMinHash
