#pragma once

#include "log.h"

namespace NOxygen {

    class TDebugGuard {
    public:
        TDebugGuard(bool enabled = true);
        TDebugGuard(const TDebugGuard& other);

        ~TDebugGuard();

    private:
        bool Enabled;
        ELogPriority Old;
    };

} // namespace NOxygen
