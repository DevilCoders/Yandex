#include "service_actor.h"

#include <ydb/core/base/appdata.h>
#include <ydb/core/mon/mon.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;

////////////////////////////////////////////////////////////////////////////////

TStorageServiceActor::TStorageServiceActor(
        TStorageConfigPtr storageConfig,
        IRequestStatsRegistryPtr statsRegistry)
    : StorageConfig{std::move(storageConfig)}
    , State{std::make_unique<TStorageServiceState>()}
    , StatsRegistry{std::move(statsRegistry)}
{
    ActivityType = TFileStoreActivities::SERVICE;
}

TStorageServiceActor::~TStorageServiceActor()
{
}

void TStorageServiceActor::Bootstrap(const TActorContext& ctx)
{
    Become(&TThis::StateWork);

    RegisterPages(ctx);
    ScheduleUpdateStats(ctx);
}

void TStorageServiceActor::RegisterPages(const NActors::TActorContext& ctx)
{
    auto mon = AppData(ctx)->Mon;
    if (mon) {
        auto* rootPage = mon->RegisterIndexPage("filestore", "FileStore");

        mon->RegisterActorPage(rootPage, "service", "Service",
            false, ctx.ExecutorThread.ActorSystem, SelfId());
    }
}

void TStorageServiceActor::ScheduleUpdateStats(const NActors::TActorContext& ctx)
{
    ctx.Schedule(
        UpdateStatsInterval,
        new TEvServicePrivate::TEvUpdateStats{});
}

////////////////////////////////////////////////////////////////////////////////

bool TStorageServiceActor::HandleRequests(STFUNC_SIG)
{
    switch (ev->GetTypeRewrite()) {
        FILESTORE_SERVICE(FILESTORE_HANDLE_REQUEST, TEvService)
        FILESTORE_SERVICE_REQUESTS(FILESTORE_HANDLE_REQUEST, TEvService)
        FILESTORE_SERVICE_REQUESTS_PRIVATE(FILESTORE_HANDLE_REQUEST, TEvServicePrivate)

        HFunc(NMon::TEvHttpInfo, HandleHttpInfo);
        HFunc(TEvServicePrivate::TEvSessionCreated, HandleSessionCreated);
        HFunc(TEvServicePrivate::TEvSessionDestroyed, HandleSessionDestroyed);
        HFunc(TEvServicePrivate::TEvUpdateStats, HandleUpdateStats);

        default:
            return false;
    }

    return true;
}

STFUNC(TStorageServiceActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        default:
            if (!HandleRequests(ev, ctx)) {
                HandleUnexpectedEvent(ctx, ev, TFileStoreComponents::SERVICE);
            }
            break;
    }
}

}   // namespace NCloud::NFileStore::NStorage
