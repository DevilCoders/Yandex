#include "volume_actor.h"

#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/storage/api/volume_proxy.h>
#include <cloud/blockstore/libs/storage/core/monitoring_utils.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;
using namespace NMonitoringUtils;

namespace {

////////////////////////////////////////////////////////////////////////////////

class THttpRemoveClientActor final
    : public TActorBootstrapped<THttpRemoveClientActor>
{
private:
    const TRequestInfoPtr RequestInfo;

    const TString DiskId;
    const TString ClientId;
    const ui64 TabletId;

public:
    THttpRemoveClientActor(
        TRequestInfoPtr requestInfo,
        TString diskId,
        TString clientId,
        ui64 tabletId);

    void Bootstrap(const TActorContext& ctx);

private:
    void Notify(
        const TActorContext& ctx,
        const TString& message,
        const EAlertLevel alertLevel);

private:
    STFUNC(StateWork);

    void HandleRemoveClientResponse(
        const TEvVolume::TEvRemoveClientResponse::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

THttpRemoveClientActor::THttpRemoveClientActor(
        TRequestInfoPtr requestInfo,
        TString diskId,
        TString clientId,
        ui64 tabletId)
    : RequestInfo(std::move(requestInfo))
    , DiskId(std::move(diskId))
    , ClientId(std::move(clientId))
    , TabletId(tabletId)
{
    ActivityType = TBlockStoreActivities::VOLUME;
}

void THttpRemoveClientActor::Bootstrap(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvVolume::TEvRemoveClientRequest>();
    request->Record.SetDiskId(DiskId);
    request->Record.MutableHeaders()->SetClientId(ClientId);
    request->Record.SetIsMonRequest(true);

    NCloud::Send(
        ctx,
        MakeVolumeProxyServiceId(),
        std::move(request));

    Become(&TThis::StateWork);
}

void THttpRemoveClientActor::Notify(
    const TActorContext& ctx,
    const TString& message,
    const EAlertLevel alertLevel)
{
    TStringStream out;
    BuildTabletNotifyPageWithRedirect(out, message, TabletId, alertLevel);

    auto response = std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str());
    NCloud::Reply(ctx, *RequestInfo, std::move(response));
}

////////////////////////////////////////////////////////////////////////////////

void THttpRemoveClientActor::HandleRemoveClientResponse(
    const TEvVolume::TEvRemoveClientResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* response = ev->Get();

    if (SUCCEEDED(response->GetStatus())) {
        Notify(ctx, "Operation completed successfully", EAlertLevel::SUCCESS);
    } else {
        Notify(
            ctx,
            TStringBuilder() << "failed to remove client "
                << ClientId.Quote() << " from volume "
                << DiskId.Quote() << ": "
                << FormatError(response->GetError()),
            EAlertLevel::DANGER);
    }

    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(THttpRemoveClientActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvVolume::TEvRemoveClientResponse, HandleRemoveClientResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::VOLUME);
            break;
    }
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::HandleHttpInfo_RemoveClient(
    const NMon::TEvRemoteHttpInfo::TPtr& ev,
    const TString& clientId,
    const TActorContext& ctx)
{
    if (!State) {
        TStringStream out;
        TString message = "Unable to remove client: volume state is not initialized";
        BuildTabletNotifyPageWithRedirect(
            out,
            message,
            TabletID(),
            EAlertLevel::DANGER);

        NCloud::Reply(
            ctx,
            *ev,
            std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str()));
        return;
    }

    if (ev->Get()->GetMethod() != HTTP_METHOD_POST) {
        RejectHttpRequest(ev->Sender, ctx);
        return;
    }

    const auto& diskId = State->GetDiskId();

    LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Removing volume client per action from monitoring page: volume %s, client %s",
        TabletID(),
        diskId.Quote().data(),
        clientId.Quote().data());

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        MakeIntrusive<TCallContext>(),
        std::move(ev->TraceId));

    NCloud::Register<THttpRemoveClientActor>(
        ctx,
        std::move(requestInfo),
        diskId,
        clientId,
        TabletID());
}

}   // namespace NCloud::NBlockStore::NStorage
