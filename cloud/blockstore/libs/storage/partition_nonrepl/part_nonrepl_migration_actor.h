#pragma once

#include "public.h"

#include "config.h"
#include "part_nonrepl_events_private.h"
#include "part_nonrepl_migration_state.h"

#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/rdma/public.h>
#include <cloud/blockstore/libs/storage/api/disk_registry.h>
#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/core/disk_counters.h>
#include <cloud/blockstore/libs/storage/core/request_info.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>
#include <library/cpp/actors/core/events.h>
#include <library/cpp/actors/core/hfunc.h>
#include <library/cpp/actors/core/mon.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

class TNonreplicatedPartitionMigrationActor final
    : public NActors::TActorBootstrapped<TNonreplicatedPartitionMigrationActor>
{
private:
    const TStorageConfigPtr Config;
    const IProfileLogPtr ProfileLog;
    const IBlockDigestGeneratorPtr BlockDigestGenerator;
    TNonreplicatedPartitionConfigPtr SrcConfig;
    google::protobuf::RepeatedPtrField<NProto::TDeviceMigration> Migrations;
    NRdma::IClientPtr RdmaClient;

    TNonreplicatedPartitionMigrationState State;
    bool MigrationInProgress = false;

    ui64 WriteAndZeroRequestCounter = 0;
    THashMap<ui64, TBlockRange64> WriteAndZeroRequest2Range;

    TInstant LastRangeMigrationStartTs;

    NActors::TActorId SrcActorId;
    NActors::TActorId DstActorId;
    TPartitionDiskCountersPtr SrcCounters;
    TPartitionDiskCountersPtr DstCounters;

    bool UpdateCountersScheduled = false;

    TRequestInfoPtr Poisoner;

public:
    TNonreplicatedPartitionMigrationActor(
        TStorageConfigPtr config,
        IProfileLogPtr profileLog,
        IBlockDigestGeneratorPtr digestGenerator,
        ui64 initialMigrationIndex,
        TString rwClientId,
        TNonreplicatedPartitionConfigPtr srcConfig,
        google::protobuf::RepeatedPtrField<NProto::TDeviceMigration> migrations,
        NRdma::IClientPtr rdmaClient);

    ~TNonreplicatedPartitionMigrationActor();

    void Bootstrap(const NActors::TActorContext& ctx);

private:
    void KillActors(const NActors::TActorContext& ctx);
    void SetupPartitions(const NActors::TActorContext& ctx);
    void ScheduleCountersUpdate(const NActors::TActorContext& ctx);
    void SendStats(const NActors::TActorContext& ctx);

    void ScheduleMigrateNextRange(const NActors::TActorContext& ctx);
    void MigrateNextRange(const NActors::TActorContext& ctx);
    void ContinueMigrationIfNeeded(const NActors::TActorContext& ctx);
    void FinishMigration(const NActors::TActorContext& ctx);

    void ReplyAndDie(const NActors::TActorContext& ctx);

private:
    STFUNC(StateWork);
    STFUNC(StateZombie);

    void HandleRangeMigrated(
        const TEvNonreplPartitionPrivate::TEvRangeMigrated::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleMigrateNextRange(
        const TEvNonreplPartitionPrivate::TEvMigrateNextRange::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleWriteOrZeroCompleted(
        const TEvNonreplPartitionPrivate::TEvWriteOrZeroCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleFinishMigrationResponse(
        const TEvDiskRegistry::TEvFinishMigrationResponse::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleMigrationStateUpdated(
        const TEvVolume::TEvMigrationStateUpdated::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleRWClientIdChanged(
        const TEvVolume::TEvRWClientIdChanged::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandlePartCounters(
        const TEvVolume::TEvNonreplicatedPartitionCounters::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleUpdateCounters(
        const TEvNonreplPartitionPrivate::TEvUpdateCounters::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandlePoisonPill(
        const NActors::TEvents::TEvPoisonPill::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandlePoisonTaken(
        const NActors::TEvents::TEvPoisonTaken::TPtr& ev,
        const NActors::TActorContext& ctx);

    template <typename TMethod>
    void MirrorRequest(
        const typename TMethod::TRequest::TPtr& ev,
        const NActors::TActorContext& ctx);

    BLOCKSTORE_IMPLEMENT_REQUEST(ReadBlocks, TEvService);
    BLOCKSTORE_IMPLEMENT_REQUEST(WriteBlocks, TEvService);
    BLOCKSTORE_IMPLEMENT_REQUEST(ReadBlocksLocal, TEvService);
    BLOCKSTORE_IMPLEMENT_REQUEST(WriteBlocksLocal, TEvService);
    BLOCKSTORE_IMPLEMENT_REQUEST(ZeroBlocks, TEvService);

    BLOCKSTORE_IMPLEMENT_REQUEST(DescribeBlocks, TEvVolume);
    BLOCKSTORE_IMPLEMENT_REQUEST(CompactRange, TEvVolume);
    BLOCKSTORE_IMPLEMENT_REQUEST(GetCompactionStatus, TEvVolume);
    BLOCKSTORE_IMPLEMENT_REQUEST(RebuildMetadata, TEvVolume);
    BLOCKSTORE_IMPLEMENT_REQUEST(GetRebuildMetadataStatus, TEvVolume);
};

}   // namespace NCloud::NBlockStore::NStorage
