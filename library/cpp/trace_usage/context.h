#pragma once

#include "trace_registry.h"

#include "global_registry.h"

#include <atomic>

namespace NTraceUsage {
    class TReportContext {
        explicit TReportContext(ui64 id)
            : Id_{id}
        {
        }

    public:
        TReportContext() = delete;

        static TReportContext New() {
            auto cur = CurrentId().fetch_add(1, std::memory_order_relaxed);
            return TReportContext{cur};
        }

        ITraceRegistry::TReportConstructor NewReport() const {
            return ITraceRegistry::TReportConstructor{Id_};
        }

        // be aware: it silently fails to register child context if global registry has not yet been set
        TReportContext NewChildContext() const {
            auto ctx = New();
            if (auto registry = TGlobalRegistryGuard::GetCurrentRegistry()) {
                registry->ReportChildContextCreation(Id_, ctx.Id_);
            }
            return ctx;
        }

        // for debugging and testing ONLY
        ui64 Id() const {
            return Id_;
        }

    private:
        static std::atomic<ui64>& CurrentId() {
            static std::atomic<ui64> id{0};
            return id;
        }

        ui64 Id_;
    };
}
