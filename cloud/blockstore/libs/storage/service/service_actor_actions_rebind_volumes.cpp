#include "service_actor.h"

#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

#include <cloud/blockstore/private/api/protos/volume.pb.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>
#include <library/cpp/actors/core/events.h>
#include <library/cpp/actors/core/hfunc.h>
#include <library/cpp/actors/core/log.h>

#include <google/protobuf/util/json_util.h>

#include <util/string/printf.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER)

namespace {

////////////////////////////////////////////////////////////////////////////////

class TRebindVolumesActor final
    : public TActorBootstrapped<TRebindVolumesActor>
{
private:
    const TRequestInfoPtr RequestInfo;
    const TString Input;
    const TVector<TString> DiskIds;
    ui32 Responses = 0;

    NProto::TError Error;

public:
    TRebindVolumesActor(
        TRequestInfoPtr requestInfo,
        TString input,
        TVector<TString> diskIds);

    void Bootstrap(const TActorContext& ctx);

private:
    void ReplyAndDie(const TActorContext& ctx);

private:
    STFUNC(StateWork);

    void HandleChangeVolumeBindingResponse(
        const TEvService::TEvChangeVolumeBindingResponse::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TRebindVolumesActor::TRebindVolumesActor(
        TRequestInfoPtr requestInfo,
        TString input,
        TVector<TString> diskIds)
    : RequestInfo(std::move(requestInfo))
    , Input(std::move(input))
    , DiskIds(std::move(diskIds))
{
    ActivityType = TBlockStoreActivities::SERVICE;
}

void TRebindVolumesActor::Bootstrap(const TActorContext& ctx)
{
    NPrivateProto::TRebindVolumesRequest rebindRequest;
    if (!google::protobuf::util::JsonStringToMessage(Input, &rebindRequest).ok()) {
        Error = MakeError(E_ARGUMENT, "Failed to parse input");
        ReplyAndDie(ctx);
        return;
    }

    if (rebindRequest.GetBinding() > TVolumeInfo::MAX_VOLUME_BINDING) {
        Error = MakeError(
            E_ARGUMENT,
            TStringBuilder() << "Bad binding: " << rebindRequest.GetBinding()
        );
        ReplyAndDie(ctx);
        return;
    }

    const auto binding =
        static_cast<NProto::EVolumeBinding>(rebindRequest.GetBinding());

    LOG_INFO(ctx, TBlockStoreComponents::SERVICE,
        "Rebinding %u disks, binding: %u",
        DiskIds.size(),
        rebindRequest.GetBinding());

    if (DiskIds.empty()) {
        Error = MakeError(S_ALREADY, "No disks - nothing to do");
        ReplyAndDie(ctx);
        return;
    }

    if (binding == NProto::BINDING_NOT_SET) {
        Error = MakeError(E_ARGUMENT, "Binding is not set");
        ReplyAndDie(ctx);
        return;
    }

    Become(&TThis::StateWork);

    for (const auto& diskId: DiskIds) {
        using TRequest = TEvService::TEvChangeVolumeBindingRequest;

        auto op = TRequest::EChangeBindingOp::ACQUIRE_FROM_HIVE;
        if (binding == NProto::BINDING_REMOTE) {
            op = TRequest::EChangeBindingOp::RELEASE_TO_HIVE;
        }

        auto request = std::make_unique<TEvService::TEvChangeVolumeBindingRequest>(
            diskId,
            op,
            NProto::EPreemptionSource::SOURCE_MANUAL
        );

        NCloud::Send(ctx, MakeStorageServiceId(), std::move(request));
    }
}

void TRebindVolumesActor::ReplyAndDie(const TActorContext& ctx)
{
    auto response = std::make_unique<TEvService::TEvExecuteActionResponse>(Error);
    google::protobuf::util::MessageToJsonString(
        NPrivateProto::TRebindVolumesResponse(),
        response->Record.MutableOutput()
    );

    LWTRACK(
        ResponseSent_Service,
        RequestInfo->CallContext->LWOrbit,
        "ExecuteAction_rebindvolumes",
        RequestInfo->CallContext->RequestId);

    NCloud::Reply(ctx, *RequestInfo, std::move(response));
    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TRebindVolumesActor::HandleChangeVolumeBindingResponse(
    const TEvService::TEvChangeVolumeBindingResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    if (HasError(msg->GetError())) {
        // RebindVolumes request should always succeed, but we should collect
        // error messages for debugging purposes.

        const auto formattedError = FormatError(msg->GetError());

        LOG_WARN(ctx, TBlockStoreComponents::SERVICE,
            "Failed to rebind disk: %s",
            formattedError.Quote().c_str());

        TStringBuilder message;
        message << Error.GetMessage();
        if (message) {
            message << "; ";
        }

        message << formattedError;

        Error.SetMessage(std::move(message));
    }

    if (++Responses == DiskIds.size()) {
        LOG_INFO(ctx, TBlockStoreComponents::SERVICE,
            "Rebound %u disks", DiskIds.size());

        ReplyAndDie(ctx);
    }
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TRebindVolumesActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(
            TEvService::TEvChangeVolumeBindingResponse,
            HandleChangeVolumeBindingResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IActorPtr TServiceActor::CreateRebindVolumesActionActor(
    TRequestInfoPtr requestInfo,
    TString input)
{
    TVector<TString> localDiskIds;
    for (const auto& x: State.GetVolumes()) {
        if (x.second->IsLocallyMounted()) {
            localDiskIds.push_back(x.first);
        }
    }

    return std::make_unique<TRebindVolumesActor>(
        std::move(requestInfo),
        std::move(input),
        std::move(localDiskIds)
    );
}

}   // namespace NCloud::NBlockStore::NStorage
