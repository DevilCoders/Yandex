#include "debug_guard.h"

using namespace NOxygen;

TDebugGuard::TDebugGuard(bool enabled)
    : Enabled(enabled)
    , Old(TOxygenLogger::GetInstance().GetLogLevel())
{
    if (Enabled) {
        TOxygenLogger::GetInstance().SetLogLevel(TLOG_DEBUG);
    }
}

TDebugGuard::TDebugGuard(const TDebugGuard& other)
    : Enabled(other.Enabled)
    , Old(other.Old)
{
}

TDebugGuard::~TDebugGuard() {
    if (Enabled) {
        TOxygenLogger::GetInstance().SetLogLevel(Old);
    }
}
