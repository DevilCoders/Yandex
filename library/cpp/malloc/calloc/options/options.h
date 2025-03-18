#pragma once

#include <library/cpp/malloc/calloc/alloc_header.h>
#include <util/system/defaults.h>

namespace NCalloc {
    extern const EAllocType DefaultAlloc;
    extern const EAllocType DefaultSlaveAlloc;
    extern const bool EnabledByDefault;
}
