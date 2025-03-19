#include "service_actor.h"

#include <cloud/blockstore/libs/storage/api/ss_proxy.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/api/volume_proxy.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>
#include <cloud/blockstore/libs/storage/core/volume_model.h>
#include <cloud/blockstore/libs/storage/protos/part.pb.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/generic/size_literals.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TAlterVolumeActor final
    : public TActorBootstrapped<TAlterVolumeActor>
{
private:
    const TActorId Sender;
    const ui64 Cookie;

    const TStorageConfigPtr Config;

    const NProto::TResizeVolumeRequestFlags Flags;
    const NProto::TVolumePerformanceProfile PerformanceProfile;
    NKikimrBlockStore::TVolumeConfig VolumeConfig;
    NProto::TError Error;

    const ui64 NewBlocksCount = 0;
    const TString DiskId;
    ui32 ConfigVersion = 0;

public:
    TAlterVolumeActor(
        const TActorId& sender,
        ui64 cookie,
        TStorageConfigPtr config,
        const NProto::TResizeVolumeRequest& request);

    TAlterVolumeActor(
        const TActorId& sender,
        ui64 cookie,
        TStorageConfigPtr config,
        const NProto::TAlterVolumeRequest& request);

    void Bootstrap(const TActorContext& ctx);

private:
    const char* GetOperationString() const
    {
        return NewBlocksCount ? "Resize" : "Alter";
    }

    TVolumeParams BuildVolumeParams(
        const NKikimrBlockStore::TVolumeConfig& volumeConfig) const
    {
        TVolumeParams volumeParams;
        volumeParams.BlockSize = volumeConfig.GetBlockSize();

        Y_VERIFY(NewBlocksCount);
        volumeParams.PartitionsCount = volumeConfig.PartitionsSize();
        volumeParams.BlocksCountPerPartition = ComputeBlocksCountPerPartition(
            NewBlocksCount,
            volumeConfig.GetBlocksPerStripe(),
            volumeConfig.PartitionsSize()
        );

        const auto mediaKind = volumeConfig.GetStorageMediaKind();
        volumeParams.MediaKind =
            static_cast<NCloud::NProto::EStorageMediaKind>(mediaKind);

        if (volumeConfig.ExplicitChannelProfilesSize()) {
            Y_VERIFY_DEBUG(volumeConfig.ExplicitChannelProfilesSize() > 3);
            for (ui32 i = 3; i < volumeConfig.ExplicitChannelProfilesSize(); ++i) {
                volumeParams.DataChannels.push_back(static_cast<EChannelDataKind>(
                    volumeConfig.GetExplicitChannelProfiles(i).GetDataKind()));
            }
        }

        volumeParams.MaxReadBandwidth =
            volumeConfig.GetPerformanceProfileMaxReadBandwidth();
        volumeParams.MaxWriteBandwidth =
            volumeConfig.GetPerformanceProfileMaxWriteBandwidth();
        volumeParams.MaxReadIops =
            volumeConfig.GetPerformanceProfileMaxReadIops();
        volumeParams.MaxWriteIops =
            volumeConfig.GetPerformanceProfileMaxWriteIops();
        return volumeParams;
    }

    void DescribeVolume(const TActorContext& ctx);

    void AlterVolume(
        const TActorContext& ctx,
        TString path,
        ui64 pathId,
        ui64 version);

    void WaitReady(const TActorContext& ctx);

    void ReplyAndDie(const TActorContext& ctx);

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

TAlterVolumeActor::TAlterVolumeActor(
        const TActorId& sender,
        ui64 cookie,
        TStorageConfigPtr config,
        const NProto::TResizeVolumeRequest& request)
    : Sender(sender)
    , Cookie(cookie)
    , Config(std::move(config))
    , Flags(request.GetFlags())
    , PerformanceProfile(request.GetPerformanceProfile())
    , NewBlocksCount(request.GetBlocksCount())
    , DiskId(request.GetDiskId())
    , ConfigVersion(request.GetConfigVersion())
{
    ActivityType = TBlockStoreActivities::SERVICE;
}

TAlterVolumeActor::TAlterVolumeActor(
        const TActorId& sender,
        ui64 cookie,
        TStorageConfigPtr config,
        const NProto::TAlterVolumeRequest& request)
    : Sender(sender)
    , Cookie(cookie)
    , Config(std::move(config))
    , DiskId(request.GetDiskId())
    , ConfigVersion(request.GetConfigVersion())
{
    ActivityType = TBlockStoreActivities::SERVICE;

    VolumeConfig.SetProjectId(request.GetProjectId());
    VolumeConfig.SetFolderId(request.GetFolderId());
    VolumeConfig.SetCloudId(request.GetCloudId());
}

void TAlterVolumeActor::Bootstrap(const TActorContext& ctx)
{
    DescribeVolume(ctx);
}

void TAlterVolumeActor::DescribeVolume(const TActorContext& ctx)
{
    Become(&TThis::StateDescribeVolume);

    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "Sending describe request for volume %s",
        DiskId.Quote().c_str());

    NCloud::Send(
        ctx,
        MakeSSProxyServiceId(),
        std::make_unique<TEvSSProxy::TEvDescribeVolumeRequest>(DiskId));
}

void TAlterVolumeActor::AlterVolume(
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
        "Sending alter request for %s in directory %s",
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

void TAlterVolumeActor::WaitReady(const TActorContext& ctx)
{
    Become(&TThis::StateWaitReady);

    auto request = std::make_unique<TEvVolume::TEvWaitReadyRequest>();
    request->Record.SetDiskId(DiskId);

    NCloud::Send(
        ctx,
        MakeVolumeProxyServiceId(),
        std::move(request),
        Cookie);
}

void TAlterVolumeActor::ReplyAndDie(const TActorContext& ctx)
{
    if (NewBlocksCount) {
        auto response = std::make_unique<TEvService::TEvResizeVolumeResponse>(Error);
        NCloud::Send(ctx, Sender, std::move(response), Cookie);
    } else {
        auto response = std::make_unique<TEvService::TEvAlterVolumeResponse>(Error);
        NCloud::Send(ctx, Sender, std::move(response), Cookie);
    }

    Die(ctx);
}

void TAlterVolumeActor::HandleDescribeVolumeResponse(
    const TEvSSProxy::TEvDescribeVolumeResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    const auto& error = msg->GetError();
    if (FAILED(error.GetCode())) {
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
            "Volume %s: describe failed: %s",
            DiskId.Quote().c_str(),
            FormatError(error).c_str());
        Error = error;

        ReplyAndDie(ctx);
        return;
    }

    const auto& path = msg->Path;
    const auto& pathDescription = msg->PathDescription;
    const auto& volumeDescription =
        pathDescription.GetBlockStoreVolumeDescription();
    const auto& volumeConfig = volumeDescription.GetVolumeConfig();

    if (!ConfigVersion) {
        ConfigVersion = volumeConfig.GetVersion();
    }

    if (NewBlocksCount) {
        auto volumeParams = BuildVolumeParams(volumeConfig);
        ui32 oldBlocksCount = 0;
        for (const auto& partition: volumeConfig.GetPartitions()) {
            oldBlocksCount += partition.GetBlockCount();
            Y_VERIFY(volumeConfig.GetPartitions(0).GetBlockCount()
                    == partition.GetBlockCount());
        }
        if (volumeParams.GetBlocksCount() < oldBlocksCount) {
            Error = MakeError(E_ARGUMENT, "Cannot decrease volume size");
            ReplyAndDie(ctx);
            return;
        }

        const auto maxBlocks = ComputeMaxBlocks(
            *Config,
            volumeParams.MediaKind,
            volumeParams.PartitionsCount
        );

        if (volumeParams.GetBlocksCount() > maxBlocks) {
            Error = MakeError(
                E_ARGUMENT,
                TStringBuilder() << "disk size for media kind "
                    << static_cast<int>(volumeParams.MediaKind)
                    << " should be <= " << maxBlocks << " blocks"
            );
            ReplyAndDie(ctx);
            return;
        }

        const auto size = volumeParams.GetBlocksCount() * volumeParams.BlockSize;

        if (volumeParams.MediaKind == NCloud::NProto::STORAGE_MEDIA_SSD_LOCAL
                && size % (Config->GetAllocationUnitLocalSSD() * 1_GB) != 0)
        {
            Error = MakeError(
                E_ARGUMENT,
                TStringBuilder() << "volume size should be divisible by "
                    << (Config->GetAllocationUnitLocalSSD() * 1_GB)
            );
            ReplyAndDie(ctx);
            return;
        }

        if (volumeParams.MediaKind == NCloud::NProto::STORAGE_MEDIA_SSD_NONREPLICATED
                && size % (Config->GetAllocationUnitNonReplicatedSSD() * 1_GB) != 0)
        {
            Error = MakeError(
                E_ARGUMENT,
                TStringBuilder() << "volume size should be divisible by "
                    << (Config->GetAllocationUnitNonReplicatedSSD() * 1_GB)
            );
            ReplyAndDie(ctx);
            return;
        }

        if (volumeParams.MediaKind == NCloud::NProto::STORAGE_MEDIA_SSD_MIRROR2
                && size % (Config->GetAllocationUnitMirror2SSD() * 1_GB) != 0)
        {
            Error = MakeError(
                E_ARGUMENT,
                TStringBuilder() << "volume size should be divisible by "
                    << (Config->GetAllocationUnitMirror2SSD() * 1_GB)
            );
            ReplyAndDie(ctx);
            return;
        }

        if (volumeParams.MediaKind == NCloud::NProto::STORAGE_MEDIA_SSD_MIRROR3
                && size % (Config->GetAllocationUnitMirror3SSD() * 1_GB) != 0)
        {
            Error = MakeError(
                E_ARGUMENT,
                TStringBuilder() << "volume size should be divisible by "
                    << (Config->GetAllocationUnitMirror3SSD() * 1_GB)
            );
            ReplyAndDie(ctx);
            return;
        }

        Y_VERIFY(
            VolumeConfig.GetCloudId().empty() &&
            VolumeConfig.GetFolderId().empty() &&
            VolumeConfig.GetProjectId().empty());

        VolumeConfig.SetCloudId(volumeConfig.GetCloudId());
        VolumeConfig.SetFolderId(volumeConfig.GetFolderId());
        VolumeConfig.SetProjectId(volumeConfig.GetProjectId());

        ResizeVolume(
            *Config,
            volumeParams,
            Flags,
            PerformanceProfile,
            VolumeConfig
        );

        if (!SetMissingParams(volumeParams, volumeConfig, VolumeConfig)
                && CompareVolumeConfigs(volumeConfig, VolumeConfig))
        {
            LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
                "Volume %s already has the required settings, size=%lu",
                DiskId.Quote().c_str(),
                volumeParams.GetBlocksCount());

            Error = MakeError(S_ALREADY, "Volume already has the required settings");
            WaitReady(ctx);
            return;
        }
    }

    if (Config->GetAllowVersionInModifyScheme()) {
        VolumeConfig.SetVersion(ConfigVersion);
    }

    VolumeConfig.SetAlterTs(ctx.Now().MicroSeconds());

    AlterVolume(
        ctx,
        path,
        pathDescription.GetSelf().GetPathId(),
        pathDescription.GetSelf().GetPathVersion());
}

void TAlterVolumeActor::HandleAlterVolumeResponse(
    const TEvSSProxy::TEvModifySchemeResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    NProto::TError error = msg->GetError();
    ui32 errorCode = error.GetCode();
    if (FAILED(errorCode)) {
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
            "%s of volume %s failed: %s",
            GetOperationString(),
            DiskId.Quote().c_str(),
            msg->GetErrorReason().c_str());

        Error = std::move(error);
        ReplyAndDie(ctx);
        return;
    }

    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "Sending WaitReady request to volume %s",
        DiskId.Quote().c_str());

    WaitReady(ctx);
}

void TAlterVolumeActor::HandleWaitReadyResponse(
    const TEvVolume::TEvWaitReadyResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    if (HasError(msg->GetError())) {
        LOG_ERROR(ctx, TBlockStoreComponents::SERVICE,
            "%s->WaitReady request failed for volume %s, error: %s",
            GetOperationString(),
            DiskId.Quote().c_str(),
            msg->GetErrorReason().Quote().c_str());
        Error = msg->GetError();
    } else {
        LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
            "Successfully %s volume %s",
            GetOperationString(),
            DiskId.Quote().c_str());
    }

    ReplyAndDie(ctx);
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TAlterVolumeActor::StateDescribeVolume)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvSSProxy::TEvDescribeVolumeResponse, HandleDescribeVolumeResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            break;
    }
}

STFUNC(TAlterVolumeActor::StateAlterVolume)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvSSProxy::TEvModifySchemeResponse, HandleAlterVolumeResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            break;
    }
}

STFUNC(TAlterVolumeActor::StateWaitReady)
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

void TServiceActor::HandleAlterVolume(
    const TEvService::TEvAlterVolumeRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    const auto& request = msg->Record;

    LOG_INFO(ctx, TBlockStoreComponents::SERVICE,
        "Altering volume: %s, %s, %s, %s, %u",
        request.GetDiskId().Quote().c_str(),
        request.GetProjectId().Quote().c_str(),
        request.GetFolderId().Quote().c_str(),
        request.GetCloudId().Quote().c_str(),
        request.GetConfigVersion());

    NCloud::Register<TAlterVolumeActor>(
        ctx,
        ev->Sender,
        ev->Cookie,
        Config,
        request);
}

void TServiceActor::HandleResizeVolume(
    const TEvService::TEvResizeVolumeRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    const auto& request = msg->Record;

    LOG_INFO(ctx, TBlockStoreComponents::SERVICE,
        "Resizing volume: %s, %llu, %u",
        request.GetDiskId().Quote().c_str(),
        request.GetBlocksCount(),
        request.GetConfigVersion());

    if (!request.GetBlocksCount()) {
        auto response = std::make_unique<TEvService::TEvResizeVolumeResponse>(
            MakeError(E_ARGUMENT, "BlocksCount should not be 0 for resize requests"));
        NCloud::Send(ctx, ev->Sender, std::move(response), ev->Cookie);
        return;
    }

    NCloud::Register<TAlterVolumeActor>(
        ctx,
        ev->Sender,
        ev->Cookie,
        Config,
        request);
}

}   // namespace NCloud::NBlockStore::NStorage
