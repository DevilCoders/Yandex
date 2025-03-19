#include <cloud/blockstore/libs/storage/testlib/test_env.h>

#include <cloud/blockstore/config/storage.pb.h>
#include <cloud/blockstore/libs/diagnostics/cgroup_stats_fetcher.h>
#include <cloud/blockstore/libs/diagnostics/volume_stats.h>
#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/api/volume_balancer.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/public.h>
#include <cloud/blockstore/libs/storage/core/features_config.h>
#include <cloud/blockstore/libs/storage/volume_balancer/volume_balancer.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

namespace {

////////////////////////////////////////////////////////////////////////////////

NProto::TVolumeBalancerDiskStats CreateVolumeStats(
    const TString& diskId,
    const TString& cloudId,
    ui64 systemCpu,
    ui64 userCpu,
    bool isLocal)
{
    NProto::TVolumeBalancerDiskStats stats;
    stats.SetDiskId(diskId);
    stats.SetCloudId(cloudId);
    stats.SetSystemCpu(systemCpu);
    stats.SetUserCpu(userCpu);
    stats.SetIsLocal(isLocal);
    stats.SetPreemptionSource(NProto::EPreemptionSource::SOURCE_BALANCER);

    return stats;
}

NProto::TVolumeBalancerDiskStats CreateVolumeStats(
    const TString& diskId,
    const TString& cloudId,
    ui64 systemCpu,
    ui64 userCpu,
    bool isLocal,
    NProto::EPreemptionSource source)
{
    NProto::TVolumeBalancerDiskStats stats;
    stats.SetDiskId(diskId);
    stats.SetCloudId(cloudId);
    stats.SetSystemCpu(systemCpu);
    stats.SetUserCpu(userCpu);
    stats.SetIsLocal(isLocal);
    stats.SetPreemptionSource(source);

    return stats;
}

////////////////////////////////////////////////////////////////////////////////

class TVolumeBalancerConfigBuilder
{
private:
    NProto::TStorageServiceConfig StorageConfig;

public:
    TVolumeBalancerConfigBuilder& WithType(NProto::EVolumePreemptionType mode)
    {
        StorageConfig.SetVolumePreemptionType(mode);
        return *this;
    }

    TVolumeBalancerConfigBuilder& WithCpuMatBenchLimits(ui64 systemNs, ui64 userNs)
    {
        StorageConfig.SetCpuMatBenchNsSystemThreshold(systemNs);
        StorageConfig.SetCpuMatBenchNsUserThreshold(userNs);
        return *this;
    }

    TVolumeBalancerConfigBuilder& WithPeemption(ui64 pushPercents, ui64 pullPercents)
    {
        StorageConfig.SetPreemptionPushPercentage(pushPercents);
        StorageConfig.SetPreemptionPullPercentage(pullPercents);
        return *this;
    }

    NProto::TStorageServiceConfig Build()
    {
        return StorageConfig;
    }
};

////////////////////////////////////////////////////////////////////////////////

using TPerfStatCallback = std::function<TVolumePerfStatuses()>;

struct TVolumeStatsTestMock final
    : public IVolumeStats
{
    TPerfStatCallback Callback;

    bool MountVolume(
        const NProto::TVolume& volume,
        const TString& clientId,
        const TString& instanceId) override
    {
        Y_UNUSED(volume);
        Y_UNUSED(clientId);
        Y_UNUSED(instanceId);
        return true;
    }

    void UnmountVolume(
        const TString& diskId,
        const TString& clientId) override
    {
        Y_UNUSED(diskId);
        Y_UNUSED(clientId);
    }

    IVolumeInfoPtr GetVolumeInfo(
        const TString& diskId,
        const TString& clientId) const override
    {
        Y_UNUSED(diskId);
        Y_UNUSED(clientId);
        return nullptr;
    }

    ui32 GetBlockSize(const TString& diskId) const override
    {
        Y_UNUSED(diskId);
        return 0;
    }

    void TrimVolumes() override
    {
    }

    void UpdateStats(bool updatePercentiles) override
    {
        Y_UNUSED(updatePercentiles);
    }

    void SetPerfCallback(TPerfStatCallback callback)
    {
        Callback = std::move(callback);
    }

    TVolumePerfStatuses GatherVolumePerfStatuses() override
    {
        if (Callback) {
            return Callback();
        }
        return {};
    }
};


////////////////////////////////////////////////////////////////////////////////

struct TCgroupStatsFetcherMock: public ICgroupStatsFetcher
{
    TDuration Value;

    void SetCpuWaitValue(TDuration value)
    {
        Value = value;
    }

    void Start() override
    {
    }

    void Stop() override
    {
    }

    TDuration GetCpuWait() override
    {
        return Value;
    };
};

////////////////////////////////////////////////////////////////////////////////

using EChangeBindingOp = TEvService::TEvChangeVolumeBindingRequest::EChangeBindingOp;
using EBalancerStatus = NPrivateProto::EBalancerOpStatus;

class TVolumeBalancerTestEnv
{
private:
    TTestEnv TestEnv;
    TActorId Sender;

public:
    TVolumeBalancerTestEnv()
    {
        Sender = TestEnv.GetRuntime().AllocateEdgeActor();
    }

    TActorId GetEdgeActor() const
    {
        return Sender;
    }

    TTestActorRuntime& GetRuntime()
    {
        return TestEnv.GetRuntime();
    }

    TActorId Register(IActorPtr actor)
    {
        auto actorId = TestEnv.GetRuntime().Register(actor.release());
        TestEnv.GetRuntime().EnableScheduleForActor(actorId);

        return actorId;
    }

    void AdjustTime()
    {
        TestEnv.GetRuntime().AdvanceCurrentTime(TDuration::Seconds(15));
        TestEnv.GetRuntime().DispatchEvents({}, TDuration::Seconds(1));
    }

    void AdjustTime(TDuration interval)
    {
        TestEnv.GetRuntime().AdvanceCurrentTime(interval);
        TestEnv.GetRuntime().DispatchEvents({}, TDuration::Seconds(1));
    }

    void Send(const TActorId& recipient, IEventBasePtr event)
    {
        TestEnv.GetRuntime().Send(new IEventHandle(recipient, Sender, event.release()));
    }

    void DispatchEvents()
    {
        TestEnv.GetRuntime().DispatchEvents(TDispatchOptions(), TDuration());
    }

    void DispatchEvents(TDuration timeout)
    {
        TestEnv.GetRuntime().DispatchEvents(TDispatchOptions(), timeout);
    }

    THolder<TEvService::TEvChangeVolumeBindingRequest> GrabBindingRequest()
    {
        return TestEnv.GetRuntime().
            GrabEdgeEvent<TEvService::TEvChangeVolumeBindingRequest>(TDuration());
    }

    THolder<TEvService::TEvGetVolumeStatsRequest> GrabVolumesStatsRequest()
    {
        return TestEnv.GetRuntime().
            GrabEdgeEvent<TEvService::TEvGetVolumeStatsRequest>(TDuration());
    }

    void SendVolumesStatsResponse(
        TActorId receiver,
        const TString diskId,
        ui64 systemCpu,
        ui64 userCpu,
        bool isLocal)
    {
        auto stats = CreateVolumeStats(
            diskId,
            "",
            systemCpu,
            userCpu,
            isLocal);

        TVector<NProto::TVolumeBalancerDiskStats> volumes;
        volumes.push_back(std::move(stats));

        Send(
            receiver,
            std::make_unique<TEvService::TEvGetVolumeStatsResponse>(std::move(volumes)));
    }

    void SendVolumesStatsResponse(
        TActorId receiver,
        const TString diskId,
        ui64 systemCpu,
        ui64 userCpu,
        bool isLocal,
        NProto::EPreemptionSource source)
    {
        auto stats = CreateVolumeStats(
            diskId,
            "",
            systemCpu,
            userCpu,
            isLocal,
            source);

        TVector<NProto::TVolumeBalancerDiskStats> volumes;
        volumes.push_back(std::move(stats));

        Send(
            receiver,
            std::make_unique<TEvService::TEvGetVolumeStatsResponse>(std::move(volumes)));
    }

    void SendVolumesStatsResponse(
        TActorId receiver,
        TVector<NProto::TVolumeBalancerDiskStats> volumes)
    {
        Send(
            receiver,
            std::make_unique<TEvService::TEvGetVolumeStatsResponse>(std::move(volumes)));
    }

    void SendConfigureVolumeBalancerRequest(TActorId receiver, EBalancerStatus status)
    {
        auto request = std::make_unique<TEvVolumeBalancer::TEvConfigureVolumeBalancerRequest>();
        request->Record.SetOpStatus(status);
        Send(receiver, std::move(request));
    }

    THolder<TEvVolumeBalancer::TEvConfigureVolumeBalancerResponse> GrabConfigureVolumeBalancerResponse()
    {
        return TestEnv.GetRuntime().
            GrabEdgeEvent<TEvVolumeBalancer::TEvConfigureVolumeBalancerResponse>(TDuration());
    }

    void AddVolumeToVolumesStatsResponse(
        TVector<NProto::TVolumeBalancerDiskStats>& volumes,
        const TString diskId,
        ui64 systemCpu,
        ui64 userCpu,
        bool isLocal)
    {
        auto stats = CreateVolumeStats(
            diskId,
            "",
            systemCpu,
            userCpu,
            isLocal);

        volumes.push_back(std::move(stats));
    }

    void SetSystemPoolCpuConsumption(ui64 consumption)
    {
        auto counter = TestEnv.GetRuntime().GetAppData(0).Counters
            ->GetSubgroup("counters", "utils")
            ->GetSubgroup("execpool", "System")
            ->GetCounter("ElapsedMicrosec", true);
        counter->Add(consumption * 15);
    }

    void SetUserPoolCpuConsumption(ui64 consumption)
    {
        auto counter = TestEnv.GetRuntime().GetAppData(0).Counters
            ->GetSubgroup("counters", "utils")
            ->GetSubgroup("execpool", "User")
            ->GetCounter("ElapsedMicrosec", true);
        counter->Add(consumption * 15);
    }

    void SetUserPoolMatBenchNs(ui64 ns)
    {
        auto counter = TestEnv.GetRuntime().GetAppData(0).Counters
            ->GetSubgroup("counters", "utils")
            ->GetSubgroup("execpool", "User")
            ->GetCounter("CpuMatBenchNs", false);
        *counter = ns;
    }

    void SetSysPoolMatBenchNs(ui64 ns)
    {
        auto counter = TestEnv.GetRuntime().GetAppData(0).Counters
            ->GetSubgroup("counters", "utils")
            ->GetSubgroup("execpool", "System")
            ->GetCounter("CpuMatBenchNs", false);
        *counter = ns;
    }

    void SetCpuConsumption(ui64 systemUs, ui64 userUs)
    {
        SetSystemPoolCpuConsumption(systemUs);
        SetUserPoolCpuConsumption(userUs);
    }

    void SetMatBenchNs(ui64 systemNs, ui64 userNs)
    {
        SetSysPoolMatBenchNs(systemNs);
        SetUserPoolMatBenchNs(userNs);
    }
};

TFeaturesConfigPtr CreateFeatureConfig(
    const TString& featureName,
    const TVector<TString>& list,
    bool blacklist = true)
{
    NProto::TFeaturesConfig config;
    if (featureName) {
        auto* feature = config.MutableFeatures()->Add();
        feature->SetName(featureName);
        if (blacklist) {
            for (const auto& c: list) {
                *feature->MutableBlacklist()->MutableCloudIds()->Add() = c;
            }
        } else {
            for (const auto& c: list) {
                *feature->MutableWhitelist()->MutableCloudIds()->Add() = c;
            }
        }
    }
    return std::make_shared<TFeaturesConfig>(config);
}

IActorPtr CreateVolumeBalancerActor(
    TStorageConfigPtr storageConfig,
    IVolumeStatsPtr volumeStats,
    ICgroupStatsFetcherPtr cgroupStatsFetcher,
    TActorId serviceActorId)
{
    NKikimrConfig::TAppConfigPtr appConfig = std::make_shared<NKikimrConfig::TAppConfig>();
    auto* system = appConfig->MutableActorSystemConfig()->AddExecutor();
    system->SetThreads(1);
    system->SetName("System");

    auto* user = appConfig->MutableActorSystemConfig()->AddExecutor();
    user->SetThreads(1);
    user->SetName("User");

    appConfig->MutableActorSystemConfig()->SetSysExecutor(0);
    appConfig->MutableActorSystemConfig()->SetUserExecutor(1);

    return CreateVolumeBalancerActor(
        appConfig,
        storageConfig,
        std::move(volumeStats),
        std::move(cgroupStatsFetcher),
        std::move(serviceActorId));
}


IActorPtr CreateVolumeBalancerActor(
    TVolumeBalancerConfigBuilder& config,
    IVolumeStatsPtr volumeStats,
    ICgroupStatsFetcherPtr cgroupStatsFetcher,
    TActorId serviceActorId)
{
    NProto::TStorageServiceConfig storageConfig = config.Build();

    return CreateVolumeBalancerActor(
         std::make_shared<TStorageConfig>(
            config.Build(),
            CreateFeatureConfig("Balancer", {})
        ),
        std::move(volumeStats),
        std::move(cgroupStatsFetcher),
        std::move(serviceActorId));
}

IActorPtr CreateVolumeBalancerActor(
    TVolumeBalancerConfigBuilder& config,
    ICgroupStatsFetcherPtr cgroupStatsFetcher,
    TActorId serviceActorId)
{
    return CreateVolumeBalancerActor(
        config,
        CreateVolumeStatsStub(),
        std::move(cgroupStatsFetcher),
        std::move(serviceActorId));
}

IActorPtr CreateVolumeBalancerActor(
    TVolumeBalancerConfigBuilder& config,
    TActorId serviceActorId)
{
    return CreateVolumeBalancerActor(
        config,
        CreateCgroupStatsFetcherStub(),
        std::move(serviceActorId));
}

void RunState(
    TVolumeBalancerTestEnv& testEnv,
    TActorId actorId,
    ui64 cpuLoad,
    ui64 volumeLoad,
    bool isLocal,
    NProto::EPreemptionSource source = NProto::EPreemptionSource::SOURCE_BALANCER)
{
    for (ui32 i = 0; i < 5; ++i) {
        testEnv.GrabVolumesStatsRequest();

        testEnv.SetCpuConsumption(cpuLoad, cpuLoad);

        testEnv.SendVolumesStatsResponse(
            actorId,
            "vol0",
            volumeLoad,
            volumeLoad,
            isLocal,
            source);

        testEnv.AdjustTime();
    }
}

void RunState(
    TVolumeBalancerTestEnv& testEnv,
    TActorId actorId,
    const TVector<NProto::TVolumeBalancerDiskStats>& vols,
    ui64 cpuLoad)
{
    for (ui32 i = 0; i < 5; ++i) {
        testEnv.GrabVolumesStatsRequest();

        testEnv.SetCpuConsumption(cpuLoad, cpuLoad);

        testEnv.SendVolumesStatsResponse(
            actorId,
            vols);

        testEnv.AdjustTime();
    }
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TVolumeBalancerTest)
{
    Y_UNIT_TEST(ShouldPushVolumesToHiveControl)
    {
        TVolumeBalancerTestEnv testEnv;
        TVolumeBalancerConfigBuilder config;

        auto volumeBindingActorID = testEnv.Register(CreateVolumeBalancerActor(
            config.WithType(NProto::PREEMPTION_MOVE_MOST_HEAVY),
            testEnv.GetEdgeActor()));

        testEnv.DispatchEvents();

        RunState(testEnv, volumeBindingActorID, 1000000, 1000000, true);

        auto ev = testEnv.GrabBindingRequest();
        UNIT_ASSERT(ev);
        UNIT_ASSERT(ev->Action == EChangeBindingOp::RELEASE_TO_HIVE);
    }

    Y_UNIT_TEST(ShouldPushVolumesMostHeavy)
    {
        TVolumeBalancerTestEnv testEnv;
        TVolumeBalancerConfigBuilder config;

        auto volumeBindingActorID = testEnv.Register(CreateVolumeBalancerActor(
            config.WithType(NProto::PREEMPTION_MOVE_MOST_HEAVY),
            testEnv.GetEdgeActor()));

        testEnv.DispatchEvents();

        TVector<NProto::TVolumeBalancerDiskStats> vol;
        testEnv.AddVolumeToVolumesStatsResponse(vol, "vol0", 1000000, 1000000, true);
        testEnv.AddVolumeToVolumesStatsResponse(vol, "vol1", 10000, 10000, true);

        RunState(testEnv, volumeBindingActorID, vol, 1000000);

        auto ev = testEnv.GrabBindingRequest();
        UNIT_ASSERT(ev);
        UNIT_ASSERT(
            ev->Action == EChangeBindingOp::RELEASE_TO_HIVE &&
            ev->DiskId == "vol0");
    }

    Y_UNIT_TEST(ShouldPushVolumesLeastHeavy)
    {
        TVolumeBalancerTestEnv testEnv;
        TVolumeBalancerConfigBuilder config;

        auto volumeBindingActorID = testEnv.Register(CreateVolumeBalancerActor(
            config.WithType(NProto::PREEMPTION_MOVE_LEAST_HEAVY),
            testEnv.GetEdgeActor()));

        testEnv.DispatchEvents();

        TVector<NProto::TVolumeBalancerDiskStats> vol;
        testEnv.AddVolumeToVolumesStatsResponse(vol, "vol0", 1000000, 1000000, true);
        testEnv.AddVolumeToVolumesStatsResponse(vol, "vol1", 10000, 10000, true);

        RunState(testEnv, volumeBindingActorID, vol, 1000000);

        auto ev = testEnv.GrabBindingRequest();
        UNIT_ASSERT(ev);
        UNIT_ASSERT(
            ev->Action == EChangeBindingOp::RELEASE_TO_HIVE &&
            ev->DiskId == "vol1");
    }

    Y_UNIT_TEST(ShouldPushVolumesToHiveControlAndReturnBack)
    {
        TVolumeBalancerTestEnv testEnv;
        TVolumeBalancerConfigBuilder configBuilder;

        auto storageConfig = std::make_shared<TStorageConfig>(
            configBuilder.WithType(NProto::PREEMPTION_MOVE_MOST_HEAVY).Build(),
            CreateFeatureConfig("Balancer", {})
        );

        auto volumeStats = std::make_shared<TVolumeStatsTestMock>();

        auto volumeBindingActorID = testEnv.Register(CreateVolumeBalancerActor(
            storageConfig,
            volumeStats,
            CreateCgroupStatsFetcherStub(),
            testEnv.GetEdgeActor()));

        testEnv.DispatchEvents();

        TVector<NProto::TVolumeBalancerDiskStats> vol;
        testEnv.AddVolumeToVolumesStatsResponse(vol, "vol0", 1000000, 1000000, true);

        RunState(testEnv, volumeBindingActorID, vol, 1000000);

        auto ev = testEnv.GrabBindingRequest();
        UNIT_ASSERT(ev);
        UNIT_ASSERT(ev->Action == EChangeBindingOp::RELEASE_TO_HIVE);

        testEnv.Send(
            volumeBindingActorID,
            std::make_unique<TEvService::TEvChangeVolumeBindingResponse>());

        RunState(testEnv, volumeBindingActorID, 0, 1000000, false);

        volumeStats->SetPerfCallback([] () {
            TVolumePerfStatuses res;

            res.emplace_back("vol0", 1);

            return res;
        });

        testEnv.AdjustTime(storageConfig->GetInitialPullDelay());

        testEnv.GrabVolumesStatsRequest();

        testEnv.SetCpuConsumption(0, 0);

        testEnv.SendVolumesStatsResponse(
            volumeBindingActorID,
            "vol0",
            1000000,
            1000000,
            false);

        testEnv.AdjustTime();

        ev = testEnv.GrabBindingRequest();
        UNIT_ASSERT(ev);
        UNIT_ASSERT(ev->Action == EChangeBindingOp::ACQUIRE_FROM_HIVE);
   }

    Y_UNIT_TEST(ShouldPushVolumesToHiveControlIfMatBenchNsIsHigh)
    {
        TVolumeBalancerTestEnv testEnv;
        TVolumeBalancerConfigBuilder config;

        auto volumeBindingActorID = testEnv.Register(CreateVolumeBalancerActor(
            config.WithType(NProto::PREEMPTION_MOVE_MOST_HEAVY).WithCpuMatBenchLimits(100, 100),
            testEnv.GetEdgeActor()));

        testEnv.DispatchEvents();

        testEnv.GrabVolumesStatsRequest();

        testEnv.SetCpuConsumption(0, 0);
        testEnv.SetMatBenchNs(101, 101);

        testEnv.SendVolumesStatsResponse(
            volumeBindingActorID,
            "vol0",
            1000000,
            1000000,
            true);

        auto ev = testEnv.GrabBindingRequest();
        UNIT_ASSERT(ev);
        UNIT_ASSERT(ev->Action == EChangeBindingOp::RELEASE_TO_HIVE);
    }

    Y_UNIT_TEST(ShouldPullVolumesFromHiveIfLoadIsLow)
    {
        TVolumeBalancerTestEnv testEnv;
        TVolumeBalancerConfigBuilder configBuilder;

        auto storageConfig = std::make_shared<TStorageConfig>(
            configBuilder.WithType(NProto::PREEMPTION_MOVE_MOST_HEAVY).Build(),
            CreateFeatureConfig("Balancer", {})
        );

        auto volumeStats = std::make_shared<TVolumeStatsTestMock>();

        auto volumeBindingActorID = testEnv.Register(CreateVolumeBalancerActor(
            storageConfig,
            volumeStats,
            CreateCgroupStatsFetcherStub(),
            testEnv.GetEdgeActor()));

        testEnv.DispatchEvents();

        RunState(testEnv, volumeBindingActorID, 1000000, 1000000, true);

        auto ev = testEnv.GrabBindingRequest();
        UNIT_ASSERT(ev);
        UNIT_ASSERT(ev->Action == EChangeBindingOp::RELEASE_TO_HIVE);


        RunState(testEnv, volumeBindingActorID, 0, 1000000, false);

        volumeStats->SetPerfCallback([] () {
            TVolumePerfStatuses res;

            res.emplace_back("vol0", 1);

            return res;
        });

        testEnv.AdjustTime(storageConfig->GetInitialPullDelay());

        testEnv.GrabVolumesStatsRequest();

        testEnv.SetCpuConsumption(0, 0);

        testEnv.SendVolumesStatsResponse(
            volumeBindingActorID,
            "vol0",
            1000000,
            1000000,
            false);

        testEnv.AdjustTime();

        ev = testEnv.GrabBindingRequest();
        UNIT_ASSERT(ev);
        UNIT_ASSERT(ev->Action == EChangeBindingOp::ACQUIRE_FROM_HIVE);
    }

    Y_UNIT_TEST(ShouldNotPullVolumeIfItWasPreemptedByUser)
    {
        TVolumeBalancerTestEnv testEnv;
        TVolumeBalancerConfigBuilder config;

        auto volumeBindingActorID = testEnv.Register(CreateVolumeBalancerActor(
            config.WithType(NProto::PREEMPTION_MOVE_MOST_HEAVY),
            testEnv.GetEdgeActor()));

        testEnv.DispatchEvents();

        RunState(testEnv, volumeBindingActorID, 0, 1000000, false, NProto::EPreemptionSource::SOURCE_MANUAL);

        auto ev = testEnv.GrabBindingRequest();
        UNIT_ASSERT(!ev);
    }

    Y_UNIT_TEST(ShouldNotDoAnythingIfBalancerIsDisabled)
    {
        TVolumeBalancerTestEnv testEnv;
        TVolumeBalancerConfigBuilder config;

        auto volumeBindingActorID = testEnv.Register(CreateVolumeBalancerActor(
            config,
            testEnv.GetEdgeActor()));

        testEnv.DispatchEvents();

        RunState(testEnv, volumeBindingActorID, 0, 1000000, false);

        auto ev = testEnv.GrabBindingRequest();
        UNIT_ASSERT(!ev);

        RunState(testEnv, volumeBindingActorID, 100, 1000000, false);

        ev = testEnv.GrabBindingRequest();
        UNIT_ASSERT(!ev);
    }

    Y_UNIT_TEST(ShouldNotDoAnythingIfBalancerIsDisabledViaPrivateApi)
    {
        TVolumeBalancerTestEnv testEnv;
        TVolumeBalancerConfigBuilder config;
        config.WithType(NProto::PREEMPTION_MOVE_MOST_HEAVY);

        auto volumeBindingActorID = testEnv.Register(CreateVolumeBalancerActor(
            config,
            testEnv.GetEdgeActor()));

        testEnv.DispatchEvents();

        testEnv.SendConfigureVolumeBalancerRequest(volumeBindingActorID, EBalancerStatus::DISABLE);
        auto response = testEnv.GrabConfigureVolumeBalancerResponse();

        UNIT_ASSERT_VALUES_EQUAL(EBalancerStatus::ENABLE, response->Record.GetOpStatus());

        RunState(testEnv, volumeBindingActorID, 0, 1000000, false);

        auto ev = testEnv.GrabBindingRequest();
        UNIT_ASSERT(!ev);


        RunState(testEnv, volumeBindingActorID, 100, 1000000, true);

        ev = testEnv.GrabBindingRequest();
        UNIT_ASSERT(!ev);
    }

    Y_UNIT_TEST(ShouldSetCpuWaitCounter)
    {
        TVolumeBalancerTestEnv testEnv;
        TVolumeBalancerConfigBuilder config;
        config.WithType(NProto::PREEMPTION_MOVE_MOST_HEAVY);

        auto fetcher = std::make_shared<TCgroupStatsFetcherMock>();

        auto volumeBalancerActorID = testEnv.Register(CreateVolumeBalancerActor(
            config,
            fetcher,
            testEnv.GetEdgeActor()));

        testEnv.DispatchEvents();

        testEnv.SendConfigureVolumeBalancerRequest(volumeBalancerActorID, EBalancerStatus::DISABLE);
        auto response = testEnv.GrabConfigureVolumeBalancerResponse();

        auto cpuWait = 0.8 * TDuration::Seconds(15);
        fetcher->SetCpuWaitValue(cpuWait);
        testEnv.GetRuntime().AdvanceCurrentTime(TDuration::Seconds(15));

        testEnv.GrabVolumesStatsRequest();
        testEnv.SendVolumesStatsResponse(volumeBalancerActorID,{});

        testEnv.DispatchEvents(TDuration::Seconds(1));

        auto counter = testEnv.GetRuntime().GetAppData(0).Counters
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "server")
            ->GetCounter("CpuWait", false);

        UNIT_ASSERT_VALUES_UNEQUAL(0, counter->Val());
        UNIT_ASSERT(counter->Val() <= 80);
    }
}

}   // namespace NCloud::NBlockStore::NStorage

template <>
inline void Out<NCloud::NBlockStore::NPrivateProto::EBalancerOpStatus>(
    IOutputStream& out,
    const NCloud::NBlockStore::NPrivateProto::EBalancerOpStatus status)
{
    out << NCloud::NBlockStore::NPrivateProto::EBalancerOpStatus(status);
}
