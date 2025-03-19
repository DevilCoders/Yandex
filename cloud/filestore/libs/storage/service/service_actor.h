#pragma once

#include "public.h"

#include "service_private.h"
#include "service_state.h"

#include <cloud/filestore/libs/diagnostics/request_stats.h>
#include <cloud/filestore/libs/service/error.h>
#include <cloud/filestore/libs/service/request.h>
#include <cloud/filestore/libs/storage/api/service.h>
#include <cloud/filestore/libs/storage/core/config.h>
#include <cloud/filestore/libs/storage/core/request_info.h>
#include <cloud/filestore/libs/storage/core/utils.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>
#include <library/cpp/actors/core/events.h>
#include <library/cpp/actors/core/hfunc.h>
#include <library/cpp/actors/core/log.h>
#include <library/cpp/actors/core/mon.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

class TStorageServiceActor final
    : public NActors::TActorBootstrapped<TStorageServiceActor>
{
private:
    const TStorageConfigPtr StorageConfig;

    std::unique_ptr<TStorageServiceState> State;
    IRequestStatsRegistryPtr StatsRegistry;

public:
    TStorageServiceActor(
        TStorageConfigPtr storageConfig,
        IRequestStatsRegistryPtr statsRegistry);
    ~TStorageServiceActor();

    void Bootstrap(const NActors::TActorContext& ctx);

private:
    void RegisterPages(const NActors::TActorContext& ctx);
    void ScheduleUpdateStats(const NActors::TActorContext& ctx);

    void HandleHttpInfo(
        const NActors::NMon::TEvHttpInfo::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleUpdateStats(
        const TEvServicePrivate::TEvUpdateStats::TPtr& ev,
        const NActors::TActorContext& ctx);

    template <typename TMethod>
    void ForwardRequest(
        const NActors::TActorContext& ctx,
        const typename TMethod::TRequest::TPtr& ev);

    bool HandleRequests(STFUNC_SIG);

    FILESTORE_SERVICE(FILESTORE_IMPLEMENT_REQUEST, TEvService)
    FILESTORE_SERVICE_REQUESTS(FILESTORE_IMPLEMENT_REQUEST, TEvService)
    FILESTORE_SERVICE_REQUESTS_PRIVATE(FILESTORE_IMPLEMENT_REQUEST, TEvServicePrivate)

    STFUNC(StateWork);

    void HandleSessionCreated(
        const TEvServicePrivate::TEvSessionCreated::TPtr& ev,
        const NActors::TActorContext& ctx);
    void HandleSessionDestroyed(
        const TEvServicePrivate::TEvSessionDestroyed::TPtr& ev,
        const NActors::TActorContext& ctx);

    void RemoveSession(TSessionInfo* session, const NActors::TActorContext& ctx);
};

}   // namespace NCloud::NFileStore::NStorage
