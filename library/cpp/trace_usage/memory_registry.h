#pragma once

#include "trace_registry.h"

#include <util/generic/vector.h>
#include <util/system/spinlock.h>
#include <util/thread/lfqueue.h>

namespace NTraceUsage {
    class TMemoryRegistry: public ITraceRegistry {
    private:
        TSpinLock Lock;
        TVector<TReportBlank> Pool;

        TReportBlank AcquireReportBlank() noexcept override final;
        void ReleaseReportBlank(TReportBlank reportBlank) noexcept override final;
    };

}
