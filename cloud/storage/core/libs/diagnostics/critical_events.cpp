#include "critical_events.h"

#include "public.h"

#include <library/cpp/monlib/dynamic_counters/counters.h>

#include <util/string/builder.h>

namespace NCloud {

using namespace NMonitoring;

namespace {

////////////////////////////////////////////////////////////////////////////////

NMonitoring::TDynamicCountersPtr CriticalEvents;

}  // namespace

////////////////////////////////////////////////////////////////////////////////

void InitCriticalEventsCounter(NMonitoring::TDynamicCountersPtr counters)
{
    CriticalEvents = std::move(counters);

#define STORAGE_INIT_CRITICAL_EVENT_COUNTER(name)                              \
    *CriticalEvents->GetCounter("AppCriticalEvents/"#name, true) = 0;          \
// STORAGE_INIT_CRITICAL_EVENT_COUNTER

    STORAGE_CRITICAL_EVENTS(STORAGE_INIT_CRITICAL_EVENT_COUNTER)
#undef STORAGE_INIT_CRITICAL_EVENT_COUNTER
}

TString ReportCriticalEvent(
    const TString& sensorName,
    const TString& message,
    bool verifyDebug)
{
    if (verifyDebug) {
        Y_VERIFY_DEBUG(0);
    }

    if (CriticalEvents) {
        auto counter = CriticalEvents->GetCounter(
            sensorName,
            true);
        counter->Inc();
    }

    TStringBuilder fullMessage;
    fullMessage << "CRITICAL_EVENT:" << sensorName;
    if (message) {
        fullMessage << ":" << message;
        Cerr << fullMessage << Endl;
    }

    return fullMessage;
}

#define STORAGE_DEFINE_CRITICAL_EVENT_ROUTINE(name)                            \
    TString Report##name(const TString& message)                               \
    {                                                                          \
        return ReportCriticalEvent("AppCriticalEvents/"#name, message, false); \
    }                                                                          \
// STORAGE_DEFINE_CRITICAL_EVENT_ROUTINE

    STORAGE_CRITICAL_EVENTS(STORAGE_DEFINE_CRITICAL_EVENT_ROUTINE)
#undef STORAGE_DEFINE_CRITICAL_EVENT_ROUTINE

}   // namespace NCloud
