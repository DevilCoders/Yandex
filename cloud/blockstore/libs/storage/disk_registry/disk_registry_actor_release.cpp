#include "disk_registry_actor.h"

#include <cloud/blockstore/libs/storage/api/disk_agent.h>

#include <cloud/blockstore/libs/kikimr/events.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TReleaseDiskActor final
    : public TActorBootstrapped<TReleaseDiskActor>
{
private:
    const TActorId Owner;
    const TRequestInfoPtr RequestInfo;
    const TString DiskId;
    const TString SessionId;
    const TDuration RequestTimeout;

    TVector<NProto::TDeviceConfig> Devices;
    int PendingRequests = 0;

public:
    TReleaseDiskActor(
        const TActorId& owner,
        TRequestInfoPtr requestInfo,
        TString diskId,
        TString sessionId,
        TDuration requestTimeout,
        TVector<NProto::TDeviceConfig> devices);

    void Bootstrap(const TActorContext& ctx);

private:
    void RemoveDiskSession(const TActorContext& ctx);
    void ReplyAndDie(const TActorContext& ctx, NProto::TError error);

    void OnReleaseResponse(
        const TActorContext& ctx,
        ui64 cookie,
        NProto::TError error);

private:
    STFUNC(StateWork);

    void HandlePoisonPill(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx);

    void HandleReleaseDevicesResponse(
        const TEvDiskAgent::TEvReleaseDevicesResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleReleaseDevicesUndelivery(
        const TEvDiskAgent::TEvReleaseDevicesRequest::TPtr& ev,
        const TActorContext& ctx);

    void HandleRemoveDiskSessionResponse(
        const TEvDiskRegistryPrivate::TEvRemoveDiskSessionResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleTimeout(
        const TEvents::TEvWakeup::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TReleaseDiskActor::TReleaseDiskActor(
        const TActorId& owner,
        TRequestInfoPtr requestInfo,
        TString diskId,
        TString sessionId,
        TDuration requestTimeout,
        TVector<NProto::TDeviceConfig> devices)
    : Owner(owner)
    , RequestInfo(std::move(requestInfo))
    , DiskId(std::move(diskId))
    , SessionId(std::move(sessionId))
    , RequestTimeout(requestTimeout)
    , Devices(std::move(devices))
{
    ActivityType = TBlockStoreActivities::DISK_REGISTRY_WORKER;
}

void TReleaseDiskActor::Bootstrap(const TActorContext& ctx)
{
    Become(&TThis::StateWork);

    SortBy(Devices, [] (auto& d) {
        return d.GetNodeId();
    });

    auto it = Devices.begin();
    while (it != Devices.end()) {
        auto request = std::make_unique<TEvDiskAgent::TEvReleaseDevicesRequest>();
        request->Record.SetSessionId(SessionId);

        const ui32 nodeId = it->GetNodeId();

        for (; it != Devices.end() && it->GetNodeId() == nodeId; ++it) {
            *request->Record.AddDeviceUUIDs() = it->GetDeviceUUID();
        }

        ++PendingRequests;
        NCloud::Send(
            ctx,
            MakeDiskAgentServiceId(nodeId),
            std::move(request),
            nodeId);
    }

    ctx.Schedule(RequestTimeout, new TEvents::TEvWakeup());
}

void TReleaseDiskActor::RemoveDiskSession(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvDiskRegistryPrivate::TEvRemoveDiskSessionRequest>(
        DiskId, SessionId);

    NCloud::Send(ctx, Owner, std::move(request));
}

void TReleaseDiskActor::ReplyAndDie(const TActorContext& ctx, NProto::TError error)
{
    auto response = std::make_unique<TEvDiskRegistry::TEvReleaseDiskResponse>(
        std::move(error));
    NCloud::Reply(ctx, *RequestInfo, std::move(response));

    NCloud::Send(
        ctx,
        Owner,
        std::make_unique<TEvDiskRegistryPrivate::TEvOperationCompleted>());

    Die(ctx);
}

void TReleaseDiskActor::OnReleaseResponse(
    const TActorContext& ctx,
    ui64 cookie,
    NProto::TError error)
{
    Y_VERIFY(PendingRequests > 0);

    if (HasError(error)) {
        LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY_WORKER,
            "ReleaseDevices error: %s, %llu",
            FormatError(error).c_str(),
            cookie);
    }

    if (--PendingRequests == 0) {
        RemoveDiskSession(ctx);
    }
}

void TReleaseDiskActor::HandleReleaseDevicesResponse(
    const TEvDiskAgent::TEvReleaseDevicesResponse::TPtr& ev,
    const TActorContext& ctx)
{
    OnReleaseResponse(ctx, ev->Cookie, ev->Get()->GetError());
}

void TReleaseDiskActor::HandleReleaseDevicesUndelivery(
    const TEvDiskAgent::TEvReleaseDevicesRequest::TPtr& ev,
    const TActorContext& ctx)
{
    OnReleaseResponse(ctx, ev->Cookie, MakeError(E_REJECTED, "not delivered"));
}

void TReleaseDiskActor::HandleRemoveDiskSessionResponse(
    const TEvDiskRegistryPrivate::TEvRemoveDiskSessionResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    ReplyAndDie(ctx, msg->GetError());
}

void TReleaseDiskActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    ReplyAndDie(ctx, MakeError(E_REJECTED, "Tablet is dead"));
}

void TReleaseDiskActor::HandleTimeout(
    const TEvents::TEvWakeup::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    const auto err = TStringBuilder()
        << "TReleaseDiskActor timeout."
        << " DiskId: " << DiskId
        << " SessionId: " << SessionId
        << " PendingRequests: " << PendingRequests;

    LOG_WARN(ctx, TBlockStoreComponents::DISK_REGISTRY_WORKER, err);

    ReplyAndDie(ctx, MakeError(E_TIMEOUT, err));
}

STFUNC(TReleaseDiskActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);
        HFunc(TEvents::TEvWakeup, HandleTimeout);

        HFunc(TEvDiskAgent::TEvReleaseDevicesResponse,
            HandleReleaseDevicesResponse);
        HFunc(TEvDiskAgent::TEvReleaseDevicesRequest,
            HandleReleaseDevicesUndelivery);

        HFunc(TEvDiskRegistryPrivate::TEvRemoveDiskSessionResponse,
            HandleRemoveDiskSessionResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::DISK_REGISTRY_WORKER);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleReleaseDisk(
    const TEvDiskRegistry::TEvReleaseDiskRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(ReleaseDisk);

    auto replyWithError = [&] (auto error) {
        auto response = std::make_unique<TEvDiskRegistry::TEvReleaseDiskResponse>(
            std::move(error));
        NCloud::Reply(ctx, *ev, std::move(response));
    };

    auto* msg = ev->Get();
    TString& diskId = *msg->Record.MutableDiskId();
    TString& sessionId = *msg->Record.MutableSessionId();

    LOG_DEBUG(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received ReleaseDisk request: DiskId=%s, SessionId=%s",
        TabletID(),
        diskId.c_str(),
        sessionId.c_str());

    if (!sessionId) {
        replyWithError(MakeError(E_ARGUMENT, "empty session id"));
        return;
    }

    if (!diskId) {
        replyWithError(MakeError(E_ARGUMENT, "empty disk id"));
        return;
    }

    TDiskInfo diskInfo;
    const auto error = State->GetDiskInfo(diskId, diskInfo);
    if (HasError(error)) {
        LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "ReleaseDisk %s. GetDiskInfo error: %s",
            diskId.c_str(),
            FormatError(error).c_str());

        replyWithError(error);
        return;
    }

    if (!State->FilterDevicesAtUnavailableAgents(diskInfo)) {
        LOG_WARN(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "ReleaseDisk %s. Nothing to release",
            diskId.c_str());

        replyWithError(MakeError(S_ALREADY, {}));
        return;
    }

    TVector<NProto::TDeviceConfig> devices = std::move(diskInfo.Devices);
    for (auto& migration: diskInfo.Migrations) {
        devices.push_back(std::move(*migration.MutableTargetDevice()));
    }
    for (auto& replica: diskInfo.Replicas) {
        for (auto& device: replica) {
            devices.push_back(std::move(device));
        }
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    auto actor = NCloud::Register<TReleaseDiskActor>(
        ctx,
        ctx.SelfID,
        std::move(requestInfo),
        std::move(diskId),
        std::move(sessionId),
        Config->GetAgentRequestTimeout(),
        std::move(devices));

    Actors.insert(actor);
}

void TDiskRegistryActor::HandleRemoveDiskSession(
    const TEvDiskRegistryPrivate::TEvRemoveDiskSessionRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    State->FinishAcquireDisk(msg->DiskId);
    auto response =
        std::make_unique<TEvDiskRegistryPrivate::TEvRemoveDiskSessionResponse>();
    NCloud::Reply(ctx, *ev, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage
