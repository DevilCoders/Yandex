#include "memory_registry.h"

namespace NTraceUsage {
    TReportBlank TMemoryRegistry::AcquireReportBlank() noexcept try {
        {
            auto guard = Guard(Lock);
            if (Pool) {
                TReportBlank ret = std::move(Pool.back());
                Pool.pop_back();
                return ret;
            }
        }
        return TReportBlank::CreateBlank();
    } catch (...) {
        Y_VERIFY(0, "Exception in trace usage library! Something very bad!");
    }
    void TMemoryRegistry::ReleaseReportBlank(TReportBlank reportBlank) noexcept try {
        ConsumeReport(reportBlank.GetDataRegion());
        reportBlank.Reset();
        {
            auto guard = Guard(Lock);
            Pool.emplace_back(std::move(reportBlank));
        }
    } catch (...) {
        // Ignore problems
    }

}
