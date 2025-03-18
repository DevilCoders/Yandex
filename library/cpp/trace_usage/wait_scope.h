#pragma once

#include "global_registry.h"

namespace NTraceUsage {
    class TWaitScope: private NNonCopyable::TNonCopyable {
    private:
        TTraceRegistryPtr Registry;

    public:
        TWaitScope();
        ~TWaitScope();
    };

}
