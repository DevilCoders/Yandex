#include "hive_proxy_actor.h"

#include "tablet_boot_info_cache.h"

#include <cloud/storage/core/libs/common/verify.h>

#include <ydb/core/base/appdata.h>

namespace NCloud::NStorage {

using namespace NActors;

using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<NTabletPipe::IClientCache> CreateTabletPipeClientCache(
    const THiveProxyConfig& config)
{
    NTabletPipe::TClientConfig clientConfig;
    clientConfig.RetryPolicy = {
        .RetryLimitCount = config.PipeClientRetryCount,
        .MinRetryTime = config.PipeClientMinRetryTime,
        .MaxRetryTime = config.HiveLockExpireTimeout
    };

    return std::unique_ptr<NTabletPipe::IClientCache>(
        NTabletPipe::CreateUnboundedClientCache(clientConfig));
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

THiveProxyActor::THiveProxyActor(
        THiveProxyConfig config,
        IFileIOServicePtr fileIO)
    : ClientCache(CreateTabletPipeClientCache(config))
    , LockExpireTimeout(config.HiveLockExpireTimeout)
    , LogComponent(config.LogComponent)
    , FileIOService(std::move(fileIO))
    , TabletBootInfoCacheFilePath(config.TabletBootInfoCacheFilePath)
{
    ActivityType = TStorageActivities::HIVE_PROXY;
}

void THiveProxyActor::Bootstrap(const TActorContext& ctx)
{
    TThis::Become(&TThis::StateWork);

    if (TabletBootInfoCacheFilePath && FileIOService) {
        auto cache = std::make_unique<TTabletBootInfoCache>(
            LogComponent,
            TabletBootInfoCacheFilePath,
            FileIOService,
            true /* syncEnabled */
        );
        TabletBootInfoCache = ctx.Register(
            cache.release(), TMailboxType::HTSwap, AppData()->IOPoolId);
    }
}

////////////////////////////////////////////////////////////////////////////////

ui64 THiveProxyActor::GetHive(
    const TActorContext& ctx,
    ui64 tabletId,
    ui32 hiveIdx)
{
    auto domainsInfo = AppData(ctx)->DomainsInfo;
    Y_VERIFY(domainsInfo->Domains);

    ui32 domainUid = TDomainsInfo::BadDomainId;
    if (tabletId) {
        domainUid = domainsInfo->GetDomainUidByTabletId(tabletId);
        if (domainUid == TDomainsInfo::BadDomainId) {
            LOG_WARN_S(ctx, LogComponent,
                "Missing domain for tablet " << tabletId
                    << " using default domain");
        }
    }
    if (domainUid == TDomainsInfo::BadDomainId) {
        domainUid = domainsInfo->Domains.begin()->first;
    }

    const auto& domain = domainsInfo->GetDomain(domainUid);
    ui32 hiveUid = domain.GetHiveUidByIdx(hiveIdx);
    ui64 hive = domainsInfo->GetHive(hiveUid);
    return hive;
}

void THiveProxyActor::SendLockRequest(
    const TActorContext& ctx, ui64 hive, ui64 tabletId, bool reconnect)
{
    auto hiveRequest =
        std::make_unique<TEvHive::TEvLockTabletExecution>(tabletId);
    hiveRequest->Record.SetMaxReconnectTimeout(
        LockExpireTimeout.MilliSeconds());
    hiveRequest->Record.SetReconnect(reconnect);
    ClientCache->Send(ctx, hive, hiveRequest.release());
}

void THiveProxyActor::SendUnlockRequest(
    const TActorContext& ctx, ui64 hive, ui64 tabletId)
{
    auto hiveRequest =
        std::make_unique<TEvHive::TEvUnlockTabletExecution>(tabletId);
    ClientCache->Send(ctx, hive, hiveRequest.release());
}

void THiveProxyActor::SendGetTabletStorageInfoRequest(
    const TActorContext& ctx,
    ui64 hive, ui64 tabletId)
{
    auto hiveRequest =
        std::make_unique<TEvHive::TEvGetTabletStorageInfo>(tabletId);
    ClientCache->Send(ctx, hive, hiveRequest.release());
}

void THiveProxyActor::SendLockReply(
    const TActorContext& ctx,
    TLockState* state,
    const NProto::TError& error)
{
    STORAGE_VERIFY(
        state->LockRequest,
        TWellKnownEntityTypes::TABLET,
        state->TabletId);
    auto response =
        std::make_unique<TEvHiveProxy::TEvLockTabletResponse>(error);
    NCloud::ReplyNoTrace(ctx, state->LockRequest, std::move(response));
    state->LockRequest.Drop();
}

void THiveProxyActor::SendUnlockReply(
    const TActorContext& ctx,
    TLockState* state,
    const NProto::TError& error)
{
    STORAGE_VERIFY(
        state->UnlockRequest,
        TWellKnownEntityTypes::TABLET,
        state->TabletId);
    auto response =
        std::make_unique<TEvHiveProxy::TEvUnlockTabletResponse>(error);
    NCloud::ReplyNoTrace(ctx, state->UnlockRequest, std::move(response));
    state->UnlockRequest.Drop();
}

void THiveProxyActor::SendLockLostNotification(
    const TActorContext& ctx,
    TLockState* state,
    const NProto::TError& error)
{
    NCloud::Send<TEvHiveProxy::TEvTabletLockLost>(
        ctx,
        state->Owner,
        state->Cookie,
        error,
        state->TabletId);
}

void THiveProxyActor::AddTabletMetrics(
    ui64 tabletId,
    const TTabletStats& tabletData,
    NKikimrHive::TEvTabletMetrics& record)
{
    auto& metrics = *record.AddTabletMetrics();
    metrics.SetTabletID(tabletId);
    if (tabletData.SlaveID != 0) {
        metrics.SetFollowerID(tabletData.SlaveID);
    }
    metrics.MutableResourceUsage()->MergeFrom(tabletData.ResourceValues);
}

void THiveProxyActor::ScheduleSendTabletMetrics(const TActorContext& ctx, ui64 hive)
{
    auto* state = HiveStates.FindPtr(hive);
    STORAGE_VERIFY(state, "Hive", hive);
    if (!state->ScheduledSendTabletMetrics) {
        ctx.Schedule(TDuration::MilliSeconds(BatchTimeout), new TEvHiveProxyPrivate::TEvSendTabletMetrics(hive));
        state->ScheduledSendTabletMetrics = true;
    }
}

void THiveProxyActor::SendTabletMetrics(
    const TActorContext& ctx,
    ui64 hive)
{
    auto* state = HiveStates.FindPtr(hive);
    STORAGE_VERIFY(state, "Hive", hive);
    state->ScheduledSendTabletMetrics = false;
    TAutoPtr<TEvHive::TEvTabletMetrics> event = MakeHolder<TEvHive::TEvTabletMetrics>();
    NKikimrHive::TEvTabletMetrics &record = event->Record;
    for (const auto &prTabletId : state->UpdatedTabletMetrics) {
        AddTabletMetrics(prTabletId.first, prTabletId.second, record);
    }
    if (record.TabletMetricsSize() > 0) {
        ClientCache->Send(ctx, hive, event.Release());
    }
}

////////////////////////////////////////////////////////////////////////////////

void THiveProxyActor::HandleConnect(
    TEvTabletPipe::TEvClientConnected::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    ui64 hive = msg->TabletId;
    if (!ClientCache->OnConnect(ev)) {
        // Connect to hive failed
        auto error = MakeKikimrError(msg->Status, TStringBuilder()
            << "Connect to hive " << hive << " failed");
        HandleConnectionError(ctx, error, hive, true);
    }
}

void THiveProxyActor::HandleDisconnect(
    TEvTabletPipe::TEvClientDestroyed::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    ui64 hive = msg->TabletId;
    ClientCache->OnDisconnect(ev);

    auto error = MakeError(E_REJECTED, TStringBuilder()
        << "Disconnected from hive " << hive);
    HandleConnectionError(ctx, error, hive, false);
}

void THiveProxyActor::HandleConnectionError(
    const TActorContext& ctx,
    const NProto::TError& error,
    ui64 hive,
    bool connectFailed)
{
    Y_UNUSED(error);
    Y_UNUSED(connectFailed);
    // Hive is a tablet, so it should eventually get up
    // Re-send all outstanding requests
    if (auto* states = HiveStates.FindPtr(hive)) {
        for (auto& kv : states->LockStates) {
            ui64 tabletId = kv.first;
            auto* state = &kv.second;
            if (state->Phase == PHASE_LOCKED) {
                // Link failed while locked, need to reconnect
                state->Phase = PHASE_RECONNECT;
            }
            if (state->Phase == PHASE_LOCKING ||
                state->Phase == PHASE_RECONNECT)
            {
                SendLockRequest(
                    ctx,
                    hive,
                    tabletId,
                    state->Phase == PHASE_RECONNECT);
            } else if (state->Phase == PHASE_UNLOCKING) {
                // Hive is a tablet, keep retrying requests
                SendUnlockRequest(ctx, hive, tabletId);
            }
        }

        for (auto& kv : states->GetInfoRequests) {
            ui64 tabletId = kv.first;
            auto& requests = kv.second;
            if (!requests.empty()) {
                SendGetTabletStorageInfoRequest(ctx, hive, tabletId);
            }
        }

        if (!states->ScheduledSendTabletMetrics) {
            SendTabletMetrics(ctx, hive);
        }

        for (const auto& actorId: states->Actors) {
            auto clientId = ClientCache->Prepare(ctx, hive);
            NCloud::Send<TEvHiveProxyPrivate::TEvChangeTabletClient>(
                ctx,
                actorId,
                0,
                clientId);
        }
    }
}

void THiveProxyActor::HandleLockTabletExecutionLost(
    const TEvHive::TEvLockTabletExecutionLost::TPtr& ev,
    const TActorContext& ctx)
{
    ui64 tabletId = ev->Get()->Record.GetTabletID();
    ui64 hive = GetHive(ctx, tabletId);
    auto* states = HiveStates.FindPtr(hive);
    auto* state = states ? states->LockStates.FindPtr(tabletId) : nullptr;
    if (!state || state->Phase == PHASE_LOCKING) {
        // Unexpected notification, ignore
        LOG_WARN_S(ctx, LogComponent,
            "Unexpected lock lost notification from hive " << hive
                << " for tablet " << tabletId);
        return;
    }

    if (state->Phase == PHASE_RECONNECT || state->Phase == PHASE_UNLOCKING) {
        // Ignore notification while in these states, since outgoing requests
        // would confirm the state of lock anyway.
        return;
    }

    STORAGE_VERIFY(
        state->Phase == PHASE_LOCKED,
        TWellKnownEntityTypes::TABLET,
        tabletId);
    SendLockLostNotification(ctx, state);
    states->LockStates.erase(tabletId);
}

bool THiveProxyActor::HandleRequests(STFUNC_SIG)
{
    switch (ev->GetTypeRewrite()) {
        STORAGE_HIVE_PROXY_REQUESTS(STORAGE_HANDLE_REQUEST, TEvHiveProxy)

        default:
            return false;
    }

    return true;
}

void THiveProxyActor::HandleRequestFinished(
    const TEvHiveProxyPrivate::TEvRequestFinished::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    ui64 hive = GetHive(ctx, msg->TabletId);

    if (auto* state = HiveStates.FindPtr(hive)) {
        state->Actors.erase(ev->Sender);
    }
}

void THiveProxyActor::HandleTabletMetrics(
    const TEvLocal::TEvTabletMetrics::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    const auto& metrics = msg->ResourceValues;
    ui64 hive = GetHive(ctx, msg->TabletId);
    auto* state = HiveStates.FindPtr(hive);
    STORAGE_VERIFY(state, "Hive", hive);
    auto& tabletData = state->UpdatedTabletMetrics[msg->TabletId];
    tabletData.SlaveID = msg->FollowerId;

    bool hasChanges = false;

    if (metrics.HasCPU()) {
        tabletData.ResourceValues.SetCPU(metrics.GetCPU());
        hasChanges = true;
    }
    if (metrics.HasMemory()) {
        tabletData.ResourceValues.SetMemory(metrics.GetMemory());
        hasChanges = true;
    }
    if (metrics.HasNetwork()) {
        tabletData.ResourceValues.SetNetwork(metrics.GetNetwork());
        hasChanges = true;
    }
    if (metrics.HasStorage()) {
        tabletData.ResourceValues.SetStorage(metrics.GetStorage());
        hasChanges = true;
    }
    if (metrics.GroupReadThroughputSize() > 0) {
        tabletData.ResourceValues.ClearGroupReadThroughput();
        for (const auto &v : metrics.GetGroupReadThroughput()) {
            tabletData.ResourceValues.AddGroupReadThroughput()->CopyFrom(v);
        }
        hasChanges = true;
    }
    if (metrics.GroupWriteThroughputSize() > 0) {
        tabletData.ResourceValues.ClearGroupWriteThroughput();
        for (const auto &v : metrics.GetGroupWriteThroughput()) {
            tabletData.ResourceValues.AddGroupWriteThroughput()->CopyFrom(v);
        }
        hasChanges = true;
    }
    if (metrics.GroupReadIopsSize() > 0) {
        tabletData.ResourceValues.ClearGroupReadIops();
        for (const auto &v : metrics.GetGroupReadIops()) {
            tabletData.ResourceValues.AddGroupReadIops()->CopyFrom(v);
        }
        hasChanges = true;
    }
    if (metrics.GroupWriteIopsSize() > 0) {
        tabletData.ResourceValues.ClearGroupWriteIops();
        for (const auto &v : metrics.GetGroupWriteIops()) {
            tabletData.ResourceValues.AddGroupWriteIops()->CopyFrom(v);
        }
        hasChanges = true;
    }

    if (!hasChanges) {
        return;
    }

    tabletData.OnStatsUpdate();

    if (!state->ScheduledSendTabletMetrics) {
        ScheduleSendTabletMetrics(ctx, hive);
    }
}

void THiveProxyActor::HandleSendTabletMetrics(
    const TEvHiveProxyPrivate::TEvSendTabletMetrics::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    SendTabletMetrics(ctx, msg->Hive);
}

void THiveProxyActor::HandleMetricsResponse(
    const TEvLocal::TEvTabletMetricsAck::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    auto size = msg->Record.TabletIdSize();
    Y_VERIFY_DEBUG(msg->Record.FollowerIdSize() == size);

    for (size_t i = 0; i < size; ++i) {
        ui64 hive = GetHive(ctx, msg->Record.GetTabletId(i));
        auto* state = HiveStates.FindPtr(hive);
        STORAGE_VERIFY(state, "Hive", hive);
        auto uit = state->UpdatedTabletMetrics.find(msg->Record.GetTabletId(i));
        if (uit != state->UpdatedTabletMetrics.end()) {
            uit->second.OnHiveAck();
            if (uit->second.IsEmpty()) {
                state->UpdatedTabletMetrics.erase(uit);
            } else {
                if (!state->ScheduledSendTabletMetrics) {
                    ScheduleSendTabletMetrics(ctx, hive);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(THiveProxyActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvTabletPipe::TEvClientConnected, HandleConnect);
        HFunc(TEvTabletPipe::TEvClientDestroyed, HandleDisconnect);
        HFunc(TEvHive::TEvLockTabletExecutionResult,
            HandleLockTabletExecutionResult);
        HFunc(TEvHive::TEvLockTabletExecutionLost,
            HandleLockTabletExecutionLost);
        HFunc(TEvHive::TEvUnlockTabletExecutionResult,
            HandleUnlockTabletExecutionResult);
        HFunc(TEvHive::TEvGetTabletStorageInfoRegistered,
            HandleGetTabletStorageInfoRegistered);
        HFunc(TEvHive::TEvGetTabletStorageInfoResult,
            HandleGetTabletStorageInfoResult);

        HFunc(TEvHive::TEvCreateTabletReply, HandleCreateTabletReply);
        HFunc(TEvHive::TEvTabletCreationResult, HandleTabletCreation);

        HFunc(TEvLocal::TEvTabletMetrics, HandleTabletMetrics)
        HFunc(TEvLocal::TEvTabletMetricsAck, HandleMetricsResponse)
        IgnoreFunc(TEvLocal::TEvReconnect);
        HFunc(TEvHiveProxyPrivate::TEvSendTabletMetrics, HandleSendTabletMetrics);

        HFunc(TEvHiveProxyPrivate::TEvRequestFinished, HandleRequestFinished);

        default:
            if (!HandleRequests(ev, ctx)) {
                HandleUnexpectedEvent(ctx, ev, LogComponent);
            }
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////

void THiveProxyActor::HandleSyncTabletBootInfoCache(
    const TEvHiveProxy::TEvSyncTabletBootInfoCacheRequest::TPtr& ev,
    const TActorContext& ctx)
{
    if (TabletBootInfoCache) {
        ctx.Send(ev->Forward(TabletBootInfoCache));
    } else {
        auto response =
            std::make_unique<TEvHiveProxy::TEvSyncTabletBootInfoCacheResponse>(
                MakeError(S_FALSE));
        NCloud::Reply(ctx, *ev, std::move(response));
    }
}

}   // namespace NCloud::NStorage
