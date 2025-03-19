#include "volume_session_actor.h"

#include "service_actor.h"

#include <cloud/blockstore/libs/kikimr/trace.h>
#include <cloud/blockstore/libs/storage/api/ss_proxy.h>
#include <cloud/blockstore/libs/storage/core/mount_token.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>
#include <cloud/blockstore/libs/storage/core/request_info.h>

#include <ydb/core/tablet/tablet_setup.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/generic/deque.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

using EChangeBindingOp = TEvService::TEvChangeVolumeBindingRequest::EChangeBindingOp;

}  // namespace

////////////////////////////////////////////////////////////////////////////////

void TVolumeSessionActor::PostponeChangeVolumeBindingRequest(
    const TEvService::TEvChangeVolumeBindingRequest::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ctx);
    MountUnmountRequests.emplace(ev.Release());
}

void TVolumeSessionActor::HandleChangeVolumeBindingRequest(
    const TEvService::TEvChangeVolumeBindingRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    auto* localMounter = VolumeInfo->GetLocalMountClientInfo();

    NProto::TError error;

    NProto::EVolumeBinding bindingType = NProto::BINDING_NOT_SET;

    if (ChangeBindingRequester) {
        error = MakeError(E_REJECTED, "ChangeVolumeBinding already in progress");
    } else if (!localMounter) {
        error = MakeError(E_ARGUMENT, "No local mounter found");
    } else if (msg->Action == EChangeBindingOp::RELEASE_TO_HIVE) {
        if (VolumeInfo->BindingType == NProto::BINDING_REMOTE) {
            error = MakeError(S_ALREADY, "Volume is already moved from host");
        } else {
            bindingType = NProto::BINDING_REMOTE;
        }
    } else {
        if (VolumeInfo->BindingType == NProto::BINDING_LOCAL) {
            error = MakeError(S_ALREADY, "Volume is already running at host");
        } else {
            bindingType = NProto::BINDING_LOCAL;
        }
    }

    if (error.GetCode()) {
        using TResponse = TEvService::TEvChangeVolumeBindingResponse;
        auto response = std::make_unique<TResponse>(std::move(error));
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    // simulate mount request
    auto request = std::make_unique<TEvService::TEvMountVolumeRequest>();
    request->Record.SetDiskId(VolumeInfo->DiskId);
    request->Record.SetVolumeAccessMode(localMounter->VolumeAccessMode);
    request->Record.SetVolumeMountMode(localMounter->VolumeMountMode);
    request->Record.MutableHeaders()->SetClientId(localMounter->ClientId);

    ChangeBindingRequester = ev->Sender;
    BindingType = bindingType;
    PreemptionSource = msg->Source;

    using TEventType = TEvService::TEvMountVolumeRequest::TPtr::TValueType;
    TEvService::TEvMountVolumeRequest::TPtr event = static_cast<TEventType*>(new IEventHandle(
        SelfId(),
        SelfId(),
        request.release()));

    HandleMountVolume(event, ctx);
}

void TVolumeSessionActor::HandleMountResponseAfterBindingChange(
    const TEvService::TEvMountVolumeResponse::TPtr& ev,
    const TActorContext& ctx)
{
    if (!ChangeBindingRequester) {
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
            "Unexpected MountVolume response in session actor");
        return;
    }

    const auto* msg = ev->Get();

    auto response = std::make_unique<TEvService::TEvChangeVolumeBindingResponse>(
        msg->GetError(),
        VolumeInfo->DiskId);

    NCloud::Send(ctx, ChangeBindingRequester, std::move(response));
    ChangeBindingRequester = {};
}

}   // namespace NCloud::NBlockStore::NStorage
