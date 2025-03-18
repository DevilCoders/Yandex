#pragma once

#include <library/cpp/monlib/dynamic_counters/counters.h>

#include <util/generic/ptr.h>

namespace NFuse {

struct TFuseLoopCounters {
    TFuseLoopCounters(NMonitoring::TDynamicCounterPtr counters);

    using TCounterPtr = NMonitoring::TDynamicCounters::TCounterPtr;

    TCounterPtr RequestsPending;
    TCounterPtr ActiveThreads;
};

} // namespace NVcs
