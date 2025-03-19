#pragma once

#include "public.h"

#include <cloud/blockstore/public/api/protos/volume.pb.h>

#include <cloud/blockstore/libs/kikimr/components.h>
#include <cloud/blockstore/libs/kikimr/events.h>
#include <cloud/blockstore/libs/storage/core/disk_counters.h>
#include <cloud/blockstore/libs/storage/protos/volume.pb.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_VOLUME_REQUESTS(xxx, ...)                                   \
    xxx(AddClient,                __VA_ARGS__)                                 \
    xxx(RemoveClient,             __VA_ARGS__)                                 \
    xxx(WaitReady,                __VA_ARGS__)                                 \
    xxx(DescribeBlocks,           __VA_ARGS__)                                 \
    xxx(GetPartitionInfo,         __VA_ARGS__)                                 \
    xxx(CompactRange,             __VA_ARGS__)                                 \
    xxx(GetCompactionStatus,      __VA_ARGS__)                                 \
    xxx(ReallocateDisk,           __VA_ARGS__)                                 \
    xxx(GetVolumeLoadInfo,        __VA_ARGS__)                                 \
    xxx(GetUsedBlocks,            __VA_ARGS__)                                 \
    xxx(DeleteCheckpointData,     __VA_ARGS__)                                 \
    xxx(UpdateUsedBlocks,         __VA_ARGS__)                                 \
    xxx(RebuildMetadata,          __VA_ARGS__)                                 \
    xxx(GetRebuildMetadataStatus, __VA_ARGS__)                                 \
// BLOCKSTORE_VOLUME_REQUESTS

// requests forwarded from service to volume
#define BLOCKSTORE_VOLUME_REQUESTS_FWD_SERVICE(xxx, ...)                       \
    xxx(ReadBlocks,         __VA_ARGS__)                                       \
    xxx(WriteBlocks,        __VA_ARGS__)                                       \
    xxx(ZeroBlocks,         __VA_ARGS__)                                       \
    xxx(StatVolume,         __VA_ARGS__)                                       \
    xxx(CreateCheckpoint,   __VA_ARGS__)                                       \
    xxx(DeleteCheckpoint,   __VA_ARGS__)                                       \
    xxx(GetChangedBlocks,   __VA_ARGS__)                                       \
    xxx(ReadBlocksLocal,    __VA_ARGS__)                                       \
    xxx(WriteBlocksLocal,   __VA_ARGS__)                                       \
// BLOCKSTORE_VOLUME_REQUESTS_FWD_SERVICE

////////////////////////////////////////////////////////////////////////////////

struct TEvVolume
{
    //
    // ReacquireDisk
    //

    struct TReacquireDisk
    {
    };

    //
    // UpdateMigrationState
    //

    struct TUpdateMigrationState
    {
        ui64 MigrationIndex;

        TUpdateMigrationState(ui64 migrationIndex)
            : MigrationIndex(migrationIndex)
        {
        }
    };

    //
    // MigrationStateUpdated
    //

    struct TMigrationStateUpdated
    {
    };

    //
    // RWClientIdChanged
    //

    struct TRWClientIdChanged
    {
        TString RWClientId;

        TRWClientIdChanged(TString rwClientId)
            : RWClientId(std::move(rwClientId))
        {
        }
    };

    //
    // NonreplicatedPartitionCounters
    //

    struct TNonreplicatedPartitionCounters
    {
        TPartitionDiskCountersPtr DiskCounters;

        TNonreplicatedPartitionCounters(
                TPartitionDiskCountersPtr diskCounters)
            : DiskCounters(std::move(diskCounters))
        {
        }
    };

    //
    // RdmaUnavailable
    //

    struct TRdmaUnavailable
    {
    };

    //
    // Events declaration
    //

    enum EEvents
    {
        EvBegin = TBlockStoreEvents::VOLUME_START,

        EvAddClientRequest = EvBegin + 5,
        EvAddClientResponse = EvBegin + 6,

        EvRemoveClientRequest = EvBegin + 7,
        EvRemoveClientResponse = EvBegin + 8,

        EvWaitReadyRequest = EvBegin + 9,
        EvWaitReadyResponse = EvBegin + 10,

        EvDescribeBlocksRequest = EvBegin + 11,
        EvDescribeBlocksResponse = EvBegin + 12,

        EvGetPartitionInfoRequest = EvBegin + 13,
        EvGetPartitionInfoResponse = EvBegin + 14,

        EvCompactRangeRequest = EvBegin + 15,
        EvCompactRangeResponse = EvBegin + 16,

        EvGetCompactionStatusRequest = EvBegin + 17,
        EvGetCompactionStatusResponse = EvBegin + 18,

        EvReacquireDisk = EvBegin + 19,

        EvReallocateDiskRequest = EvBegin + 20,
        EvReallocateDiskResponse = EvBegin + 21,

        EvGetVolumeLoadInfoRequest = EvBegin + 22,
        EvGetVolumeLoadInfoResponse = EvBegin + 23,

        EvGetUsedBlocksRequest = EvBegin + 24,
        EvGetUsedBlocksResponse = EvBegin + 25,

        EvDeleteCheckpointDataRequest = EvBegin + 26,
        EvDeleteCheckpointDataResponse = EvBegin + 27,

        EvUpdateUsedBlocksRequest = EvBegin + 28,
        EvUpdateUsedBlocksResponse = EvBegin + 29,

        EvUpdateMigrationState = EvBegin + 30,
        EvMigrationStateUpdated = EvBegin + 31,

        EvRWClientIdChanged = EvBegin + 32,

        EvNonreplicatedPartitionCounters = EvBegin + 33,

        EvRebuildMetadataRequest = EvBegin + 34,
        EvRebuildMetadataResponse = EvBegin + 35,

        EvGetRebuildMetadataStatusRequest = EvBegin + 36,
        EvGetRebuildMetadataStatusResponse = EvBegin + 37,

        EvRdmaUnavailable = EvBegin + 38,

        EvEnd
    };

    static_assert(EvEnd < (int)TBlockStoreEvents::VOLUME_END,
        "EvEnd expected to be < TBlockStoreEvents::VOLUME_END");

    BLOCKSTORE_VOLUME_REQUESTS(BLOCKSTORE_DECLARE_PROTO_EVENTS)

    using TEvReacquireDisk = TRequestEvent<
        TReacquireDisk,
        EvReacquireDisk
    >;

    using TEvUpdateMigrationState = TRequestEvent<
        TUpdateMigrationState,
        EvUpdateMigrationState
    >;

    using TEvMigrationStateUpdated = TRequestEvent<
        TMigrationStateUpdated,
        EvMigrationStateUpdated
    >;

    using TEvRWClientIdChanged = TRequestEvent<
        TRWClientIdChanged,
        EvRWClientIdChanged
    >;

    using TEvNonreplicatedPartitionCounters = TRequestEvent<
        TNonreplicatedPartitionCounters,
        EvNonreplicatedPartitionCounters
    >;

    using TEvRdmaUnavailable = TRequestEvent<
        TRdmaUnavailable,
        EvRdmaUnavailable
    >;
};

}   // namespace NCloud::NBlockStore::NStorage
