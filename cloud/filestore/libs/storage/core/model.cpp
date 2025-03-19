#include "model.h"

#include <cloud/filestore/libs/storage/model/channel_data_kind.h>

#include <cloud/storage/core/protos/media.pb.h>

#include <ydb/core/protos/filestore_config.pb.h>

namespace NCloud::NFileStore::NStorage {

namespace {

////////////////////////////////////////////////////////////////////////////////

#define THROTTLING_PARAM(paramName)                                            \
    ui64 paramName(                                                            \
        const TStorageConfig& config,                                          \
        const ui32 mediaKind)                                                  \
    {                                                                          \
        switch (mediaKind) {                                                   \
            case NCloud::NProto::STORAGE_MEDIA_SSD:                            \
                return config.GetSSD ## paramName();                           \
            default:                                                           \
                return config.GetHDD ## paramName();                           \
        }                                                                      \
    }                                                                          \
// THROTTLING_PARAM

THROTTLING_PARAM(UnitReadBandwidth);
THROTTLING_PARAM(UnitWriteBandwidth);
THROTTLING_PARAM(UnitReadIops);
THROTTLING_PARAM(UnitWriteIops);
THROTTLING_PARAM(MaxReadBandwidth);
THROTTLING_PARAM(MaxWriteBandwidth);
THROTTLING_PARAM(MaxReadIops);
THROTTLING_PARAM(MaxWriteIops);

#undef THROTTLING_PARAM

ui32 ReadBandwidth(
    const TStorageConfig& config,
    const NKikimrFileStore::TConfig& fileStore,
    const ui32 unitCount)
{
    const auto unitBandwidth = UnitReadBandwidth(config, fileStore.GetStorageMediaKind());
    const auto maxBandwidth = MaxReadBandwidth(config, fileStore.GetStorageMediaKind());

    return Max(
        static_cast<ui64>(fileStore.GetPerformanceProfileMaxReadBandwidth()),
        Min(maxBandwidth, unitCount * unitBandwidth) * 1_MB
    );
}

ui32 WriteBandwidth(
    const TStorageConfig& config,
    const NKikimrFileStore::TConfig& fileStore,
    const ui32 unitCount)
{
    const auto unitBandwidth = UnitWriteBandwidth(config, fileStore.GetStorageMediaKind());
    const auto maxBandwidth = MaxWriteBandwidth(config, fileStore.GetStorageMediaKind());

    auto fileStoreMaxWriteBandwidth = fileStore.GetPerformanceProfileMaxWriteBandwidth();
    if (!fileStoreMaxWriteBandwidth) {
        fileStoreMaxWriteBandwidth = fileStore.GetPerformanceProfileMaxReadBandwidth();
    }

   return Max(
        static_cast<ui64>(fileStoreMaxWriteBandwidth),
        Min(maxBandwidth, unitCount * unitBandwidth) * 1_MB
    );
}

ui32 ReadIops(
    const TStorageConfig& config,
    const NKikimrFileStore::TConfig& fileStore,
    const ui32 unitCount)
{
    const auto unitIops = UnitReadIops(config, fileStore.GetStorageMediaKind());
    const auto maxIops = MaxReadIops(config, fileStore.GetStorageMediaKind());

    return Max(
        static_cast<ui64>(fileStore.GetPerformanceProfileMaxReadIops()),
        Min(maxIops, unitCount * unitIops)
    );
}

ui32 WriteIops(
    const TStorageConfig& config,
    const NKikimrFileStore::TConfig& fileStore,
    const ui32 unitCount)
{
    const auto unitIops = UnitWriteIops(config, fileStore.GetStorageMediaKind());
    const auto maxIops = MaxWriteIops(config, fileStore.GetStorageMediaKind());

    auto fileStoreMaxWriteIops = fileStore.GetPerformanceProfileMaxWriteIops();
    if (!fileStoreMaxWriteIops) {
        fileStoreMaxWriteIops = fileStore.GetPerformanceProfileMaxReadIops();
    }

    return Max(
        static_cast<ui64>(fileStoreMaxWriteIops),
        Min(maxIops, unitCount * unitIops)
    );
}

auto GetAllocationUnit(
    const TStorageConfig& config,
    ui32 mediaKind)
{
    ui64 unit = 0;
    switch (mediaKind) {
        case NCloud::NProto::STORAGE_MEDIA_SSD:
            unit = config.GetAllocationUnitSSD();
            break;

        default:
            unit = config.GetAllocationUnitHDD();
            break;
    }

    Y_VERIFY(unit != 0);
    return unit;
}

////////////////////////////////////////////////////////////////////////////////

struct TPoolKinds
{
    TString System;
    TString Log;
    TString Index;
    TString Fresh;
    TString Mixed;
};

TPoolKinds GetPoolKinds(
    const TStorageConfig& config,
    ui32 mediaKind)
{
    switch (mediaKind) {
        case NCloud::NProto::STORAGE_MEDIA_SSD:
            return {
                config.GetSSDSystemChannelPoolKind(),
                config.GetSSDLogChannelPoolKind(),
                config.GetSSDIndexChannelPoolKind(),
                config.GetSSDFreshChannelPoolKind(),
                config.GetSSDMixedChannelPoolKind(),
            };
        case NCloud::NProto::STORAGE_MEDIA_HYBRID:
            return {
                config.GetHybridSystemChannelPoolKind(),
                config.GetHybridLogChannelPoolKind(),
                config.GetHybridIndexChannelPoolKind(),
                config.GetHybridFreshChannelPoolKind(),
                config.GetHybridMixedChannelPoolKind(),
            };
        case NCloud::NProto::STORAGE_MEDIA_HDD:
        default:
            return {
                config.GetHDDSystemChannelPoolKind(),
                config.GetHDDLogChannelPoolKind(),
                config.GetHDDIndexChannelPoolKind(),
                config.GetHDDFreshChannelPoolKind(),
                config.GetHDDMixedChannelPoolKind(),
            };
    }
}

////////////////////////////////////////////////////////////////////////////////

ui32 ComputeAllocationUnitCount(
    const TStorageConfig& config,
    const NKikimrFileStore::TConfig& fileStore)
{
    if (!fileStore.GetBlocksCount()) {
        return 1;
    }

    double fileStoreSize =
        fileStore.GetBlocksCount() * fileStore.GetBlockSize() / double(1_GB);

    const auto unit = GetAllocationUnit(config, fileStore.GetStorageMediaKind());

    ui32 unitCount = std::ceil(fileStoreSize / unit);
    Y_VERIFY_DEBUG(unitCount >= 1, "size %f unit %lu", fileStoreSize, unit);

    return unitCount;
}

ui32 ComputeMixedChannelCount(
    const TStorageConfig& config,
    const ui32 allocationUnitCount,
    const NKikimrFileStore::TConfig& fileStore)
{
    ui32 mixed = 0;
    for (const auto& channel: fileStore.GetExplicitChannelProfiles()) {
        if (channel.GetDataKind() == static_cast<ui32>(EChannelDataKind::Mixed)) {
            ++mixed;
        }
    }

    return Min(
        Max(
            allocationUnitCount,
            mixed,
            config.GetMinChannelCount()
        ),
        MaxChannelsCount
    );

}

void AddOrModifyChannel(
    const TString& poolKind,
    const ui32 channelId,
    const ui64 size,
    const EChannelDataKind dataKind,
    NKikimrFileStore::TConfig& config)
{
    while (channelId >= config.ExplicitChannelProfilesSize()) {
        config.AddExplicitChannelProfiles();
    }

    auto* profile = config.MutableExplicitChannelProfiles(channelId);
    if (profile->GetPoolKind().Empty()) {
        profile->SetPoolKind(poolKind);
    }

    profile->SetDataKind(static_cast<ui32>(dataKind));
    profile->SetSize(size);
    profile->SetDataKind((ui32)dataKind);
    profile->SetReadIops(config.GetPerformanceProfileMaxReadIops());
    profile->SetWriteIops(config.GetPerformanceProfileMaxWriteIops());
    profile->SetReadBandwidth(config.GetPerformanceProfileMaxReadBandwidth());
    profile->SetWriteBandwidth(config.GetPerformanceProfileMaxWriteBandwidth());
}

void SetupChannels(
    ui32 unitCount,
    bool allocateMixed0Channel,
    const TStorageConfig& config,
    NKikimrFileStore::TConfig& fileStore)
{
    const auto unit = GetAllocationUnit(config, fileStore.GetStorageMediaKind());
    const auto poolKinds = GetPoolKinds(config, fileStore.GetStorageMediaKind());

    AddOrModifyChannel(
        poolKinds.System,
        0,
        128_MB,
        EChannelDataKind::System,
        fileStore
    );

    AddOrModifyChannel(
        poolKinds.Index,
        1,
        16_MB,
        EChannelDataKind::Index,
        fileStore
    );
    AddOrModifyChannel(
        poolKinds.Fresh,
        2,
        128_MB,
        EChannelDataKind::Fresh,
        fileStore
    );

    const ui32 mixed = ComputeMixedChannelCount(config, unitCount, fileStore);
    ui32 mixedChannelStart = 3;

    if (allocateMixed0Channel) {
        AddOrModifyChannel(
            poolKinds.Mixed,
            mixedChannelStart,
            unit * 1_GB,
            EChannelDataKind::Mixed0,
            fileStore
        );

        ++mixedChannelStart;
    }

    for (ui32 i = 0; i < mixed; ++i) {
        AddOrModifyChannel(
            poolKinds.Mixed,
            mixedChannelStart + i,
            unit * 1_GB,
            EChannelDataKind::Mixed,
            fileStore);
    }
}

void OverrideStorageMediaKind(const TStorageConfig& config, NKikimrFileStore::TConfig& fileStore)
{
    using namespace ::NCloud::NProto;
    if (fileStore.GetStorageMediaKind() == STORAGE_MEDIA_HDD) {
        switch(static_cast<EStorageMediaKind>(config.GetHDDMediaKindOverride())) {
            case STORAGE_MEDIA_HYBRID:
                fileStore.SetStorageMediaKind(STORAGE_MEDIA_HYBRID);
                break;
            case STORAGE_MEDIA_SSD:
                fileStore.SetStorageMediaKind(STORAGE_MEDIA_SSD);
                break;
            default:
                break; // pass
        }
    }
}

ui32 NodesLimit(const TStorageConfig& config, NKikimrFileStore::TConfig& fileStore)
{
    ui64 size = fileStore.GetBlocksCount() * fileStore.GetBlockSize();
    ui64 limit = size / config.GetSizeToNodesRatio();

    return Max(limit, (ui64)config.GetDefaultNodesLimit());
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void SetupFileStorePerformanceAndChannels(
    bool allocateMixed0Channel,
    const TStorageConfig& config,
    NKikimrFileStore::TConfig& fileStore)
{
    const auto allocationUnitCount =
        ComputeAllocationUnitCount(config, fileStore);

    OverrideStorageMediaKind(config, fileStore);

    fileStore.SetPerformanceProfileMaxReadBandwidth(
        ReadBandwidth(config, fileStore, allocationUnitCount));
    fileStore.SetPerformanceProfileMaxWriteBandwidth(
        WriteBandwidth(config, fileStore, allocationUnitCount));
    fileStore.SetPerformanceProfileMaxReadIops(
        ReadIops(config, fileStore, allocationUnitCount));
    fileStore.SetPerformanceProfileMaxWriteIops(
        WriteIops(config, fileStore, allocationUnitCount));

    fileStore.SetNodesCount(
        NodesLimit(config, fileStore));

    SetupChannels(allocationUnitCount, allocateMixed0Channel, config, fileStore);
}

}   // namespace NCloud::NFileStore::NStorage
