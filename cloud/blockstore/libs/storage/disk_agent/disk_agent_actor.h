#pragma once

#include "public.h"

#include "config.h"
#include "disk_agent_counters.h"
#include "disk_agent_private.h"
#include "disk_agent_state.h"

#include <cloud/blockstore/config/disk.pb.h>

#include <cloud/blockstore/libs/kikimr/helpers.h>
#include <cloud/blockstore/libs/kikimr/trace.h>
#include <cloud/blockstore/libs/spdk/env.h>
#include <cloud/blockstore/libs/storage/api/disk_agent.h>
#include <cloud/blockstore/libs/storage/api/disk_registry.h>
#include <cloud/blockstore/libs/storage/api/disk_registry_proxy.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/pending_request.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>
#include <library/cpp/actors/core/events.h>
#include <library/cpp/actors/core/hfunc.h>
#include <library/cpp/actors/core/log.h>
#include <library/cpp/actors/core/mon.h>

#include <util/generic/deque.h>
#include <util/generic/hash.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

class TDiskAgentActor final
    : public NActors::TActorBootstrapped<TDiskAgentActor>
{
private:
    const TStorageConfigPtr Config;
    const TDiskAgentConfigPtr AgentConfig;
    const NSpdk::ISpdkEnvPtr Spdk;
    const ICachingAllocatorPtr Allocator;
    const IStorageProviderPtr StorageProvider;
    const IProfileLogPtr ProfileLog;
    const IBlockDigestGeneratorPtr BlockDigestGenerator;

    ILoggingServicePtr Logging;
    NRdma::IServerPtr RdmaServer;

    std::unique_ptr<TDiskAgentState> State;
    std::unique_ptr<NKikimr::TTabletCountersBase> Counters;

    // Pending WaitReady requests
    TDeque<TPendingRequest> PendingRequests;

    bool RegistrationInProgress = false;

    NActors::TActorId StatsActor;

    THashMap<TString, TDeque<TRequestInfoPtr>> SecureErasePendingRequests;

public:
    TDiskAgentActor(
        TStorageConfigPtr config,
        TDiskAgentConfigPtr agentConfig,
        NSpdk::ISpdkEnvPtr spdk,
        ICachingAllocatorPtr allocator,
        IStorageProviderPtr storageProvider,
        IProfileLogPtr profileLog,
        IBlockDigestGeneratorPtr blockDigestGenerator,
        ILoggingServicePtr logging,
        NRdma::IServerPtr rdmaServer);

    ~TDiskAgentActor();

    void Bootstrap(const NActors::TActorContext& ctx);

private:
    void RegisterPages(const NActors::TActorContext& ctx);
    void RegisterCounters(const NActors::TActorContext& ctx);

    void ScheduleCountersUpdate(const NActors::TActorContext& ctx);
    void UpdateCounters(const NActors::TActorContext& ctx);

    void UpdateActorStats(const NActors::TActorContext& ctx);
    void UpdateActorStatsSampled(const NActors::TActorContext& ctx)
    {
        static constexpr int SampleRate = 128;
        if (Y_UNLIKELY(GetHandledEvents() % SampleRate == 0)) {
            UpdateActorStats(ctx);
        }
    }

    void InitAgent(const NActors::TActorContext& ctx);
    void InitLocalStorageProvider(const NActors::TActorContext& ctx);

    void ScheduleUpdateStats(const NActors::TActorContext& ctx);

    void SendRegisterRequest(const NActors::TActorContext& ctx);

    template <typename TMethod, typename TOp>
    void PerformIO(
        const NActors::TActorContext& ctx,
        const typename TMethod::TRequest::TPtr& ev,
        TOp operation);

    void RenderDevices(IOutputStream& out) const;

    void SecureErase(
        const NActors::TActorContext& ctx,
        const TString& deviceId);

private:
    STFUNC(StateInit);
    STFUNC(StateWork);

    void HandlePoisonPill(
        const NActors::TEvents::TEvPoisonPill::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleWakeup(
        const NActors::TEvents::TEvWakeup::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleHttpInfo(
        const NActors::NMon::TEvHttpInfo::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleInitAgentCompleted(
        const TEvDiskAgentPrivate::TEvInitAgentCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleSecureEraseCompleted(
        const TEvDiskAgentPrivate::TEvSecureEraseCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleRegisterAgentResponse(
        const TEvDiskAgentPrivate::TEvRegisterAgentResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleSubscribeResponse(
        const TEvDiskRegistryProxy::TEvSubscribeResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleConnectionLost(
        const TEvDiskRegistryProxy::TEvConnectionLost::TPtr& ev,
        const NActors::TActorContext& ctx);

    bool HandleRequests(STFUNC_SIG);

    BLOCKSTORE_DISK_AGENT_REQUESTS(BLOCKSTORE_IMPLEMENT_REQUEST, TEvDiskAgent)
    BLOCKSTORE_DISK_AGENT_REQUESTS_PRIVATE(BLOCKSTORE_IMPLEMENT_REQUEST, TEvDiskAgentPrivate)
};

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_DISK_AGENT_COUNTER(name)                                    \
    if (Counters) {                                                            \
        auto& counter = Counters->Cumulative()                                 \
            [TDiskAgentCounters::CUMULATIVE_COUNTER_Request_##name];           \
        counter.Increment(1);                                                  \
    }                                                                          \
// BLOCKSTORE_DISK_AGENT_COUNTER

}   // namespace NCloud::NBlockStore::NStorage
