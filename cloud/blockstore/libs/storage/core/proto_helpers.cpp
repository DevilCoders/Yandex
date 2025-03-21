#include "proto_helpers.h"

#include "config.h"

#include <cloud/blockstore/libs/storage/model/channel_data_kind.h>
#include <cloud/blockstore/libs/storage/protos/part.pb.h>

namespace NCloud::NBlockStore::NStorage {

namespace {

////////////////////////////////////////////////////////////////////////////////

template <class T>
void VolumeConfigToVolumeModelFields(
    const NKikimrBlockStore::TVolumeConfig& volumeConfig,
    T& volumeModel)
{
    ui64 blocksCount = 0;
    for (const auto& partition: volumeConfig.GetPartitions()) {
        blocksCount += partition.GetBlockCount();
    }

    volumeModel.SetBlockSize(volumeConfig.GetBlockSize());
    volumeModel.SetBlocksCount(blocksCount);
    volumeModel.SetPartitionsCount(volumeConfig.PartitionsSize());

    ui32 mergedChannels = 0;
    ui32 mixedChannels = 0;
    ui32 freshChannels = 0;
    for (const auto& ecp: volumeConfig.GetExplicitChannelProfiles()) {
        switch (static_cast<EChannelDataKind>(ecp.GetDataKind())) {
            case EChannelDataKind::Merged: {
                ++mergedChannels;
                break;
            }

            case EChannelDataKind::Mixed: {
                ++mixedChannels;
                break;
            }

            case EChannelDataKind::Fresh: {
                ++freshChannels;
                break;
            }

            default: {}
        }
    }
    volumeModel.SetMergedChannelsCount(mergedChannels);
    volumeModel.SetMixedChannelsCount(mixedChannels);
    volumeModel.SetFreshChannelsCount(freshChannels);

    NCloud::NProto::EStorageMediaKind storageMediaKind =
        NCloud::NProto::STORAGE_MEDIA_DEFAULT;
    if (volumeConfig.GetStorageMediaKind() >= NCloud::NProto::EStorageMediaKind_MIN &&
        volumeConfig.GetStorageMediaKind() <= NCloud::NProto::EStorageMediaKind_MAX)
    {
        storageMediaKind = static_cast<NCloud::NProto::EStorageMediaKind>(
            volumeConfig.GetStorageMediaKind());
    }

    volumeModel.SetStorageMediaKind(storageMediaKind);

    auto pp = volumeModel.MutablePerformanceProfile();
    pp->SetMaxReadBandwidth(
        volumeConfig.GetPerformanceProfileMaxReadBandwidth());
    pp->SetMaxWriteBandwidth(
        volumeConfig.GetPerformanceProfileMaxWriteBandwidth());
    pp->SetMaxReadIops(
        volumeConfig.GetPerformanceProfileMaxReadIops());
    pp->SetMaxWriteIops(
        volumeConfig.GetPerformanceProfileMaxWriteIops());
    pp->SetBurstPercentage(
        volumeConfig.GetPerformanceProfileBurstPercentage());
    pp->SetMaxPostponedWeight(
        volumeConfig.GetPerformanceProfileMaxPostponedWeight());
    pp->SetBoostTime(
        volumeConfig.GetPerformanceProfileBoostTime());
    pp->SetBoostRefillTime(
        volumeConfig.GetPerformanceProfileBoostRefillTime());
    pp->SetBoostPercentage(
        volumeConfig.GetPerformanceProfileBoostPercentage());
    pp->SetThrottlingEnabled(
        volumeConfig.GetPerformanceProfileThrottlingEnabled());
}

using TChannelProfile = NKikimrBlockStore::TChannelProfile;
using TChannelProfiles = google::protobuf::RepeatedPtrField<TChannelProfile>;

bool CompareExplicitChannelProfiles(
    const TChannelProfiles& prevProfiles,
    const TChannelProfiles& newProfiles)
{
    if (prevProfiles.size() != newProfiles.size()) {
        return false;
    }

    for (int i = 0; i < prevProfiles.size(); ++i) {
        const auto& p = prevProfiles[i];
        const auto& n = newProfiles[i];
        if (p.GetDataKind() != n.GetDataKind()
                || p.GetPoolKind() != n.GetPoolKind()
                || p.GetReadIops() != n.GetReadIops()
                || p.GetWriteIops() != n.GetWriteIops()
                || p.GetReadBandwidth() != n.GetReadBandwidth()
                || p.GetWriteBandwidth() != n.GetWriteBandwidth())
        {
            return false;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

void FillDevice(
    const NProto::TDeviceConfig& deviceConfig,
    NProto::TDevice* device)
{
    device->SetBaseName(deviceConfig.GetBaseName());
    device->SetBlockCount(deviceConfig.GetBlocksCount());
    device->SetTransportId(deviceConfig.GetTransportId());
    device->SetAgentId(deviceConfig.GetAgentId());
    device->SetDeviceUUID(deviceConfig.GetDeviceUUID());
    device->SetDeviceName(deviceConfig.GetDeviceName());
    device->MutableRdmaEndpoint()->CopyFrom(deviceConfig.GetRdmaEndpoint());
}

template <typename T>
void FillDevices(
    const google::protobuf::RepeatedPtrField<NProto::TDeviceConfig>& devices,
    T& t)
{
    t.MutableDevices()->Reserve(devices.size());
    for (const auto& deviceConfig: devices) {
        FillDevice(deviceConfig, t.AddDevices());
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void VolumeConfigToVolume(
    const NKikimrBlockStore::TVolumeConfig& volumeConfig,
    NProto::TVolume& volume)
{
    VolumeConfigToVolumeModelFields(volumeConfig, volume);

    volume.SetDiskId(volumeConfig.GetDiskId());
    volume.SetProjectId(volumeConfig.GetProjectId());
    volume.SetFolderId(volumeConfig.GetFolderId());
    volume.SetCloudId(volumeConfig.GetCloudId());
    volume.SetConfigVersion(volumeConfig.GetVersion());
    volume.SetTabletVersion(volumeConfig.GetTabletVersion());
    volume.SetCreationTs(volumeConfig.GetCreationTs());
    volume.SetAlterTs(volumeConfig.GetAlterTs());
    volume.SetPlacementGroupId(volumeConfig.GetPlacementGroupId());
    volume.SetEncryptionKeyHash(volumeConfig.GetEncryptionKeyHash());
    volume.SetBaseDiskId(volumeConfig.GetBaseDiskId());
    volume.SetBaseDiskCheckpointId(volumeConfig.GetBaseDiskCheckpointId());
    volume.SetIsSystem(volumeConfig.GetIsSystem());
}

void VolumeConfigToVolumeModel(
    const NKikimrBlockStore::TVolumeConfig& volumeConfig,
    NProto::TVolumeModel& volumeModel)
{
    VolumeConfigToVolumeModelFields(volumeConfig, volumeModel);
}

void FillDeviceInfo(
    const google::protobuf::RepeatedPtrField<NProto::TDeviceConfig>& devices,
    const google::protobuf::RepeatedPtrField<NProto::TDeviceMigration>& migrations,
    const google::protobuf::RepeatedPtrField<NProto::TReplica>& replicas,
    NProto::TVolume& volume)
{
    FillDevices(devices, volume);

    for (const auto& migration: migrations) {
        auto* m = volume.AddMigrations();
        auto* source = FindIfPtr(
            devices,
            [&] (const NProto::TDeviceConfig& d) {
                return d.GetDeviceUUID() == migration.GetSourceDeviceId();
            }
        );

        m->SetSourceDeviceId(migration.GetSourceDeviceId());
        if (source) {
            m->SetSourceTransportId(source->GetTransportId());
        } else {
            Y_VERIFY_DEBUG(0);
        }
        const auto& deviceConfig = migration.GetTargetDevice();
        auto* device = m->MutableTargetDevice();
        FillDevice(deviceConfig, device);
    }

    for (const auto& replica: replicas) {
        FillDevices(replica.GetDevices(), *volume.AddReplicas());
    }
}

bool GetThrottlingEnabled(
    const TStorageConfig& config,
    const NProto::TPartitionConfig& partitionConfig)
{
    if (!partitionConfig.GetPerformanceProfile().GetThrottlingEnabled()) {
        return false;
    }

    switch (partitionConfig.GetStorageMediaKind()) {
        case NCloud::NProto::EStorageMediaKind::STORAGE_MEDIA_SSD:
            return config.GetThrottlingEnabledSSD();

        default:
            return config.GetThrottlingEnabled();
    }
}

ui32 GetWriteBlobThreshold(
    const TStorageConfig& config,
    const NCloud::NProto::EStorageMediaKind mediaKind)
{
    if (mediaKind == NCloud::NProto::STORAGE_MEDIA_SSD) {
        return config.GetWriteBlobThresholdSSD();
    }

    return config.GetWriteBlobThreshold();
}

bool CompareVolumeConfigs(
    const NKikimrBlockStore::TVolumeConfig& prevConfig,
    const NKikimrBlockStore::TVolumeConfig& newConfig)
{
    const auto& prevPartitions = prevConfig.GetPartitions();
    const auto& newPartitions = newConfig.GetPartitions();

    if (prevPartitions.size() != newPartitions.size()) {
        return false;
    }

    for (ui32 i = 0; i < static_cast<ui32>(prevPartitions.size()); ++i) {
        if (prevPartitions[i].GetBlockCount()
                != newPartitions[i].GetBlockCount())
        {
            return false;
        }
    }

    return CompareExplicitChannelProfiles(
            prevConfig.GetExplicitChannelProfiles(),
            newConfig.GetExplicitChannelProfiles())
        && prevConfig.GetPerformanceProfileThrottlingEnabled()
            == newConfig.GetPerformanceProfileThrottlingEnabled()
        && prevConfig.GetPerformanceProfileMaxReadBandwidth()
            == newConfig.GetPerformanceProfileMaxReadBandwidth()
        && prevConfig.GetPerformanceProfileMaxWriteBandwidth()
            == newConfig.GetPerformanceProfileMaxWriteBandwidth()
        && prevConfig.GetPerformanceProfileMaxReadIops()
            == newConfig.GetPerformanceProfileMaxReadIops()
        && prevConfig.GetPerformanceProfileMaxWriteIops()
            == newConfig.GetPerformanceProfileMaxWriteIops()
        && prevConfig.GetPerformanceProfileBurstPercentage()
            == newConfig.GetPerformanceProfileBurstPercentage()
        && prevConfig.GetPerformanceProfileMaxPostponedWeight()
            == newConfig.GetPerformanceProfileMaxPostponedWeight()
        && prevConfig.GetPerformanceProfileBoostTime()
            == newConfig.GetPerformanceProfileBoostTime()
        && prevConfig.GetPerformanceProfileBoostRefillTime()
            == newConfig.GetPerformanceProfileBoostRefillTime()
        && prevConfig.GetPerformanceProfileBoostPercentage()
            == newConfig.GetPerformanceProfileBoostPercentage();
}

}   // namespace NCloud::NBlockStore::NStorage
