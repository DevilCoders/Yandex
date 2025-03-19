#pragma once

#include "public.h"

#include <cloud/blockstore/libs/common/public.h>
#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/service/request.h>
#include <cloud/blockstore/libs/throttling/public.h>

namespace NCloud::NBlockStore::NClient {

////////////////////////////////////////////////////////////////////////////////

struct THostPerformanceProfile
{
    ui32 CpuCount = 0;
    ui32 NetworkMbitThroughput = 0;
};

////////////////////////////////////////////////////////////////////////////////

struct IThrottlerProvider
{
    virtual ~IThrottlerProvider() = default;

    virtual IThrottlerPtr GetThrottler(
        const NProto::TClientConfig& clientConfig,
        const NProto::TClientProfile& clientProfile,
        NProto::TClientPerformanceProfile& performanceProfile) = 0;

    virtual void Clean() = 0;
};

////////////////////////////////////////////////////////////////////////////////

bool PreparePerformanceProfile(
    const THostPerformanceProfile& hostProfile,
    const NProto::TClientConfig& config,
    const NProto::TClientProfile& profile,
    NProto::TClientPerformanceProfile& performanceProfile);

IThrottlerPolicyPtr CreateThrottlerPolicy(
    NProto::TClientPerformanceProfile performanceProfile);

IThrottlerMetricsPtr CreateThrottlerMetrics(
    ITimerPtr timer,
    NMonitoring::TDynamicCountersPtr rootGroup,
    ILoggingServicePtr logging,
    const TString& clientId);

IThrottlerTrackerPtr CreateThrottlerTracker();

IBlockStorePtr CreateThrottlingClient(
    IBlockStorePtr client,
    IThrottlerPtr throttler);

IThrottlerProviderPtr CreateThrottlerProvider(
    THostPerformanceProfile hostProfile,
    ILoggingServicePtr logging,
    ITimerPtr timer,
    ISchedulerPtr scheduler,
    NMonitoring::TDynamicCountersPtr rootGroup,
    IRequestStatsPtr requestStats,
    IVolumeStatsPtr volumeStats);

}   // namespace NCloud::NBlockStore::NClient
