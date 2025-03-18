#pragma once

#include "trace_registry.h"

namespace NTraceUsage {
    class TGlobalRegistryGuard: public NNonCopyable::TNonCopyable {
    public:
        TGlobalRegistryGuard(TTraceRegistryPtr registry);
        ~TGlobalRegistryGuard();

        static TTraceRegistryPtr GetCurrentRegistry();
    };

}
