#include "part_nonrepl_migration_state.h"

#include "config.h"

#include <cloud/blockstore/libs/storage/core/config.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

////////////////////////////////////////////////////////////////////////////////

TNonreplicatedPartitionMigrationState::TNonreplicatedPartitionMigrationState(
        TStorageConfigPtr config,
        ui64 initialMigrationIndex,
        TString rwClientId,
        TNonreplicatedPartitionConfigPtr srcPartitionActorConfig)
    : Config(std::move(config))
    , RWClientId(std::move(rwClientId))
    , SrcPartitionActorConfig(std::move(srcPartitionActorConfig))
    , LastReportedMigrationIndex(initialMigrationIndex)
{
    MigrationBlockMap = std::make_unique<TCompressedBitmap>(
        SrcPartitionActorConfig->GetBlockCount());
    CurrentMigrationIndex = initialMigrationIndex;
    NextMigrationIndex = CalculateNextMigrationIndex();
    if (CurrentMigrationIndex) {
        MarkMigrated(TBlockRange64(0, CurrentMigrationIndex - 1));
    }
}

////////////////////////////////////////////////////////////////////////////////

void TNonreplicatedPartitionMigrationState::SetupDstPartition(
    TNonreplicatedPartitionConfigPtr config)
{
    DstPartitionActorConfig = std::move(config);
}

void TNonreplicatedPartitionMigrationState::AbortMigration()
{
    MigrationBlockMap.reset();
    CurrentMigrationIndex = 0;
    NextMigrationIndex = 0;
}

bool TNonreplicatedPartitionMigrationState::IsMigrationStarted() const
{
    return !!MigrationBlockMap;
}

void TNonreplicatedPartitionMigrationState::MarkMigrated(TBlockRange64 range)
{
    MigrationBlockMap->Set(
        range.Start,
        Min(SrcPartitionActorConfig->GetBlockCount(), range.End + 1)
    );
}

bool TNonreplicatedPartitionMigrationState::AdvanceMigrationIndex()
{
    auto range = BuildMigrationRange();
    MarkMigrated(range);

    CurrentMigrationIndex = NextMigrationIndex;
    return SkipMigratedRanges();
}

bool TNonreplicatedPartitionMigrationState::SkipMigratedRanges()
{
    while (CurrentMigrationIndex < SrcPartitionActorConfig->GetBlockCount()
            && MigrationBlockMap->Test(CurrentMigrationIndex))
    {
        ++CurrentMigrationIndex;
    }

    NextMigrationIndex = CalculateNextMigrationIndex();
    if (NextMigrationIndex == CurrentMigrationIndex) {
        // migration finished
        MigrationBlockMap.reset();
        return false;
    }

    return true;
}

TBlockRange64 TNonreplicatedPartitionMigrationState::BuildMigrationRange() const
{
    return TBlockRange64::WithLength(
        CurrentMigrationIndex,
        NextMigrationIndex - CurrentMigrationIndex
    );
}

ui64 TNonreplicatedPartitionMigrationState::GetMigratedBlockCount() const
{
    return MigrationBlockMap->Count();
}

TDuration TNonreplicatedPartitionMigrationState::CalculateMigrationTimeout(
    ui32 maxMigrationBandwidthMiBs,
    ui32 expectedDiskAgentSize) const
{
    Y_VERIFY(DstPartitionActorConfig);

    // migration range is 4_MB
    const auto migrationFactorPerAgent = maxMigrationBandwidthMiBs / 4;
    ui32 agentDeviceCount = expectedDiskAgentSize;

    const auto& sourceDevices = SrcPartitionActorConfig->GetDevices();
    const auto& targetDevices = DstPartitionActorConfig->GetDevices();
    Y_VERIFY(sourceDevices.size() == targetDevices.size());
    const auto requests =
        SrcPartitionActorConfig->ToDeviceRequests(BuildMigrationRange());
    // requests.size() should be always equal to 1 if device size is divisible
    // by BlockSize and should almost always be equal to 1 otherwise (may be
    // equal to 2 if our current migration range contains a junction of 2
    // devices)
    for (const auto& request: requests) {
        // calculating device count allocated to this volume among migration
        // sources at agent request.Device.GetAgentId()
        ui32 requestAgentDeviceCount = 0;
        for (int i = 0; i < sourceDevices.size(); ++i) {
            if (sourceDevices[i].GetAgentId() != request.Device.GetAgentId()) {
                continue;
            }

            // checking whether i-th device needs to be migrated
            if (!targetDevices[i].GetDeviceUUID()) {
                continue;
            }

            ++requestAgentDeviceCount;
        }

        if (requestAgentDeviceCount > 0) {
            // choosing the lowest device count among all agents that need to be
            // requested
            if (requestAgentDeviceCount < agentDeviceCount) {
                agentDeviceCount = requestAgentDeviceCount;
            }
        }
    }

    const auto factor = Max(
        migrationFactorPerAgent * agentDeviceCount / expectedDiskAgentSize,
        1U
    );

    return TDuration::Seconds(1) / factor;
}

ui64 TNonreplicatedPartitionMigrationState::CalculateNextMigrationIndex() const
{
    return Min(
        SrcPartitionActorConfig->GetBlockCount(),
        CurrentMigrationIndex + 4_MB / SrcPartitionActorConfig->GetBlockSize());
}

}   // namespace NCloud::NBlockStore::NStorage
