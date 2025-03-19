#include "service_actor.h"

#include <cloud/blockstore/libs/storage/api/disk_registry.h>
#include <cloud/blockstore/libs/storage/api/disk_registry_proxy.h>
#include <cloud/blockstore/libs/storage/api/ss_proxy.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/api/volume_proxy.h>
#include <cloud/blockstore/libs/storage/core/config.h>

#include <cloud/storage/core/libs/common/media.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TDestroyVolumeActor final
    : public TActorBootstrapped<TDestroyVolumeActor>
{
private:
    const TActorId Sender;
    const ui64 Cookie;

    const TString DiskId;
    const bool DestroyIfBroken;

public:
    TDestroyVolumeActor(
        const TActorId& sender,
        ui64 cookie,
        TString diskId,
        bool destroyIfBroken);

    void Bootstrap(const TActorContext& ctx);

private:
    void WaitReady(const TActorContext& ctx);
    void DestroyVolume(const TActorContext& ctx);
    void NotifyDiskRegistry(const TActorContext& ctx);
    void DescribeVolume(const TActorContext& ctx);

    void HandleModifyResponse(
        const TEvSSProxy::TEvModifyVolumeResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleWaitReadyResponse(
        const TEvVolume::TEvWaitReadyResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleDescribeVolumeResponse(
        const TEvSSProxy::TEvDescribeVolumeResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleMarkDiskForCleanupResponse(
        const TEvDiskRegistry::TEvMarkDiskForCleanupResponse::TPtr& ev,
        const TActorContext& ctx);

    void ReplyAndDie(
        const TActorContext& ctx,
        std::unique_ptr<TEvService::TEvDestroyVolumeResponse> response);

private:
    STFUNC(StateWork);
};

////////////////////////////////////////////////////////////////////////////////

TDestroyVolumeActor::TDestroyVolumeActor(
        const TActorId& sender,
        ui64 cookie,
        TString diskId,
        bool destroyIfBroken)
    : Sender(sender)
    , Cookie(cookie)
    , DiskId(std::move(diskId))
    , DestroyIfBroken(destroyIfBroken)
{
    ActivityType = TBlockStoreActivities::SERVICE;
}

void TDestroyVolumeActor::Bootstrap(const TActorContext& ctx)
{
    if (DestroyIfBroken) {
        WaitReady(ctx);
    } else {
        DescribeVolume(ctx);
    }

    Become(&TThis::StateWork);
}

void TDestroyVolumeActor::WaitReady(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvVolume::TEvWaitReadyRequest>();
    request->Record.SetDiskId(DiskId);

    NCloud::Send(
        ctx,
        MakeVolumeProxyServiceId(),
        std::move(request)
    );
}

void TDestroyVolumeActor::DestroyVolume(const TActorContext& ctx)
{
    NCloud::Send(
        ctx,
        MakeSSProxyServiceId(),
        std::make_unique<TEvSSProxy::TEvModifyVolumeRequest>(
            TEvSSProxy::TModifyVolumeRequest::EOpType::Destroy,
            DiskId,
            "",
            0));
}

void TDestroyVolumeActor::NotifyDiskRegistry(const TActorContext& ctx)
{
    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE, "notify disk registry");

    auto request = std::make_unique<TEvDiskRegistry::TEvMarkDiskForCleanupRequest>();
    request->Record.SetDiskId(DiskId);

    NCloud::Send(ctx, MakeDiskRegistryProxyServiceId(), std::move(request));
}

void TDestroyVolumeActor::DescribeVolume(const TActorContext& ctx)
{
    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE, "describe volume");

    NCloud::Send(
        ctx,
        MakeSSProxyServiceId(),
        std::make_unique<TEvSSProxy::TEvDescribeVolumeRequest>(DiskId));
}

void TDestroyVolumeActor::HandleModifyResponse(
    const TEvSSProxy::TEvModifyVolumeResponse::TPtr& ev,
    const TActorContext& ctx)
{
    // TODO: nonreplicated disk deallocation
    const auto* msg = ev->Get();
    const NProto::TError& error = msg->GetError();

    if (FAILED(error.GetCode())) {
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
            "Volume %s: drop failed, error %s",
            DiskId.Quote().data(),
            FormatError(error).data());

        ReplyAndDie(
            ctx,
            std::make_unique<TEvService::TEvDestroyVolumeResponse>(error));
        return;
    }

    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "Volume %s dropped successfully",
        DiskId.Quote().data());

    ReplyAndDie(
        ctx,
        std::make_unique<TEvService::TEvDestroyVolumeResponse>(error));
}

void TDestroyVolumeActor::HandleWaitReadyResponse(
    const TEvVolume::TEvWaitReadyResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    const auto& error = msg->GetError();

    if (HasError(error)) {
        LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
            "Volume %s WaitReady error %s",
            DiskId.Quote().c_str(),
            error.GetMessage().Quote().c_str());

        DestroyVolume(ctx);
    } else {
        LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
            "Volume %s WaitReady success",
            DiskId.Quote().c_str());

        ReplyAndDie(
            ctx,
            std::make_unique<TEvService::TEvDestroyVolumeResponse>(
                MakeError(
                    E_INVALID_STATE,
                    "this volume is online, it can't be destroyed")
            )
        );
    }
}

void TDestroyVolumeActor::HandleMarkDiskForCleanupResponse(
    const TEvDiskRegistry::TEvMarkDiskForCleanupResponse::TPtr& ev,
    const TActorContext& ctx)
{
    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE, "handle response from disk registry");

    const auto* msg = ev->Get();
    const auto& error = msg->GetError();

    // disk is broken and will be removed by DR at some point
    if (error.GetCode() == E_NOT_FOUND) {
        LOG_INFO(ctx, TBlockStoreComponents::SERVICE,
            "volume %s not found in registry", DiskId.Quote().data());
    } else if (FAILED(error.GetCode())) {
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
            "Volume %s: unable to notify DR about disk destruction: %s",
            DiskId.Quote().data(),
            FormatError(error).data());

        ReplyAndDie(
            ctx,
            std::make_unique<TEvService::TEvDestroyVolumeResponse>(error));
        return;
    }

    DestroyVolume(ctx);
}

void TDestroyVolumeActor::HandleDescribeVolumeResponse(
    const TEvSSProxy::TEvDescribeVolumeResponse::TPtr& ev,
    const TActorContext& ctx)
{
    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE, "handle describe response");

    const auto* msg = ev->Get();

    if (msg->GetStatus() ==
        MAKE_SCHEMESHARD_ERROR(NKikimrScheme::StatusPathDoesNotExist))
    {
        ReplyAndDie(
            ctx,
            std::make_unique<TEvService::TEvDestroyVolumeResponse>(
                MakeError(S_ALREADY, "volume not found")));
        return;
    }

    const auto& error = msg->GetError();

    if (FAILED(error.GetCode())) {
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
            "Volume %s: unable to determine disk media kind: %s",
            DiskId.Quote().data(),
            FormatError(error).data());

        ReplyAndDie(
            ctx,
            std::make_unique<TEvService::TEvDestroyVolumeResponse>(error));
        return;
    }

    auto mediaKind = static_cast<NCloud::NProto::EStorageMediaKind>(
        msg->PathDescription
            .GetBlockStoreVolumeDescription()
            .GetVolumeConfig()
            .GetStorageMediaKind());

    if (IsDiskRegistryMediaKind(mediaKind)) {
        NotifyDiskRegistry(ctx);
    } else {
        DestroyVolume(ctx);
    }
}

void TDestroyVolumeActor::ReplyAndDie(
    const TActorContext& ctx,
    std::unique_ptr<TEvService::TEvDestroyVolumeResponse> response)
{
    NCloud::Send(ctx, Sender, std::move(response), Cookie);
    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TDestroyVolumeActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvSSProxy::TEvModifyVolumeResponse, HandleModifyResponse);
        HFunc(TEvVolume::TEvWaitReadyResponse, HandleWaitReadyResponse);
        HFunc(
            TEvDiskRegistry::TEvMarkDiskForCleanupResponse,
            HandleMarkDiskForCleanupResponse);
        HFunc(
            TEvSSProxy::TEvDescribeVolumeResponse,
            HandleDescribeVolumeResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TServiceActor::HandleDestroyVolume(
    const TEvService::TEvDestroyVolumeRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    const auto& request = msg->Record;
    const auto& diskId = request.GetDiskId();
    const auto destroyIfBroken = request.GetDestroyIfBroken();

    LOG_INFO(ctx, TBlockStoreComponents::SERVICE,
        "Deleting volume: %s, %d",
        diskId.Quote().c_str(),
        destroyIfBroken);

    NCloud::Register<TDestroyVolumeActor>(
        ctx,
        ev->Sender,
        ev->Cookie,
        diskId,
        destroyIfBroken);
}

}   // namespace NCloud::NBlockStore::NStorage
