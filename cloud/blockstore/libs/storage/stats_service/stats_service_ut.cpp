#include <cloud/blockstore/config/features.pb.h>
#include <cloud/blockstore/config/diagnostics.pb.h>

#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/diagnostics/stats_aggregator.h>
#include <cloud/blockstore/libs/diagnostics/config.h>

#include <library/cpp/testing/unittest/registar.h>

#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/api/stats_service.h>
#include <cloud/blockstore/libs/storage/stats_service/stats_service_events_private.h>
#include <cloud/blockstore/libs/storage/stats_service/stats_service.h>

#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/testlib/test_runtime.h>
#include <cloud/blockstore/libs/ydbstats/ydbstats.h>

#include <cloud/blockstore/libs/storage/service/service_events_private.h>

#include <util/generic/size_literals.h>
#include <util/string/printf.h>
#include <util/datetime/base.h>
#include <util/generic/string.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

using namespace NYdbStats;

namespace {

////////////////////////////////////////////////////////////////////////////////

static const TString DefaultDiskId = "path/to/test_volume";

////////////////////////////////////////////////////////////////////////////////

using TYdbStatsCallback =
    std::function<NThreading::TFuture<NProto::TError>(const TVector<TYdbRow>& stats)>;

class TYdbStatsMock:
    public IYdbVolumesStatsUploader
{
private:
    TYdbStatsCallback Callback;

public:
    TYdbStatsMock(TYdbStatsCallback callback)
        : Callback(std::move(callback))
    {}

    virtual ~TYdbStatsMock() = default;

    NThreading::TFuture<NProto::TError> UploadStats(
        const TVector<TYdbRow>& stats,
        const TVector<TYdbBlobLoadMetricRow>&) override
    {
        return Callback(stats);
    }

    void Start() override
    {
    }

    void Stop() override
    {
    }
};

////////////////////////////////////////////////////////////////////////////////

enum EVolumeTestOptions
{
    VOLUME_HASCHECKPOINT = 1,
    VOLUME_HASCLIENTS = 2
};

////////////////////////////////////////////////////////////////////////////////

TDiagnosticsConfigPtr CreateTestDiagnosticsConfig()
{
    return std::make_shared<TDiagnosticsConfig>(NProto::TDiagnosticsConfig());
}

////////////////////////////////////////////////////////////////////////////////

NMonitoring::TDynamicCounters::TCounterPtr GetCounterToCheck(
    NMonitoring::TDynamicCounters& counters)
{
    auto volumeCounters = counters.GetSubgroup("counters", "blockstore")
        ->GetSubgroup("component", "service_volume")
        ->GetSubgroup("host", "cluster")
        ->GetSubgroup("volume", DefaultDiskId);
    return volumeCounters->GetCounter("MixedBytesCount");
}

bool VolumeMetricsExists(NMonitoring::TDynamicCounters& counters)
{
    auto volumeCounters = counters.GetSubgroup("counters", "blockstore")
        ->GetSubgroup("component", "service_volume")
        ->GetSubgroup("host", "cluster");

    return (bool)volumeCounters->FindSubgroup("volume", DefaultDiskId);
}

////////////////////////////////////////////////////////////////////////////////

void UnregisterVolume(TTestActorRuntime& runtime, const TString& diskId)
{
    auto unregisterMsg = std::make_unique<TEvStatsService::TEvUnregisterVolume>(diskId);
    runtime.Send(
        new IEventHandle(
            MakeStorageStatsServiceId(),
            MakeStorageStatsServiceId(),
            unregisterMsg.release(),
            0, // flags
            0),
            0);
}

void RegisterVolume(
    TTestActorRuntime& runtime,
    const TString& diskId,
    NProto::EStorageMediaKind kind,
    bool isSystem)
{
    NProto::TVolume volume;
    volume.SetDiskId(diskId);
    volume.SetStorageMediaKind(kind);
    volume.SetIsSystem(isSystem);
    volume.SetPartitionsCount(1);

    auto registerMsg = std::make_unique<TEvStatsService::TEvRegisterVolume>(
        diskId,
        0,
        std::move(volume));
    runtime.Send(
        new IEventHandle(
            MakeStorageStatsServiceId(),
            MakeStorageStatsServiceId(),
            registerMsg.release(),
            0, // flags
            0),
            0);
}

void RegisterVolume(
    TTestActorRuntime& runtime,
    const TString& diskId)
{
    RegisterVolume(runtime, diskId, NProto::STORAGE_MEDIA_SSD, false);
}

void SendDiskStats(
    TTestActorRuntime& runtime,
    const TString& diskId,
    TPartitionDiskCountersPtr diskCounters,
    TVolumeSelfCountersPtr volumeCounters,
    EVolumeTestOptions volumeOptions,
    ui32 nodeIdx)
{
    auto countersMsg = std::make_unique<TEvStatsService::TEvVolumePartCounters>(
        MakeIntrusive<TCallContext>(),
        diskId,
        std::move(diskCounters),
        0,
        0,
        volumeOptions & EVolumeTestOptions::VOLUME_HASCHECKPOINT,
        NBlobMetrics::TBlobLoadMetrics());

    auto volumeMsg = std::make_unique<TEvStatsService::TEvVolumeSelfCounters>(
        diskId,
        volumeOptions & EVolumeTestOptions::VOLUME_HASCLIENTS,
        false,
        std::move(volumeCounters));

    runtime.Send(
        new IEventHandle(
            MakeStorageStatsServiceId(),
            MakeStorageStatsServiceId(),
            countersMsg.release(),
            0, // flags
            0),
            nodeIdx);

    runtime.Send(
        new IEventHandle(
            MakeStorageStatsServiceId(),
            MakeStorageStatsServiceId(),
            volumeMsg.release(),
            0, // flags
            0),
            nodeIdx);
}

TVector<ui64> BroadcastVolumeCounters(
    TTestActorRuntime& runtime,
    const TVector<ui64>& nodes,
    EVolumeTestOptions volumeOptions
)
{
    TDispatchOptions options;

    for (ui32 i = 0; i < nodes.size(); ++i) {
        auto counters = CreatePartitionDiskCounters(EPublishingPolicy::Repl);
        auto volume = CreateVolumeSelfCounters(EPublishingPolicy::Repl);
        counters->Simple.MixedBytesCount.Set(1);

        SendDiskStats(
            runtime,
            DefaultDiskId,
            std::move(counters),
            std::move(volume),
            volumeOptions,
            0);

        auto updateMsg = std::make_unique<TEvents::TEvWakeup>();
        runtime.Send(
            new IEventHandle(
                MakeStorageStatsServiceId(),
                MakeStorageStatsServiceId(),
                updateMsg.release(),
                0, // flags
                0),
            0);

        options.FinalEvents.emplace_back(NActors::TEvents::TSystem::Wakeup);
    }

    runtime.DispatchEvents(options);

    TVector<ui64> res;
    for (const auto& nodeIdx : nodes) {
        auto counters = runtime.GetAppData(nodeIdx).Counters;
        auto val = GetCounterToCheck(*counters)->Val();
        res.push_back(val);
    }

    return res;
}

void ForceYdbStatsUpdate(
    TTestActorRuntime& runtime,
    const TVector<TString>& volumes,
    ui32 cnt,
    ui32 uploadTriggers)
{
    TDispatchOptions options;

    for (ui32 i = 0; i < volumes.size(); ++i) {
        auto counters = CreatePartitionDiskCounters(EPublishingPolicy::Repl);
        auto volume = CreateVolumeSelfCounters(EPublishingPolicy::Repl);
        counters->Simple.MixedBytesCount.Set(1);

        SendDiskStats(
            runtime,
            volumes[i],
            std::move(counters),
            std::move(volume),
            {},
            0);
    }

    while (uploadTriggers--) {
        auto uploadTrigger = std::make_unique<TEvStatsServicePrivate::TEvUploadDisksStats>();
        runtime.Send(
            new IEventHandle(
                MakeStorageStatsServiceId(),
                MakeStorageStatsServiceId(),
                uploadTrigger.release(),
                0, // flags
                0),
            0);
    }

    if (cnt) {
        options.FinalEvents.clear();
        options.FinalEvents.emplace_back(
            TEvStatsServicePrivate::EvUploadDisksStatsCompleted,
            cnt);

        runtime.DispatchEvents(options);
    }
}

////////////////////////////////////////////////////////////////////////////////

struct TTestEnv
{
    TTestActorRuntime& Runtime;

    TTestEnv(
            TTestActorRuntime& runtime,
            NProto::TStorageServiceConfig storageConfig,
            NYdbStats::IYdbVolumesStatsUploaderPtr ydbStatsUpdater)
        : Runtime(runtime)
    {
        SetupLogging();

        auto config = std::make_shared<TStorageConfig>(
            std::move(storageConfig),
            std::make_shared<TFeaturesConfig>(NProto::TFeaturesConfig())
        );

        SetupTabletServices(Runtime);

        auto storageStatsService = CreateStorageStatsService(
            std::move(config),
            CreateTestDiagnosticsConfig(),
            std::move(ydbStatsUpdater),
            CreateStatsAggregatorStub());

        auto storageStatsServiceId = Runtime.Register(
            storageStatsService.release(),
            0);

        Runtime.RegisterService(
            MakeStorageStatsServiceId(),
            storageStatsServiceId,
            0);

        Runtime.EnableScheduleForActor(storageStatsServiceId);
    }

    explicit TTestEnv(TTestActorRuntime& runtime)
        : TTestEnv(runtime, {}, NYdbStats::CreateVolumesStatsUploaderStub())
    {}

    void SetupLogging()
    {
        Runtime.AppendToLogSettings(
            TBlockStoreComponents::START,
            TBlockStoreComponents::END,
            GetComponentName);

        // for (ui32 i = TBlockStoreComponents::START; i < TBlockStoreComponents::END; ++i) {
        //   Runtime.SetLogPriority(i, NLog::PRI_DEBUG);
        // }
        // Runtime.SetLogPriority(NLog::InvalidComponent, NLog::PRI_DEBUG);
    }
};

////////////////////////////////////////////////////////////////////////////////

class TStatsServiceClient
{
private:
    NKikimr::TTestActorRuntime& Runtime;
    ui32 NodeIdx;
    NActors::TActorId Sender;

public:
    TStatsServiceClient(
            NKikimr::TTestActorRuntime& runtime,
            ui32 nodeIdx = 0)
        : Runtime(runtime)
        , NodeIdx(nodeIdx)
        , Sender(runtime.AllocateEdgeActor(nodeIdx))
    {}

    const NActors::TActorId& GetSender() const
    {
        return Sender;
    }

    template <typename TRequest>
    void SendRequest(
        const NActors::TActorId& recipient,
        std::unique_ptr<TRequest> request)
    {
        auto* ev = new NActors::IEventHandle(
            recipient,
            Sender,
            request.release(),
            0,          // flags
            0,          // cookie
            nullptr,    // forwardOnNondelivery
            NWilson::TTraceId::NewTraceId());

        Runtime.Send(ev, NodeIdx);
    }

    template <typename TResponse>
    std::unique_ptr<TResponse> RecvResponse()
    {
        TAutoPtr<NActors::IEventHandle> handle;
        Runtime.GrabEdgeEventRethrow<TResponse>(handle);
        return std::unique_ptr<TResponse>(handle->Release<TResponse>().Release());
    }

    std::unique_ptr<TEvService::TEvUploadClientMetricsRequest> CreateUploadClientMetricsRequest()
    {
        return std::make_unique<TEvService::TEvUploadClientMetricsRequest>();
    }

    std::unique_ptr<TEvStatsService::TEvGetVolumeStatsRequest> CreateGetVolumeStatsRequest()
    {
        return std::make_unique<TEvStatsService::TEvGetVolumeStatsRequest>();
    }

#define BLOCKSTORE_DECLARE_METHOD(name, ns)                                    \
    template <typename... Args>                                                \
    void Send##name##Request(Args&&... args)                                   \
    {                                                                          \
        auto request = Create##name##Request(std::forward<Args>(args)...);     \
        SendRequest(MakeStorageStatsServiceId(), std::move(request));          \
    }                                                                          \
                                                                               \
    std::unique_ptr<ns::TEv##name##Response> Recv##name##Response()            \
    {                                                                          \
        return RecvResponse<ns::TEv##name##Response>();                        \
    }                                                                          \
                                                                               \
    template <typename... Args>                                                \
    std::unique_ptr<ns::TEv##name##Response> name(Args&&... args)              \
    {                                                                          \
        auto request = Create##name##Request(std::forward<Args>(args)...);     \
        SendRequest(MakeStorageStatsServiceId(), std::move(request));          \
                                                                               \
        auto response = RecvResponse<ns::TEv##name##Response>();               \
        UNIT_ASSERT_C(                                                         \
            SUCCEEDED(response->GetStatus()),                                  \
            response->GetErrorReason());                                       \
        return response;                                                       \
    }                                                                          \
// BLOCKSTORE_DECLARE_METHOD

    BLOCKSTORE_DECLARE_METHOD(UploadClientMetrics, TEvService)
    BLOCKSTORE_DECLARE_METHOD(GetVolumeStats, TEvStatsService)

#undef BLOCKSTORE_DECLARE_METHOD
};

} // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TServiceVolumeStatsTest)
{
    Y_UNIT_TEST(ShouldNotReportSolomonMetricsIfNotMounted)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        RegisterVolume(runtime, DefaultDiskId);
        auto counters = BroadcastVolumeCounters(runtime, {0}, {});
        UNIT_ASSERT(counters[0]== 0);
    }

    Y_UNIT_TEST(ShouldReportSolomonMetricsIfVolumeRunsLocallyAndMounted)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        RegisterVolume(runtime, DefaultDiskId);
        auto counters = BroadcastVolumeCounters(runtime, {0}, EVolumeTestOptions::VOLUME_HASCLIENTS);
        UNIT_ASSERT(counters[0]== 1);
    }

    Y_UNIT_TEST(ShouldStopReportSolomonMetricsIfIsMoved)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        RegisterVolume(runtime, DefaultDiskId);
        auto c1 = BroadcastVolumeCounters(runtime, {0}, EVolumeTestOptions::VOLUME_HASCLIENTS);
        UNIT_ASSERT(c1[0]== 1);

        UnregisterVolume(runtime, DefaultDiskId);

        auto counters = CreatePartitionDiskCounters(EPublishingPolicy::Repl);
        auto volume = CreateVolumeSelfCounters(EPublishingPolicy::Repl);
        counters->Simple.MixedBytesCount.Set(1);

        SendDiskStats(
            runtime,
            DefaultDiskId,
            std::move(counters),
            std::move(volume),
            EVolumeTestOptions::VOLUME_HASCLIENTS,
            0);

        auto updateMsg = std::make_unique<TEvents::TEvWakeup>();
        runtime.Send(
            new IEventHandle(
                MakeStorageStatsServiceId(),
                MakeStorageStatsServiceId(),
                updateMsg.release(),
                0, // flags
                0),
            0);

        TDispatchOptions options;
        options.FinalEvents.emplace_back(NActors::TEvents::TSystem::Wakeup);
        runtime.DispatchEvents(options);

        UNIT_ASSERT_VALUES_EQUAL(false, VolumeMetricsExists(*runtime.GetAppData(0).Counters));
    }

    Y_UNIT_TEST(ShouldReportSolomonMetricsIfVolumeRunsLocallyAndHasCheckpoint)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        RegisterVolume(runtime, DefaultDiskId);
        auto counters = BroadcastVolumeCounters(runtime, {0}, EVolumeTestOptions::VOLUME_HASCHECKPOINT);
        UNIT_ASSERT(counters[0] == 1);
    }

    Y_UNIT_TEST(ShouldReportMaximumsForCompactionScore)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        TDispatchOptions options;

        RegisterVolume(runtime, "vol0");
        RegisterVolume(runtime, "vol1");

        auto counters1 = CreatePartitionDiskCounters(EPublishingPolicy::Repl);
        counters1->Simple.CompactionScore.Set(1);
        SendDiskStats(
            runtime,
            "vol0",
            std::move(counters1),
            CreateVolumeSelfCounters(EPublishingPolicy::Repl),
            EVolumeTestOptions::VOLUME_HASCLIENTS,
            0);

        auto counters2 = CreatePartitionDiskCounters(EPublishingPolicy::Repl);
        counters2->Simple.CompactionScore.Set(3);
        SendDiskStats(
            runtime,
            "vol1",
            std::move(counters2),
            CreateVolumeSelfCounters(EPublishingPolicy::Repl),
            EVolumeTestOptions::VOLUME_HASCLIENTS,
            0);

        auto updateMsg = std::make_unique<TEvents::TEvWakeup>();
        runtime.Send(
            new IEventHandle(
                MakeStorageStatsServiceId(),
                MakeStorageStatsServiceId(),
                updateMsg.release(),
                0, // flags
                0),
            0);

        options.FinalEvents.emplace_back(NActors::TEvents::TSystem::Wakeup);

        runtime.DispatchEvents(options);

        auto counter = runtime.GetAppData(0).Counters
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "service")
            ->GetCounter("CompactionScore");

        UNIT_ASSERT(*counter == 3);
    }

    Y_UNIT_TEST(ShouldReportBytesCountForSSDSystemVolumes)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        RegisterVolume(runtime, DefaultDiskId, NProto::STORAGE_MEDIA_SSD, true);

        auto counters = CreatePartitionDiskCounters(EPublishingPolicy::Repl);
        counters->Simple.BytesCount.Set(100500);
        SendDiskStats(
            runtime,
            DefaultDiskId,
            std::move(counters),
            CreateVolumeSelfCounters(EPublishingPolicy::Repl),
            EVolumeTestOptions::VOLUME_HASCLIENTS,
            0);
        auto updateMsg = std::make_unique<TEvents::TEvWakeup>();
        runtime.Send(
            new IEventHandle(
                MakeStorageStatsServiceId(),
                MakeStorageStatsServiceId(),
                updateMsg.release(),
                0, // flags
                0),
            0);

        TDispatchOptions options;
        options.FinalEvents.emplace_back(NActors::TEvents::TSystem::Wakeup);
        runtime.DispatchEvents(options);

        // should report "ssd_system" metrics.
        ui64 actual = *runtime.GetAppData(0).Counters
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "service")
            ->GetSubgroup("type", "ssd_system")
            ->GetCounter("BytesCount");
        UNIT_ASSERT_VALUES_EQUAL(100500, actual);

        // should not report "ssd" metrics.
        actual = *runtime.GetAppData(0).Counters
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "service")
            ->GetSubgroup("type", "ssd")
            ->GetCounter("BytesCount");
        UNIT_ASSERT_VALUES_EQUAL(0, actual);
    }

    Y_UNIT_TEST(ShouldReportDiskCountAndPartitionCount)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        for (const auto& diskId: {"disk-1", "disk-2", "disk-3"}) {
            RegisterVolume(runtime, diskId);
            SendDiskStats(
                runtime,
                diskId,
                CreatePartitionDiskCounters(EPublishingPolicy::Repl),
                CreateVolumeSelfCounters(EPublishingPolicy::Repl),
                EVolumeTestOptions::VOLUME_HASCLIENTS,
                0);
        }

        auto updateMsg = std::make_unique<TEvents::TEvWakeup>();
        runtime.Send(
            new IEventHandle(
                MakeStorageStatsServiceId(),
                MakeStorageStatsServiceId(),
                updateMsg.release(),
                0, // flags
                0),
            0);

        TDispatchOptions options;
        options.FinalEvents.emplace_back(NActors::TEvents::TSystem::Wakeup);
        runtime.DispatchEvents(options);

        auto ssd = runtime.GetAppData(0).Counters
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "service")
            ->GetSubgroup("type", "ssd");

        UNIT_ASSERT_VALUES_EQUAL(3, ssd->GetCounter("TotalDiskCount")->Val());
        UNIT_ASSERT_VALUES_EQUAL(3, ssd->GetCounter("TotalPartitionCount")->Val());

        auto totalCounters = runtime.GetAppData(0).Counters
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "service");

        UNIT_ASSERT_VALUES_EQUAL(3, totalCounters->GetCounter("TotalDiskCount")->Val());
        UNIT_ASSERT_VALUES_EQUAL(3, totalCounters->GetCounter("TotalPartitionCount")->Val());
    }

    Y_UNIT_TEST(ShouldReportYdbStatsInBatches)
    {
        auto callback = [] (const TVector<TYdbRow>& stats)
        {
            Y_UNUSED(stats);
            return NThreading::MakeFuture(MakeError(S_OK));
        };

        IYdbVolumesStatsUploaderPtr ydbStats = std::make_shared<TYdbStatsMock>(callback);

        NProto::TStorageServiceConfig storageServiceConfig;
        storageServiceConfig.SetStatsUploadDiskCount(1);
        storageServiceConfig.SetStatsUploadInterval(TDuration::Seconds(300).MilliSeconds());
        storageServiceConfig.SetStatsUploadRetryTimeout(TDuration::Seconds(20).MilliSeconds());

        TTestBasicRuntime runtime;
        TTestEnv env(runtime, std::move(storageServiceConfig), std::move(ydbStats));

        RegisterVolume(runtime, "disk1");
        RegisterVolume(runtime, "disk2");
        ForceYdbStatsUpdate(runtime, {"disk1", "disk2"}, 2, 2);
    }

    Y_UNIT_TEST(ShouldRetryStatsUploadInCaseOfFailure)
    {
        ui32 attemptCount = 0;
        auto callback = [&] (const TVector<TYdbRow>& stats)
        {
            UNIT_ASSERT_VALUES_EQUAL(1, stats.size());

            if (++attemptCount == 1) {
                return NThreading::MakeFuture(MakeError(E_REJECTED));
            }
            return NThreading::MakeFuture(MakeError(S_OK));
        };

        IYdbVolumesStatsUploaderPtr ydbStats = std::make_shared<TYdbStatsMock>(callback);

        NProto::TStorageServiceConfig storageServiceConfig;
        storageServiceConfig.SetStatsUploadDiskCount(1);
        storageServiceConfig.SetStatsUploadInterval(TDuration::Seconds(300).MilliSeconds());
        storageServiceConfig.SetStatsUploadRetryTimeout(TDuration::MilliSeconds(1).MilliSeconds());

        TTestBasicRuntime runtime;
        TTestEnv env(runtime, std::move(storageServiceConfig), std::move(ydbStats));

        RegisterVolume(runtime, "disk1");
        RegisterVolume(runtime, "disk2");
        ForceYdbStatsUpdate(runtime, {"disk1", "disk2"}, 3, 1);

        UNIT_ASSERT_VALUES_EQUAL(3, attemptCount);
    }

    Y_UNIT_TEST(ShouldForgetTooOldStats)
    {
        bool failUpload = true;
        ui32 callCnt = 0;

        auto callback = [&] (const TVector<TYdbRow>& stats)
        {
            UNIT_ASSERT_VALUES_EQUAL(1, stats.size());

            if (failUpload) {
                return NThreading::MakeFuture(MakeError(E_REJECTED));
            } else {
                ++callCnt;
                return NThreading::MakeFuture(MakeError(S_OK));
            }
        };

        IYdbVolumesStatsUploaderPtr ydbStats = std::make_shared<TYdbStatsMock>(callback);

        NProto::TStorageServiceConfig storageServiceConfig;
        storageServiceConfig.SetStatsUploadDiskCount(1);
        storageServiceConfig.SetStatsUploadInterval(TDuration::Seconds(2).MilliSeconds());
        storageServiceConfig.SetStatsUploadRetryTimeout(TDuration::MilliSeconds(99).MilliSeconds());

        TTestBasicRuntime runtime;
        TTestEnv env(runtime, std::move(storageServiceConfig), std::move(ydbStats));

        RegisterVolume(runtime, "disk1");
        RegisterVolume(runtime, "disk2");
        ForceYdbStatsUpdate(runtime, {"disk1", "disk2"}, 2, 0);

        {
            TDispatchOptions options;
            options.FinalEvents.emplace_back(TEvStatsServicePrivate::EvUploadDisksStats);
            runtime.DispatchEvents(options);
        }

        failUpload = false;

        TDispatchOptions options;
        options.FinalEvents.emplace_back(TEvStatsServicePrivate::EvUploadDisksStatsCompleted, 2);
        runtime.DispatchEvents(options);

        UNIT_ASSERT_VALUES_EQUAL(2, callCnt);
    }

    Y_UNIT_TEST(ShouldCorrectlyPrepareYdbStatsRequests)
    {
        TVector<TVector<TString>> batches;
        auto callback = [&] (const TVector<TYdbRow>& stats)
        {
            TVector<TString> batch;
            for (const auto& x: stats) {
                batch.push_back(x.DiskId);
            }

            batches.push_back(std::move(batch));

            return NThreading::MakeFuture(MakeError(S_OK));
        };

        IYdbVolumesStatsUploaderPtr ydbStats = std::make_shared<TYdbStatsMock>(callback);

        NProto::TStorageServiceConfig storageServiceConfig;
        storageServiceConfig.SetStatsUploadDiskCount(2);
        storageServiceConfig.SetStatsUploadInterval(TDuration::Seconds(300).MilliSeconds());
        storageServiceConfig.SetStatsUploadRetryTimeout(TDuration::Seconds(20).MilliSeconds());

        TTestBasicRuntime runtime;
        TTestEnv env(runtime, std::move(storageServiceConfig), std::move(ydbStats));

        TVector<TString> diskIds;
        for (ui32 i = 0; i < 5; ++i) {
            auto diskId = Sprintf("disk%u", i);
            diskIds.push_back(diskId);
            RegisterVolume(runtime, diskId);
        }

        ForceYdbStatsUpdate(runtime, diskIds, 1, 3);

        UNIT_ASSERT_VALUES_EQUAL(3, batches.size());
        UNIT_ASSERT_VALUES_EQUAL(2, batches[0].size());
        UNIT_ASSERT_VALUES_EQUAL(2, batches[1].size());
        UNIT_ASSERT_VALUES_EQUAL(1, batches[2].size());

        TVector<TString> observedDiskIds;
        for (const auto& batch: batches) {
            for (const auto& x: batch) {
                observedDiskIds.push_back(x);
            }
        }

        Sort(observedDiskIds);

        UNIT_ASSERT_VALUES_EQUAL(diskIds, observedDiskIds);
    }

    Y_UNIT_TEST(ShouldNotTryToPushStatsIfNothingToReportToYDB)
    {
        TVector<TVector<TString>> batches;
        bool uploadSeen = false;
        auto callback = [&] (const TVector<TYdbRow>& stats)
        {
            Y_UNUSED(stats);
            uploadSeen = true;
            return NThreading::MakeFuture(MakeError(S_OK));
        };

        IYdbVolumesStatsUploaderPtr ydbStats = std::make_shared<TYdbStatsMock>(callback);

        NProto::TStorageServiceConfig storageServiceConfig;
        storageServiceConfig.SetStatsUploadDiskCount(2);
        storageServiceConfig.SetStatsUploadInterval(TDuration::Seconds(300).MilliSeconds());
        storageServiceConfig.SetStatsUploadRetryTimeout(TDuration::Seconds(20).MilliSeconds());

        TTestBasicRuntime runtime;
        TTestEnv env(runtime, std::move(storageServiceConfig), std::move(ydbStats));

        ForceYdbStatsUpdate(runtime, {}, 0, 1);

        runtime.DispatchEvents({}, TDuration::Seconds(1));

        UNIT_ASSERT_VALUES_EQUAL(false, uploadSeen);
    }

    Y_UNIT_TEST(ShouldAcceptAndReplyToClientMetrics)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);
        TStatsServiceClient client(runtime);

        client.UploadClientMetrics();
    }

    Y_UNIT_TEST(ShouldReportReadWriteZeroCountersForNonreplDisks)
    {
        TTestBasicRuntime runtime;
        TTestEnv env(runtime);

        RegisterVolume(runtime, "non_repl", NProto::STORAGE_MEDIA_SSD_NONREPLICATED, true);

        auto counters = CreatePartitionDiskCounters(EPublishingPolicy::NonRepl);
        counters->RequestCounters.ReadBlocks.Count = 42;
        counters->RequestCounters.ReadBlocks.RequestBytes = 100500;
        SendDiskStats(
            runtime,
            "non_repl",
            std::move(counters),
            CreateVolumeSelfCounters(EPublishingPolicy::NonRepl),
            EVolumeTestOptions::VOLUME_HASCLIENTS,
            0);
        auto updateMsg = std::make_unique<TEvents::TEvWakeup>();
        runtime.Send(
            new IEventHandle(
                MakeStorageStatsServiceId(),
                MakeStorageStatsServiceId(),
                updateMsg.release(),
                0, // flags
                0),
            0);

        TDispatchOptions options;
        options.FinalEvents.emplace_back(NActors::TEvents::TSystem::Wakeup);
        runtime.DispatchEvents(options);

        {
            ui64 actual = *runtime.GetAppData(0).Counters
                ->GetSubgroup("counters", "blockstore")
                ->GetSubgroup("component", "service_volume")
                ->GetSubgroup("host", "cluster")
                ->GetSubgroup("volume", "non_repl")
                ->GetSubgroup("request", "ReadBlocks")
                ->GetCounter("Count");
            UNIT_ASSERT_VALUES_EQUAL(42, actual);
        }

        {
            ui64 actual = *runtime.GetAppData(0).Counters
                ->GetSubgroup("counters", "blockstore")
                ->GetSubgroup("component", "service_volume")
                ->GetSubgroup("host", "cluster")
                ->GetSubgroup("volume", "non_repl")
                ->GetSubgroup("request", "ReadBlocks")
                ->GetCounter("RequestBytes");
            UNIT_ASSERT_VALUES_EQUAL(100500, actual);
        }
    }
}

}   // namespace NCloud::NBlockStore::NStorage
