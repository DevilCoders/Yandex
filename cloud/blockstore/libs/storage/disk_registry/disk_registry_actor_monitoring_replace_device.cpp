#include "disk_registry_actor.h"

#include <cloud/blockstore/libs/storage/core/monitoring_utils.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

using namespace NMonitoringUtils;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TReplaceActor final
    : public TActorBootstrapped<TReplaceActor>
{
private:
    const TActorId Owner;
    const ui64 TabletID;
    const TRequestInfoPtr RequestInfo;
    const TString DiskId;
    const TString DeviceId;

public:
    TReplaceActor(
        const TActorId& owner,
        ui64 tabletID,
        TRequestInfoPtr requestInfo,
        TString diskId,
        TString deviceId);

    void Bootstrap(const TActorContext& ctx);

private:
    void Notify(
        const TActorContext& ctx,
        const TString& message,
        const EAlertLevel alertLevel);

    void ReplyAndDie(const TActorContext& ctx, NProto::TError error);

private:
    STFUNC(StateWork);

    void HandleReplaceDeviceResponse(
        const TEvDiskRegistry::TEvReplaceDeviceResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandlePoisonPill(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TReplaceActor::TReplaceActor(
        const TActorId& owner,
        ui64 tabletID,
        TRequestInfoPtr requestInfo,
        TString diskId,
        TString deviceId)
    : Owner(owner)
    , TabletID(tabletID)
    , RequestInfo(std::move(requestInfo))
    , DiskId(std::move(diskId))
    , DeviceId(std::move(deviceId))
{
    ActivityType = TBlockStoreActivities::SERVICE;
}

void TReplaceActor::Bootstrap(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvDiskRegistry::TEvReplaceDeviceRequest>();

    request->Record.SetDiskId(DiskId);
    request->Record.SetDeviceUUID(DeviceId);

    NCloud::Send(
        ctx,
        Owner,
        std::move(request));

    Become(&TThis::StateWork);
}

void TReplaceActor::Notify(
    const TActorContext& ctx,
    const TString& message,
    const EAlertLevel alertLevel)
{
    TStringStream out;

    BuildNotifyPageWithRedirect(
        out,
        message,
        TStringBuilder() << "./app?action=disk&TabletID=" << TabletID
            << "&DiskID=" << DiskId,
        alertLevel);

    auto response = std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str());
    NCloud::Reply(ctx, *RequestInfo, std::move(response));
}

void TReplaceActor::ReplyAndDie(const TActorContext& ctx, NProto::TError error)
{
    if (SUCCEEDED(error.GetCode())) {
        Notify(ctx, "Operation completed successfully", EAlertLevel::SUCCESS);
    } else {
        Notify(
            ctx,
            TStringBuilder()
                << "failed to replace device "
                << DeviceId.Quote()
                << " for volume "
                << DiskId.Quote()
                << ": " << FormatError(error),
            EAlertLevel::DANGER);
    }

    NCloud::Send(
        ctx,
        Owner,
        std::make_unique<TEvDiskRegistryPrivate::TEvOperationCompleted>());

    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TReplaceActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);
    ReplyAndDie(ctx, MakeError(E_REJECTED, "Tablet is dead"));
}

void TReplaceActor::HandleReplaceDeviceResponse(
    const TEvDiskRegistry::TEvReplaceDeviceResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* response = ev->Get();

    ReplyAndDie(ctx, response->GetError());
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TReplaceActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        HFunc(
            TEvDiskRegistry::TEvReplaceDeviceResponse,
            HandleReplaceDeviceResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::DISK_REGISTRY_WORKER);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleHttpInfo_ReplaceDevice(
    const NMon::TEvRemoteHttpInfo::TPtr& ev,
    const TString& diskId,
    const TString& deviceId,
    const TActorContext& ctx)
{
    if (ev->Get()->GetMethod() != HTTP_METHOD_POST) {
        RejectHttpRequest(ev->Sender, ctx);
        return;
    }

    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "Replace device from monitoring page: volume %s, device %s",
        diskId.Quote().data(),
        deviceId.Quote().data());

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        MakeIntrusive<TCallContext>(),
        std::move(ev->TraceId));

    auto actor = NCloud::Register<TReplaceActor>(
        ctx,
        SelfId(),
        TabletID(),
        std::move(requestInfo),
        diskId,
        deviceId);

    Actors.insert(actor);
}

}   // namespace NCloud::NBlockStore::NStorage
