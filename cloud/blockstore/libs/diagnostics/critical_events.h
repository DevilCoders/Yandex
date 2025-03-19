#pragma once

#include "public.h"

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_CRITICAL_EVENTS(xxx)                                        \
    xxx(InvalidTabletConfig)                                                   \
    xxx(ReassignTablet)                                                        \
    xxx(TabletBSFailure)                                                       \
    xxx(DiskAllocationFailure)                                                 \
    xxx(CollectGarbageError)                                                   \
    xxx(VhostQueueRunningError)                                                \
    xxx(MigrationFailed)                                                       \
    xxx(BadMigrationConfig)                                                    \
    xxx(InitFreshBlocksError)                                                  \
    xxx(TrimFreshLogError)                                                     \
    xxx(NrdDestructionError)                                                   \
    xxx(FailedToStartVolumeLocally)                                            \
    xxx(PublishDiskStateError)                                                 \
    xxx(EndpointRestoringError)                                                \
    xxx(AcquiredDiskEraseAttempt)                                              \
    xxx(HangingYdbStatsRequest)                                                \
    xxx(UserNotificationError)                                                 \
    xxx(CpuWaitCounterReadError)                                               \
    xxx(PathDescriptionCacheSyncFailure)                                       \
    xxx(RdmaError)                                                             \
    xxx(MirroredDiskAllocationCleanupFailure)                                  \
    xxx(MirroredDiskAllocationPlacementGroupCleanupFailure)                    \
    xxx(MirroredDiskDeviceReplacementForbidden)                                \
// BLOCKSTORE_CRITICAL_EVENTS

#define BLOCKSTORE_IMPOSSIBLE_EVENTS(xxx)                                      \
    xxx(TabletCommitIdOverflow)                                                \
    xxx(TabletCollectCounterOverflow)                                          \
    xxx(DiskRegistryLogicalPhysicalBlockSizeMismatch)                          \
    xxx(DiskRegistryAgentDeviceNodeIdMismatch)                                 \
    xxx(DiskRegistryPoolDeviceRackMismatch)                                    \
    xxx(DiskRegistryAgentNotFound)                                             \
    xxx(DiskRegistryBadDeviceSizeAdjustment)                                   \
    xxx(DiskRegistryDuplicateDiskInPlacementGroup)                             \
    xxx(DiskRegistryDeviceLocationNotFound)                                    \
    xxx(DiskRegistryDiskNotFound)                                              \
    xxx(DiskRegistryPlacementGroupNotFound)                                    \
    xxx(DiskRegistryDeviceListReferencesNonexistentDisk)                       \
    xxx(DiskRegistryPlacementGroupDiskNotFound)                                \
    xxx(DiskRegistryDeviceNotFound)                                            \
    xxx(DiskRegistryNoScheduledNotification)                                   \
    xxx(DiskRegistryDeviceDoesNotBelongToDisk)                                 \
// BLOCKSTORE_CRITICAL_EVENTS

////////////////////////////////////////////////////////////////////////////////

void InitCriticalEventsCounter(NMonitoring::TDynamicCountersPtr counters);

#define BLOCKSTORE_DECLARE_CRITICAL_EVENT_ROUTINE(name)                        \
    TString Report##name(const TString& message = "");                         \
// BLOCKSTORE_DECLARE_CRITICAL_EVENT_ROUTINE

    BLOCKSTORE_CRITICAL_EVENTS(BLOCKSTORE_DECLARE_CRITICAL_EVENT_ROUTINE)
#undef BLOCKSTORE_DECLARE_CRITICAL_EVENT_ROUTINE

#define BLOCKSTORE_DECLARE_IMPOSSIBLE_EVENT_ROUTINE(name)                      \
    TString Report##name(const TString& message = "");                         \
// BLOCKSTORE_DECLARE_IMPOSSIBLE_EVENT_ROUTINE
    BLOCKSTORE_IMPOSSIBLE_EVENTS(BLOCKSTORE_DECLARE_IMPOSSIBLE_EVENT_ROUTINE)
#undef BLOCKSTORE_DECLARE_IMPOSSIBLE_EVENT_ROUTINE

}   // namespace NCloud::NBlockStore
