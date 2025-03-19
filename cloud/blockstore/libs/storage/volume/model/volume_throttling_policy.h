#pragma once

#include "public.h"

#include <cloud/blockstore/libs/storage/api/public.h>

#include <util/datetime/base.h>
#include <util/generic/maybe.h>

#include <memory>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

TDuration CalculateBoostTime(const NProto::TVolumePerformanceProfile& config);

////////////////////////////////////////////////////////////////////////////////

struct TThrottlerConfig
{
    const TDuration MaxDelay;
    const ui32 MaxWriteCostMultiplier;
    const ui32 DefaultPostponedRequestWeight;
    const TDuration InitialBoostBudget;

    TThrottlerConfig(
            TDuration maxDelay,
            ui32 maxWriteCostMultiplier,
            ui32 defaultPostponedRequestWeight,
            TDuration initialBoostBudget)
        : MaxDelay(maxDelay)
        , MaxWriteCostMultiplier(maxWriteCostMultiplier)
        , DefaultPostponedRequestWeight(defaultPostponedRequestWeight)
        , InitialBoostBudget(initialBoostBudget)
    {}
};

////////////////////////////////////////////////////////////////////////////////

class TVolumeThrottlingPolicy
{
public:
    enum class EOpType
    {
        Read,
        Write,
        Zero,
        Describe,
        Last,
    };

    struct TRequestInfo
    {
        ui32 ByteCount = 0;
        EOpType OpType = EOpType::Read;
        ui32 PolicyVersion = 0;
    };

private:
    struct TImpl;
    std::unique_ptr<TImpl> Impl;
    ui32 PolicyVersion = 0;

public:
    TVolumeThrottlingPolicy(
        const NProto::TVolumePerformanceProfile& config,
        const TThrottlerConfig& throttlerConfig);
    ~TVolumeThrottlingPolicy();

    void Reset(
        const NProto::TVolumePerformanceProfile& config,
        const TThrottlerConfig& throttlerConfig);

public:
    void OnBackpressureReport(
        TInstant ts,
        const TBackpressureReport& report,
        ui32 partitionIdx);
    void OnPostponedEvent(TInstant ts, const TRequestInfo& requestInfo);

    bool TryPostpone(TInstant ts, const TRequestInfo& requestInfo);
    TMaybe<TDuration> SuggestDelay(
        TInstant ts,
        TDuration queueTime,
        const TRequestInfo& requestInfo);

    double GetWriteCostMultiplier() const;
    TDuration GetCurrentBoostBudget() const;
    ui32 CalculatePostponedWeight() const;
    double CalculateCurrentSpentBudgetShare(TInstant ts) const;
    const TBackpressureReport& GetCurrentBackpressure() const;
    const NProto::TVolumePerformanceProfile& GetConfig() const;

    ui32 GetVersion() const
    {
        return PolicyVersion;
    }

    // the following funcs were made public to display the results on monpages
    ui32 C1(EOpType opType) const;
    ui32 C2(EOpType opType) const;
};

}   // namespace NCloud::NBlockStore::NStorage
