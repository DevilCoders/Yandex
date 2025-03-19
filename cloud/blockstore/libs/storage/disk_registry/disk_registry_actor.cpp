#include "disk_registry_actor.h"

#include <cloud/blockstore/libs/diagnostics/critical_events.h>
#include <cloud/blockstore/libs/storage/api/disk_agent.h>

#include <ydb/core/base/appdata.h>
#include <ydb/core/base/tablet_pipe.h>
#include <ydb/core/mon/mon.h>
#include <ydb/core/node_whiteboard/node_whiteboard.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

////////////////////////////////////////////////////////////////////////////////

const TDiskRegistryActor::TStateInfo TDiskRegistryActor::States[STATE_MAX] = {
    { "Boot",    (IActor::TReceiveFunc)&TDiskRegistryActor::StateBoot    },
    { "Init",    (IActor::TReceiveFunc)&TDiskRegistryActor::StateInit    },
    { "Work",    (IActor::TReceiveFunc)&TDiskRegistryActor::StateWork    },
    { "Restore", (IActor::TReceiveFunc)&TDiskRegistryActor::StateRestore },
    { "Zombie",  (IActor::TReceiveFunc)&TDiskRegistryActor::StateZombie  },
};

TDiskRegistryActor::TDiskRegistryActor(
        const TActorId& owner,
        TTabletStorageInfoPtr storage,
        TStorageConfigPtr config,
        TDiagnosticsConfigPtr diagnosticsConfig,
        TLogbrokerServicePtr logbrokerService,
        NNotify::IServicePtr notifyService)
    : TActor(&TThis::StateBoot)
    , TTabletBase(owner, std::move(storage))
    , Config(std::move(config))
    , DiagnosticsConfig(std::move(diagnosticsConfig))
    , LogbrokerService(std::move(logbrokerService))
    , NotifyService(std::move(notifyService))
{
    ActivityType = TBlockStoreActivities::DISK_REGISTRY;
}

TDiskRegistryActor::~TDiskRegistryActor()
{}

TString TDiskRegistryActor::GetStateName(ui32 state)
{
    if (state < STATE_MAX) {
        return States[state].Name;
    }
    return "<unknown>";
}

void TDiskRegistryActor::ScheduleCleanup(const TActorContext& ctx)
{
    const auto recyclingPeriod = Config->GetNonReplicatedDiskRecyclingPeriod();

    LOG_DEBUG_S(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "Schedule cleanup at " << recyclingPeriod.ToDeadLine());

    auto request = std::make_unique<TEvDiskRegistryPrivate::TEvCleanupDisksRequest>();

    ctx.ExecutorThread.Schedule(
        recyclingPeriod,
        new IEventHandle(ctx.SelfID, ctx.SelfID, request.get()));

    request.release();
}

void TDiskRegistryActor::BecomeAux(const TActorContext& ctx, EState state)
{
    Y_VERIFY_DEBUG(state < STATE_MAX);

    Become(States[state].Func);
    CurrentState = state;

    LOG_DEBUG(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Switched to state %s (system: %s, user: %s, executor: %s)",
        TabletID(),
        States[state].Name.data(),
        ToString(Tablet()).data(),
        ToString(SelfId()).data(),
        ToString(ExecutorID()).data());

    ReportTabletState(ctx);
}

void TDiskRegistryActor::ReportTabletState(const TActorContext& ctx)
{
    auto service = NNodeWhiteboard::MakeNodeWhiteboardServiceId(SelfId().NodeId());

    auto request = std::make_unique<NNodeWhiteboard::TEvWhiteboard::TEvTabletStateUpdate>(
        TabletID(),
        CurrentState);

    NCloud::Send(ctx, service, std::move(request));
}

void TDiskRegistryActor::OnActivateExecutor(const TActorContext& ctx)
{
    RegisterCounters(ctx);

    if (!Executor()->GetStats().IsFollower) {
        ExecuteTx<TInitSchema>(ctx);
    }

    BecomeAux(ctx, STATE_INIT);
}

bool TDiskRegistryActor::OnRenderAppHtmlPage(
    NMon::TEvRemoteHttpInfo::TPtr ev,
    const TActorContext& ctx)
{
    if (!Executor() || !Executor()->GetStats().IsActive) {
        return false;
    }

    if (ev) {
        HandleHttpInfo(ev, ctx);
    }
    return true;
}

void TDiskRegistryActor::BeforeDie(const NActors::TActorContext& ctx)
{
    UnregisterCounters(ctx);
    KillActors(ctx);
    CancelPendingRequests(ctx, PendingRequests);
}

void TDiskRegistryActor::OnDetach(const TActorContext& ctx)
{
    Counters = nullptr;

    BeforeDie(ctx);
    Die(ctx);
}

void TDiskRegistryActor::OnTabletDead(
    TEvTablet::TEvTabletDead::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    BeforeDie(ctx);
    Die(ctx);
}

void TDiskRegistryActor::RegisterCounters(const TActorContext& ctx)
{
    if (!Counters) {
        auto counters = CreateDiskRegistryCounters();

        // LAME: ownership transferred to executor
        Counters = counters.get();
        Executor()->RegisterExternalTabletCounters(counters.release());

        // only aggregated statistics will be reported by default
        // (you can always turn on per-tablet statistics on monitoring page)
        // TabletCountersAddTablet(TabletID(), ctx);

        ScheduleCountersUpdate(ctx);
    }

    if (auto counters = AppData(ctx)->Counters) {
        ComponentGroup = counters
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "disk_registry");
    }
}

void TDiskRegistryActor::ScheduleCountersUpdate(const TActorContext& ctx)
{
    ctx.Schedule(UpdateCountersInterval, new TEvents::TEvWakeup());
}

void TDiskRegistryActor::UpdateCounters(const TActorContext& ctx)
{
    if (State) {
        State->PublishCounters(ctx.Now());
    }
}

void TDiskRegistryActor::UpdateActorStats(const TActorContext& ctx)
{
    if (Counters) {
        auto& actorQueue = Counters->Percentile()[TDiskRegistryCounters::PERCENTILE_COUNTER_Actor_ActorQueue];
        auto& mailboxQueue = Counters->Percentile()[TDiskRegistryCounters::PERCENTILE_COUNTER_Actor_MailboxQueue];

        auto actorQueues = ctx.CountMailboxEvents(1001);
        actorQueue.IncrementFor(actorQueues.first);
        mailboxQueue.IncrementFor(actorQueues.second);
    }
}

void TDiskRegistryActor::KillActors(const TActorContext& ctx)
{
    for (auto& actor: Actors) {
        NCloud::Send<TEvents::TEvPoisonPill>(ctx, actor);
    }
}

void TDiskRegistryActor::UnregisterCounters(const TActorContext& ctx)
{
    auto counters = AppData(ctx)->Counters;

    if (counters) {
        counters
            ->GetSubgroup("counters", "blockstore")
            ->RemoveSubgroup("component", "disk_registry");
    }
}

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    NCloud::Send<TEvents::TEvPoisonPill>(ctx, Tablet());
    BecomeAux(ctx, STATE_ZOMBIE);
}

void TDiskRegistryActor::HandleWakeup(
    const TEvents::TEvWakeup::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    UpdateCounters(ctx);
    ScheduleCountersUpdate(ctx);
}

bool TDiskRegistryActor::HandleRequests(STFUNC_SIG)
{
    switch (ev->GetTypeRewrite()) {
        BLOCKSTORE_DISK_REGISTRY_REQUESTS(BLOCKSTORE_HANDLE_REQUEST, TEvDiskRegistry)
        BLOCKSTORE_DISK_REGISTRY_REQUESTS_FWD_SERVICE(BLOCKSTORE_HANDLE_REQUEST, TEvService)
        BLOCKSTORE_DISK_REGISTRY_REQUESTS_PRIVATE(BLOCKSTORE_HANDLE_REQUEST, TEvDiskRegistryPrivate)

        default:
            return false;
    }

    return true;
}

bool TDiskRegistryActor::RejectRequests(STFUNC_SIG)
{
    switch (ev->GetTypeRewrite()) {
        BLOCKSTORE_DISK_REGISTRY_REQUESTS(BLOCKSTORE_REJECT_REQUEST, TEvDiskRegistry)
        BLOCKSTORE_DISK_REGISTRY_REQUESTS_PRIVATE(BLOCKSTORE_REJECT_REQUEST, TEvDiskRegistryPrivate)

        default:
            return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleServerConnected(
    const TEvTabletPipe::TEvServerConnected::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ctx);
    auto* msg = ev->Get();
    auto [it, inserted] = ServerToAgentId.emplace(msg->ServerId, TString());
    Y_VERIFY_DEBUG(inserted);
}

void TDiskRegistryActor::HandleServerDisconnected(
    const TEvTabletPipe::TEvServerDisconnected::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    auto it = ServerToAgentId.find(msg->ServerId);
    if (it == ServerToAgentId.end()) {
        return;
    }

    const auto& agentId = it->second;

    if (agentId) {
        LOG_WARN_S(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "Agent " << agentId.Quote() << " disconnected");

        auto& info = AgentRegInfo[agentId];
        info.Connected = false;

        ScheduleRejectAgent(ctx, agentId, info.SeqNo);
    }

    ServerToAgentId.erase(it);
}

void TDiskRegistryActor::ScheduleRejectAgent(
    const NActors::TActorContext& ctx,
    TString agentId,
    ui64 seqNo)
{
    auto timeout = Config->GetNonReplicatedAgentTimeout();
    if (!timeout) {
        return;
    }

    auto deadline = timeout.ToDeadLine(ctx.Now());
    LOG_INFO_S(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "Schedule reject agent " << agentId.Quote() << " at " << deadline);

    auto request = std::make_unique<TEvDiskRegistryPrivate::TEvAgentConnectionLost>(
        std::move(agentId), seqNo);

    ctx.Schedule(deadline, request.release());
}

void TDiskRegistryActor::HandleAgentConnectionLost(
    const TEvDiskRegistryPrivate::TEvAgentConnectionLost::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    auto it = AgentRegInfo.find(msg->AgentId);
    if (it != AgentRegInfo.end() && msg->SeqNo < it->second.SeqNo) {
        LOG_DEBUG_S(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "Agent " << msg->AgentId.Quote() << " is connected");

        return;
    }

    LOG_WARN_S(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "Reject agent " << msg->AgentId.Quote());

    auto request =
        std::make_unique<TEvDiskRegistry::TEvChangeAgentStateRequest>();
    request->Record.SetAgentId(msg->AgentId);
    request->Record.SetAgentState(NProto::AGENT_STATE_UNAVAILABLE);
    request->Record.SetReason("connection lost");

    NCloud::Send(ctx, ctx.SelfID, std::move(request));
}

void TDiskRegistryActor::HandleOperationCompleted(
    const TEvDiskRegistryPrivate::TEvOperationCompleted::TPtr& ev,
    const NActors::TActorContext& ctx)
{
    Y_UNUSED(ctx);

    Actors.erase(ev->Sender);
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TDiskRegistryActor::StateBoot)
{
    UpdateActorStatsSampled(ctx);
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);
        HFunc(TEvents::TEvWakeup, HandleWakeup);

        IgnoreFunc(TEvTabletPipe::TEvServerConnected);
        IgnoreFunc(TEvTabletPipe::TEvServerDisconnected);

        BLOCKSTORE_HANDLE_REQUEST(WaitReady, TEvDiskRegistry)

        default:
            StateInitImpl(ev, ctx);
            break;
    }
}

STFUNC(TDiskRegistryActor::StateInit)
{
    UpdateActorStatsSampled(ctx);
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);
        HFunc(TEvents::TEvWakeup, HandleWakeup);

        IgnoreFunc(TEvTabletPipe::TEvServerConnected);
        IgnoreFunc(TEvTabletPipe::TEvServerDisconnected);

        BLOCKSTORE_HANDLE_REQUEST(WaitReady, TEvDiskRegistry)

        default:
            if (!RejectRequests(ev, ctx) && !HandleDefaultEvents(ev, ctx)) {
                HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::DISK_REGISTRY);
            }
            break;
    }
}

STFUNC(TDiskRegistryActor::StateWork)
{
    UpdateActorStatsSampled(ctx);
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);
        HFunc(TEvents::TEvWakeup, HandleWakeup);

        HFunc(TEvTabletPipe::TEvServerConnected, HandleServerConnected);
        HFunc(TEvTabletPipe::TEvServerDisconnected, HandleServerDisconnected);

        IgnoreFunc(TEvDiskRegistry::TEvReleaseDiskResponse);
        IgnoreFunc(TEvDiskRegistry::TEvUnregisterAgentResponse);

        IgnoreFunc(TEvDiskRegistry::TEvChangeAgentStateResponse);
        IgnoreFunc(TEvDiskRegistry::TEvChangeDeviceStateResponse);

        IgnoreFunc(TEvDiskRegistryPrivate::TEvCleanupDevicesResponse);

        HFunc(TEvDiskRegistryPrivate::TEvCleanupDisksResponse,
            HandleCleanupDisksResponse);

        HFunc(TEvDiskRegistryPrivate::TEvSecureEraseResponse,
            HandleSecureEraseResponse);

        HFunc(TEvDiskRegistryPrivate::TEvDestroyBrokenDisksResponse,
            HandleDestroyBrokenDisksResponse);

        HFunc(TEvDiskRegistryPrivate::TEvStartMigrationResponse,
            HandleStartMigrationResponse);

        HFunc(TEvDiskRegistryPrivate::TEvNotifyDisksResponse,
            HandleNotifyDisksResponse);

        HFunc(TEvDiskRegistryPrivate::TEvNotifyUsersResponse,
            HandleNotifyUsersResponse);

        HFunc(TEvDiskRegistryPrivate::TEvPublishDiskStatesResponse,
            HandlePublishDiskStatesResponse);

        HFunc(TEvDiskRegistryPrivate::TEvAgentConnectionLost,
            HandleAgentConnectionLost);

        HFunc(TEvDiskRegistryPrivate::TEvOperationCompleted,
            HandleOperationCompleted);

        HFunc(TEvDiskRegistryPrivate::TEvUpdateVolumeConfigResponse,
            HandleUpdateVolumeConfigResponse);

        default:
            if (!HandleRequests(ev, ctx) && !HandleDefaultEvents(ev, ctx)) {
                HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::DISK_REGISTRY);
            }
            break;
    }
}

STFUNC(TDiskRegistryActor::StateRestore)
{
    UpdateActorStatsSampled(ctx);
    switch (ev->GetTypeRewrite()) {
        IgnoreFunc(TEvents::TEvWakeup);

        IgnoreFunc(TEvTabletPipe::TEvServerConnected);
        IgnoreFunc(TEvTabletPipe::TEvServerDisconnected);

        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);
        HFunc(TEvTablet::TEvTabletDead, HandleTabletDead);
        HFunc(NMon::TEvRemoteHttpInfo, RenderHtmlPage);

        HFunc(
            TEvDiskRegistry::TEvBackupDiskRegistryStateRequest,
            HandleBackupDiskRegistryState);
        HFunc(
            TEvDiskRegistry::TEvRestoreDiskRegistryStateRequest,
            HandleRestoreDiskRegistryState);

        default:
            if (!RejectRequests(ev, ctx)) {
                LogUnexpectedEvent(
                    ctx,
                    ev,
                    TBlockStoreComponents::DISK_REGISTRY);
            }
            break;
    }
}

STFUNC(TDiskRegistryActor::StateZombie)
{
    UpdateActorStatsSampled(ctx);
    switch (ev->GetTypeRewrite()) {
        IgnoreFunc(TEvents::TEvPoisonPill);
        IgnoreFunc(TEvents::TEvWakeup);

        HFunc(TEvTablet::TEvTabletDead, HandleTabletDead);

        IgnoreFunc(TEvTabletPipe::TEvServerConnected);
        IgnoreFunc(TEvTabletPipe::TEvServerDisconnected);

        default:
            if (!RejectRequests(ev, ctx)) {
                HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::DISK_REGISTRY);
            }
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////

bool ToLogicalBlocks(NProto::TDeviceConfig& device, ui32 logicalBlockSize)
{
    const auto blockSize = device.GetBlockSize();
    if (logicalBlockSize % blockSize != 0) {
        ReportDiskRegistryLogicalPhysicalBlockSizeMismatch();

        return false;
    }

    device.SetBlocksCount(device.GetBlocksCount() * blockSize / logicalBlockSize);
    device.SetBlockSize(logicalBlockSize);

    return true;
}

}   // namespace NCloud::NBlockStore::NStorage
