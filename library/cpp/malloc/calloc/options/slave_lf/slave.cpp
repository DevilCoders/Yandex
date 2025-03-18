#include <library/cpp/malloc/calloc/options/options.h>
#include <util/system/compiler.h>

namespace NCalloc {
    // Overriding a weak symbol defined in library/cpp/malloc/calloc/options/options.cpp.
    // Don't link with this object if your platform doesn't support weak linkage.
    extern const EAllocType DefaultSlaveAlloc = EAllocType::LF;
}
