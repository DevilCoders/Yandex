#pragma once

#include "public.h"

#include <cloud/blockstore/libs/storage/protos/disk.pb.h>

#include <cloud/blockstore/libs/kikimr/components.h>
#include <cloud/blockstore/libs/kikimr/events.h>

#include <library/cpp/actors/core/actorid.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_DISK_REGISTRY_REQUESTS_LOCAL(xxx, ...)                      \
    xxx(WaitReady,          __VA_ARGS__)                                       \
// BLOCKSTORE_DISK_REGISTRY_REQUESTS

#define BLOCKSTORE_DISK_REGISTRY_REQUESTS_PROTO(xxx, ...)                      \
    xxx(RegisterAgent,                  __VA_ARGS__)                           \
    xxx(UnregisterAgent,                __VA_ARGS__)                           \
    xxx(AllocateDisk,                   __VA_ARGS__)                           \
    xxx(DeallocateDisk,                 __VA_ARGS__)                           \
    xxx(AcquireDisk,                    __VA_ARGS__)                           \
    xxx(ReleaseDisk,                    __VA_ARGS__)                           \
    xxx(DescribeDisk,                   __VA_ARGS__)                           \
    xxx(UpdateConfig,                   __VA_ARGS__)                           \
    xxx(DescribeConfig,                 __VA_ARGS__)                           \
    xxx(UpdateAgentStats,               __VA_ARGS__)                           \
    xxx(ReplaceDevice,                  __VA_ARGS__)                           \
    xxx(ChangeDeviceState,              __VA_ARGS__)                           \
    xxx(ChangeAgentState,               __VA_ARGS__)                           \
    xxx(FinishMigration,                __VA_ARGS__)                           \
    xxx(BackupDiskRegistryState,        __VA_ARGS__)                           \
    xxx(AllowDiskAllocation,            __VA_ARGS__)                           \
    xxx(MarkDiskForCleanup,             __VA_ARGS__)                           \
    xxx(SetUserId,                      __VA_ARGS__)                           \
    xxx(UpdateDiskBlockSize,            __VA_ARGS__)                           \
    xxx(UpdateDiskReplicaCount,         __VA_ARGS__)                           \
    xxx(MarkReplacementDevice,          __VA_ARGS__)                           \
    xxx(SuspendDevice,                  __VA_ARGS__)                           \
    xxx(ResumeDevice,                   __VA_ARGS__)                           \
    xxx(UpdatePlacementGroupSettings,   __VA_ARGS__)                           \
    xxx(RestoreDiskRegistryState,       __VA_ARGS__)                           \
    xxx(CreateDiskFromDevices,          __VA_ARGS__)                           \
// BLOCKSTORE_DISK_REGISTRY_REQUESTS_PROTO

// requests forwarded from service to disk_registry
#define BLOCKSTORE_DISK_REGISTRY_REQUESTS_FWD_SERVICE(xxx, ...)                \
    xxx(CreatePlacementGroup,           __VA_ARGS__)                           \
    xxx(DestroyPlacementGroup,          __VA_ARGS__)                           \
    xxx(AlterPlacementGroupMembership,  __VA_ARGS__)                           \
    xxx(ListPlacementGroups,            __VA_ARGS__)                           \
    xxx(DescribePlacementGroup,         __VA_ARGS__)                           \
    xxx(CmsAction,                      __VA_ARGS__)                           \
    xxx(QueryAvailableStorage,          __VA_ARGS__)                           \
// BLOCKSTORE_DISK_REGISTRY_REQUESTS_FWD_SERVICE

#define BLOCKSTORE_DISK_REGISTRY_REQUESTS(xxx, ...)                            \
    BLOCKSTORE_DISK_REGISTRY_REQUESTS_LOCAL(xxx,  __VA_ARGS__)                 \
    BLOCKSTORE_DISK_REGISTRY_REQUESTS_PROTO(xxx,  __VA_ARGS__)                 \
// BLOCKSTORE_DISK_REGISTRY_REQUESTS

////////////////////////////////////////////////////////////////////////////////

struct TEvDiskRegistry
{
    //
    // WaitReady
    //

    struct TWaitReadyRequest
    {
    };

    struct TWaitReadyResponse
    {
    };

    //
    // Events declaration
    //

    enum EEvents
    {
        EvBegin = TBlockStoreEvents::DISK_REGISTRY_START,

        EvWaitReadyRequest = EvBegin + 1,
        EvWaitReadyResponse = EvBegin + 2,

        EvRegisterAgentRequest = EvBegin + 3,
        EvRegisterAgentResponse = EvBegin + 4,

        EvUnregisterAgentRequest = EvBegin + 5,
        EvUnregisterAgentResponse = EvBegin + 6,

        EvAllocateDiskRequest = EvBegin + 7,
        EvAllocateDiskResponse = EvBegin + 8,

        EvDeallocateDiskRequest = EvBegin + 9,
        EvDeallocateDiskResponse = EvBegin + 10,

        EvAcquireDiskRequest = EvBegin + 11,
        EvAcquireDiskResponse = EvBegin + 12,

        EvReleaseDiskRequest = EvBegin + 13,
        EvReleaseDiskResponse = EvBegin + 14,

        EvDescribeDiskRequest = EvBegin + 15,
        EvDescribeDiskResponse = EvBegin + 16,

        EvUpdateConfigRequest = EvBegin + 17,
        EvUpdateConfigResponse = EvBegin + 18,

        EvDescribeConfigRequest = EvBegin + 19,
        EvDescribeConfigResponse = EvBegin + 20,

        EvUpdateAgentStatsRequest = EvBegin + 21,
        EvUpdateAgentStatsResponse = EvBegin + 22,

        EvReplaceDeviceRequest = EvBegin + 23,
        EvReplaceDeviceResponse = EvBegin + 24,

        EvChangeDeviceStateRequest = EvBegin + 25,
        EvChangeDeviceStateResponse = EvBegin + 26,

        EvChangeAgentStateRequest = EvBegin + 27,
        EvChangeAgentStateResponse = EvBegin + 28,

        EvFinishMigrationRequest = EvBegin + 29,
        EvFinishMigrationResponse = EvBegin + 30,

        EvBackupDiskRegistryStateRequest = EvBegin + 31,
        EvBackupDiskRegistryStateResponse = EvBegin + 32,

        EvAllowDiskAllocationRequest = EvBegin + 33,
        EvAllowDiskAllocationResponse = EvBegin + 34,

        EvMarkDiskForCleanupRequest = EvBegin + 35,
        EvMarkDiskForCleanupResponse = EvBegin + 36,

        EvSetUserIdRequest = EvBegin + 37,
        EvSetUserIdResponse = EvBegin + 38,

        EvUpdateDiskBlockSizeRequest = EvBegin + 39,
        EvUpdateDiskBlockSizeResponse = EvBegin + 40,

        EvUpdateDiskReplicaCountRequest = EvBegin + 41,
        EvUpdateDiskReplicaCountResponse = EvBegin + 42,

        EvMarkReplacementDeviceRequest = EvBegin + 43,
        EvMarkReplacementDeviceResponse = EvBegin + 44,

        EvSuspendDeviceRequest = EvBegin + 45,
        EvSuspendDeviceResponse = EvBegin + 46,

        EvResumeDeviceRequest = EvBegin + 47,
        EvResumeDeviceResponse = EvBegin + 48,

        EvUpdatePlacementGroupSettingsRequest = EvBegin + 49,
        EvUpdatePlacementGroupSettingsResponse = EvBegin + 50,

        EvRestoreDiskRegistryStateRequest = EvBegin + 51,
        EvRestoreDiskRegistryStateResponse = EvBegin + 52,

        EvCreateDiskFromDevicesRequest = EvBegin + 53,
        EvCreateDiskFromDevicesResponse = EvBegin + 54,

        EvEnd
    };

    static_assert(EvEnd < (int)TBlockStoreEvents::DISK_REGISTRY_END,
        "EvEnd expected to be < TBlockStoreEvents::DISK_REGISTRY_END");

    BLOCKSTORE_DISK_REGISTRY_REQUESTS_LOCAL(BLOCKSTORE_DECLARE_EVENTS)
    BLOCKSTORE_DISK_REGISTRY_REQUESTS_PROTO(BLOCKSTORE_DECLARE_PROTO_EVENTS)
};

}   // namespace NCloud::NBlockStore::NStorage
