#include "traced_guard.h"

namespace NTraceUsage {
    TAcquireGuard::TAcquireGuard()
        : Registry(TGlobalRegistryGuard::GetCurrentRegistry())
    {
        if (Registry) {
            Registry->ReportStartAcquiringMutex();
        }
    }
    TAcquireGuard::~TAcquireGuard() {
        if (Registry) {
            Registry->ReportAcquiredMutex();
        }
    }

    TReleaseGuard::TReleaseGuard()
        : Registry(TGlobalRegistryGuard::GetCurrentRegistry())
    {
        if (Registry) {
            Registry->ReportStartReleasingMutex();
        }
    }
    TReleaseGuard::~TReleaseGuard() {
        if (Registry) {
            Registry->ReportReleasedMutex();
        }
    }

}
