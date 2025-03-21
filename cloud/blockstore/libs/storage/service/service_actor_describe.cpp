#include "service_actor.h"

#include <cloud/blockstore/libs/storage/api/disk_registry.h>
#include <cloud/blockstore/libs/storage/api/disk_registry_proxy.h>
#include <cloud/blockstore/libs/storage/api/ss_proxy.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>

#include <cloud/storage/core/libs/common/media.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TDescribeVolumeActor final
    : public TActorBootstrapped<TDescribeVolumeActor>
{
private:
    const TRequestInfoPtr RequestInfo;
    const TStorageConfigPtr Config;
    const TString DiskId;

    NProto::TVolume Volume;

public:
    TDescribeVolumeActor(
        TRequestInfoPtr requestInfo,
        TStorageConfigPtr config,
        TString diskId);

    void Bootstrap(const TActorContext& ctx);

private:
    void DescribeVolume(const TActorContext& ctx);
    void DescribeDiskRegistryVolume(const TActorContext& ctx);

    void HandleDescribeVolumeResponse(
        const TEvSSProxy::TEvDescribeVolumeResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleDescribeDiskResponse(
        const TEvDiskRegistry::TEvDescribeDiskResponse::TPtr& ev,
        const TActorContext& ctx);

    void ReplyAndDie(
        const TActorContext& ctx,
        std::unique_ptr<TEvService::TEvDescribeVolumeResponse> response);

private:
    STFUNC(StateDescribeVolume);
};

////////////////////////////////////////////////////////////////////////////////

TDescribeVolumeActor::TDescribeVolumeActor(
        TRequestInfoPtr requestInfo,
        TStorageConfigPtr config,
        TString diskId)
    : RequestInfo(std::move(requestInfo))
    , Config(std::move(config))
    , DiskId(std::move(diskId))
{
    ActivityType = TBlockStoreActivities::SERVICE;
}

void TDescribeVolumeActor::Bootstrap(const TActorContext& ctx)
{
    DescribeVolume(ctx);
}

void TDescribeVolumeActor::DescribeVolume(const TActorContext& ctx)
{
    Become(&TThis::StateDescribeVolume);

    auto traceId = RequestInfo->TraceId.Clone();
    auto request = std::make_unique<TEvSSProxy::TEvDescribeVolumeRequest>(DiskId);
    BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

    NCloud::Send(
        ctx,
        MakeSSProxyServiceId(),
        std::move(request),
        RequestInfo->Cookie,
        std::move(traceId));
}

void TDescribeVolumeActor::DescribeDiskRegistryVolume(const TActorContext& ctx)
{
    auto traceId = RequestInfo->TraceId.Clone();
    auto request = std::make_unique<TEvDiskRegistry::TEvDescribeDiskRequest>();
    request->Record.SetDiskId(DiskId);
    BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

    NCloud::Send(
        ctx,
        MakeDiskRegistryProxyServiceId(),
        std::move(request),
        RequestInfo->Cookie,
        std::move(traceId));
}

void TDescribeVolumeActor::HandleDescribeVolumeResponse(
    const TEvSSProxy::TEvDescribeVolumeResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, msg, &ev->TraceId);

    const auto& error = msg->GetError();
    if (FAILED(error.GetCode())) {
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
            "Volume %s: describe failed: %s",
            DiskId.Quote().data(),
            FormatError(error).data());

        ReplyAndDie(
            ctx,
            std::make_unique<TEvService::TEvDescribeVolumeResponse>(error));
        return;
    }

    const auto& pathDescription = msg->PathDescription;
    const auto& volumeDescription =
        pathDescription.GetBlockStoreVolumeDescription();
    const auto& volumeConfig = volumeDescription.GetVolumeConfig();

    VolumeConfigToVolume(volumeConfig, Volume);
    Volume.SetTokenVersion(volumeDescription.GetTokenVersion());

    if (IsDiskRegistryMediaKind(Volume.GetStorageMediaKind())) {
        DescribeDiskRegistryVolume(ctx);
        return;
    }

    auto response = std::make_unique<TEvService::TEvDescribeVolumeResponse>();
    *response->Record.MutableVolume() = std::move(Volume);

    ReplyAndDie(ctx, std::move(response));
}

void TDescribeVolumeActor::HandleDescribeDiskResponse(
    const TEvDiskRegistry::TEvDescribeDiskResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, msg, &ev->TraceId);

    const auto& error = msg->GetError();
    if (FAILED(error.GetCode())) {
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
            "Non-replicated volume %s: describe failed: %s",
            DiskId.Quote().data(),
            FormatError(error).data());

        ReplyAndDie(
            ctx,
            std::make_unique<TEvService::TEvDescribeVolumeResponse>(error));
        return;
    }

    FillDeviceInfo(
        msg->Record.GetDevices(),
        msg->Record.GetMigrations(),
        msg->Record.GetReplicas(),
        Volume);

    auto response = std::make_unique<TEvService::TEvDescribeVolumeResponse>();
    *response->Record.MutableVolume() = std::move(Volume);

    ReplyAndDie(ctx, std::move(response));
}

void TDescribeVolumeActor::ReplyAndDie(
    const TActorContext& ctx,
    std::unique_ptr<TEvService::TEvDescribeVolumeResponse> response)
{
    BLOCKSTORE_TRACE_SENT(ctx, &RequestInfo->TraceId, this, response);
    NCloud::Reply(ctx, *RequestInfo, std::move(response));
    Die(ctx);
}


////////////////////////////////////////////////////////////////////////////////

STFUNC(TDescribeVolumeActor::StateDescribeVolume)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvSSProxy::TEvDescribeVolumeResponse, HandleDescribeVolumeResponse);
        HFunc(TEvDiskRegistry::TEvDescribeDiskResponse, HandleDescribeDiskResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TServiceActor::HandleDescribeVolume(
    const TEvService::TEvDescribeVolumeRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    const auto& request = msg->Record;

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    if (request.GetDiskId().empty()) {
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
            "Empty DiskId in DescribeVolume");

        auto response = std::make_unique<TEvService::TEvDescribeVolumeResponse>(
            MakeError(E_ARGUMENT, "Volume name cannot be empty"));
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "Describing volume: %s",
        request.GetDiskId().Quote().data());

    NCloud::Register<TDescribeVolumeActor>(
        ctx,
        std::move(requestInfo),
        Config,
        request.GetDiskId());
}

}   // namespace NCloud::NBlockStore::NStorage
