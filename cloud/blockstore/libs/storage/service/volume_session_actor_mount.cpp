#include "volume_session_actor.h"

#include "service_actor.h"

#include <cloud/blockstore/libs/kikimr/trace.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/storage/api/ss_proxy.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/api/volume_proxy.h>
#include <cloud/blockstore/libs/storage/core/mount_token.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>
#include <cloud/blockstore/libs/storage/core/request_info.h>

#include <ydb/core/tablet/tablet_setup.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/datetime/base.h>
#include <util/generic/deque.h>
#include <util/generic/flags.h>
#include <util/generic/scope.h>
#include <util/string/builder.h>
#include <util/system/hostname.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

static constexpr ui32 InitialAddClientMultiplier = 5;

////////////////////////////////////////////////////////////////////////////////

inline TString AccessModeToString(const NProto::EVolumeAccessMode mode)
{
    switch (mode) {
        case NProto::VOLUME_ACCESS_READ_WRITE:
            return "read-write";
        case NProto::VOLUME_ACCESS_READ_ONLY:
            return "read-only";
        case NProto::VOLUME_ACCESS_REPAIR:
            return "repair";
        case NProto::VOLUME_ACCESS_USER_READ_ONLY:
            return "user-read-only";
        default:
            Y_VERIFY_DEBUG(false, "Unknown EVolumeAccessMode: %d", mode);
            return "undefined";
    }
}

////////////////////////////////////////////////////////////////////////////////

constexpr TDuration RemountDelayWarn = TDuration::MilliSeconds(300);

////////////////////////////////////////////////////////////////////////////////

struct TMountRequestParams
{
    ui64 MountStartTick = 0;
    TDuration InitialAddClientTimeout;

    TActorId SessionActorId;
    TActorId VolumeClient;

    NProto::EVolumeBinding BindingType = NProto::BINDING_REMOTE;
    NProto::EPreemptionSource PreemptionSource = NProto::SOURCE_NONE;

    bool IsLocalMounter = false;
    bool RejectOnAddClientTimeout = false;
};

////////////////////////////////////////////////////////////////////////////////

class TMountRequestActor final
    : public TActorBootstrapped<TMountRequestActor>
{
private:
    const TStorageConfigPtr Config;
    const TRequestInfoPtr RequestInfo;
    NProto::TMountVolumeRequest Request;
    const TString SessionId;
    bool AddClientRequestCompleted = false;

    // Most recent error code from suboperation (add client, start volume)
    NProto::TError Error;

    ui64 VolumeTabletId = 0;
    NProto::TVolume Volume;

    bool VolumeStarted = false;
    TMountRequestParams Params;
    NProto::EVolumeMountMode MountMode;
    const bool MountOptionsChanged;

public:
    TMountRequestActor(
        TStorageConfigPtr config,
        TRequestInfoPtr requestInfo,
        NProto::TMountVolumeRequest request,
        TString sessionId,
        const TMountRequestParams& params,
        bool mountOptionsChanged);

    void Bootstrap(const TActorContext& ctx);

private:
    void DescribeVolume(const TActorContext& ctx);

    void AddClient(const TActorContext& ctx, TDuration timeout);

    void RequestVolumeStart(const TActorContext& ctx);

    void RequestVolumeStop(const TActorContext& ctx);

    void NotifyAndDie(const TActorContext& ctx);

private:
    STFUNC(StateWork);

    void HandleDescribeVolumeResponse(
        const TEvSSProxy::TEvDescribeVolumeResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleDescribeVolumeError(
        const TActorContext& ctx,
        const NProto::TError& error);

    void HandleVolumeAddClientResponse(
        const TEvVolume::TEvAddClientResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleStartVolumeResponse(
        const TEvServicePrivate::TEvStartVolumeResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleStopVolumeResponse(
        const TEvServicePrivate::TEvStopVolumeResponse::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TMountRequestActor::TMountRequestActor(
        TStorageConfigPtr config,
        TRequestInfoPtr requestInfo,
        NProto::TMountVolumeRequest request,
        TString sessionId,
        const TMountRequestParams& params,
        bool mountOptionsChanged)
    : Config(std::move(config))
    , RequestInfo(std::move(requestInfo))
    , Request(std::move(request))
    , SessionId(std::move(sessionId))
    , Params(params)
    , MountOptionsChanged(mountOptionsChanged)
{
    ActivityType = TBlockStoreActivities::SERVICE;

    MountMode = Request.GetVolumeMountMode();
    if (Params.BindingType == NProto::BINDING_REMOTE) {
        MountMode = NProto::VOLUME_MOUNT_REMOTE;
    // XXX the following 'else if' seems correct but breaks test
    //} else if (Params.BindingType == TVolumeInfo::LOCAL) {
    //    MountMode = NProto::VOLUME_MOUNT_LOCAL;
    }
}

////////////////////////////////////////////////////////////////////////////////

void TMountRequestActor::Bootstrap(const TActorContext& ctx)
{
    Become(&TThis::StateWork);
    DescribeVolume(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TMountRequestActor::DescribeVolume(const TActorContext& ctx)
{
    const auto& diskId = Request.GetDiskId();

    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "Sending describe request for volume: %s",
        diskId.Quote().data());

    NCloud::Send(
        ctx,
        MakeSSProxyServiceId(),
        std::make_unique<TEvSSProxy::TEvDescribeVolumeRequest>(diskId));
}

////////////////////////////////////////////////////////////////////////////////

void TMountRequestActor::HandleDescribeVolumeResponse(
    const TEvSSProxy::TEvDescribeVolumeResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    const auto& error = msg->GetError();
    if (FAILED(error.GetCode())) {
        HandleDescribeVolumeError(ctx, error);
        return;
    }

    const auto& pathDescription = msg->PathDescription;
    // Verify assigned owner is correct
    const auto& volumeDescription =
        pathDescription.GetBlockStoreVolumeDescription();

    const auto& requestSource =
        Request.GetHeaders().GetInternal().GetRequestSource();
    if (IsDataChannel(requestSource)) {
        // Requests coming from data channel are authorized with mount token.
        TMountToken publicToken;
        auto parseStatus = publicToken.ParseString(
            volumeDescription.GetMountToken());

        if (parseStatus != TMountToken::EStatus::OK) {
            LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
                "Invalid mount token: %s",
                volumeDescription.GetMountToken().Quote().data());

            auto error = MakeError(E_FAIL, TStringBuilder()
                << "Cannot parse mount token (" << parseStatus << ")");
            HandleDescribeVolumeError(ctx, error);
            return;
        }

        const auto& mountSecret = Request.GetToken();

        if (!publicToken.VerifySecret(mountSecret)) {
            if (publicToken.Format != TMountToken::EFormat::EMPTY) {
                TMountToken current;

                current.SetSecret(
                    publicToken.Format,
                    mountSecret,
                    publicToken.Salt);

                LOG_WARN(ctx, TBlockStoreComponents::SERVICE,
                    "Invalid mount secret: %s. Expected: %s",
                    current.ToString().data(),
                    publicToken.ToString().data());
            } else {
                LOG_WARN(ctx, TBlockStoreComponents::SERVICE,
                    "Invalid mount secret. Expected empty secret");
            }

            auto error = MakeError(E_ARGUMENT, "Mount token verification failed");
            HandleDescribeVolumeError(ctx, error);
            return;
        }
    } else if (Request.GetToken()) {
        // Requests coming from control plane are authorized in Auth component
        // and so cannot also have mount token.
        auto error = MakeError(
            E_ARGUMENT,
            "Mount token is prohibited for authorized mount request");
        HandleDescribeVolumeError(ctx, error);
        return;
    }

    const auto& volumeConfig = volumeDescription.GetVolumeConfig();
    if (volumeConfig.GetEncryptionKeyHash() != Request.GetEncryptionKeyHash()) {
        auto error = MakeError(
            E_ARGUMENT,
            "Encryption key hash verification failed");
        HandleDescribeVolumeError(ctx, error);
        return;
    }

    VolumeTabletId = volumeDescription.GetVolumeTabletId();

    AddClient(ctx, Params.InitialAddClientTimeout);
}

////////////////////////////////////////////////////////////////////////////////

void TMountRequestActor::HandleDescribeVolumeError(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    const auto& diskId = Request.GetDiskId();

    LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
        "Describe request failed for volume %s: %s",
        diskId.Quote().data(),
        FormatError(error).data());

    Error = error;

    if (Params.IsLocalMounter) {
        RequestVolumeStop(ctx);
    } else {
        NotifyAndDie(ctx);
    }
}

////////////////////////////////////////////////////////////////////////////////

void TMountRequestActor::AddClient(const TActorContext& ctx, TDuration timeout)
{
    const auto& diskId = Request.GetDiskId();
    const auto& clientId = Request.GetHeaders().GetClientId();
    const auto accessMode = Request.GetVolumeAccessMode();
    const auto mountMode = Request.GetVolumeMountMode();
    const auto mountSeqNumber = Request.GetMountSeqNumber();

    auto request = std::make_unique<TEvVolume::TEvAddClientRequest>();
    request->Record.MutableHeaders()->SetClientId(clientId);
    request->Record.SetDiskId(diskId);
    request->Record.SetVolumeAccessMode(accessMode);
    request->Record.SetVolumeMountMode(mountMode);
    request->Record.SetMountFlags(Request.GetMountFlags());
    request->Record.SetMountSeqNumber(mountSeqNumber);
    request->Record.SetHost(FQDNHostName());

    auto requestInfo = CreateRequestInfo(
        SelfId(),
        RequestInfo->Cookie,
        RequestInfo->CallContext,
        RequestInfo->TraceId.Clone());

    NCloud::Register(ctx, CreateAddClientActor(
        std::move(request),
        std::move(requestInfo),
        timeout,
        Params.VolumeClient));
}

////////////////////////////////////////////////////////////////////////////////

void TMountRequestActor::RequestVolumeStart(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvServicePrivate::TEvStartVolumeRequest>(
        VolumeTabletId);

    NCloud::Send(ctx, Params.SessionActorId, std::move(request));
}

void TMountRequestActor::RequestVolumeStop(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvServicePrivate::TEvStopVolumeRequest>();

    NCloud::Send(ctx, Params.SessionActorId, std::move(request));
}

////////////////////////////////////////////////////////////////////////////////

void TMountRequestActor::NotifyAndDie(const TActorContext& ctx)
{
    if (MountOptionsChanged && Error.GetCode() == S_ALREADY) {
        Error = {};
    }

    using TNotification = TEvServicePrivate::TEvMountRequestProcessed;
    auto notification = std::make_unique<TNotification>(
        Error,
        Volume,
        std::move(Request),
        Params.MountStartTick,
        RequestInfo,
        VolumeTabletId,
        VolumeStarted,
        Params.BindingType,
        Params.PreemptionSource);

    NCloud::Send(ctx, Params.SessionActorId, std::move(notification));

    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TMountRequestActor::HandleVolumeAddClientResponse(
    const TEvVolume::TEvAddClientResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    const auto& error = msg->GetError();

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, ev->Get(), &ev->TraceId);

    Error = error;
    Volume = msg->Record.GetVolume();

    if (SUCCEEDED(error.GetCode())) {
        AddClientRequestCompleted = true;
        if (!VolumeStarted && MountMode == NProto::VOLUME_MOUNT_LOCAL) {
            RequestVolumeStart(ctx);
            return;
        }

        const bool mayStopVolume = Params.IsLocalMounter || VolumeStarted;
        if (mayStopVolume && MountMode == NProto::VOLUME_MOUNT_REMOTE) {
            RequestVolumeStop(ctx);
            return;
        }
    } else if (VolumeStarted) {
        RequestVolumeStop(ctx);
        return;
    } else if (error.GetCode() == E_REJECTED && !Params.RejectOnAddClientTimeout) {
        RequestVolumeStart(ctx);
        return;
    }

    NotifyAndDie(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TMountRequestActor::HandleStartVolumeResponse(
    const TEvServicePrivate::TEvStartVolumeResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto& msg = ev->Get();
    const auto& error = msg->GetError();
    const auto& mountMode = Request.GetVolumeMountMode();

    if (!AddClientRequestCompleted) {
        Volume = msg->VolumeInfo;
    }

    Error = error;
    if (SUCCEEDED(error.GetCode())) {
        VolumeStarted = true;
        if (AddClientRequestCompleted && mountMode == NProto::VOLUME_MOUNT_LOCAL) {
            NotifyAndDie(ctx);
            return;
        }
    } else {
        if (mountMode == NProto::VOLUME_MOUNT_LOCAL) {
            NotifyAndDie(ctx);
            return;
        }
    }

    AddClient(ctx, Config->GetLocalStartAddClientTimeout());
}

void TMountRequestActor::HandleStopVolumeResponse(
    const TEvServicePrivate::TEvStopVolumeResponse::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);
    NotifyAndDie(ctx);
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TMountRequestActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvSSProxy::TEvDescribeVolumeResponse, HandleDescribeVolumeResponse);

        HFunc(TEvVolume::TEvAddClientResponse, HandleVolumeAddClientResponse);

        HFunc(TEvServicePrivate::TEvStartVolumeResponse, HandleStartVolumeResponse);
        HFunc(TEvServicePrivate::TEvStopVolumeResponse, HandleStopVolumeResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TVolumeSessionActor::LogNewClient(
    const TActorContext& ctx,
    const TEvService::TEvMountVolumeRequest::TPtr& ev,
    ui64 tick)
{
    const auto* msg = ev->Get();
    const auto& diskId = GetDiskId(*msg);
    const auto& clientId = GetClientId(*msg);
    const auto& accessMode = msg->Record.GetVolumeAccessMode();
    const auto& mountMode = msg->Record.GetVolumeMountMode();
    const auto& mountSeqNumber = msg->Record.GetMountSeqNumber();

    LOG_INFO(ctx, TBlockStoreComponents::SERVICE,
        "Mounting volume: %s (client: %s, %s access, %s mount, throttling %s, seq_num %lu) ts: %lu",
        diskId.Quote().data(),
        clientId.Quote().data(),
        AccessModeToString(accessMode).c_str(),
        (mountMode == NProto::VOLUME_MOUNT_LOCAL ? "local" : "remote"),
        (IsThrottlingDisabled(msg->Record) ? "disabled" : "enabled"),
        mountSeqNumber,
        tick);
}

TVolumeSessionActor::TMountRequestProcResult TVolumeSessionActor::ProcessMountRequest(
    const TActorContext& ctx,
    const TEvService::TEvMountVolumeRequest::TPtr& ev,
    ui64 tick)
{
    const auto* msg = ev->Get();
    const auto& diskId = GetDiskId(*msg);
    const auto& clientId = GetClientId(*msg);
    const auto& accessMode = msg->Record.GetVolumeAccessMode();
    const auto& mountMode = msg->Record.GetVolumeMountMode();
    const auto& mountSeqNumber = msg->Record.GetMountSeqNumber();

    auto* clientInfo = VolumeInfo->GetClientInfo(clientId);
    if (!clientInfo) {
        LogNewClient(ctx, ev, tick);
        return {{}, false};
    }

    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "Mounting volume: %s "
        "(client: %s, %s access, %s mount, throttling %s, seq_num %lu, binding %i) ts: %lu",
        diskId.Quote().data(),
        clientId.Quote().data(),
        AccessModeToString(accessMode).c_str(),
        (mountMode == NProto::VOLUME_MOUNT_LOCAL ? "local" : "remote"),
        (IsThrottlingDisabled(msg->Record) ? "disabled" : "enabled"),
        mountSeqNumber,
        VolumeInfo->BindingType,
        tick);

    // TODO: check for duplicate requests
    auto prevActivityTime = clientInfo->LastActivityTime;
    clientInfo->LastActivityTime = ctx.Now();

    if (VolumeInfo->State == TVolumeInfo::INITIAL &&
        mountMode == NProto::VOLUME_MOUNT_LOCAL &&
        VolumeInfo->BindingType != NProto::BINDING_REMOTE)
    {
        // Volume tablet is not started but needs to be
        return {{}, false};
    }

    bool mountOptionsChanged = clientInfo->VolumeMountMode != mountMode;

    if (mountOptionsChanged) {
        LOG_INFO(ctx, TBlockStoreComponents::SERVICE,
            "Re-mounting volume with new options: %s"
            " (client: %s, %s access, %s mount, throttling %s, binding %i seqnum %u) ts: %lu",
            diskId.Quote().data(),
            clientId.Quote().data(),
            AccessModeToString(accessMode).c_str(),
            (mountMode == NProto::VOLUME_MOUNT_LOCAL ? "local" : "remote"),
            (IsThrottlingDisabled(msg->Record) ? "disabled" : "enabled"),
            VolumeInfo->BindingType,
            mountSeqNumber,
            tick
        );

        return {{}, true};
    }

    if (mountMode == NProto::VOLUME_MOUNT_LOCAL &&
        !StartVolumeActor &&
        VolumeInfo->BindingType != NProto::BINDING_REMOTE)
    {
        // Remount for locally mounted volume and volume has gone.
        // Need to return tablet back.
        return {{}, false};
    }

    if (mountSeqNumber != clientInfo->MountSeqNumber) {
        // If mountSeqNumber has changed in MountVolumeRequest from client
        // then let volume know about this
        return {{}, false};
    }

    if (clientInfo->LastMountTick < LastPipeResetTick) {
        // Pipe reset happened after last execution of AddClient
        // we need to run MountActor again and send AddClient
        // to update client information at volume.
        return {{}, false};
    }

    if (clientInfo->VolumeAccessMode != accessMode) {
        return {{}, false};
    }

    auto now = ctx.Now();
    auto remountPeriod = Config->GetClientRemountPeriod();
    if (prevActivityTime &&
        (now - prevActivityTime) > remountPeriod + RemountDelayWarn)
    {
        LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
            "Late ping from client %s for volume %s, no activity for %s",
            clientId.Quote().data(),
            diskId.Quote().data(),
            ToString(remountPeriod + RemountDelayWarn).data());
    }

    return {MakeError(S_ALREADY, "Volume already mounted"), false};
}

template <typename TProtoRequest>
void TVolumeSessionActor::AddClientToVolume(
    const TActorContext& ctx,
    const TProtoRequest& mountRequest,
    ui64 mountTick)
{
    auto* clientInfo = VolumeInfo->AddClientInfo(mountRequest.GetHeaders().GetClientId());
    clientInfo->VolumeAccessMode = mountRequest.GetVolumeAccessMode();
    clientInfo->VolumeMountMode = mountRequest.GetVolumeMountMode();
    clientInfo->MountFlags = mountRequest.GetMountFlags();
    clientInfo->IpcType = mountRequest.GetIpcType();
    clientInfo->LastActivityTime = ctx.Now();
    clientInfo->LastMountTick = mountTick;
    clientInfo->ClientVersionInfo = std::move(mountRequest.GetClientVersionInfo());
    clientInfo->MountSeqNumber = mountRequest.GetMountSeqNumber();
}

void TVolumeSessionActor::SendMountVolumeResponse(
    const TActorContext& ctx,
    const TRequestInfoPtr& requestInfo,
    const NProto::TError& error)
{
    auto response = std::make_unique<TEvService::TEvMountVolumeResponse>(error);

    if (!HasError(error)) {
        Y_VERIFY(VolumeInfo->VolumeInfo.Defined());

        response->Record.SetSessionId(VolumeInfo->SessionId);
        response->Record.MutableVolume()->CopyFrom(*VolumeInfo->VolumeInfo);
        response->Record.SetInactiveClientsTimeout(
            static_cast<ui32>(Config->GetClientRemountPeriod().MilliSeconds()));
        response->Record.SetServiceVersionInfo(Config->GetServiceVersionInfo());
    }

    BLOCKSTORE_TRACE_SENT(ctx, &requestInfo->TraceId, this, response);
    NCloud::Reply(ctx, *requestInfo, std::move(response));
}

void TVolumeSessionActor::HandleMountVolume(
    const TEvService::TEvMountVolumeRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();
    const auto& diskId = GetDiskId(*msg);
    const auto& clientId = GetClientId(*msg);

    BLOCKSTORE_TRACE_RECEIVED(ctx, &ev->TraceId, this, msg);

    if (MountRequestActor || UnmountRequestActor) {
        LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
            "Queuing mount volume %s by client %s request",
            diskId.Quote().data(),
            clientId.Quote().data());

        MountUnmountRequests.emplace(ev.Release());
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        ev->TraceId.Clone());

    auto tick = GetCycleCount();
    auto procResult = ProcessMountRequest(ctx, ev, tick);
    auto error = std::move(procResult.Error);

    auto bindingType = VolumeInfo->BindingType;
    bool isBindingChanging = ChangeBindingRequester && ev->Sender == SelfId();

    bool shouldReply = false;

    if (error.Defined() && FAILED(error->GetCode())) {
        shouldReply = true;
    }

    TClientInfo* clientInfo = nullptr;

    if (!shouldReply) {
        clientInfo = VolumeInfo->GetClientInfo(clientId);
        if (isBindingChanging &&
            (!clientInfo || VolumeInfo->GetLocalMountClientInfo() != clientInfo))
        {
            error = MakeError(E_ARGUMENT, "No local mounter found");
            shouldReply = true;
        } else {
            bindingType = VolumeInfo->OnMountStarted(
                *SharedCounters,
                (isBindingChanging) ? PreemptionSource : NProto::SOURCE_NONE,
                (isBindingChanging) ? BindingType : NProto::BINDING_NOT_SET,
                msg->Record.GetVolumeMountMode());
            shouldReply =
                (bindingType == VolumeInfo->BindingType) &&
                clientInfo &&
                !procResult.MountOptionsChanged;
            if (shouldReply) {
                VolumeInfo->OnMountCancelled(*SharedCounters);
            }
        }
    }

    if (shouldReply && error.Defined()) {
        SendMountVolumeResponse(
            ctx,
            requestInfo,
            *error);
        ReceiveNextMountOrUnmountRequest(ctx);
        return;
    }

    TDuration timeout = VolumeInfo->InitialAddClientTimeout;
    if (!timeout) {
        timeout = Config->GetInitialAddClientTimeout();
    }

    NProto::EPreemptionSource preemptionSource = NProto::SOURCE_NONE;
    if (isBindingChanging) {
        preemptionSource = PreemptionSource;
    }

    TMountRequestParams params {
        .MountStartTick = tick,
        .InitialAddClientTimeout = timeout,
        .SessionActorId = SelfId(),
        .VolumeClient = VolumeClient,
        .BindingType = bindingType,
        .PreemptionSource = preemptionSource,
        .IsLocalMounter = clientInfo &&
            clientInfo->VolumeMountMode == NProto::VOLUME_MOUNT_LOCAL,
        .RejectOnAddClientTimeout = Config->GetRejectMountOnAddClientTimeout()};

    MountRequestActor = NCloud::Register<TMountRequestActor>(
        ctx,
        Config,
        std::move(requestInfo),
        std::move(msg->Record),
        VolumeInfo->SessionId,
        params,
        procResult.MountOptionsChanged);
}

void TVolumeSessionActor::PostponeMountVolume(
    const TEvService::TEvMountVolumeRequest::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ctx);

    if (ShuttingDown) {
        const auto requestInfo = CreateRequestInfo(
            ev->Sender,
            ev->Cookie,
            ev->Get()->CallContext,
            ev->TraceId.Clone());

        SendMountVolumeResponse(
            ctx,
            requestInfo,
            Error);
        ReceiveNextMountOrUnmountRequest(ctx);
        return;
    }

    MountUnmountRequests.emplace(ev.Release());
}

void TVolumeSessionActor::HandleMountRequestProcessed(
    const TEvServicePrivate::TEvMountRequestProcessed::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    auto& mountRequest = msg->Request;
    const auto& diskId = mountRequest.GetDiskId();
    const auto& clientId = mountRequest.GetHeaders().GetClientId();
    const auto mountStartTick = msg->MountStartTick;
    const bool hadLocalStart = msg->HadLocalStart;

    LOG_INFO(ctx, TBlockStoreComponents::SERVICE,
        "Mount completed for client %s , volume %s, time %s, result (%u): %s",
        clientId.Quote().data(),
        diskId.Quote().data(),
        ToString(mountStartTick).Quote().data(),
        msg->GetStatus(),
        msg->GetError().GetMessage().Quote().data());

    VolumeInfo->TabletId = msg->VolumeTabletId;

    if (HasError(msg->GetError())) {
        VolumeInfo->RemoveClientInfo(clientId);
        VolumeInfo->OnMountFinished(
            *SharedCounters,
            msg->PreemptionSource,
            msg->BindingType,
            msg->GetError());

        SendMountVolumeResponse(
            ctx,
            msg->RequestInfo,
            msg->GetError());

        MountRequestActor = {};
        if (!MountUnmountRequests.empty()) {
            ReceiveNextMountOrUnmountRequest(ctx);
        } else if (!VolumeInfo->IsMounted()) {
            NotifyAndDie(ctx);
        }
        return;
    }

    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "Client %s is added to volume %s",
        clientId.Quote().data(),
        diskId.Quote().data());

    VolumeInfo->VolumeInfo = msg->Volume;

    AddClientToVolume(
        ctx,
        mountRequest,
        mountStartTick);

    VolumeInfo->OnMountFinished(
        *SharedCounters,
        msg->PreemptionSource,
        msg->BindingType,
        msg->GetError());

    if (hadLocalStart) {
        VolumeInfo->InitialAddClientTimeout =
            Config->GetInitialAddClientTimeout() * InitialAddClientMultiplier;
    }

    SendMountVolumeResponse(
        ctx,
        msg->RequestInfo,
        msg->GetError());

    ScheduleInactiveClientsRemoval(ctx);

    MountRequestActor = {};
    ReceiveNextMountOrUnmountRequest(ctx);
}

}   // namespace NCloud::NBlockStore::NStorage

