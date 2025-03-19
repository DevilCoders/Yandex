#include "clampticandmatches.h"

#include <kernel/xref/xmap.h>     // for OwnersMatchedCategoryBits

#include <util/generic/utility.h> // for Min

static const ui16 MaxOwnersMatchedCategoryCount = (1 << OwnersMatchedCategoryBits) - 1;

ui16 ClampCatMatches(ui16 catMatches) {
    return Min<ui16>(catMatches, MaxOwnersMatchedCategoryCount);
}

ui32 ClampTic(ui32 tic) {
    return Min<ui32>( tic, (1U << 20) - 1);
}
