#pragma once

#include "public.h"

#include "config.h"
#include "part_nonrepl_events_private.h"

#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/core/disk_counters.h>
#include <cloud/blockstore/libs/storage/core/request_info.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>
#include <library/cpp/actors/core/events.h>
#include <library/cpp/actors/core/hfunc.h>
#include <library/cpp/actors/core/mon.h>
#include <library/cpp/containers/stack_vector/stack_vec.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

class TNonreplicatedPartitionActor final
    : public NActors::TActorBootstrapped<TNonreplicatedPartitionActor>
{
private:
    const TStorageConfigPtr Config;
    const TNonreplicatedPartitionConfigPtr PartConfig;
    const NActors::TActorId StatActorId;

    struct TDeviceStat
    {
        TInstant LastTimeoutTs;
        TDuration TimedOutStateDuration;
        TDuration CurrentTimeout;
    };
    TVector<TDeviceStat> DeviceStats;
    bool HasBrokenDevice = false;

    struct TRequest
    {
        TInstant Ts;
        TStackVec<int, 2> DeviceIndices;
        TDuration Timeout;
    };

    THashMap<NActors::TActorId, TRequest> RequestsInProgress;

    bool UpdateCountersScheduled = false;
    TPartitionDiskCountersPtr PartCounters;

    TRequestInfoPtr Poisoner;

public:
    TNonreplicatedPartitionActor(
        TStorageConfigPtr config,
        TNonreplicatedPartitionConfigPtr partConfig,
        NActors::TActorId statActorId);

    ~TNonreplicatedPartitionActor();

    void Bootstrap(const NActors::TActorContext& ctx);

private:
    void KillActors(const NActors::TActorContext& ctx);
    bool CheckReadWriteBlockRange(const TBlockRange64& range) const;
    void ScheduleCountersUpdate(const NActors::TActorContext& ctx);
    void SendStats(const NActors::TActorContext& ctx);
    bool IsInflightLimitReached() const;

    void ReplyAndDie(const NActors::TActorContext& ctx);

private:
    //STFUNC(StateInit);
    STFUNC(StateWork);
    STFUNC(StateZombie);

    template <typename TMethod>
    bool InitRequests(
        const typename TMethod::TRequest& msg,
        const NActors::TActorContext& ctx,
        TRequestInfo& requestInfo,
        const TBlockRange64& blockRange,
        TVector<TDeviceRequest>* deviceRequests,
        TRequest* request);

    void HandleUpdateCounters(
        const TEvNonreplPartitionPrivate::TEvUpdateCounters::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleWakeup(
        const NActors::TEvents::TEvWakeup::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandlePoisonPill(
        const NActors::TEvents::TEvPoisonPill::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleReadBlocksCompleted(
        const TEvNonreplPartitionPrivate::TEvReadBlocksCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleWriteBlocksCompleted(
        const TEvNonreplPartitionPrivate::TEvWriteBlocksCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    void HandleZeroBlocksCompleted(
        const TEvNonreplPartitionPrivate::TEvZeroBlocksCompleted::TPtr& ev,
        const NActors::TActorContext& ctx);

    bool HandleRequests(STFUNC_SIG);

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
