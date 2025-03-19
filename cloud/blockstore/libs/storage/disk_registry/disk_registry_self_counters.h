#pragma once

#include "public.h"

#include <cloud/storage/core/libs/diagnostics/solomon_counters.h>

#include <library/cpp/monlib/dynamic_counters/counters.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TDiskRegistrySelfCounters
{
    NMonitoring::TDynamicCounters::TCounterPtr FreeBytes;
    NMonitoring::TDynamicCounters::TCounterPtr TotalBytes;
    NMonitoring::TDynamicCounters::TCounterPtr AllocatedDisks;
    NMonitoring::TDynamicCounters::TCounterPtr AllocatedDevices;
    NMonitoring::TDynamicCounters::TCounterPtr DirtyDevices;
    NMonitoring::TDynamicCounters::TCounterPtr DevicesInOnlineState;
    NMonitoring::TDynamicCounters::TCounterPtr DevicesInWarningState;
    NMonitoring::TDynamicCounters::TCounterPtr DevicesInErrorState;
    NMonitoring::TDynamicCounters::TCounterPtr AgentsInOnlineState;
    NMonitoring::TDynamicCounters::TCounterPtr AgentsInWarningState;
    NMonitoring::TDynamicCounters::TCounterPtr AgentsInUnavailableState;
    NMonitoring::TDynamicCounters::TCounterPtr DisksInOnlineState;
    NMonitoring::TDynamicCounters::TCounterPtr DisksInMigrationState;
    NMonitoring::TDynamicCounters::TCounterPtr DevicesInMigrationState;
    NMonitoring::TDynamicCounters::TCounterPtr DisksInTemporarilyUnavailableState;
    NMonitoring::TDynamicCounters::TCounterPtr DisksInErrorState;
    NMonitoring::TDynamicCounters::TCounterPtr PlacementGroups;
    NMonitoring::TDynamicCounters::TCounterPtr FullPlacementGroups;
    NMonitoring::TDynamicCounters::TCounterPtr AllocatedDisksInGroups;
    NMonitoring::TDynamicCounters::TCounterPtr MaxMigrationTime;
    NMonitoring::TDynamicCounters::TCounterPtr PlacementGroupsWithRecentlyBrokenSingleDisk;
    NMonitoring::TDynamicCounters::TCounterPtr PlacementGroupsWithRecentlyBrokenTwoOrMoreDisks;
    NMonitoring::TDynamicCounters::TCounterPtr PlacementGroupsWithBrokenSingleDisk;
    NMonitoring::TDynamicCounters::TCounterPtr PlacementGroupsWithBrokenTwoOrMoreDisks;

    TCumulativeCounter RejectedAllocations;
    TCumulativeCounter QueryAvailableStorageErrors;

    void Init(NMonitoring::TDynamicCountersPtr counters);
};

}   // namespace NCloud::NBlockStore::NStorage
