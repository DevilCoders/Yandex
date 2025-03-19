#include "volume_actor.h"

#include "volume_database.h"
#include "volume_tx.h"

#include <cloud/blockstore/libs/storage/api/bootstrapper.h>
#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/api/stats_service.h>
#include <cloud/blockstore/libs/storage/api/undelivered.h>
#include <cloud/blockstore/libs/storage/core/monitoring_utils.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>

// TODO: invalid reference
#include <cloud/blockstore/libs/storage/service/service_events_private.h>

#include <ydb/core/base/tablet_pipe.h>
#include <ydb/core/node_whiteboard/node_whiteboard.h>
#include <ydb/core/tablet/tablet_counters_aggregator.h>

#include <library/cpp/lwtrace/log_shuttle.h>
#include <library/cpp/lwtrace/protos/lwtrace.pb.h>
#include <library/cpp/lwtrace/signature.h>

#include <util/string/builder.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NNodeWhiteboard;
using namespace NKikimr::NTabletFlatExecutor;

using namespace NCloud::NBlockStore::NStorage::NPartition;

using namespace NCloud::NStorage;

////////////////////////////////////////////////////////////////////////////////

const TVolumeActor::TStateInfo TVolumeActor::States[STATE_MAX] = {
    { "Boot",   (IActor::TReceiveFunc)&TVolumeActor::StateBoot   },
    { "Init",   (IActor::TReceiveFunc)&TVolumeActor::StateInit   },
    { "Work",   (IActor::TReceiveFunc)&TVolumeActor::StateWork   },
    { "Zombie", (IActor::TReceiveFunc)&TVolumeActor::StateZombie },
};

TVolumeActor::TVolumeActor(
        const TActorId& owner,
        TTabletStorageInfoPtr storage,
        TStorageConfigPtr config,
        TDiagnosticsConfigPtr diagnosticsConfig,
        IProfileLogPtr profileLog,
        IBlockDigestGeneratorPtr blockDigestGenerator,
        ITraceSerializerPtr traceSerializer,
        NRdma::IClientPtr rdmaClient,
        EVolumeStartMode startMode)
    : TActor(&TThis::StateBoot)
    , TTabletBase(owner, std::move(storage))
    , Config(std::move(config))
    , DiagnosticsConfig(std::move(diagnosticsConfig))
    , ProfileLog(std::move(profileLog))
    , BlockDigestGenerator(std::move(blockDigestGenerator))
    , TraceSerializer(std::move(traceSerializer))
    , RdmaClient(std::move(rdmaClient))
    , StartMode(startMode)
{
    ActivityType = TBlockStoreActivities::VOLUME;
}

TVolumeActor::~TVolumeActor()
{
    // For unit tests under asan only, since test actor
    // runtime just calls destructor upon test completion
    // Usually we terminate inflight transactions
    // in BeforeDie.
    ReleaseTransactions();
}

TString TVolumeActor::GetStateName(ui32 state)
{
    if (state < STATE_MAX) {
        return States[state].Name;
    }
    return "<unknown>";
}

void TVolumeActor::Enqueue(STFUNC_SIG)
{
    LOG_ERROR_S(ctx, TBlockStoreComponents::VOLUME,
        "[" << TabletID() << "]"
        << " IGNORING message type# " << ev->GetTypeRewrite()
        << " from Sender# " << ToString(ev->Sender)
        << " in StateBoot");
}

void TVolumeActor::DefaultSignalTabletActive(const TActorContext& ctx)
{
    Y_UNUSED(ctx); // postpone until LoadState transaction completes
}

void TVolumeActor::BecomeAux(const TActorContext& ctx, EState state)
{
    Y_VERIFY_DEBUG(state < STATE_MAX);

    Become(States[state].Func);
    CurrentState = state;

    LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Switched to state %s (system: %s, user: %s, executor: %s)",
        TabletID(),
        States[state].Name.data(),
        ToString(Tablet()).data(),
        ToString(SelfId()).data(),
        ToString(ExecutorID()).data());

    ReportTabletState(ctx);
}

void TVolumeActor::ReportTabletState(const TActorContext& ctx)
{
    auto service = MakeNodeWhiteboardServiceId(SelfId().NodeId());

    auto request = std::make_unique<TEvWhiteboard::TEvTabletStateUpdate>(
        TabletID(),
        CurrentState);

    NCloud::Send(ctx, service, std::move(request));
}

void TVolumeActor::RegisterCounters(const TActorContext& ctx)
{
    Y_UNUSED(ctx);

    if (!Counters) {
        auto counters = CreateVolumeCounters();

        // LAME: ownership transferred to executor
        Counters = counters.get();
        Executor()->RegisterExternalTabletCounters(counters.release());

        // only aggregated statistics will be reported by default
        // (you can always turn on per-tablet statistics on monitoring page)
        // TabletCountersAddTablet(TabletID(), ctx);

        ScheduleCountersUpdate(ctx);
    }

    if (!VolumeSelfCounters) {
        VolumeSelfCounters = CreateVolumeSelfCounters(CountersPolicy);
    }
}

void TVolumeActor::ScheduleCountersUpdate(const TActorContext& ctx)
{
    if (!UpdateCountersScheduled) {
        ctx.Schedule(UpdateCountersInterval,
            new TEvVolumePrivate::TEvUpdateCounters());
        UpdateCountersScheduled = true;
    }

    if (!UpdateLeakyBucketCountersScheduled) {
        ctx.Schedule(UpdateLeakyBucketCountersInterval,
            new TEvVolumePrivate::TEvUpdateThrottlerState());
        UpdateLeakyBucketCountersScheduled = true;
    }
}

void TVolumeActor::UpdateLeakyBucketCounters(const TActorContext& ctx)
{
    Y_UNUSED(ctx);

    if (!State) {
        return;
    }

    auto& simple = VolumeSelfCounters->Simple;
    auto& cumulative = VolumeSelfCounters->Cumulative;
    const auto& tp = State->GetThrottlingPolicy();
    ui64 currentRate = tp.CalculateCurrentSpentBudgetShare(ctx.Now()) * 100;
    simple.MaxUsedQuota.Set(Max(simple.MaxUsedQuota.Value, currentRate));
    cumulative.UsedQuota.Increment(currentRate);
}

void TVolumeActor::UpdateCounters(const TActorContext& ctx)
{
    Y_UNUSED(ctx);

    if (!State) {
        return;
    }

    SendPartStatsToService(ctx);
    SendSelfStatsToService(ctx);
}

void TVolumeActor::OnActivateExecutor(const TActorContext& ctx)
{
    LOG_INFO(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Activated executor",
        TabletID());

    RegisterCounters(ctx);

    if (!Executor()->GetStats().IsFollower) {
        ExecuteTx<TInitSchema>(ctx);
    }

    UsageIntervalBegin = ctx.Now();
    ctx.Schedule(
        Config->GetUsageStatsLogPeriod(),
        new TEvVolumePrivate::TEvLogUsageStats());

    BecomeAux(ctx, STATE_INIT);
}

bool TVolumeActor::ReassignChannelsEnabled() const
{
    return true;
}

bool TVolumeActor::OnRenderAppHtmlPage(
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

void TVolumeActor::OnDetach(const TActorContext& ctx)
{
    Counters = nullptr;

    BeforeDie(ctx);
    Die(ctx);
}

void TVolumeActor::OnTabletDead(
    TEvTablet::TEvTabletDead::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    if (State) {
        DumpUsageStats(ctx, GetVolumeStatus());
    }

    BeforeDie(ctx);
    Die(ctx);
}

void TVolumeActor::BeforeDie(const TActorContext& ctx)
{
    UnregisterVolume(ctx);
    StopPartitions(ctx);
    TerminateTransactions(ctx);
    KillActors(ctx);
    CancelRequests(ctx);

    for (auto& [part, handler]: WaitForPartitions) {
        if (handler) {
            std::invoke(handler, ctx, MakeError(E_REJECTED, "Tablet is dead"));
        }
    }
    StoppedPartitions.clear();
    WaitForPartitions.clear();
}

void TVolumeActor::KillActors(const TActorContext& ctx)
{
    for (auto& actor: Actors) {
        NCloud::Send<TEvents::TEvPoisonPill>(ctx, actor);
    }
}

void TVolumeActor::AddTransaction(TRequestInfo& requestInfo)
{
    requestInfo.Ref();

    Y_VERIFY(requestInfo.Empty());
    ActiveTransactions.PushBack(&requestInfo);
}

void TVolumeActor::RemoveTransaction(TRequestInfo& requestInfo)
{
    Y_VERIFY(!requestInfo.Empty());
    requestInfo.Unlink();

    Y_VERIFY(requestInfo.RefCount() > 1);
    requestInfo.UnRef();
}

void TVolumeActor::TerminateTransactions(const TActorContext& ctx)
{
    while (ActiveTransactions) {
        TRequestInfo* requestInfo = ActiveTransactions.PopFront();
        if (requestInfo->CancelRoutine) {
            requestInfo->CancelRequest(ctx);
        }

        Y_VERIFY(requestInfo->RefCount() >= 1);
        requestInfo->UnRef();
    }
}

void TVolumeActor::ReleaseTransactions()
{
    while (ActiveTransactions) {
        TRequestInfo* requestInfo = ActiveTransactions.PopFront();
        Y_VERIFY(requestInfo->RefCount() >= 1);
        requestInfo->UnRef();
    }
}

TString TVolumeActor::GetVolumeStatusString(TVolumeActor::EStatus status)
{
    switch (status)
    {
        case TVolumeActor::STATUS_ONLINE: return "online";
        case TVolumeActor::STATUS_INACTIVE: return "inactive";
        case TVolumeActor::STATUS_MOUNTED: return "mounted";
        default: return "offline";
    }
}

TVolumeActor::EStatus TVolumeActor::GetVolumeStatus() const
{
    if (State) {
        auto state = State->GetPartitionsState();
        if (state == TPartitionInfo::READY) {
            return (StartMode == EVolumeStartMode::MOUNTED)
                ? TVolumeActor::STATUS_MOUNTED
                : TVolumeActor::STATUS_ONLINE;
        } else if (state == TPartitionInfo::UNKNOWN) {
            return TVolumeActor::STATUS_INACTIVE;
        }
    }
    return TVolumeActor::STATUS_OFFLINE;
}

ui64 TVolumeActor::GetBlocksCount() const
{
    return State ? State->GetBlocksCount() : 0;
}

void TVolumeActor::OnServicePipeDisconnect(
    const TActorContext& ctx,
    const TActorId& serverId,
    TInstant timestamp)
{
    Y_UNUSED(ctx);
    if (State) {
        State->SetServiceDisconnected(serverId, timestamp);
    }
}

void TVolumeActor::RegisterVolume(const TActorContext& ctx)
{
    NProto::TVolume volume;
    VolumeConfigToVolume(State->GetMeta().GetVolumeConfig(), volume);

    {
        auto request = std::make_unique<TEvService::TEvRegisterVolume>(
            State->GetDiskId(),
            TabletID(),
            volume);
        NCloud::Send(ctx, MakeStorageServiceId(), std::move(request));
    }

    {
        auto request = std::make_unique<TEvStatsService::TEvRegisterVolume>(
            State->GetDiskId(),
            TabletID(),
            std::move(volume));
        NCloud::Send(ctx, MakeStorageStatsServiceId(), std::move(request));
    }
}

void TVolumeActor::UnregisterVolume(const TActorContext& ctx)
{
    if (State) {
        {
            auto request = std::make_unique<TEvService::TEvUnregisterVolume>(
                State->GetDiskId());
            NCloud::Send(ctx, MakeStorageServiceId(), std::move(request));
        }

        {
            auto request = std::make_unique<TEvStatsService::TEvUnregisterVolume>(
                State->GetDiskId());
            NCloud::Send(ctx, MakeStorageStatsServiceId(), std::move(request));
        }
    }
}

void TVolumeActor::SendVolumeConfigUpdated(const TActorContext& ctx)
{
    NProto::TVolume volume;
    VolumeConfigToVolume(State->GetMeta().GetVolumeConfig(), volume);
    {
        auto request = std::make_unique<TEvService::TEvVolumeConfigUpdated>(
            State->GetDiskId(),
            volume);
        NCloud::Send(ctx, MakeStorageServiceId(), std::move(request));
    }

    {
        auto request = std::make_unique<TEvStatsService::TEvVolumeConfigUpdated>(
            State->GetDiskId(),
            std::move(volume));
        NCloud::Send(ctx, MakeStorageStatsServiceId(), std::move(request));
    }
}

void TVolumeActor::SendVolumeSelfCounters(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvStatsService::TEvVolumeSelfCounters>(
        State->GetConfig().GetDiskId(),
        State->HasActiveClients(ctx.Now()),
        State->IsPreempted(SelfId()),
        std::move(VolumeSelfCounters));
    NCloud::Send(ctx, MakeStorageStatsServiceId(), std::move(request));
}

void TVolumeActor::CancelRequests(const TActorContext& ctx)
{
    if (ShuttingDown) {
        return;
    }
    ShuttingDown = true;
    PostponedQueueFlushScheduled = false;

    for (auto& r: VolumeRequests) {
        r.second.CancelRequest(
            ctx,
            MakeError(E_REJECTED, "Shutting down"));
    }
    VolumeRequests.clear();

    while (PostponedRequests.size()) {
        TAutoPtr<IEventHandle> ev = PostponedRequests.front().Event.release();
        Receive(ev, ctx);

        Y_VERIFY(!PostponedQueueFlushScheduled);
        PostponedRequests.pop_front();
    }

    while (ActiveReadHistoryRequests) {
        TRequestInfo* requestInfo = ActiveReadHistoryRequests.PopFront();
        TStringStream out;
        NMonitoringUtils::DumpTabletNotReady(out);
        NCloud::Reply(
            ctx,
            *requestInfo,
            std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str()));

        Y_VERIFY(requestInfo->RefCount() >= 1);
        requestInfo->UnRef();
    }

    CancelPendingRequests(ctx, PendingRequests);
}

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr&,
    const TActorContext& ctx)
{
    CancelRequests(ctx);
    NCloud::Send<TEvents::TEvPoisonPill>(ctx, Tablet());
    BecomeAux(ctx, STATE_ZOMBIE);
}

void TVolumeActor::HandlePoisonTaken(
    const TEvents::TEvPoisonTaken::TPtr& ev,
    const TActorContext& ctx)
{
    LOG_INFO(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Partition %s stopped",
        TabletID(),
        ev->Sender.ToString().c_str());

    if (WaitForPartitions.empty()) {
        return;
    }

    StoppedPartitions.insert(ev->Sender);

    auto it = WaitForPartitions.begin();

    for (; it != WaitForPartitions.end(); ++it) {
        auto& [part, handler] = *it;

        if (!StoppedPartitions.erase(part)) {
            break;
        }

        if (handler) {
            std::invoke(handler, ctx, NProto::TError());
        }
    }

    WaitForPartitions.erase(WaitForPartitions.begin(), it);

    if (WaitForPartitions.empty()) {
        StoppedPartitions.clear();
    }
}

void TVolumeActor::HandleUpdateThrottlerState(
    const TEvVolumePrivate::TEvUpdateThrottlerState::TPtr& ev,
    const TActorContext& ctx)
{
    UpdateLeakyBucketCountersScheduled = false;

    UpdateLeakyBucketCounters(ctx);
    ScheduleCountersUpdate(ctx);

    if (!State) {
        return;
    }

    const auto mediaKind = static_cast<NCloud::NProto::EStorageMediaKind>(
        GetNewestConfig().GetStorageMediaKind());
    if ((mediaKind == NCloud::NProto::EStorageMediaKind::STORAGE_MEDIA_HDD
            || mediaKind == NCloud::NProto::EStorageMediaKind::STORAGE_MEDIA_HYBRID)
            && ctx.Now() - Config->GetThrottlerStateWriteInterval() >= LastThrottlerStateWrite)
    {
        auto requestInfo = CreateRequestInfo(
            ev->Sender,
            ev->Cookie,
            ev->Get()->CallContext,
            std::move(ev->TraceId));

        ExecuteTx<TWriteThrottlerState>(
            ctx,
            std::move(requestInfo),
            TVolumeDatabase::TThrottlerStateInfo{
                State->GetThrottlingPolicy().GetCurrentBoostBudget().MilliSeconds()
            });
    }
}

void TVolumeActor::HandleUpdateCounters(
    const TEvVolumePrivate::TEvUpdateCounters::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    UpdateCountersScheduled = false;

    UpdateCounters(ctx);
    ScheduleCountersUpdate(ctx);

    if (ctx.Now() - Config->GetVolumeHistoryDuration() >= LastHistoryCleanup && State) {
        State->CleanupHistoryIfNeeded(ctx.Now() - Config->GetVolumeHistoryDuration());

        auto requestInfo = CreateRequestInfo(
            ev->Sender,
            ev->Cookie,
            ev->Get()->CallContext,
            std::move(ev->TraceId));

        ExecuteTx<TCleanupHistory>(
            ctx,
            std::move(requestInfo),
            ctx.Now() - Config->GetVolumeHistoryDuration());
    }
}

void TVolumeActor::HandleServerConnected(
    const TEvTabletPipe::TEvServerConnected::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Pipe client %s (server %s) connected to volume %s",
        TabletID(),
        ToString(msg->ClientId).data(),
        ToString(msg->ServerId).data(),
        (State ? State->GetDiskId().Quote().data() : "<empty>"));
}

void TVolumeActor::HandleServerDisconnected(
    const TEvTabletPipe::TEvServerDisconnected::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    const auto now = ctx.Now();

    LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Pipe client %s (server %s) disconnected from volume %s at %s",
        TabletID(),
        ToString(msg->ClientId).data(),
        ToString(msg->ServerId).data(),
        (State ? State->GetDiskId().Quote().data() : "<empty>"),
        now.ToString().data());

    OnServicePipeDisconnect(ctx, msg->ServerId, now);
}

void TVolumeActor::HandleServerDestroyed(
    const TEvTabletPipe::TEvServerDestroyed::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    const auto now = ctx.Now();

    LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Pipe client %s's server %s got destroyed for volume %s at %s",
        TabletID(),
        ToString(msg->ClientId).data(),
        ToString(msg->ServerId).data(),
        (State ? State->GetDiskId().Quote().data() : "<empty>"),
        now.ToString().data());

    OnServicePipeDisconnect(ctx, msg->ServerId, now);
}

void TVolumeActor::ResetServicePipes(const TActorContext& ctx)
{
    for (const auto& actor : State->ClearPipeServerIds()) {
        NCloud::Send<TEvents::TEvPoisonPill>(ctx, actor);
    }
}

void TVolumeActor::HandleTabletMetrics(
    NKikimr::TEvLocal::TEvTabletMetrics::TPtr& ev,
    const TActorContext& ctx)
{
    ctx.Send(ev->Forward(MakeHiveProxyServiceId()));
}

bool TVolumeActor::HandleRequests(STFUNC_SIG)
{
    switch (ev->GetTypeRewrite()) {
        BLOCKSTORE_VOLUME_REQUESTS(BLOCKSTORE_HANDLE_REQUEST, TEvVolume)
        BLOCKSTORE_VOLUME_REQUESTS_PRIVATE(BLOCKSTORE_HANDLE_REQUEST, TEvVolumePrivate)
        BLOCKSTORE_VOLUME_REQUESTS_FWD_SERVICE(BLOCKSTORE_HANDLE_REQUEST, TEvService)

        HFunc(TEvVolume::TEvDescribeBlocksResponse, HandleDescribeBlocksResponse);
        HFunc(TEvVolume::TEvGetUsedBlocksResponse, HandleGetUsedBlocksResponse);
        HFunc(TEvVolume::TEvGetPartitionInfoResponse, HandleGetPartitionInfoResponse);
        HFunc(TEvVolume::TEvCompactRangeResponse, HandleCompactRangeResponse);
        HFunc(TEvVolume::TEvGetCompactionStatusResponse, HandleGetCompactionStatusResponse);
        HFunc(TEvVolume::TEvDeleteCheckpointDataResponse, HandleDeleteCheckpointDataResponse);
        HFunc(TEvVolume::TEvRebuildMetadataResponse, HandleRebuildMetadataResponse);
        HFunc(TEvVolume::TEvGetRebuildMetadataStatusResponse, HandleGetRebuildMetadataStatusResponse);

        HFunc(TEvService::TEvReadBlocksResponse, HandleReadBlocksResponse);
        HFunc(TEvService::TEvWriteBlocksResponse, HandleWriteBlocksResponse);
        HFunc(TEvService::TEvZeroBlocksResponse, HandleZeroBlocksResponse);
        HFunc(TEvService::TEvCreateCheckpointResponse, HandleCreateCheckpointResponse);
        HFunc(TEvService::TEvDeleteCheckpointResponse, HandleDeleteCheckpointResponse);
        HFunc(TEvService::TEvGetChangedBlocksResponse, HandleGetChangedBlocksResponse);
        HFunc(TEvService::TEvReadBlocksLocalResponse, HandleReadBlocksLocalResponse);
        HFunc(TEvService::TEvWriteBlocksLocalResponse, HandleWriteBlocksLocalResponse);

        default:
            return false;
    }

    return true;
}

bool TVolumeActor::RejectRequests(STFUNC_SIG)
{
    switch (ev->GetTypeRewrite()) {
        BLOCKSTORE_VOLUME_REQUESTS(BLOCKSTORE_REJECT_REQUEST, TEvVolume)
        BLOCKSTORE_VOLUME_REQUESTS_PRIVATE(BLOCKSTORE_REJECT_REQUEST, TEvVolumePrivate)
        BLOCKSTORE_VOLUME_REQUESTS_FWD_SERVICE(BLOCKSTORE_REJECT_REQUEST, TEvService)

        default:
            return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TVolumeActor::StateBoot)
{
    UpdateActorStatsSampled(ctx);
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        IgnoreFunc(TEvTabletPipe::TEvServerConnected);
        IgnoreFunc(TEvTabletPipe::TEvServerDisconnected);

        HFunc(TEvVolumePrivate::TEvLogUsageStats, HandleLogUsageStats);
        HFunc(TEvVolumePrivate::TEvUpdateThrottlerState, HandleUpdateThrottlerState);
        HFunc(TEvVolumePrivate::TEvUpdateCounters, HandleUpdateCounters);

        BLOCKSTORE_HANDLE_REQUEST(WaitReady, TEvVolume)

        default:
            if (!RejectRequests(ev, ctx)) {
                StateInitImpl(ev, ctx);
            }
            break;
    }
}

STFUNC(TVolumeActor::StateInit)
{
    UpdateActorStatsSampled(ctx);
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        IgnoreFunc(TEvTabletPipe::TEvServerConnected);
        IgnoreFunc(TEvTabletPipe::TEvServerDisconnected);

        HFunc(TEvVolumePrivate::TEvProcessUpdateVolumeConfig, HandleProcessUpdateVolumeConfig);
        HFunc(TEvBlockStore::TEvUpdateVolumeConfig, HandleUpdateVolumeConfig);
        HFunc(TEvVolumePrivate::TEvAllocateDiskIfNeeded, HandleAllocateDiskIfNeeded);
        HFunc(TEvDiskRegistry::TEvAllocateDiskResponse, HandleAllocateDiskResponse);
        HFunc(TEvVolumePrivate::TEvLogUsageStats, HandleLogUsageStats);
        HFunc(TEvVolumePrivate::TEvUpdateThrottlerState, HandleUpdateThrottlerState);
        HFunc(TEvVolumePrivate::TEvUpdateCounters, HandleUpdateCounters);

        BLOCKSTORE_HANDLE_REQUEST(WaitReady, TEvVolume)

        default:
            if (!RejectRequests(ev, ctx) && !HandleDefaultEvents(ev, ctx)) {
                HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::VOLUME);
            }
            break;
    }
}

STFUNC(TVolumeActor::StateWork)
{
    UpdateActorStatsSampled(ctx);
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);
        HFunc(TEvents::TEvPoisonTaken, HandlePoisonTaken);
        HFunc(TEvents::TEvWakeup, HandleWakeup);

        HFunc(TEvTabletPipe::TEvServerConnected, HandleServerConnected);
        HFunc(TEvTabletPipe::TEvServerDisconnected, HandleServerDisconnected);
        HFunc(TEvTabletPipe::TEvServerDestroyed, HandleServerDestroyed);

        HFunc(TEvBootstrapper::TEvStatus, HandleTabletStatus);
        HFunc(
            TEvVolumePrivate::TEvProcessUpdateVolumeConfig,
            HandleProcessUpdateVolumeConfig);
        HFunc(TEvBlockStore::TEvUpdateVolumeConfig, HandleUpdateVolumeConfig);
        HFunc(
            TEvVolumePrivate::TEvAllocateDiskIfNeeded,
            HandleAllocateDiskIfNeeded);
        HFunc(
            TEvVolumePrivate::TEvUpdateThrottlerState,
            HandleUpdateThrottlerState);
        HFunc(TEvVolumePrivate::TEvUpdateCounters, HandleUpdateCounters);
        HFunc(
            TEvVolume::TEvNonreplicatedPartitionCounters,
            HandleNonreplicatedPartCounters);
        HFunc(TEvStatsService::TEvVolumePartCounters, HandlePartCounters);
        HFunc(TEvVolumePrivate::TEvPartStatsSaved, HandlePartStatsSaved);
        HFunc(
            TEvVolumePrivate::TEvMultipartitionWriteOrZeroCompleted,
            HandleMultipartitionWriteOrZeroCompleted);
        HFunc(
            TEvVolumePrivate::TEvWriteOrZeroCompleted,
            HandleWriteOrZeroCompleted);
        HFunc(
            TEvDiskRegistry::TEvAcquireDiskResponse,
            HandleAcquireDiskResponse);
        HFunc(
            TEvVolumePrivate::TEvAcquireDiskIfNeeded,
            HandleAcquireDiskIfNeeded);
        HFunc(TEvVolume::TEvReacquireDisk, HandleReacquireDisk);
        HFunc(TEvVolume::TEvRdmaUnavailable, HandleRdmaUnavailable);
        HFunc(
            TEvDiskRegistry::TEvReleaseDiskResponse,
            HandleReleaseDiskResponse);
        HFunc(
            TEvDiskRegistry::TEvAllocateDiskResponse,
            HandleAllocateDiskResponse);

        HFunc(
            TEvVolumePrivate::TEvRetryStartPartition,
            HandleRetryStartPartition);

        HFunc(TEvHiveProxy::TEvBootExternalResponse, HandleBootExternalResponse);
        HFunc(TEvPartition::TEvWaitReadyResponse, HandleWaitReadyResponse);
        HFunc(TEvPartition::TEvBackpressureReport, HandleBackpressureReport);
        HFunc(TEvVolumePrivate::TEvLogUsageStats, HandleLogUsageStats);

        HFunc(TEvLocal::TEvTabletMetrics, HandleTabletMetrics);

        HFunc(TEvVolume::TEvUpdateMigrationState, HandleUpdateMigrationState);

        default:
            if (!HandleRequests(ev, ctx) && !HandleDefaultEvents(ev, ctx)) {
                HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::VOLUME);
            }
            break;
    }
}

STFUNC(TVolumeActor::StateZombie)
{
    UpdateActorStatsSampled(ctx);
    switch (ev->GetTypeRewrite()) {
        IgnoreFunc(TEvTabletPipe::TEvServerConnected);
        IgnoreFunc(TEvTabletPipe::TEvServerDisconnected);

        HFunc(TEvTablet::TEvTabletDead, HandleTabletDead);

        HFunc(TEvVolumePrivate::TEvLogUsageStats, HandleLogUsageStats);
        IgnoreFunc(TEvVolumePrivate::TEvUpdateCounters);

        IgnoreFunc(TEvStatsService::TEvVolumePartCounters);

        IgnoreFunc(TEvPartition::TEvWaitReadyResponse);

        IgnoreFunc(TEvents::TEvPoisonPill);
        IgnoreFunc(TEvents::TEvPoisonTaken);

        HFunc(TEvLocal::TEvTabletMetrics, HandleTabletMetrics);

        default:
            if (!RejectRequests(ev, ctx)) {
                HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::VOLUME);
            }
            break;
    }
}

}   // namespace NCloud::NBlockStore::NStorage
