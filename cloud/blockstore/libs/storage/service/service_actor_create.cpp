#include "service_actor.h"

#include <cloud/blockstore/libs/encryption/encryptor.h>
#include <cloud/blockstore/libs/storage/api/ss_proxy.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/api/volume_proxy.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/disk_validation.h>
#include <cloud/blockstore/libs/storage/core/public.h>
#include <cloud/blockstore/libs/storage/core/volume_model.h>
#include <cloud/blockstore/libs/storage/protos/part.pb.h>

#include <cloud/storage/core/libs/common/helpers.h>
#include <cloud/storage/core/libs/common/media.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/generic/size_literals.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TCreateVolumeActor final
    : public TActorBootstrapped<TCreateVolumeActor>
{
private:
    const TRequestInfoPtr RequestInfo;

    const TStorageConfigPtr Config;
    const NProto::TCreateVolumeRequest Request;

public:
    TCreateVolumeActor(
        TRequestInfoPtr requestInfo,
        TStorageConfigPtr config,
        NProto::TCreateVolumeRequest request);

    void Bootstrap(const TActorContext& ctx);

private:
    ui32 GetBlockSize() const;
    NCloud::NProto::EStorageMediaKind GetStorageMediaKind() const;

    void CreateVolume(const TActorContext& ctx);

    void HandleCreateVolumeResponse(
        const TEvSSProxy::TEvCreateVolumeResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleWaitReadyResponse(
        const TEvVolume::TEvWaitReadyResponse::TPtr& ev,
        const TActorContext& ctx);

    void ReplyAndDie(
        const TActorContext& ctx,
        std::unique_ptr<TEvService::TEvCreateVolumeResponse> response);

private:
    STFUNC(StateWork);
};

////////////////////////////////////////////////////////////////////////////////

TCreateVolumeActor::TCreateVolumeActor(
        TRequestInfoPtr requestInfo,
        TStorageConfigPtr config,
        NProto::TCreateVolumeRequest request)
    : RequestInfo(std::move(requestInfo))
    , Config(std::move(config))
    , Request(std::move(request))
{
    ActivityType = TBlockStoreActivities::SERVICE;
}

void TCreateVolumeActor::Bootstrap(const TActorContext& ctx)
{
    CreateVolume(ctx);
}

ui32 TCreateVolumeActor::GetBlockSize() const
{
    ui32 blockSize = Request.GetBlockSize();
    if (!blockSize) {
        blockSize = DefaultBlockSize;
    }
    return blockSize;
}

NCloud::NProto::EStorageMediaKind TCreateVolumeActor::GetStorageMediaKind() const
{
    switch (Request.GetStorageMediaKind()) {
        case NCloud::NProto::STORAGE_MEDIA_DEFAULT:
            return NCloud::NProto::STORAGE_MEDIA_HDD;
        default:
            return Request.GetStorageMediaKind();
    }
}

void TCreateVolumeActor::CreateVolume(const TActorContext& ctx)
{
    Become(&TThis::StateWork);

    NKikimrBlockStore::TVolumeConfig config;

    config.SetBlockSize(GetBlockSize());
    const auto maxBlocksInBlob = CalculateMaxBlocksInBlob(
        Config->GetMaxBlobSize(),
        GetBlockSize()
    );
    if (maxBlocksInBlob != MaxBlocksCount) {
        // MaxBlocksInBlob is not equal to the default value
        // => it needs to be stored
        config.SetMaxBlocksInBlob(maxBlocksInBlob);
    }
    config.SetZoneBlockCount(Config->GetZoneBlockCount());
    config.SetDiskId(Request.GetDiskId());
    config.SetFolderId(Request.GetFolderId());
    config.SetCloudId(Request.GetCloudId());
    config.SetProjectId(Request.GetProjectId());
    config.SetTabletVersion(
        Request.GetTabletVersion()
        ? Request.GetTabletVersion()
        : Config->GetDefaultTabletVersion()
    );
    config.SetStorageMediaKind(GetStorageMediaKind());
    config.SetBaseDiskId(Request.GetBaseDiskId());
    config.SetBaseDiskCheckpointId(Request.GetBaseDiskCheckpointId());
    config.SetIsSystem(Request.GetIsSystem());

    TVolumeParams volumeParams;
    volumeParams.BlockSize = GetBlockSize();
    volumeParams.MediaKind = GetStorageMediaKind();
    if (!IsDiskRegistryMediaKind(volumeParams.MediaKind)) {
        TPartitionsInfo partitionsInfo;
        if (Request.GetPartitionsCount()) {
            partitionsInfo.PartitionsCount = Request.GetPartitionsCount();
            partitionsInfo.BlocksCountPerPartition =
                ComputeBlocksCountPerPartition(
                    Request.GetBlocksCount(),
                    Config->GetBytesPerStripe(),
                    volumeParams.BlockSize,
                    partitionsInfo.PartitionsCount
                );
        } else {
            partitionsInfo = ComputePartitionsInfo(
                *Config,
                Request.GetCloudId(),
                Request.GetFolderId(),
                GetStorageMediaKind(),
                Request.GetBlocksCount(),
                volumeParams.BlockSize,
                Request.GetIsSystem(),
                !Request.GetBaseDiskId().Empty()
            );
        }
        volumeParams.PartitionsCount = partitionsInfo.PartitionsCount;
        volumeParams.BlocksCountPerPartition =
            partitionsInfo.BlocksCountPerPartition;
    } else {
        volumeParams.BlocksCountPerPartition = Request.GetBlocksCount();
        volumeParams.PartitionsCount = 1;
    }

    if (volumeParams.PartitionsCount > 1) {
        config.SetBlocksPerStripe(
            ceil(double(Config->GetBytesPerStripe()) / volumeParams.BlockSize)
        );
    }

    ResizeVolume(
        *Config,
        volumeParams,
        {},
        Request.GetPerformanceProfile(),
        config
    );
    config.SetCreationTs(ctx.Now().MicroSeconds());

    config.SetPlacementGroupId(Request.GetPlacementGroupId());
    config.SetStoragePoolName(Request.GetStoragePoolName());
    config.MutableAgentIds()->CopyFrom(Request.GetAgentIds());

    const auto& encryptionSpec = Request.GetEncryptionSpec();
    if (encryptionSpec.HasKeyHash() && !encryptionSpec.GetKeyHash().empty()) {
        config.SetEncryptionKeyHash(encryptionSpec.GetKeyHash());
    } else {
        auto keyHashOrError = ComputeEncryptionKeyHash(encryptionSpec.GetKey());
        if (HasError(keyHashOrError)) {
            const auto& error = keyHashOrError.GetError();
            LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
                "Creation of volume %s failed: %s",
                Request.GetDiskId().Quote().c_str(),
                error.GetMessage().c_str());

            ReplyAndDie(
                ctx,
                std::make_unique<TEvService::TEvCreateVolumeResponse>(error));
            return;
        }

        config.SetEncryptionKeyHash(keyHashOrError.GetResult());
    }

    auto traceId = RequestInfo->TraceId.Clone();
    auto request =
        std::make_unique<TEvSSProxy::TEvCreateVolumeRequest>(std::move(config));
    BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "Sending createvolume request for volume %s",
        Request.GetDiskId().Quote().c_str());

    NCloud::Send(
        ctx,
        MakeSSProxyServiceId(),
        std::move(request),
        RequestInfo->Cookie,
        std::move(traceId));
}

void TCreateVolumeActor::HandleCreateVolumeResponse(
    const TEvSSProxy::TEvCreateVolumeResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    const auto& error = msg->GetError();

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, msg, &ev->TraceId);

    if (HasError(error)) {
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
            "Creation of volume %s failed: %s",
            Request.GetDiskId().Quote().c_str(),
            msg->GetErrorReason().c_str());

        ReplyAndDie(
            ctx,
            std::make_unique<TEvService::TEvCreateVolumeResponse>(error));

        return;
    }

    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "Sending WaitReady request to volume %s",
        Request.GetDiskId().Quote().c_str());

    auto traceId = RequestInfo->TraceId.Clone();
    auto request = std::make_unique<TEvVolume::TEvWaitReadyRequest>();
    request->Record.SetDiskId(Request.GetDiskId());

    NCloud::Send(
        ctx,
        MakeVolumeProxyServiceId(),
        std::move(request),
        RequestInfo->Cookie,
        std::move(traceId));
}

void TCreateVolumeActor::HandleWaitReadyResponse(
    const TEvVolume::TEvWaitReadyResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    auto error = msg->GetError();

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, msg, &ev->TraceId);

    if (HasError(error)) {
        LOG_WARN(ctx, TBlockStoreComponents::SERVICE,
            "Volume %s creation failed with error: %s",
            Request.GetDiskId().Quote().c_str(),
            error.GetMessage().Quote().c_str());
    } else {
        LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
            "Successfully created volume %s",
            Request.GetDiskId().Quote().c_str());
    }

    ReplyAndDie(
        ctx,
        std::make_unique<TEvService::TEvCreateVolumeResponse>(std::move(error)));
}

void TCreateVolumeActor::ReplyAndDie(
    const TActorContext& ctx,
    std::unique_ptr<TEvService::TEvCreateVolumeResponse> response)
{
    BLOCKSTORE_TRACE_SENT(ctx, &RequestInfo->TraceId, this, response);
    NCloud::Reply(ctx, *RequestInfo, std::move(response));
    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TCreateVolumeActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvSSProxy::TEvCreateVolumeResponse, HandleCreateVolumeResponse);
        HFunc(TEvVolume::TEvWaitReadyResponse, HandleWaitReadyResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            break;
    }
}

bool ValidateCreateVolumeRequest(
    const TActorContext& ctx,
    const TStorageConfig& config,
    const NProto::TCreateVolumeRequest& request,
    TRequestInfo& requestInfo)
{
    TString errorMessage;

    if (!request.GetDiskId()) {
        errorMessage = "DiskId cannot be empty";
    } else if (request.GetTabletVersion() > MaxSupportedTabletVersion) {
        errorMessage = TStringBuilder() << "bad tablet version: "
            << request.GetTabletVersion()
            << " < " << MaxSupportedTabletVersion;
    }

    if (request.GetBaseDiskId() && !request.GetBaseDiskCheckpointId()) {
        errorMessage = "BaseDiskCheckpointId cannot be empty for overlay disk";
    }

    if (request.GetPartitionsCount() > 1
            && (request.GetBaseDiskId() || request.GetIsSystem()))
    {
        errorMessage = TStringBuilder() << "base and overlay disks with "
            << request.GetPartitionsCount()
            << " partitions are not implemented";
    }

    if (request.GetPartitionsCount() > config.GetMaxPartitionsPerVolume()) {
        errorMessage = TStringBuilder() << "too many partitions specified: "
            << request.GetPartitionsCount() << " > "
            << config.GetMaxPartitionsPerVolume();
    }

    const auto mediaKind = request.GetStorageMediaKind();
    const auto maxBlocks = ComputeMaxBlocks(config, mediaKind, 0);
    if (request.GetBlocksCount() > maxBlocks) {
        errorMessage = TStringBuilder() << "disk size for media kind "
            << static_cast<int>(mediaKind)
            << " should be <= " << maxBlocks << " blocks";
    }

    if (!request.GetBlocksCount()) {
        errorMessage = TStringBuilder() << "disk size should not be 0";
    }

    const auto vbsError = ValidateBlockSize(request.GetBlockSize());

    if (HasError(vbsError)) {
        errorMessage = vbsError.GetMessage();
    }

    if (!IsDiskRegistryMediaKind(mediaKind)) {
        if (request.GetPlacementGroupId()) {
            errorMessage = "PlacementGroupId not allowed for replicated disks";
        }

        if (request.GetStoragePoolName()) {
            errorMessage = "StoragePoolName not allowed for replicated disks";
        }

        if (request.AgentIdsSize()) {
            errorMessage = "AgentIds not allowed for replicated disks";
        }
    } else {
        const ui64 volumeSize = request.GetBlockSize() * request.GetBlocksCount();
        const ui64 unit = request.GetStoragePoolName()
            ? 4_KB    // custom pool can have any size
            : GetAllocationUnit(config, mediaKind);

        if (volumeSize % unit != 0) {
            errorMessage = TStringBuilder()
                << "volume size should be divisible by " << unit;
        }
    }

    if (errorMessage) {
        LOG_ERROR(
            ctx,
            TBlockStoreComponents::SERVICE,
            "CreateVolumeRequest validation failed, volume %s error %s",
            request.GetDiskId().data(),
            errorMessage.data());

        NCloud::Reply(
            ctx,
            requestInfo,
            std::make_unique<TEvService::TEvCreateVolumeResponse>(
                MakeError(E_ARGUMENT, std::move(errorMessage))
            )
        );

        return false;
    }

    return true;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TServiceActor::HandleCreateVolume(
    const TEvService::TEvCreateVolumeRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    const auto& request = msg->Record;

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    if (!ValidateCreateVolumeRequest(ctx, *Config, request, *requestInfo)) {
        return;
    }

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    LOG_INFO(ctx, TBlockStoreComponents::SERVICE,
        "Creating volume: %s, %s, %s, %s, %llu, %u, %d",
        request.GetDiskId().Quote().c_str(),
        request.GetProjectId().Quote().c_str(),
        request.GetFolderId().Quote().c_str(),
        request.GetCloudId().Quote().c_str(),
        request.GetBlocksCount(),
        request.GetBlockSize(),
        int(request.GetStorageMediaKind()));

    NCloud::Register<TCreateVolumeActor>(
        ctx,
        std::move(requestInfo),
        Config,
        request);
}

}   // namespace NCloud::NBlockStore::NStorage
