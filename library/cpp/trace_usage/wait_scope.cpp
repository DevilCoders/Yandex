#include "wait_scope.h"

namespace NTraceUsage {
    TWaitScope::TWaitScope()
        : Registry(TGlobalRegistryGuard::GetCurrentRegistry())
    {
        if (Registry) {
            Registry->ReportStartWaitEvent();
        }
    }
    TWaitScope::~TWaitScope() {
        if (Registry) {
            Registry->ReportFinishWaitEvent();
        }
    }

}
