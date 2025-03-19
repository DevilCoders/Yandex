#include "throttling.h"

#include "config.h"

#include <cloud/blockstore/libs/common/leaky_bucket.h>
#include <cloud/blockstore/libs/diagnostics/probes.h>
#include <cloud/blockstore/libs/diagnostics/quota_metrics.h>
#include <cloud/blockstore/libs/diagnostics/request_stats.h>
#include <cloud/blockstore/libs/diagnostics/volume_stats.h>
#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/service/service.h>
#include <cloud/blockstore/libs/throttling/throttler.h>
#include <cloud/blockstore/libs/throttling/throttler_formula.h>
#include <cloud/blockstore/libs/throttling/throttler_logger.h>
#include <cloud/blockstore/libs/throttling/throttler_metrics_base.h>
#include <cloud/blockstore/libs/throttling/throttler_policy.h>
#include <cloud/blockstore/libs/throttling/throttler_tracker.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/format.h>
#include <cloud/storage/core/libs/common/scheduler.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/common/verify.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/diagnostics/max_calculator.h>

#include <cloud/blockstore/config/client.pb.h>

#include <library/cpp/monlib/dynamic_counters/counters.h>

#include <util/generic/deque.h>
#include <util/generic/size_literals.h>
#include <util/stream/format.h>
#include <util/string/builder.h>
#include <util/system/mutex.h>

#include <type_traits>

namespace NCloud::NBlockStore::NClient {

using namespace NThreading;

LWTRACE_USING(BLOCKSTORE_SERVER_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

class TThrottlerPolicy final
    : public IThrottlerPolicy
{
private:
    TBoostedTimeBucket Bucket;
    NProto::TClientPerformanceProfile Profile;

public:
    TThrottlerPolicy(const NProto::TClientPerformanceProfile& pp)
        : Bucket(TBoostedTimeBucket::D(pp.GetBurstTime() / 1'000.))
        , Profile(pp)
    {
    }

    TDuration SuggestDelay(
        TInstant now,
        NCloud::NProto::EStorageMediaKind mediaKind,
        EBlockStoreRequest requestType,
        size_t byteCount) override
    {
        const NProto::TClientMediaKindPerformanceProfile* mediaKindProfile
            = nullptr;

        switch (mediaKind) {
            case NCloud::NProto::STORAGE_MEDIA_SSD_NONREPLICATED: {
                mediaKindProfile = &Profile.GetNonreplProfile();
                break;
            }

            case NCloud::NProto::STORAGE_MEDIA_SSD_MIRROR2: {
                mediaKindProfile = &Profile.GetMirror2Profile();
                break;
            }

            case NCloud::NProto::STORAGE_MEDIA_SSD_MIRROR3: {
                mediaKindProfile = &Profile.GetMirror3Profile();
                break;
            }

            case NCloud::NProto::STORAGE_MEDIA_SSD: {
                mediaKindProfile = &Profile.GetSSDProfile();
                break;
            }

            default: {
                mediaKindProfile = &Profile.GetHDDProfile();
                break;
            }
        }

        ui64 maxIops = 0;
        ui64 maxBandwidth = 0;

        switch (requestType) {
            case EBlockStoreRequest::ReadBlocks:
            case EBlockStoreRequest::ReadBlocksLocal: {
                maxIops = mediaKindProfile->GetMaxReadIops();
                maxBandwidth = mediaKindProfile->GetMaxReadBandwidth();

                break;
            }

            default: {
                maxIops = mediaKindProfile->GetMaxWriteIops();
                maxBandwidth = mediaKindProfile->GetMaxWriteBandwidth();

                break;
            }
        }

        if (!maxIops || !maxBandwidth) {
            return TDuration::Zero();
        }

        return Bucket.Register(
            now,
            CostPerIO(
                CalculateThrottlerC1(maxIops, maxBandwidth),
                CalculateThrottlerC2(maxIops, maxBandwidth),
                byteCount)
        );
    }

    double CalculateCurrentSpentBudgetShare(TInstant ts) const override
    {
        return Bucket.CalculateCurrentSpentBudgetShare(ts);
    }
};

////////////////////////////////////////////////////////////////////////////////

using TQuotaKey = TString;

class TThrottlerMetrics final
    : public TThrottlerMetricsBase<TThrottlerMetrics, TQuotaKey>
{
private:
    TLog Log;

    const TString ClientId;

public:
    TThrottlerMetrics(
            ITimerPtr timer,
            NMonitoring::TDynamicCountersPtr rootGroup,
            ILoggingServicePtr logging,
            const TString& clientId)
        : TThrottlerMetricsBase(
            std::move(timer),
            rootGroup->GetSubgroup("component", "client"),
            rootGroup
                ->GetSubgroup("component", "client_volume")
                ->GetSubgroup("host", "cluster"))
        , Log(logging->CreateLog("BLOCKSTORE_CLIENT"))
        , ClientId(clientId.empty() ? "none" : clientId)
    {}

    void Register(
        const TString& diskId,
        const TString& clientId) override
    {
        Y_UNUSED(clientId);

        RegisterQuota(MakeQuotaKey(diskId));
    }

    void Unregister(
        const TString& diskId,
        const TString& clientId) override
    {
        Y_UNUSED(clientId);

        UnregisterQuota(MakeQuotaKey(diskId));
    }

    std::pair<TString, TString> GetMetricsPath(const TQuotaKey& diskId) const
    {
        return std::make_pair(diskId, ClientId);
    }

    TQuotaKey MakeQuotaKey(const TString& diskId) const
    {
        if (diskId.empty()) {
            STORAGE_ERROR("Disk id is empty");
        }

        return diskId.empty() ? "none" : diskId;
    }
};

////////////////////////////////////////////////////////////////////////////////

class TThrottlerTracker final
    : public IThrottlerTracker
{
public:

#define BLOCKSTORE_IMPLEMENT_METHOD(name, ...)                                 \
    void TrackReceivedRequest(                                                 \
        TCallContext& callContext,                                             \
        IVolumeInfo* volumeInfo,                                               \
        const NProto::T##name##Request& request) override                      \
    {                                                                          \
        if (volumeInfo) {                                                      \
            LWTRACK(                                                           \
                RequestReceived,                                               \
                callContext.LWOrbit,                                           \
                GetBlockStoreRequestName(EBlockStoreRequest::name),            \
                static_cast<ui32>(volumeInfo->GetInfo().GetStorageMediaKind()),\
                GetRequestId(request),                                         \
                GetDiskId(request));                                           \
        }                                                                      \
    }                                                                          \
                                                                               \
    void TrackPostponedRequest(                                                \
        TCallContext& callContext,                                             \
        const NProto::T##name##Request& request) override                      \
    {                                                                          \
        LWTRACK(                                                               \
            RequestPostponed,                                                  \
            callContext.LWOrbit,                                               \
            GetBlockStoreRequestName(EBlockStoreRequest::name),                \
            GetRequestId(request),                                             \
            GetDiskId(request));                                               \
   }                                                                           \
                                                                               \
    void TrackAdvancedRequest(                                                 \
        TCallContext& callContext,                                             \
        const NProto::T##name##Request& request) override                      \
    {                                                                          \
        LWTRACK(                                                               \
            RequestAdvanced,                                                   \
            callContext.LWOrbit,                                               \
            GetBlockStoreRequestName(EBlockStoreRequest::name),                \
            GetRequestId(request),                                             \
            GetDiskId(request));                                               \
    }                                                                          \
// BLOCKSTORE_IMPLEMENT_METHOD

    BLOCKSTORE_SERVICE(BLOCKSTORE_IMPLEMENT_METHOD)

#undef BLOCKSTORE_IMPLEMENT_METHOD
};

////////////////////////////////////////////////////////////////////////////////

class TThrottlingClient final
    : public IBlockStore
{
private:
    IThrottlerPtr Throttler;
    IBlockStorePtr Client;

public:
    TThrottlingClient(
            IBlockStorePtr client,
            IThrottlerPtr throttler)
        : Throttler(std::move(throttler))
        , Client(std::move(client))
    {}

    void Start() override
    {
        Client->Start();
    }

    void Stop() override
    {
        Client->Stop();
    }

    TStorageBuffer AllocateBuffer(size_t bytesCount) override
    {
        return Client->AllocateBuffer(bytesCount);
    }

#define BLOCKSTORE_IMPLEMENT_METHOD(name, ...)                                 \
    TFuture<NProto::T##name##Response> name(                                   \
        TCallContextPtr callContext,                                           \
        std::shared_ptr<NProto::T##name##Request> request) override            \
    {                                                                          \
        return Throttler->name(                                                \
            Client,                                                            \
            std::move(callContext),                                            \
            std::move(request));                                               \
    }                                                                          \
// BLOCKSTORE_IMPLEMENT_METHOD

    BLOCKSTORE_SERVICE(BLOCKSTORE_IMPLEMENT_METHOD)

#undef BLOCKSTORE_IMPLEMENT_METHOD
};

////////////////////////////////////////////////////////////////////////////////

class TThrottlerProvider final
    : public IThrottlerProvider
{
    struct TThrottlerInfo
    {
        std::shared_ptr<IThrottler> Throttler;
        NProto::TClientPerformanceProfile PerformanceProfile;
    };

private:
    const THostPerformanceProfile HostProfile;
    const ILoggingServicePtr Logging;
    const ITimerPtr Timer;
    const ISchedulerPtr Scheduler;
    const NMonitoring::TDynamicCountersPtr RootGroup;
    const IRequestStatsPtr RequestStats;
    const IVolumeStatsPtr VolumeStats;

    TMutex ThrottlerLock;
    THashMap<TString, TThrottlerInfo> Throttlers;

public:
    TThrottlerProvider(
            THostPerformanceProfile hostProfile,
            ILoggingServicePtr logging,
            ITimerPtr timer,
            ISchedulerPtr scheduler,
            NMonitoring::TDynamicCountersPtr rootGroup,
            IRequestStatsPtr requestStats,
            IVolumeStatsPtr volumeStats)
        : HostProfile(hostProfile)
        , Logging(std::move(logging))
        , Timer(std::move(timer))
        , Scheduler(std::move(scheduler))
        , RootGroup(std::move(rootGroup))
        , RequestStats(std::move(requestStats))
        , VolumeStats(std::move(volumeStats))
    {}

    IThrottlerPtr GetThrottler(
        const NProto::TClientConfig& clientConfig,
        const NProto::TClientProfile& clientProfile,
        NProto::TClientPerformanceProfile& performanceProfile) override
    {
        if (clientConfig.GetClientId().empty()) {
            return CreateClientThrottler(
                clientConfig,
                clientProfile,
                performanceProfile);
        }

        with_lock (ThrottlerLock) {
            auto it = Throttlers.find(clientConfig.GetClientId());

            if (it != Throttlers.end()) {
                auto info = it->second;

                performanceProfile = info.PerformanceProfile;
                return info.Throttler;
            }

            auto throttler = CreateClientThrottler(
                clientConfig,
                clientProfile,
                performanceProfile);

            if (!throttler) {
                return nullptr;
            }

            TThrottlerInfo info = {
                .Throttler = throttler,
                .PerformanceProfile = performanceProfile,
            };

            info.Throttler->Start();

            auto inserted = Throttlers.emplace(
                clientConfig.GetClientId(),
                std::move(info)).second;

            STORAGE_VERIFY(
                inserted,
                TWellKnownEntityTypes::CLIENT,
                clientConfig.GetClientId());
            return throttler;
        }
    }

    void Clean() override
    {
        with_lock (ThrottlerLock) {
            for (auto it = Throttlers.begin(); it != Throttlers.end();) {
                if (it->second.Throttler.use_count() == 1) {
                    it->second.Throttler->Stop();
                    Throttlers.erase(it++);
                } else {
                    ++it;
                }
            }
        }
    }

private:
    IThrottlerPtr CreateClientThrottler(
        const NProto::TClientConfig& clientConfig,
        const NProto::TClientProfile& clientProfile,
        NProto::TClientPerformanceProfile& performanceProfile) const
    {
        bool prepared = PreparePerformanceProfile(
            HostProfile,
            clientConfig,
            clientProfile,
            performanceProfile);

        if (!prepared) {
            return nullptr;
        }

        auto throttlerPolicy = CreateThrottlerPolicy(performanceProfile);
        auto throttlerLogger = CreateThrottlerLoggerDefault(
            RequestStats,
            Logging,
            "BLOCKSTORE_CLIENT");
        auto throttlerMetrics = CreateThrottlerMetrics(
            Timer,
            RootGroup,
            Logging,
            clientConfig.GetInstanceId().empty()
                ? clientConfig.GetClientId()
                : clientConfig.GetInstanceId());
        auto throttlerTracker = CreateThrottlerTracker();

        return CreateThrottler(
            std::move(throttlerLogger),
            std::move(throttlerMetrics),
            std::move(throttlerPolicy),
            std::move(throttlerTracker),
            Timer,
            Scheduler,
            VolumeStats);
    }
};

////////////////////////////////////////////////////////////////////////////////

ui64 MinNonzero(ui64 l, ui64 r)
{
    if (l == 0 || r == 0) {
        return Max(l, r);
    }
    return Min(l, r);
}

template <typename... Args>
ui64 MinNonzero(ui64 a, ui64 b, Args... args)
{
    return MinNonzero(a, MinNonzero(b, args...));
}

template <typename... Args>
ui64 GetMinLimit(Args... args)
{
    auto limit = MinNonzero(args...);
    if (limit > Max<ui32>()) {
        limit = Max<ui32>();
    }
    return limit;
}

////////////////////////////////////////////////////////////////////////////////

bool PrepareMediaKindPerformanceProfile(
    const NProto::TClientThrottlingConfig& config,
    const NProto::TClientMediaKindThrottlingConfig& mediaKindConfig,
    const NProto::TClientProfile& profile,
    ui64 maxIopsPerGuest,
    ui64 maxBandwidthPerGuest,
    const bool useLegacyFallback,
    NProto::TClientMediaKindPerformanceProfile& performanceProfile)
{
    maxIopsPerGuest *= mediaKindConfig.GetHostOvercommitPercentage() / 100.0;
    maxBandwidthPerGuest *= mediaKindConfig.GetHostOvercommitPercentage() / 100.0;

    const ui32 cpuUnitsPerCore = 100u;
    auto cpuUnitCount = Max(cpuUnitsPerCore, profile.GetCpuUnitCount());

    if (!performanceProfile.GetMaxReadIops()) {
        ui64 iopsPerCpuUnit = mediaKindConfig.GetReadIopsPerCpuUnit();
        if (!iopsPerCpuUnit && useLegacyFallback) {
            iopsPerCpuUnit = config.GetIopsPerCpuUnit();
        }

        ui64 maxIops = mediaKindConfig.GetMaxReadIops();

        const auto iops = GetMinLimit(
            cpuUnitCount * iopsPerCpuUnit,
            maxIopsPerGuest,
            maxIops
        );
        performanceProfile.SetMaxReadIops(iops);
    }

    if (!performanceProfile.GetMaxWriteIops()) {
        ui64 iopsPerCpuUnit = mediaKindConfig.GetWriteIopsPerCpuUnit();
        if (!iopsPerCpuUnit && useLegacyFallback) {
            iopsPerCpuUnit = config.GetIopsPerCpuUnit();
        }

        ui64 maxIops = mediaKindConfig.GetMaxWriteIops();

        const auto iops = GetMinLimit(
            cpuUnitCount * iopsPerCpuUnit,
            maxIopsPerGuest,
            maxIops
        );
        performanceProfile.SetMaxWriteIops(iops);
    }

    if (!performanceProfile.GetMaxReadBandwidth()) {
        ui64 bandwidthPerCpuUnit = mediaKindConfig.GetReadBandwidthPerCpuUnit();
        if (!bandwidthPerCpuUnit && useLegacyFallback) {
            bandwidthPerCpuUnit = config.GetBandwidthPerCpuUnit();
        }

        ui64 maxBw = mediaKindConfig.GetMaxReadBandwidth() * 1_MB;

        const auto bw = GetMinLimit(
            cpuUnitCount * bandwidthPerCpuUnit * 1_MB,
            maxBandwidthPerGuest,
            maxBw
        );
        performanceProfile.SetMaxReadBandwidth(bw);
    }

    if (!performanceProfile.GetMaxWriteBandwidth()) {
        ui64 bandwidthPerCpuUnit = mediaKindConfig.GetWriteBandwidthPerCpuUnit();
        if (!bandwidthPerCpuUnit && useLegacyFallback) {
            bandwidthPerCpuUnit = config.GetBandwidthPerCpuUnit();
        }

        ui64 maxBw = mediaKindConfig.GetMaxWriteBandwidth() * 1_MB;

        const auto bw = GetMinLimit(
            cpuUnitCount * bandwidthPerCpuUnit * 1_MB,
            maxBandwidthPerGuest,
            maxBw
        );
        performanceProfile.SetMaxWriteBandwidth(bw);
    }

    return performanceProfile.GetMaxReadIops()
        && performanceProfile.GetMaxWriteIops()
        && performanceProfile.GetMaxReadBandwidth()
        && performanceProfile.GetMaxWriteBandwidth();
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

bool PreparePerformanceProfile(
    const THostPerformanceProfile& hostProfile,
    const NProto::TClientConfig& config,
    const NProto::TClientProfile& profile,
    NProto::TClientPerformanceProfile& performanceProfile)
{
    if (!profile.GetCpuUnitCount()) {
        return false;
    }

    if (profile.GetHostType() == NProto::HOST_TYPE_DEDICATED) {
        return false;
    }

    const auto& tc = config.GetThrottlingConfig();
    auto burstPercentage = tc.GetBurstPercentage();

    if (!burstPercentage) {
        burstPercentage = 100 / Max(tc.GetBurstDivisor(), 1u);
    }

    if (!performanceProfile.GetBurstTime()) {
        performanceProfile.SetBurstTime(1'000 * burstPercentage / 100);
    }

    double hostFraction = 0.0;
    if (hostProfile.CpuCount > 0) {
        hostFraction = profile.GetCpuUnitCount() / (hostProfile.CpuCount * 100.0);
    }

    ui64 networkBandwidth = (hostProfile.NetworkMbitThroughput / 8) * 1_MB
        * (tc.GetNetworkThroughputPercentage() / 100.0);

    if (!networkBandwidth) {
        networkBandwidth = Max<ui32>();
    }

    ui64 maxIopsPerGuest = hostFraction * tc.GetMaxIopsPerHost();
    ui64 maxBandwidthPerGuest = hostFraction
        * Min(tc.GetMaxBandwidthPerHost(), networkBandwidth);

    const auto init = {
        PrepareMediaKindPerformanceProfile(
            tc,
            tc.GetHDDThrottlingConfig(),
            profile,
            maxIopsPerGuest,
            maxBandwidthPerGuest,
            true,
            *performanceProfile.MutableHDDProfile()
        ),
        PrepareMediaKindPerformanceProfile(
            tc,
            tc.GetSSDThrottlingConfig(),
            profile,
            maxIopsPerGuest,
            maxBandwidthPerGuest,
            true,
            *performanceProfile.MutableSSDProfile()
        ),
        PrepareMediaKindPerformanceProfile(
            tc,
            tc.GetNonreplThrottlingConfig(),
            profile,
            maxIopsPerGuest,
            maxBandwidthPerGuest,
            false,
            *performanceProfile.MutableNonreplProfile()
        ),
        PrepareMediaKindPerformanceProfile(
            tc,
            tc.GetMirror2ThrottlingConfig(),
            profile,
            maxIopsPerGuest,
            maxBandwidthPerGuest,
            false,
            *performanceProfile.MutableMirror2Profile()
        ),
        PrepareMediaKindPerformanceProfile(
            tc,
            tc.GetMirror3ThrottlingConfig(),
            profile,
            maxIopsPerGuest,
            maxBandwidthPerGuest,
            false,
            *performanceProfile.MutableMirror3Profile()
        ),
    };

    return std::any_of(init.begin(), init.end(), [] (bool x) {return x;});
}

IThrottlerPolicyPtr CreateThrottlerPolicy(
    NProto::TClientPerformanceProfile performanceProfile)
{
    return std::make_shared<TThrottlerPolicy>(
        std::move(performanceProfile));
}

IThrottlerMetricsPtr CreateThrottlerMetrics(
    ITimerPtr timer,
    NMonitoring::TDynamicCountersPtr rootGroup,
    ILoggingServicePtr logging,
    const TString& clientId)
{
    return std::make_shared<TThrottlerMetrics>(
        std::move(timer),
        std::move(rootGroup),
        std::move(logging),
        clientId);
}

IThrottlerTrackerPtr CreateThrottlerTracker()
{
    return std::make_shared<TThrottlerTracker>();
}

IBlockStorePtr CreateThrottlingClient(
    IBlockStorePtr client,
    IThrottlerPtr throttler)
{
    return std::make_shared<TThrottlingClient>(
        std::move(client),
        std::move(throttler));
}

IThrottlerProviderPtr CreateThrottlerProvider(
    THostPerformanceProfile hostProfile,
    ILoggingServicePtr logging,
    ITimerPtr timer,
    ISchedulerPtr scheduler,
    NMonitoring::TDynamicCountersPtr rootGroup,
    IRequestStatsPtr requestStats,
    IVolumeStatsPtr volumeStats)
{
    return std::make_shared<TThrottlerProvider>(
        hostProfile,
        std::move(logging),
        std::move(timer),
        std::move(scheduler),
        std::move(rootGroup),
        std::move(requestStats),
        std::move(volumeStats));
}

}   // namespace NCloud::NBlockStore::NClient
