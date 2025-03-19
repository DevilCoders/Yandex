#include "disk_registry_self_counters.h"

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistrySelfCounters::Init(NMonitoring::TDynamicCountersPtr counters)
{
    FreeBytes = counters->GetCounter("FreeBytes");
    TotalBytes = counters->GetCounter("TotalBytes");
    AllocatedDisks = counters->GetCounter("AllocatedDisks");
    AllocatedDevices = counters->GetCounter("AllocatedDevices");
    DirtyDevices = counters->GetCounter("DirtyDevices");
    DevicesInOnlineState = counters->GetCounter("DevicesInOnlineState");
    DevicesInWarningState = counters->GetCounter("DevicesInWarningState");
    DevicesInErrorState = counters->GetCounter("DevicesInErrorState");
    AgentsInOnlineState = counters->GetCounter("AgentsInOnlineState");
    AgentsInWarningState = counters->GetCounter("AgentsInWarningState");
    AgentsInUnavailableState = counters->GetCounter("AgentsInUnavailableState");
    DisksInOnlineState = counters->GetCounter("DisksInOnlineState");
    DisksInMigrationState = counters->GetCounter("DisksInMigrationState");
    DevicesInMigrationState = counters->GetCounter("DevicesInMigrationState");
    DisksInTemporarilyUnavailableState = counters->GetCounter("DisksInTemporarilyUnavailableState");
    DisksInErrorState = counters->GetCounter("DisksInErrorState");
    PlacementGroups = counters->GetCounter("PlacementGroups");
    FullPlacementGroups = counters->GetCounter("FullPlacementGroups");
    AllocatedDisksInGroups = counters->GetCounter("AllocatedDisksInGroups");
    MaxMigrationTime = counters->GetCounter("MaxMigrationTime");
    PlacementGroupsWithRecentlyBrokenSingleDisk =
        counters->GetCounter("PlacementGroupsWithRecentlyBrokenSingleDisk");
    PlacementGroupsWithRecentlyBrokenTwoOrMoreDisks =
        counters->GetCounter("PlacementGroupsWithRecentlyBrokenTwoOrMoreDisks");
    PlacementGroupsWithBrokenSingleDisk=
        counters->GetCounter("PlacementGroupsWithBrokenSingleDisk");
    PlacementGroupsWithBrokenTwoOrMoreDisks=
        counters->GetCounter("PlacementGroupsWithBrokenTwoOrMoreDisks");

    RejectedAllocations.Register(counters, "RejectedAllocations");
    QueryAvailableStorageErrors.Register(counters, "QueryAvailableStorageErrors");
}

}   // namespace NCloud::NBlockStore::NStorage
