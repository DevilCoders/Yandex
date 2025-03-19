#pragma once

#include "public.h"

#include <cloud/blockstore/libs/common/block_range.h>
#include <cloud/blockstore/libs/common/compressed_bitmap.h>

#include <cloud/blockstore/libs/storage/core/public.h>

#include <library/cpp/actors/core/actorid.h>

#include <util/generic/vector.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

class TNonreplicatedPartitionMigrationState
{
private:
    const TStorageConfigPtr Config;
    TString RWClientId;

    TNonreplicatedPartitionConfigPtr SrcPartitionActorConfig;
    TNonreplicatedPartitionConfigPtr DstPartitionActorConfig;

    std::unique_ptr<TCompressedBitmap> MigrationBlockMap;
    ui64 LastReportedMigrationIndex = 0;
    ui64 CurrentMigrationIndex = 0;
    ui64 NextMigrationIndex = 0;

public:
    TNonreplicatedPartitionMigrationState(
        TStorageConfigPtr config,
        ui64 initialMigrationIndex,
        TString rwClientId,
        TNonreplicatedPartitionConfigPtr srcPartitionActorConfig);

public:
    void SetupDstPartition(TNonreplicatedPartitionConfigPtr config);
    void AbortMigration();
    bool IsMigrationStarted() const;
    void MarkMigrated(TBlockRange64 range);
    bool SkipMigratedRanges();
    bool AdvanceMigrationIndex();
    TBlockRange64 BuildMigrationRange() const;
    ui64 GetMigratedBlockCount() const;
    TDuration CalculateMigrationTimeout(
        ui32 maxMigrationBandwidthMiBs,
        ui32 expectedDiskAgentSize) const;
    void SetRWClientId(TString rwClientId)
    {
        RWClientId = std::move(rwClientId);
    }
    const TString& GetRWClientId() const
    {
        return RWClientId;
    }

    ui64 GetLastReportedMigrationIndex() const
    {
        return LastReportedMigrationIndex;
    }

    void SetLastReportedMigrationIndex(ui64 i)
    {
        LastReportedMigrationIndex = i;
    }

private:
    ui64 CalculateNextMigrationIndex() const;
};

}   // namespace NCloud::NBlockStore::NStorage
