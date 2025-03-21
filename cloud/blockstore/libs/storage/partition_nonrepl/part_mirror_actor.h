#pragma once

#include "public.h"

#include "config.h"
#include "part_nonrepl_events_private.h"
#include "part_mirror_state.h"

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

#include <util/generic/deque.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

class TMirrorPartitionActor final
    : public NActors::TActorBootstrapped<TMirrorPartitionActor>
{
private:
    const TStorageConfigPtr Config;
    const IProfileLogPtr ProfileLog;
    const IBlockDigestGeneratorPtr BlockDigestGenerator;
    NRdma::IClientPtr RdmaClient;

    TMirrorPartitionState State;

    TDeque<TPartitionDiskCountersPtr> ReplicaCounters;
    bool UpdateCountersScheduled = false;

    TRequestInfoPtr Poisoner;
    size_t AliveReplicas = 0;

    ui64 RequestId = 0;

    NProto::TError Status;

public:
    TMirrorPartitionActor(
        TStorageConfigPtr config,
        IProfileLogPtr profileLog,
        IBlockDigestGeneratorPtr digestGenerator,
        TString rwClientId,
        TNonreplicatedPartitionConfigPtr partConfig,
        TVector<TDevices> replicas,
        NRdma::IClientPtr rdmaClient);

    ~TMirrorPartitionActor();

    void Bootstrap(const NActors::TActorContext& ctx);

private:
    void KillActors(const NActors::TActorContext& ctx);
    void SetupPartitions(const NActors::TActorContext& ctx);
    void ScheduleCountersUpdate(const NActors::TActorContext& ctx);
    void SendStats(const NActors::TActorContext& ctx);
    void ReplyAndDie(const NActors::TActorContext& ctx);

private:
    STFUNC(StateWork);
    STFUNC(StateZombie);

    void HandleWriteOrZeroCompleted(
        const TEvNonreplPartitionPrivate::TEvWriteOrZeroCompleted::TPtr& ev,
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

    template <typename TMethod>
    void ReadBlocks(
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
