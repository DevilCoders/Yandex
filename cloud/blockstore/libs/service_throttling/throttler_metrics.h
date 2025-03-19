#pragma once

#include <cloud/blockstore/libs/throttling/public.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

IThrottlerMetricsPtr CreateThrottlingServiceMetrics(
    ITimerPtr timer,
    NMonitoring::TDynamicCountersPtr rootGroup,
    ILoggingServicePtr logging);

}   // namespace NCloud::NBlockStore
