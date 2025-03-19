#include "service_actor.h"
#include "service.h"

#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/storage/core/monitoring_utils.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;
using namespace NMonitoringUtils;

namespace {

////////////////////////////////////////////////////////////////////////////////

class THttpUnmountVolumeActor final
    : public TActorBootstrapped<THttpUnmountVolumeActor>
{
private:
    const TRequestInfoPtr RequestInfo;

    const TString DiskId;
    const TString ClientId;
    const TString SessionId;

public:
    THttpUnmountVolumeActor(
        TRequestInfoPtr requestInfo,
        TString diskId,
        TString clientId,
        TString sessionId);

    void Bootstrap(const TActorContext& ctx);

private:
    void Notify(
        const TActorContext& ctx,
        const TString& message,
        const EAlertLevel alertLevel);

private:
    STFUNC(StateWork);

    void HandleUnmountVolumeResponse(
        const TEvService::TEvUnmountVolumeResponse::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

THttpUnmountVolumeActor::THttpUnmountVolumeActor(
        TRequestInfoPtr requestInfo,
        TString diskId,
        TString clientId,
        TString sessionId)
    : RequestInfo(std::move(requestInfo))
    , DiskId(std::move(diskId))
    , ClientId(std::move(clientId))
    , SessionId(std::move(sessionId))
{
    ActivityType = TBlockStoreActivities::SERVICE;
}

void THttpUnmountVolumeActor::Bootstrap(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvService::TEvUnmountVolumeRequest>();
    auto& headers = *request->Record.MutableHeaders();
    headers.SetClientId(ClientId);
    headers.MutableInternal()->SetControlSource(NProto::SOURCE_SERVICE_MONITORING);
    request->Record.SetDiskId(DiskId);
    request->Record.SetSessionId(SessionId);

    NCloud::Send(
        ctx,
        MakeStorageServiceId(),
        std::move(request));

    Become(&TThis::StateWork);
}

void THttpUnmountVolumeActor::Notify(
    const TActorContext& ctx,
    const TString& message,
    const EAlertLevel alertLevel)
{
    TStringStream out;
    BuildNotifyPageWithRedirect(
        out,
        message,
        TStringBuilder()
            << "../blockstore/service?Volume=" << DiskId << "&action=listclients",
        alertLevel);

    auto response = std::make_unique<NMon::TEvHttpInfoRes>(out.Str());
    NCloud::Reply(ctx, *RequestInfo, std::move(response));
}

////////////////////////////////////////////////////////////////////////////////

void THttpUnmountVolumeActor::HandleUnmountVolumeResponse(
    const TEvService::TEvUnmountVolumeResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* response = ev->Get();

    if (SUCCEEDED(response->GetStatus())) {
        Notify(ctx, "Operation completed successfully", EAlertLevel::SUCCESS);
    } else {
        Notify(
            ctx,
            TStringBuilder() << "failed to unmount volume "
                << DiskId.Quote()
                << ": " << FormatError(response->GetError()),
            EAlertLevel::DANGER);
    }

    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(THttpUnmountVolumeActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvService::TEvUnmountVolumeResponse, HandleUnmountVolumeResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TServiceActor::HandleHttpInfo_Unmount(
    const NMon::TEvHttpInfo::TPtr& ev,
    const TString& diskId,
    const TString& clientId,
    const TActorContext& ctx)
{
    if (ev->Get()->Request.GetMethod() != HTTP_METHOD_POST) {
        RejectHttpRequest(ev->Sender, ctx);
        return;
    }

    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "Unmounting volume per action from monitoring page: volume %s, client %s",
        diskId.Quote().data(),
        clientId.Quote().data());

    auto volume = State.GetVolume(diskId);
    if (!volume) {
        TString error = TStringBuilder()
            << "Failed to unmount volume: volume " << diskId << " not found";
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE, error.data());

        TStringStream out;
        BuildNotifyPageWithRedirect(
            out,
            error,
            TStringBuilder() << "../blockstore/service",
            EAlertLevel::DANGER);

        auto response = std::make_unique<NMon::TEvHttpInfoRes>(out.Str());
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        MakeIntrusive<TCallContext>(),
        std::move(ev->TraceId));

    NCloud::Register<THttpUnmountVolumeActor>(
        ctx,
        std::move(requestInfo),
        diskId,
        clientId,
        volume->SessionId);
}

}   // namespace NCloud::NBlockStore::NStorage
