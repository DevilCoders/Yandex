#include "options.h"
#include <util/system/compiler.h>

namespace NCalloc {

    extern const EAllocType DefaultAlloc Y_WEAK = EAllocType::BAlloc;
#ifndef _musl_
    extern const EAllocType DefaultSlaveAlloc Y_WEAK = EAllocType::System;
#else
    extern const EAllocType DefaultSlaveAlloc Y_WEAK = EAllocType::LF;
#endif

    extern const bool EnabledByDefault Y_WEAK = true;

}
