#include "volume_throttling_policy.h"

#include <cloud/blockstore/libs/common/leaky_bucket.h>
#include <cloud/blockstore/libs/storage/protos/part.pb.h>
#include <cloud/blockstore/libs/throttling/throttler_formula.h>

#include <util/generic/size_literals.h>
#include <util/generic/vector.h>
#include <util/generic/ymath.h>

namespace NCloud::NBlockStore::NStorage {

namespace {

////////////////////////////////////////////////////////////////////////////////
// IOPS throttle
//
// Read ops are limited based solely on the requested MaxRead{Bandwidth,Iops}.
// Write ops are limited based on some 'Target{Bandwidth,Iops}', which equals
// MaxWrite{Bandwidth,Iops} when the partition's health is fine and is
// gradually decreased if the partition's not feeling OK until it actually starts
// feeling OK

double CalculateWriteCostMultiplier(const TBackpressureReport& lastReport)
{
    const auto features = {
        lastReport.FreshIndexScore,
        lastReport.CompactionScore,
        lastReport.DiskSpaceScore
    };

    double x = 1;
    for (const auto f: features) {
        x = Max(x, f);
    }

    return x;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

double CalculateBoostRate(const NProto::TVolumePerformanceProfile& config)
{
    return config.GetBoostPercentage() / 100.;
}

TDuration CalculateBoostTime(const NProto::TVolumePerformanceProfile& config)
{
    const auto rate = CalculateBoostRate(config);
    if (rate <= 1.) {
        return TDuration::MilliSeconds(0);
    }

    return TDuration::MilliSeconds(
        static_cast<ui64>((rate - 1.) * config.GetBoostTime()));
}

TDuration CalculateBoostRefillTime(const NProto::TVolumePerformanceProfile& config)
{
    return TDuration::MilliSeconds(config.GetBoostRefillTime());
}

////////////////////////////////////////////////////////////////////////////////

struct TVolumeThrottlingPolicy::TImpl
{
    const NProto::TVolumePerformanceProfile Config;
    const ui32 PolicyVersion;
    const TDuration MaxDelay;
    const ui32 MaxWriteCostMultiplier;
    const ui32 DefaultPostponedRequestWeight;
    TBoostedTimeBucket Bucket;
    TVector<TBackpressureReport> PartitionBackpressures;
    TBackpressureReport CurrentBackpressure;
    double WriteCostMultiplier = 1;
    ui32 PostponedWeight = 0;

    TImpl(
            const NProto::TVolumePerformanceProfile& config,
            const ui32 policyVersion,
            const TDuration maxDelay,
            const ui32 maxWriteCostMultiplier,
            const ui32 defaultPostponedRequestWeight,
            const TDuration initialBoostBudget)
        : Config(config)
        , PolicyVersion(policyVersion)
        , MaxDelay(maxDelay)
        , MaxWriteCostMultiplier(maxWriteCostMultiplier)
        , DefaultPostponedRequestWeight(defaultPostponedRequestWeight)
        , Bucket(
            CalcBurstTime(),
            CalculateBoostRate(Config),
            CalculateBoostTime(Config),
            CalculateBoostRefillTime(Config),
            initialBoostBudget
        )
    {
    }

    TDuration CalcBurstTime() const
    {
        return TBoostedTimeBucket::D(
            (Config.GetBurstPercentage() ? Config.GetBurstPercentage() : 10)
            / 100.);
    }

    void OnBackpressureReport(
        TInstant ts,
        const TBackpressureReport& report,
        ui32 partitionIdx)
    {
        Y_UNUSED(ts);
        Y_VERIFY(partitionIdx < 256);

        if (PartitionBackpressures.size() <= partitionIdx) {
            PartitionBackpressures.resize(partitionIdx + 1);
        }

        PartitionBackpressures[partitionIdx] = report;

        CurrentBackpressure = {};
        for (const auto& report: PartitionBackpressures) {
            if (CurrentBackpressure.CompactionScore < report.CompactionScore) {
                CurrentBackpressure.CompactionScore = report.CompactionScore;
            }

            if (CurrentBackpressure.DiskSpaceScore < report.DiskSpaceScore) {
                CurrentBackpressure.DiskSpaceScore = report.DiskSpaceScore;
            }

            if (CurrentBackpressure.FreshIndexScore < report.FreshIndexScore) {
                CurrentBackpressure.FreshIndexScore = report.FreshIndexScore;
            }
        }

        WriteCostMultiplier = Min(
            CalculateWriteCostMultiplier(CurrentBackpressure),
            double(Max(MaxWriteCostMultiplier, 1u))
        );
    }

    void OnPostponedEvent(TInstant ts, const TRequestInfo& requestInfo)
    {
        Y_UNUSED(ts);

        if (requestInfo.PolicyVersion < PolicyVersion) {
            return;
        }

        const auto weight =
            PostponedRequestWeight(requestInfo.OpType, requestInfo.ByteCount);
        if (PostponedWeight < weight) {
            Y_VERIFY_DEBUG(false);
            PostponedWeight = 0;
        } else {
            PostponedWeight -= weight;
        }
    }

    bool TryPostpone(TInstant ts, ui32 weight)
    {
        Y_UNUSED(ts);

        const auto newWeight = PostponedWeight + weight;
        if (newWeight <= Config.GetMaxPostponedWeight()) {
            PostponedWeight = newWeight;
            return true;
        }

        return false;
    }

    bool TryPostpone(TInstant ts, const TRequestInfo& requestInfo)
    {
        return TryPostpone(
            ts,
            PostponedRequestWeight(requestInfo.OpType, requestInfo.ByteCount)
        );
    }

    ui32 MaxIops(EOpType opType) const
    {
        if (opType == EOpType::Write && Config.GetMaxWriteIops()) {
            return Config.GetMaxWriteIops();
        }

        return Config.GetMaxReadIops();
    }

    ui32 MaxBandwidth(EOpType opType) const
    {
        if (opType == EOpType::Write && Config.GetMaxWriteBandwidth()) {
            return Config.GetMaxWriteBandwidth();
        }

        if (opType == EOpType::Describe) {
            // Disabling throttling by bandwidth for DescribeBlocks requests -
            // they will be throttled only by iops
            // See https://st.yandex-team.ru/NBS-2733
            return 0;
        }

        return Config.GetMaxReadBandwidth();
    }

    TMaybe<TDuration> SuggestDelay(
        TInstant ts,
        TDuration queueTime,
        const TRequestInfo& requestInfo)
    {
        if (requestInfo.PolicyVersion < PolicyVersion) {
            // could be VERIFY_DEBUG, but it's easier to test it this way
            // requests with old versions are expected only in OnPostponedEvent
            return TMaybe<TDuration>();
        }

        if (!requestInfo.ByteCount) {
            return TDuration::Zero();
        }

        ui32 bandwidthUpdate = requestInfo.ByteCount;
        double m = requestInfo.OpType == EOpType::Read ? 1 : WriteCostMultiplier;

        const auto maxBandwidth = MaxBandwidth(requestInfo.OpType);
        const auto maxIops = MaxIops(requestInfo.OpType);

        auto d = Bucket.Register(
            ts,
            m * CostPerIO(
                CalculateThrottlerC1(maxIops, maxBandwidth), 
                CalculateThrottlerC2(maxIops, maxBandwidth),
                bandwidthUpdate
            )
        );

        if (!d.GetValue()) {
            return TDuration::Zero();
        }

        if (d + queueTime > MaxDelay) {
            return TMaybe<TDuration>();
        }

        const auto postponed = TryPostpone(
            ts,
            PostponedRequestWeight(requestInfo.OpType, requestInfo.ByteCount)
        );

        return postponed ? d : TMaybe<TDuration>();
    }

    ui32 PostponedRequestWeight(EOpType opType, ui32 byteCount) const
    {
        return opType == EOpType::Write
            ? byteCount
            : DefaultPostponedRequestWeight;
    }

    double CalculateCurrentSpentBudgetShare(TInstant ts) const
    {
        return Bucket.CalculateCurrentSpentBudgetShare(ts);
    }
};

////////////////////////////////////////////////////////////////////////////////

TVolumeThrottlingPolicy::TVolumeThrottlingPolicy(
    const NProto::TVolumePerformanceProfile& config,
    const TThrottlerConfig& throttlerConfig)
{
    Reset(
        config,
        throttlerConfig
    );
}

TVolumeThrottlingPolicy::~TVolumeThrottlingPolicy()
{}

void TVolumeThrottlingPolicy::Reset(
    const NProto::TVolumePerformanceProfile& config,
    const TThrottlerConfig& throttlerConfig)
{
    Impl.reset(new TImpl(
        config,
        ++PolicyVersion,
        throttlerConfig.MaxDelay,
        throttlerConfig.MaxWriteCostMultiplier,
        throttlerConfig.DefaultPostponedRequestWeight,
        throttlerConfig.InitialBoostBudget
    ));
}

void TVolumeThrottlingPolicy::OnBackpressureReport(
    TInstant ts,
    const TBackpressureReport& report,
    ui32 partitionIdx)
{
    Impl->OnBackpressureReport(ts, report, partitionIdx);
}

void TVolumeThrottlingPolicy::OnPostponedEvent(
    TInstant ts,
    const TRequestInfo& requestInfo)
{
    Impl->OnPostponedEvent(ts, requestInfo);
}

bool TVolumeThrottlingPolicy::TryPostpone(
    TInstant ts,
    const TRequestInfo& requestInfo)
{
    return Impl->TryPostpone(ts, requestInfo);
}

TMaybe<TDuration> TVolumeThrottlingPolicy::SuggestDelay(
    TInstant ts,
    TDuration queueTime,
    const TRequestInfo& requestInfo)
{
    return Impl->SuggestDelay(ts, queueTime, requestInfo);
}

double TVolumeThrottlingPolicy::GetWriteCostMultiplier() const
{
    return Impl->WriteCostMultiplier;
}

TDuration TVolumeThrottlingPolicy::GetCurrentBoostBudget() const
{
    return Impl->Bucket.GetCurrentBoostBudget();
}

ui32 TVolumeThrottlingPolicy::CalculatePostponedWeight() const
{
    return Impl->PostponedWeight;
}

double TVolumeThrottlingPolicy::CalculateCurrentSpentBudgetShare(TInstant ts) const
{
    return Impl->CalculateCurrentSpentBudgetShare(ts);
}

const TBackpressureReport& TVolumeThrottlingPolicy::GetCurrentBackpressure() const
{
    return Impl->CurrentBackpressure;
}

const NProto::TVolumePerformanceProfile& TVolumeThrottlingPolicy::GetConfig() const
{
    return Impl->Config;
}

ui32 TVolumeThrottlingPolicy::C1(EOpType opType) const
{
    return CalculateThrottlerC1(Impl->MaxIops(opType), Impl->MaxBandwidth(opType));
}

ui32 TVolumeThrottlingPolicy::C2(EOpType opType) const
{
    return CalculateThrottlerC2(Impl->MaxIops(opType), Impl->MaxBandwidth(opType));
}

}   // namespace NCloud::NBlockStore::NStorage
