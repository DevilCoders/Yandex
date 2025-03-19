#include "service_actor.h"

#include <cloud/blockstore/libs/storage/api/ss_proxy.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/api/volume_proxy.h>
#include <cloud/blockstore/libs/storage/core/probes.h>
#include <cloud/blockstore/private/api/protos/volume.pb.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>
#include <library/cpp/actors/core/events.h>
#include <library/cpp/actors/core/hfunc.h>
#include <library/cpp/actors/core/log.h>

#include <google/protobuf/util/json_util.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER)

namespace {

////////////////////////////////////////////////////////////////////////////////

class TRebaseVolumeActionActor final
    : public TActorBootstrapped<TRebaseVolumeActionActor>
{
private:
    const TRequestInfoPtr RequestInfo;
    const TString Input;

    NPrivateProto::TRebaseVolumeRequest Request;
    NKikimrBlockStore::TVolumeConfig VolumeConfig;
    NProto::TError Error;

public:
    TRebaseVolumeActionActor(
        TRequestInfoPtr requestInfo,
        TString input);

    void Bootstrap(const TActorContext& ctx);

private:
    void DescribeVolume(const TActorContext& ctx);
    void AlterVolume(
        const TActorContext& ctx,
        const TString& path,
        ui64 pathId,
        ui64 version);
    void WaitReady(const TActorContext& ctx);
    void ReplyAndDie(const TActorContext& ctx, NProto::TError error);

private:
    STFUNC(StateDescribeVolume);
    STFUNC(StateAlterVolume);
    STFUNC(StateWaitReady);

    void HandleDescribeVolumeResponse(
        const TEvSSProxy::TEvDescribeVolumeResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleAlterVolumeResponse(
        const TEvSSProxy::TEvModifySchemeResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleWaitReadyResponse(
        const TEvVolume::TEvWaitReadyResponse::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TRebaseVolumeActionActor::TRebaseVolumeActionActor(
        TRequestInfoPtr requestInfo,
        TString input)
    : RequestInfo(std::move(requestInfo))
    , Input(std::move(input))
{
    ActivityType = TBlockStoreActivities::SERVICE;
}

void TRebaseVolumeActionActor::Bootstrap(const TActorContext& ctx)
{
    if (!google::protobuf::util::JsonStringToMessage(Input, &Request).ok()) {
        ReplyAndDie(ctx, MakeError(E_ARGUMENT, "Failed to parse input"));
        return;
    }

    if (!Request.GetDiskId()) {
        ReplyAndDie(ctx, MakeError(E_ARGUMENT, "DiskId should be supplied"));
        return;
    }

    if (!Request.GetTargetBaseDiskId()) {
        ReplyAndDie(
            ctx,
            MakeError(E_ARGUMENT, "TargetBaseDiskId should be supplied")
        );
        return;
    }

    if (!Request.GetConfigVersion()) {
        ReplyAndDie(
            ctx,
            MakeError(E_ARGUMENT, "ConfigVersion should be supplied")
        );
        return;
    }

    VolumeConfig.SetDiskId(Request.GetDiskId());
    VolumeConfig.SetBaseDiskId(Request.GetTargetBaseDiskId());
    VolumeConfig.SetVersion(Request.GetConfigVersion());
    DescribeVolume(ctx);
}

void TRebaseVolumeActionActor::DescribeVolume(const TActorContext& ctx)
{
    Become(&TThis::StateDescribeVolume);

    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "Sending describe request for volume %s",
        Request.GetDiskId().Quote().c_str());

    NCloud::Send(
        ctx,
        MakeSSProxyServiceId(),
        std::make_unique<TEvSSProxy::TEvDescribeVolumeRequest>(Request.GetDiskId()));
}

void TRebaseVolumeActionActor::AlterVolume(
    const TActorContext& ctx,
    const TString& path,
    ui64 pathId,
    ui64 version)
{
    Become(&TThis::StateAlterVolume);

    TString volumeDir;
    TString volumeName;

    {
        TStringBuf dir;
        TStringBuf name;
        TStringBuf(path).RSplit('/', dir, name);
        volumeDir = TString{dir};
        volumeName = TString{name};
    }

    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "Sending RebaseVolume->Alter request for %s in directory %s",
        volumeName.Quote().c_str(),
        volumeDir.Quote().c_str());

    NKikimrSchemeOp::TModifyScheme modifyScheme;
    modifyScheme.SetWorkingDir(volumeDir);
    modifyScheme.SetOperationType(
        NKikimrSchemeOp::ESchemeOpAlterBlockStoreVolume);

    auto* op = modifyScheme.MutableAlterBlockStoreVolume();
    op->SetName(volumeName);

    op->MutableVolumeConfig()->CopyFrom(VolumeConfig);
    auto* applyIf = modifyScheme.MutableApplyIf()->Add();
    applyIf->SetPathId(pathId);
    applyIf->SetPathVersion(version);

    auto request = std::make_unique<TEvSSProxy::TEvModifySchemeRequest>(
        std::move(modifyScheme));
    NCloud::Send(ctx, MakeSSProxyServiceId(), std::move(request));
}

void TRebaseVolumeActionActor::WaitReady(const TActorContext& ctx)
{
    Become(&TThis::StateWaitReady);

    auto request = std::make_unique<TEvVolume::TEvWaitReadyRequest>();
    request->Record.SetDiskId(Request.GetDiskId());

    NCloud::Send(
        ctx,
        MakeVolumeProxyServiceId(),
        std::move(request)
    );
}

void TRebaseVolumeActionActor::ReplyAndDie(
    const TActorContext& ctx,
    NProto::TError error)
{
    auto response =
        std::make_unique<TEvService::TEvExecuteActionResponse>(std::move(error));
    google::protobuf::util::MessageToJsonString(
        NPrivateProto::TRebaseVolumeResponse(),
        response->Record.MutableOutput()
    );

    LWTRACK(
        ResponseSent_Service,
        RequestInfo->CallContext->LWOrbit,
        "ExecuteAction_rebasevolume",
        RequestInfo->CallContext->RequestId);

    NCloud::Reply(ctx, *RequestInfo, std::move(response));
    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TRebaseVolumeActionActor::HandleDescribeVolumeResponse(
    const TEvSSProxy::TEvDescribeVolumeResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    NProto::TError error = msg->GetError();
    if (FAILED(error.GetCode())) {
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
            "Volume %s: describe failed: %s",
            Request.GetDiskId().Quote().c_str(),
            FormatError(error).c_str());
        ReplyAndDie(ctx, std::move(error));
        return;
    }

    AlterVolume(
        ctx,
        msg->Path,
        msg->PathDescription.GetSelf().GetPathId(),
        msg->PathDescription.GetSelf().GetPathVersion());
}

void TRebaseVolumeActionActor::HandleAlterVolumeResponse(
    const TEvSSProxy::TEvModifySchemeResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    NProto::TError error = msg->GetError();
    ui32 errorCode = error.GetCode();

    if (FAILED(errorCode)) {
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
            "RebaseVolume->Alter of volume %s failed: %s",
            Request.GetDiskId().Quote().c_str(),
            msg->GetErrorReason().c_str());

        ReplyAndDie(ctx, std::move(error));
        return;
    }

    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "Sending WaitReady request to volume %s",
        Request.GetDiskId().Quote().c_str());

    WaitReady(ctx);
}

void TRebaseVolumeActionActor::HandleWaitReadyResponse(
    const TEvVolume::TEvWaitReadyResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    NProto::TError error = msg->GetError();

    if (HasError(error)) {
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
            "RebaseVolume->WaitReady request failed for volume %s, error: %s",
            Request.GetDiskId().Quote().c_str(),
            msg->GetErrorReason().Quote().c_str());
    } else {
        LOG_INFO(ctx, TBlockStoreComponents::SERVICE,
            "Successfully done RebaseVolume with TargetBaseDiskId %s for volume %s",
            Request.GetTargetBaseDiskId().Quote().c_str(),
            Request.GetDiskId().Quote().c_str());
    }

    ReplyAndDie(ctx, std::move(error));
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TRebaseVolumeActionActor::StateDescribeVolume)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvSSProxy::TEvDescribeVolumeResponse, HandleDescribeVolumeResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            break;
    }
}

STFUNC(TRebaseVolumeActionActor::StateAlterVolume)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvSSProxy::TEvModifySchemeResponse, HandleAlterVolumeResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            break;
    }
}

STFUNC(TRebaseVolumeActionActor::StateWaitReady)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvVolume::TEvWaitReadyResponse, HandleWaitReadyResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IActorPtr TServiceActor::CreateRebaseVolumeActionActor(
    TRequestInfoPtr requestInfo,
    TString input)
{
    return std::make_unique<TRebaseVolumeActionActor>(
        std::move(requestInfo),
        std::move(input)
    );
}

}   // namespace NCloud::NBlockStore::NStorage
