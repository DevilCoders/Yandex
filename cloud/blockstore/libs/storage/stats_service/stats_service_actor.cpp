#include "stats_service_actor.h"

#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/storage/api/user_stats.h>

#include <ydb/core/base/appdata.h>
#include <ydb/core/mon/mon.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

////////////////////////////////////////////////////////////////////////////////

TStatsServiceActor::TStatsServiceActor(
        TStorageConfigPtr config,
        TDiagnosticsConfigPtr diagnosticsConfig,
        NYdbStats::IYdbVolumesStatsUploaderPtr uploader,
        IStatsAggregatorPtr clientStatsAggregator)
    : Config(std::move(config))
    , DiagnosticsConfig(std::move(diagnosticsConfig))
    , StatsUploader(std::move(uploader))
    , ClientStatsAggregator(std::move(clientStatsAggregator))
    , State(*Config)
    , UserCounters(std::make_shared<NUserCounter::TUserCounterSupplier>())
{
    ActivityType = TBlockStoreActivities::STATS_SERVICE;
}

TStatsServiceActor::~TStatsServiceActor()
{}

void TStatsServiceActor::Bootstrap(const TActorContext& ctx)
{
    Become(&TThis::StateWork);

    LOG_WARN(ctx, TBlockStoreComponents::STATS_SERVICE,
        "Stats service running");

    RegisterCounters(ctx);
}

void TStatsServiceActor::RegisterCounters(const TActorContext& ctx)
{
    auto counters = AppData(ctx)->Counters;
    auto rootGroup = counters->GetSubgroup("counters", "blockstore");
    auto totalCounters = rootGroup->GetSubgroup("component", "service");
    auto hddCounters = totalCounters->GetSubgroup("type", "hdd");
    auto ssdCounters = totalCounters->GetSubgroup("type", "ssd");
    auto ssdNonreplCounters = totalCounters->GetSubgroup("type", "ssd_nonrepl");
    auto ssdSystemCounters = totalCounters->GetSubgroup("type", "ssd_system");
    auto localCounters = totalCounters->GetSubgroup("binding", "local");
    auto nonlocalCounters = totalCounters->GetSubgroup("binding", "remote");

    State.GetTotalCounters().Register(totalCounters);
    State.GetHddCounters().Register(hddCounters);
    State.GetSsdCounters().Register(ssdCounters);
    State.GetSsdNonreplCounters().Register(ssdNonreplCounters);
    State.GetSsdSystemCounters().Register(ssdSystemCounters);
    State.GetLocalVolumesCounters().Register(localCounters);
    State.GetNonlocalVolumesCounters().Register(nonlocalCounters);
    State.GetSsdBlobCounters().Register(ssdCounters);
    State.GetHddBlobCounters().Register(hddCounters);

    YDbFailedRequests = totalCounters->GetCounter("Ydb/FailedRequests", true);

    UpdateVolumeSelfCounters(ctx);

    ScheduleCountersUpdate(ctx);
    ScheduleStatsUpload(ctx);

    auto request = std::make_unique<TEvUserStats::TEvUserStatsProviderCreate>(
        UserCounters
    );

    NCloud::Send(ctx, MakeStorageUserStatsId(), std::move(request));
}

void TStatsServiceActor::ScheduleCountersUpdate(const TActorContext& ctx)
{
    ctx.Schedule(UpdateCountersInterval, new TEvents::TEvWakeup());
}

////////////////////////////////////////////////////////////////////////////////

void TStatsServiceActor::HandleWakeup(
    const TEvents::TEvWakeup::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    UpdateVolumeSelfCounters(ctx);
    ScheduleCountersUpdate(ctx);
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TStatsServiceActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvWakeup, HandleWakeup);

        HFunc(TEvService::TEvUploadClientMetricsRequest, HandleUploadClientMetrics);

        HFunc(TEvStatsService::TEvRegisterVolume, HandleRegisterVolume);
        HFunc(TEvStatsService::TEvUnregisterVolume, HandleUnregisterVolume);
        HFunc(TEvStatsService::TEvVolumeConfigUpdated, HandleVolumeConfigUpdated);
        HFunc(TEvStatsService::TEvVolumePartCounters, HandleVolumePartCounters);
        HFunc(TEvStatsService::TEvVolumeSelfCounters, HandleVolumeSelfCounters);
        HFunc(TEvStatsService::TEvGetVolumeStatsRequest, HandleGetVolumeStats);

        HFunc(TEvStatsServicePrivate::TEvUploadDisksStats, HandleUploadDisksStats);
        HFunc(TEvStatsServicePrivate::TEvUploadDisksStatsCompleted, HandleUploadDisksStatsCompleted);

        HFunc(TEvStatsServicePrivate::TEvStatsUploadRetryTimeout, HandleStatsUploadRetryTimeout);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::STATS_SERVICE);
            break;
    }
}

}   // namespace NCloud::NBlockStore::NStorage
