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

#include <util/charset/unidata.h>
#include <util/string/join.h>

#include <google/protobuf/util/json_util.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER)

namespace {

////////////////////////////////////////////////////////////////////////////////

class TModifyTagsActionActor final
    : public TActorBootstrapped<TModifyTagsActionActor>
{
private:
    const TRequestInfoPtr RequestInfo;
    const TString Input;

    NPrivateProto::TModifyTagsRequest Request;
    NKikimrBlockStore::TVolumeConfig VolumeConfig;

public:
    TModifyTagsActionActor(
        TRequestInfoPtr requestInfo,
        TString input);

    void Bootstrap(const TActorContext& ctx);

private:
    void DescribeVolume(const TActorContext& ctx);
    void AlterVolume(
        const TActorContext& ctx,
        TString path,
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

    bool ValidateTag(
        const TActorContext& ctx,
        const TString& tag);
};

////////////////////////////////////////////////////////////////////////////////

TModifyTagsActionActor::TModifyTagsActionActor(
    TRequestInfoPtr requestInfo,
    TString input)
    : RequestInfo(std::move(requestInfo))
    , Input(std::move(input))
{
    ActivityType = TBlockStoreActivities::SERVICE;
}

bool TModifyTagsActionActor::ValidateTag(
    const TActorContext& ctx,
    const TString& tag)
{
    for (const auto c: tag) {
        if (!IsAlnum(c) && c != '-' && c != '_') {
            auto error = MakeError(
                E_ARGUMENT,
                Sprintf("Invalid tag: %s", tag.c_str())
            );
            ReplyAndDie(ctx, std::move(error));
            return false;
        }
    }

    return true;
}

void TModifyTagsActionActor::Bootstrap(const TActorContext& ctx)
{
    if (!google::protobuf::util::JsonStringToMessage(Input, &Request).ok()) {
        ReplyAndDie(ctx, MakeError(E_ARGUMENT, "Failed to parse input"));
        return;
    }

    if (!Request.GetDiskId()) {
        ReplyAndDie(ctx, MakeError(E_ARGUMENT, "DiskId should be supplied"));
        return;
    }

    if (!Request.TagsToAddSize() && !Request.TagsToRemoveSize()) {
        auto error = MakeError(
            E_ARGUMENT,
            "Either TagsToAdd or TagsToRemove should be supplied"
        );
        ReplyAndDie(ctx, std::move(error));
        return;
    }

    for (const auto& tag: Request.GetTagsToAdd()) {
        if (!ValidateTag(ctx, tag)) {
            return;
        }
    }

    for (const auto& tag: Request.GetTagsToRemove()) {
        if (!ValidateTag(ctx, tag)) {
            return;
        }
    }

    VolumeConfig.SetDiskId(Request.GetDiskId());
    DescribeVolume(ctx);
}

void TModifyTagsActionActor::DescribeVolume(const TActorContext& ctx)
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

void TModifyTagsActionActor::AlterVolume(
    const TActorContext& ctx,
    TString path,
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
        "Sending ModifyTags->Alter request for %s in directory %s",
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

void TModifyTagsActionActor::WaitReady(const TActorContext& ctx)
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

void TModifyTagsActionActor::ReplyAndDie(
    const TActorContext& ctx,
    NProto::TError error)
{
    auto response =
        std::make_unique<TEvService::TEvExecuteActionResponse>(std::move(error));
    google::protobuf::util::MessageToJsonString(
        NPrivateProto::TModifyTagsResponse(),
        response->Record.MutableOutput()
    );

    LWTRACK(
        ResponseSent_Service,
        RequestInfo->CallContext->LWOrbit,
        "ExecuteAction_modifytags",
        RequestInfo->CallContext->RequestId);

    NCloud::Reply(ctx, *RequestInfo, std::move(response));
    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TModifyTagsActionActor::HandleDescribeVolumeResponse(
    const TEvSSProxy::TEvDescribeVolumeResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    auto error = msg->GetError();
    if (FAILED(error.GetCode())) {
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
            "Volume %s: describe failed: %s",
            Request.GetDiskId().Quote().c_str(),
            FormatError(error).c_str());
        ReplyAndDie(ctx, std::move(error));
        return;
    }

    const auto& path = msg->Path;
    const auto& pathDescription = msg->PathDescription;
    const auto& volumeDescription =
        pathDescription.GetBlockStoreVolumeDescription();
    VolumeConfig.SetVersion(
        volumeDescription.GetVolumeConfig().GetVersion()
    );

    TSet<TString> tags;
    {
        TStringBuf tok;
        TStringBuf sit(volumeDescription.GetVolumeConfig().GetTagsStr());
        while (sit.NextTok(',', tok)) {
            tags.insert(TString(tok));
        }
    }

    for (const auto& tag: Request.GetTagsToRemove()) {
        tags.erase(tag);
    }

    for (const auto& tag: Request.GetTagsToAdd()) {
        tags.insert(tag);
    }

    *VolumeConfig.MutableTagsStr() = JoinSeq(",", tags);

    AlterVolume(
        ctx,
        path,
        pathDescription.GetSelf().GetPathId(),
        pathDescription.GetSelf().GetPathVersion());
}

void TModifyTagsActionActor::HandleAlterVolumeResponse(
    const TEvSSProxy::TEvModifySchemeResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    NProto::TError error = msg->GetError();
    ui32 errorCode = error.GetCode();

    if (FAILED(errorCode)) {
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
            "ModifyTags->Alter of volume %s failed: %s",
            Request.GetDiskId().Quote().c_str(),
            msg->GetErrorReason().c_str());

        ReplyAndDie(ctx, error);
        return;
    }

    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "Sending WaitReady request to volume %s",
        Request.GetDiskId().Quote().c_str());

    WaitReady(ctx);
}

void TModifyTagsActionActor::HandleWaitReadyResponse(
    const TEvVolume::TEvWaitReadyResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    NProto::TError error = msg->GetError();

    if (HasError(error)) {
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
            "ModifyTags->WaitReady request failed for volume %s, error: %s",
            Request.GetDiskId().Quote().c_str(),
            msg->GetErrorReason().Quote().c_str());
    } else {
        LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
            "Successfully modified tags for volume %s",
            Request.GetDiskId().Quote().c_str());
    }

    ReplyAndDie(ctx, std::move(error));
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TModifyTagsActionActor::StateDescribeVolume)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvSSProxy::TEvDescribeVolumeResponse, HandleDescribeVolumeResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            break;
    }
}

STFUNC(TModifyTagsActionActor::StateAlterVolume)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvSSProxy::TEvModifySchemeResponse, HandleAlterVolumeResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            break;
    }
}

STFUNC(TModifyTagsActionActor::StateWaitReady)
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

IActorPtr TServiceActor::CreateModifyTagsActionActor(
    TRequestInfoPtr requestInfo,
    TString input)
{
    return std::make_unique<TModifyTagsActionActor>(
        std::move(requestInfo),
        std::move(input)
    );
}

}   // namespace NCloud::NBlockStore::NStorage
