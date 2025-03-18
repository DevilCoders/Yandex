#include "counters.h"

namespace NFuse {

TFuseLoopCounters::TFuseLoopCounters(NMonitoring::TDynamicCounterPtr counters)
    : RequestsPending(counters->GetCounter("RequestsPending"))
    , ActiveThreads(counters->GetCounter("ActiveThreads"))
{
    Y_ENSURE(counters);
}

} // namespace NVcs
