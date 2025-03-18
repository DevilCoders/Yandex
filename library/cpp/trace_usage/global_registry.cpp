#include "global_registry.h"

#include <util/system/spinlock.h>

namespace NTraceUsage {
    TSpinLock globalLock;
    TTraceRegistryPtr globalRegistry;

    TGlobalRegistryGuard::TGlobalRegistryGuard(TTraceRegistryPtr registry) {
        TGuard<TSpinLock> guard(globalLock);
        Y_VERIFY(!globalRegistry, "There can be only one global registry!");
        globalRegistry.Swap(registry);
    }
    TGlobalRegistryGuard::~TGlobalRegistryGuard() {
        TGuard<TSpinLock> guard(globalLock);
        globalRegistry.Drop();
    }

    TTraceRegistryPtr TGlobalRegistryGuard::GetCurrentRegistry() {
        TGuard<TSpinLock> guard(globalLock);
        return globalRegistry;
    }

}
