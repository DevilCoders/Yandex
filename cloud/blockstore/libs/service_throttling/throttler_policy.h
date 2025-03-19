#pragma once

#include <cloud/blockstore/libs/throttling/public.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

struct TThrottlingServiceConfig
{
    const ui32 MaxReadBandwidth;
    const ui32 MaxWriteBandwidth;
    const ui32 MaxReadIops;
    const ui32 MaxWriteIops;
    const TDuration MaxBurstTime;

    TThrottlingServiceConfig(
            ui32 maxReadBandwidth,
            ui32 maxWriteBandwidth,
            ui32 maxReadIops,
            ui32 maxWriteIops,
            TDuration maxBurstTime)
        : MaxReadBandwidth(maxReadBandwidth)
        , MaxWriteBandwidth(maxWriteBandwidth)
        , MaxReadIops(maxReadIops)
        , MaxWriteIops(maxWriteIops)
        , MaxBurstTime(maxBurstTime)
    {}
};

////////////////////////////////////////////////////////////////////////////////

IThrottlerPolicyPtr CreateThrottlingServicePolicy(
    const TThrottlingServiceConfig& config);

}   // namespace NCloud::NBlockStore
