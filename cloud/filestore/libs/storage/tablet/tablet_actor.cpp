#include "tablet_actor.h"

#include <cloud/filestore/libs/diagnostics/storage_counters.h>

#include <cloud/storage/core/libs/api/hive_proxy.h>

#include <ydb/core/base/tablet_pipe.h>
#include <ydb/core/node_whiteboard/node_whiteboard.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NNodeWhiteboard;
using namespace NTabletFlatExecutor;

using namespace NCloud::NStorage;

////////////////////////////////////////////////////////////////////////////////

const TIndexTabletActor::TStateInfo TIndexTabletActor::States[STATE_MAX] = {
    { "Boot",   (IActor::TReceiveFunc)&TIndexTabletActor::StateBoot   },
    { "Init",   (IActor::TReceiveFunc)&TIndexTabletActor::StateInit   },
    { "Work",   (IActor::TReceiveFunc)&TIndexTabletActor::StateWork   },
    { "Zombie", (IActor::TReceiveFunc)&TIndexTabletActor::StateZombie },
    { "Broken", (IActor::TReceiveFunc)&TIndexTabletActor::StateBroken },
};

////////////////////////////////////////////////////////////////////////////////

TIndexTabletActor::TIndexTabletActor(
        const TActorId& owner,
        TTabletStorageInfoPtr storage,
        TStorageConfigPtr config,
        IProfileLogPtr profileLog,
        IStorageCountersPtr storageCounters)
    : TActor(&TThis::StateBoot)
    , TTabletBase(owner, std::move(storage))
    , Config(std::move(config))
    , ProfileLog(std::move(profileLog))
    , StorageCounters(std::move(storageCounters))
{
    ActivityType = TFileStoreActivities::TABLET;
}

TIndexTabletActor::~TIndexTabletActor()
{}

TString TIndexTabletActor::GetStateName(ui32 state)
{
    if (state < STATE_MAX) {
        return States[state].Name;
    }
    return "<unknown>";
}

void TIndexTabletActor::Enqueue(STFUNC_SIG)
{
    LOG_ERROR_S(ctx, TFileStoreComponents::TABLET,
        "[" << TabletID() << "]"
        << " IGNORING message type# " << ev->GetTypeRewrite()
        << " from Sender# " << ToString(ev->Sender)
        << " in StateBoot");
}

void TIndexTabletActor::DefaultSignalTabletActive(const TActorContext& ctx)
{
    Y_UNUSED(ctx);
}

void TIndexTabletActor::Suicide(const TActorContext& ctx)
{
    NCloud::Send(ctx, Tablet(), std::make_unique<TEvents::TEvPoisonPill>());
    BecomeAux(ctx, STATE_ZOMBIE);
}

void TIndexTabletActor::BecomeAux(const TActorContext& ctx, EState state)
{
    Y_VERIFY_DEBUG(state < STATE_MAX);

    Become(States[state].Func);
    CurrentState = state;

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] Switched to state %s (system: %s, user: %s, executor: %s)",
        TabletID(),
        States[state].Name.c_str(),
        ToString(Tablet()).c_str(),
        ToString(SelfId()).c_str(),
        ToString(ExecutorID()).c_str());

    ReportTabletState(ctx);
}

void TIndexTabletActor::ReportTabletState(const TActorContext& ctx)
{
    auto service = MakeNodeWhiteboardServiceId(SelfId().NodeId());

    auto request = std::make_unique<TEvWhiteboard::TEvTabletStateUpdate>(
        TabletID(),
        CurrentState);

    NCloud::Send(ctx, service, std::move(request));
}

void TIndexTabletActor::OnActivateExecutor(const TActorContext& ctx)
{
    BecomeAux(ctx, STATE_INIT);

    RegisterCounters(ctx);

    if (!Executor()->GetStats().IsFollower) {
        ExecuteTx<TInitSchema>(ctx);
    }
}

bool TIndexTabletActor::ReassignChannelsEnabled() const
{
    return true;
}

void TIndexTabletActor::ReassignDataChannelsIfNeeded(const NActors::TActorContext& ctx)
{
    auto channels = GetChannelsToMove();

    if (channels.empty()) {
        return;
    }

    if (ReassignRequestSentTs.GetValue()) {
        const auto timeout = TDuration::Minutes(1);
        if (ReassignRequestSentTs + timeout < ctx.Now()) {
            LOG_WARN(ctx, TFileStoreComponents::TABLET,
                "[%lu] No reaction to reassign request in %lu seconds, retrying",
                TabletID(),
                timeout.Seconds());
            ReassignRequestSentTs = TInstant::Zero();
        } else {
            return;
        }
    }

    {
        TStringBuilder sb;
        for (const auto channel: channels) {
            if (sb.Size()) {
                sb << ", ";
            }

            sb << channel;
        }

        LOG_WARN(ctx, TFileStoreComponents::TABLET,
            "[%lu] Reassign request sent for channels: %s",
            TabletID(),
            sb.c_str());
    }

    NCloud::Send<TEvHiveProxy::TEvReassignTabletRequest>(
        ctx,
        MakeHiveProxyServiceId(),
        0,  // cookie
        TabletID(),
        std::move(channels));

    ReassignRequestSentTs = ctx.Now();
}

bool TIndexTabletActor::OnRenderAppHtmlPage(
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

void TIndexTabletActor::OnDetach(const TActorContext& ctx)
{
    Counters = nullptr;

    Die(ctx);
}

void TIndexTabletActor::OnTabletDead(
    TEvTablet::TEvTabletDead::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    for (const auto& actor: WorkerActors) {
        ctx.Send(actor, new TEvents::TEvPoisonPill());
    }

    WorkerActors.clear();
    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

NProto::TError TIndexTabletActor::ValidateRangeRequest(TByteRange byteRange) const
{
    Y_UNUSED(byteRange);

    // TODO: should check request against the file size!
    return {};
}

NProto::TError TIndexTabletActor::ValidateDataRequest(TByteRange byteRange) const
{
    if (!byteRange.Length) {
        return MakeError(E_ARGUMENT, TStringBuilder() << "empty request");
    }

    if (byteRange.LastBlock() + 1 > MaxFileBlocks) {
        return ErrorFileTooBig();
    }

    return {};
}

////////////////////////////////////////////////////////////////////////////////

using TThresholds = TIndexTabletState::TBackpressureThresholds;
TThresholds TIndexTabletActor::BuildBackpressureThresholds() const
{
    return {
        Config->GetFlushThresholdForBackpressure(),
        Config->GetFlushBytesThresholdForBackpressure(),
        Config->GetCompactionThresholdForBackpressure(),
        Config->GetCleanupThresholdForBackpressure(),
    };
}

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    LOG_INFO(ctx, TFileStoreComponents::TABLET,
        "[%lu] Stop tablet because of PoisonPill request",
        TabletID());

    Suicide(ctx);
}

void TIndexTabletActor::HandleTabletMetrics(
    const TEvLocal::TEvTabletMetrics::TPtr& ev,
    const TActorContext& ctx)
{
    // TODO
    Y_UNUSED(ev);
    Y_UNUSED(ctx);
}

void TIndexTabletActor::HandleSessionDisconnected(
    const TEvTabletPipe::TEvServerDisconnected::TPtr& ev,
    const TActorContext& ctx)
{
    OrphanSession(ev->Sender, ctx.Now());
}

void TIndexTabletActor::HandleGetFileSystemConfig(
    const TEvIndexTablet::TEvGetFileSystemConfigRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto response = std::make_unique<TEvIndexTablet::TEvGetFileSystemConfigResponse>();
    response->Record.MutableConfig()->CopyFrom(CopyFrom(GetFileSystem()));

    NCloud::Reply(ctx, *ev, std::move(response));
}

bool TIndexTabletActor::HandleRequests(STFUNC_SIG)
{
    switch (ev->GetTypeRewrite()) {
        FILESTORE_SERVICE_REQUESTS_FWD(FILESTORE_HANDLE_REQUEST, TEvService)

        FILESTORE_TABLET_REQUESTS(FILESTORE_HANDLE_REQUEST, TEvIndexTablet)
        FILESTORE_TABLET_REQUESTS_PRIVATE(FILESTORE_HANDLE_REQUEST, TEvIndexTabletPrivate)

        default:
            return false;
    }

    return true;
}

bool TIndexTabletActor::RejectRequests(STFUNC_SIG)
{
    switch (ev->GetTypeRewrite()) {
        FILESTORE_SERVICE_REQUESTS_FWD(FILESTORE_REJECT_REQUEST, TEvService)

        FILESTORE_TABLET_REQUESTS(FILESTORE_REJECT_REQUEST, TEvIndexTablet)
        FILESTORE_TABLET_REQUESTS_PRIVATE(FILESTORE_REJECT_REQUEST, TEvIndexTabletPrivate)

        default:
            return false;
    }

    return true;
}

bool TIndexTabletActor::RejectRequestsByBrokenTablet(STFUNC_SIG)
{
    switch (ev->GetTypeRewrite()) {
        FILESTORE_SERVICE_REQUESTS_FWD(FILESTORE_REJECT_REQUEST_BY_BROKEN_TABLET, TEvService)

        FILESTORE_TABLET_REQUESTS(FILESTORE_REJECT_REQUEST_BY_BROKEN_TABLET, TEvIndexTablet)
        FILESTORE_TABLET_REQUESTS_PRIVATE(FILESTORE_REJECT_REQUEST_BY_BROKEN_TABLET, TEvIndexTabletPrivate)

        default:
            return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TIndexTabletActor::StateBoot)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        IgnoreFunc(TEvTabletPipe::TEvServerConnected);
        IgnoreFunc(TEvTabletPipe::TEvServerDisconnected);

        IgnoreFunc(TEvLocal::TEvTabletMetrics);
        IgnoreFunc(TEvIndexTabletPrivate::TEvUpdateCounters);

        IgnoreFunc(TEvHiveProxy::TEvReassignTabletResponse);

        FILESTORE_HANDLE_REQUEST(WaitReady, TEvIndexTablet)

        default:
            StateInitImpl(ev, ctx);
            break;
    }
}

STFUNC(TIndexTabletActor::StateInit)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        IgnoreFunc(TEvTabletPipe::TEvServerConnected);
        IgnoreFunc(TEvTabletPipe::TEvServerDisconnected);

        HFunc(TEvLocal::TEvTabletMetrics, HandleTabletMetrics);
        HFunc(TEvFileStore::TEvUpdateConfig, HandleUpdateConfig);
        HFunc(TEvIndexTabletPrivate::TEvUpdateCounters, HandleUpdateCounters);

        IgnoreFunc(TEvHiveProxy::TEvReassignTabletResponse);

        FILESTORE_HANDLE_REQUEST(WaitReady, TEvIndexTablet)

        default:
            if (!RejectRequests(ev, ctx) &&
                !HandleDefaultEvents(ev, ctx))
            {
                HandleUnexpectedEvent(ctx, ev, TFileStoreComponents::TABLET);
            }
            break;
    }
}

STFUNC(TIndexTabletActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        IgnoreFunc(TEvTabletPipe::TEvServerConnected);
        IgnoreFunc(TEvTabletPipe::TEvServerDisconnected);

        HFunc(TEvLocal::TEvTabletMetrics, HandleTabletMetrics);
        HFunc(TEvFileStore::TEvUpdateConfig, HandleUpdateConfig);

        HFunc(TEvIndexTabletPrivate::TEvUpdateCounters, HandleUpdateCounters);
        HFunc(TEvIndexTabletPrivate::TEvCleanupSessionsCompleted, HandleCleanupSessionsCompleted);
        HFunc(TEvIndexTabletPrivate::TEvReadDataCompleted, HandleReadDataCompleted);
        HFunc(TEvIndexTabletPrivate::TEvReadBlobCompleted, HandleReadBlobCompleted);
        HFunc(TEvIndexTabletPrivate::TEvWriteDataCompleted, HandleWriteDataCompleted);
        HFunc(TEvIndexTabletPrivate::TEvWriteBatchCompleted, HandleWriteBatchCompleted);
        HFunc(TEvIndexTabletPrivate::TEvWriteBlobCompleted, HandleWriteBlobCompleted);
        HFunc(TEvIndexTabletPrivate::TEvFlushCompleted, HandleFlushCompleted);
        HFunc(TEvIndexTabletPrivate::TEvFlushBytesCompleted, HandleFlushBytesCompleted);
        HFunc(TEvIndexTabletPrivate::TEvCompactionCompleted, HandleCompactionCompleted);
        HFunc(TEvIndexTabletPrivate::TEvCollectGarbageCompleted, HandleCollectGarbageCompleted);
        HFunc(TEvIndexTabletPrivate::TEvDestroyCheckpointCompleted, HandleDestroyCheckpointCompleted);

        // ignoring errors - will resend reassign request after a timeout anyway
        IgnoreFunc(TEvHiveProxy::TEvReassignTabletResponse);

        default:
            if (!HandleRequests(ev, ctx) &&
                !HandleDefaultEvents(ev, ctx))
            {
                HandleUnexpectedEvent(ctx, ev, TFileStoreComponents::TABLET);
            }
            break;
    }
}

STFUNC(TIndexTabletActor::StateZombie)
{
    switch (ev->GetTypeRewrite()) {
        IgnoreFunc(TEvents::TEvPoisonPill);

        HFunc(TEvTablet::TEvTabletDead, HandleTabletDead);

        IgnoreFunc(TEvTabletPipe::TEvServerConnected);
        HFunc(TEvTabletPipe::TEvServerDisconnected, HandleSessionDisconnected);

        IgnoreFunc(TEvLocal::TEvTabletMetrics);
        IgnoreFunc(TEvFileStore::TEvUpdateConfig);

        IgnoreFunc(TEvIndexTabletPrivate::TEvUpdateCounters);
        IgnoreFunc(TEvIndexTabletPrivate::TEvCleanupSessionsCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvReadDataCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvWriteDataCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvWriteBatchCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvReadBlobCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvWriteBlobCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvFlushCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvFlushBytesCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvCompactionCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvCollectGarbageCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvDestroyCheckpointCompleted);

        IgnoreFunc(TEvHiveProxy::TEvReassignTabletResponse);

        default:
            if (!RejectRequests(ev, ctx)) {
                HandleUnexpectedEvent(ctx, ev, TFileStoreComponents::TABLET);
            }
            break;
    }
}

STFUNC(TIndexTabletActor::StateBroken)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        HFunc(TEvTablet::TEvTabletDead, HandleTabletDead);

        IgnoreFunc(TEvTabletPipe::TEvServerConnected);
        HFunc(TEvTabletPipe::TEvServerDisconnected, HandleSessionDisconnected);

        IgnoreFunc(TEvLocal::TEvTabletMetrics);
        IgnoreFunc(TEvFileStore::TEvUpdateConfig);

        IgnoreFunc(TEvIndexTabletPrivate::TEvUpdateCounters);
        IgnoreFunc(TEvIndexTabletPrivate::TEvCleanupSessionsCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvReadDataCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvWriteDataCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvWriteBatchCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvReadBlobCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvWriteBlobCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvFlushCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvFlushBytesCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvCompactionCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvCollectGarbageCompleted);
        IgnoreFunc(TEvIndexTabletPrivate::TEvDestroyCheckpointCompleted);

        IgnoreFunc(TEvHiveProxy::TEvReassignTabletResponse);

        default:
            if (!RejectRequestsByBrokenTablet(ev, ctx)) {
                HandleUnexpectedEvent(ctx, ev, TFileStoreComponents::TABLET);
            }
            break;
    }
}

TString TIndexTabletActor::LogTag() const
{
    return Sprintf("[f:%s][t:%lu]",
        GetFileSystemId().Quote().c_str(),
        TabletID());
}

}   // namespace NCloud::NFileStore::NStorage
