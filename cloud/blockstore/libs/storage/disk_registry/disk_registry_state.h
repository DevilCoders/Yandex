#pragma once

#include "public.h"

#include "disk_registry_database.h"
#include "disk_registry_private.h"
#include "disk_registry_self_counters.h"

#include <cloud/blockstore/libs/storage/core/public.h>
#include <cloud/blockstore/libs/storage/disk_registry/model/agent_list.h>
#include <cloud/blockstore/libs/storage/disk_registry/model/device_list.h>
#include <cloud/blockstore/libs/storage/disk_registry/model/replica_table.h>
#include <cloud/blockstore/libs/storage/protos/disk.pb.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TAgentStorageInfo
{
    ui64 ChunkSize = 0;
    ui32 ChunkCount = 0;
};

////////////////////////////////////////////////////////////////////////////////

struct TDiskInfo
{
    TVector<NProto::TDeviceConfig> Devices;
    TVector<NProto::TDeviceMigration> Migrations;
    TVector<TFinishedMigration> FinishedMigrations;
    TVector<TVector<NProto::TDeviceConfig>> Replicas;
    ui32 LogicalBlockSize = 0;
    NProto::EDiskState State = NProto::DISK_STATE_ONLINE;
    TInstant StateTs;
    TString PlacementGroupId;
    TString CloudId;
    TString FolderId;
    TString UserId;
    TVector<TString> DeviceReplacementIds;
};

struct TRackInfo
{
    // devices on agents in unavailable/warning states are considired to
    // be unavailable/warning in this struct's counters

    struct TAgentInfo
    {
        TString AgentId;
        ui32 NodeId = 0;
        ui32 FreeDevices = 0;
        ui32 DirtyDevices = 0;
        ui32 AllocatedDevices = 0;
        ui32 WarningDevices = 0;
        ui32 UnavailableDevices = 0;
        ui32 BrokenDevices = 0;
        ui32 TotalDevices = 0;

        TAgentInfo() = default;

        TAgentInfo(TString agentId, ui32 nodeId)
            : AgentId(std::move(agentId))
            , NodeId(nodeId)
        {
        }
    };

    TString Name;
    TVector<TAgentInfo> AgentInfos;
    ui64 FreeBytes = 0;
    ui64 WarningBytes = 0;
    ui64 TotalBytes = 0;
    TSet<TString> PlacementGroups;

    TRackInfo(TString name)
        : Name(std::move(name))
    {
    }
};

struct TBrokenGroupInfo
{
    ui32 TotalBrokenDiskCount = 0;
    ui32 RecentlyBrokenDiskCount = 0;
};

struct TPlacementGroupInfo
{
    NProto::TPlacementGroupConfig Config;

    // updated asynchronously during PublishCounters()
    bool Full = false;
    ui64 BiggestDiskSize = 0;
    TString BiggestDiskId;

    TPlacementGroupInfo() = default;

    TPlacementGroupInfo(NProto::TPlacementGroupConfig config)
        : Config(std::move(config))
    {
    }
};

class TDiskRegistryState
{
    using TDeviceId = TString;
    using TDiskId = TString;
    using TNodeId = ui32;

    struct TDiskState
    {
        TString CloudId;
        TString FolderId;
        TString UserId;
        TVector<TDeviceId> Devices;
        THashMap<TDeviceId, TDeviceId> MigrationTarget2Source;
        THashMap<TDeviceId, TDeviceId> MigrationSource2Target;
        TVector<TFinishedMigration> FinishedMigrations;

        bool AcquireInProgress = false;
        ui32 LogicalBlockSize = 0;
        TString PlacementGroupId;

        NProto::EDiskState State = NProto::DISK_STATE_ONLINE;
        TInstant StateTs;

        ui32 ReplicaCount = 0;
        TString MasterDiskId;

        TVector<TDeviceId> DeviceReplacementIds;
    };

    struct TVolumeDeviceOverrides
    {
        THashMap<TDeviceId, ui64> Device2BlockCount;
    };

    using TKnownAgents = THashMap<TString, THashSet<TString>>;
    using TDeviceOverrides = THashMap<TString, TVolumeDeviceOverrides>;

    using TPlacementGroups = THashMap<TString, TPlacementGroupInfo>;

private:
    const TStorageConfigPtr StorageConfig;
    const NMonitoring::TDynamicCountersPtr Counters;
    mutable TDiskRegistrySelfCounters SelfCounters;

    TAgentList AgentList;
    TDeviceList DeviceList;

    THashMap<TDiskId, TDiskState> Disks;
    THashSet<TDiskId> DisksToCleanup;
    TKnownAgents KnownAgents;
    TDeviceOverrides DeviceOverrides;
    TPlacementGroups PlacementGroups;
    TVector<TBrokenDiskInfo> BrokenDisks;
    THashMap<TString, ui64> DisksToNotify;
    ui64 DisksToNotifySeqNo = 1;

    NProto::TDiskRegistryConfig CurrentConfig;

    TVector<TDiskStateUpdate> DiskStateUpdates;
    ui64 DiskStateSeqNo = 0;

    THashMap<TDiskId, ui64> OutdatedVolumeConfigs;
    ui64 VolumeConfigSeqNo = 0;

    bool DiskAllocationAllowed = false;

    struct TDeviceMigrationCompare
    {
        bool operator ()(
            const TDeviceMigration& lhs,
            const TDeviceMigration& rhs) const
        {
            return std::less<>()(
                std::tie(lhs.DiskId, lhs.SourceDeviceId),
                std::tie(rhs.DiskId, rhs.SourceDeviceId));
        }
    };

    TSet<TDeviceMigration, TDeviceMigrationCompare> Migrations;
    ui32 DeviceMigrationsInProgress = 0;

    TReplicaTable ReplicaTable;

    THashSet<TDiskId> ErrorNotifications;

    THashMap<TString, NProto::TDevicePoolConfig> DevicePoolConfigs;

public:
    TDiskRegistryState(
        TStorageConfigPtr storageConfig,
        NMonitoring::TDynamicCountersPtr counters,
        NProto::TDiskRegistryConfig config,
        TVector<NProto::TAgentConfig> agents,
        TVector<NProto::TDiskConfig> disks,
        TVector<NProto::TPlacementGroupConfig> placementGroups,
        TVector<TBrokenDiskInfo> brokenDisks,
        TVector<TString> disksToNotify,
        TVector<TDiskStateUpdate> diskStateUpdates,
        ui64 diskStateSeqNo,
        TVector<TDeviceId> dirtyDevices,
        bool diskAllocationAllowed,
        TVector<TString> disksToCleanup,
        TVector<TDiskId> errorNotifications,
        TVector<TString> outdatedVolumeConfigs,
        TVector<TDeviceId> suspendedDevices);

public:
    NProto::TError RegisterAgent(
        TDiskRegistryDatabase& db,
        NProto::TAgentConfig config,
        TInstant timestamp,
        TVector<TDiskStateUpdate>* affectedDisks,
        THashMap<TString, ui64>* notifiedDisks);

    NProto::TError UnregisterAgent(
        TDiskRegistryDatabase& db,
        ui32 nodeId);

    struct TAllocateDiskParams
    {
        TString DiskId;
        TString CloudId;
        TString FolderId;
        TString PlacementGroupId;
        ui32 BlockSize = 0;
        ui64 BlocksCount = 0;
        ui32 ReplicaCount = 0;
        TString MasterDiskId;

        TVector<TString> AgentIds;
        TString PoolName;

        NProto::EStorageMediaKind MediaKind =
            NProto::STORAGE_MEDIA_SSD_NONREPLICATED;
    };

    struct TAllocateDiskResult
    {
        TVector<NProto::TDeviceConfig> Devices;
        TVector<NProto::TDeviceMigration> Migrations;
        TVector<TVector<NProto::TDeviceConfig>> Replicas;
        TVector<TString> DeviceReplacementIds;

        NProto::EVolumeIOMode IOMode = {};
        TInstant IOModeTs;
        bool MuteIOErrors = false;
    };

    NProto::TError AllocateDisk(
        TInstant now,
        TDiskRegistryDatabase& db,
        const TAllocateDiskParams& params,
        TAllocateDiskResult* result);

    NProto::TError DeallocateDisk(
        TDiskRegistryDatabase& db,
        const TString& diskId);

    NProto::TError GetDiskDevices(
        const TString& diskId,
        TVector<NProto::TDeviceConfig>& devices) const;

    NProto::TError GetDiskInfo(const TString& diskId, TDiskInfo& diskInfo) const;

    bool FilterDevicesAtUnavailableAgents(TDiskInfo& diskInfo) const;

    NProto::TError StartAcquireDisk(
        const TString& diskId,
        TDiskInfo& diskInfo);

    NProto::TError MarkDiskForCleanup(
        TDiskRegistryDatabase& db,
        const TString& diskId);

    void FinishAcquireDisk(const TString& diskId);

    bool IsAcquireInProgress(const TString& diskId) const;

    const TVector<NProto::TAgentConfig>& GetAgents() const;

    NProto::TError UpdateConfig(
        TDiskRegistryDatabase& db,
        NProto::TDiskRegistryConfig config,
        bool ignoreVersion,
        TVector<TString>& affectedDisks);

    const NProto::TDiskRegistryConfig& GetConfig() const;

    ui32 GetConfigVersion() const;

    TVector<TString> GetDiskIds() const;
    TVector<TString> GetMasterDiskIds() const;
    bool IsMasterDisk(const TString& diskId) const;

    NProto::TDeviceConfig GetDevice(const TString& id) const;
    TString GetDeviceId(const TString& agentId, const TString& path) const;

    NProto::TError GetDependentDisks(
        const TString& agentId,
        const TString& path,
        TVector<TDiskId>* diskIds) const;

    TVector<NProto::TDeviceConfig> GetDirtyDevices() const;
    bool MarkDeviceAsClean(TDiskRegistryDatabase& db, const TDeviceId& uuid);
    bool MarkDeviceAsDirty(TDiskRegistryDatabase& db, const TDeviceId& uuid);

    NProto::TError CreatePlacementGroup(
        TDiskRegistryDatabase& db,
        const TString& groupId);
    NProto::TError UpdatePlacementGroupSettings(
        TDiskRegistryDatabase& db,
        const TString& groupId,
        ui32 configVersion,
        NProto::TPlacementGroupSettings settings);
    NProto::TError DestroyPlacementGroup(
        TDiskRegistryDatabase& db,
        const TString& groupId,
        TVector<TDiskId>& affectedDisks);
    NProto::TError AlterPlacementGroupMembership(
        TDiskRegistryDatabase& db,
        const TString& groupId,
        ui32 configVersion,
        TVector<TString>& disksToAdd,
        const TVector<TString>& disksToRemove);
    const TPlacementGroups& GetPlacementGroups() const
    {
        return PlacementGroups;
    }
    const NProto::TPlacementGroupConfig* FindPlacementGroup(const TString& groupId) const;

    const TVector<TBrokenDiskInfo>& GetBrokenDisks() const
    {
        return BrokenDisks;
    }

    void DeleteBrokenDisks(TDiskRegistryDatabase& db);

    const THashMap<TString, ui64>& GetDisksToNotify() const
    {
        return DisksToNotify;
    }

    ui64 AddDiskToNotify(TDiskRegistryDatabase& db, TString diskId);

    void DeleteDiskToNotify(
        TDiskRegistryDatabase& db,
        const TString& diskId,
        ui64 seqNo);

    const TVector<TDiskStateUpdate>& GetDiskStateUpdates() const
    {
        return DiskStateUpdates;
    }

    void DeleteDiskStateUpdate(TDiskRegistryDatabase& db, ui64 maxSeqNo);

    void AddErrorNotification(TDiskRegistryDatabase& db, TDiskId diskId);
    void DeleteErrorNotification(TDiskRegistryDatabase& db, const TDiskId& diskId);
    const THashSet<TDiskId>& GetErrorNotifications() const
    {
        return ErrorNotifications;
    }

    TVector<TString> CollectBrokenDevices(const NProto::TAgentStats& stats) const;
    NProto::TError UpdateAgentCounters(const NProto::TAgentStats& source);
    void PublishCounters(TInstant now);

    void DeleteDiskStateChanges(
        TDiskRegistryDatabase& db,
        const TString& diskId,
        ui64 seqNo);

    NProto::TError UpdateAgentState(
        TDiskRegistryDatabase& db,
        TString agentId,
        NProto::EAgentState state,
        TInstant now,
        TString reason,
        TVector<TDiskStateUpdate>& affectedDisks);

    NProto::TError UpdateCmsHostState(
        TDiskRegistryDatabase& db,
        TString agentId,
        NProto::EAgentState state,
        TInstant now,
        bool dryRun,
        TVector<TDiskStateUpdate>& affectedDisks,
        TDuration& timeout);

    TMaybe<NProto::EAgentState> GetAgentState(const TString& agentId) const;
    TMaybe<TInstant> GetAgentCmsTs(const TString& agentId) const;

    NProto::TError UpdateDeviceState(
        TDiskRegistryDatabase& db,
        const TString& deviceId,
        NProto::EDeviceState state,
        TInstant now,
        TString reason,
        TMaybe<TDiskStateUpdate>& affectedDisk);

    NProto::TError UpdateCmsDeviceState(
        TDiskRegistryDatabase& db,
        const TString& deviceId,
        NProto::EDeviceState state,
        TInstant now,
        bool dryRun,
        TMaybe<TDiskStateUpdate>& affectedDisk,
        TDuration& timeout);

    NProto::TError ReplaceDevice(
        TDiskRegistryDatabase& db,
        const TString& diskId,
        const TString& deviceId,
        TInstant timestamp,
        TMaybe<TDiskStateUpdate>* update);

    TString GetAgentId(TNodeId nodeId) const;

    TDeque<TRackInfo> GatherRacksInfo() const;
    THashMap<TString, TBrokenGroupInfo> GatherBrokenGroupsInfo(
        TInstant now,
        TDuration period) const;

    const NProto::TAgentConfig* FindAgent(const TString& id) const
    {
        return AgentList.FindAgent(id);
    }

    const NProto::TAgentConfig* FindAgent(TNodeId nodeId) const
    {
        return AgentList.FindAgent(nodeId);
    }

    TDiskId FindDisk(const TDeviceId& uuid) const;

    NProto::TDiskRegistryStateBackup BackupState() const;

    NProto::TError AllowDiskAllocation(TDiskRegistryDatabase& db, bool allow);
    bool IsDiskAllocationAllowed() const;

    TResultOrError<NProto::TDeviceConfig> StartDeviceMigration(
        TDiskRegistryDatabase& db,
        const TDiskId& diskId,
        const TDeviceId& deviceId);

    NProto::TError FinishDeviceMigration(
        TDiskRegistryDatabase& db,
        const TDiskId& diskId,
        const TDeviceId& sourceId,
        const TDeviceId& targetId,
        TInstant timestamp,
        TMaybe<TDiskStateUpdate>* affectedDisk);

    TVector<TDeviceMigration> BuildMigrationList() const;

    bool IsMigrationListEmpty() const
    {
        return Migrations.empty();
    }

    bool IsReadyForCleanup(const TDiskId& diskId) const;

    NProto::TError SetUserId(
        TDiskRegistryDatabase& db,
        const TDiskId& diskId,
        const TString& userId);

    TVector<TDiskId> GetOutdatedVolumeConfigs() const;

    std::pair<TVolumeConfig, ui64> GetVolumeConfigUpdate(const TDiskId& diskId) const;

    void DeleteOutdatedVolumeConfig(TDiskRegistryDatabase& db, const TDiskId& diskId);

    NProto::TError UpdateDiskBlockSize(TDiskRegistryDatabase& db,
        const TDiskId& diskId, ui32 blockSize, bool force);

    NProto::TError AllocateDiskReplicas(
        TInstant now,
        TDiskRegistryDatabase& db,
        const TDiskId& masterDiskId,
        ui32 replicaCount);

    NProto::TError DeallocateDiskReplicas(
        TDiskRegistryDatabase& db,
        const TDiskId& masterDiskId,
        ui32 replicaCount);

    NProto::TError UpdateDiskReplicaCount(
        TDiskRegistryDatabase& db,
        const TDiskId& masterDiskId,
        ui32 replicaCount);

    TResultOrError<TVector<TAgentStorageInfo>> QueryAvailableStorage(
        const TString& agentId,
        const TString& poolName,
        NProto::EDevicePoolKind poolKind) const;

    NProto::TError MarkReplacementDevice(
        TDiskRegistryDatabase& db,
        const TDiskId& diskId,
        const TDeviceId& deviceId,
        bool isReplacement);

    NProto::TError SuspendDevice(TDiskRegistryDatabase& db, const TDeviceId& id);
    NProto::TError ResumeDevice(TDiskRegistryDatabase& db, const TDeviceId& id);
    bool IsSuspendedDevice(const TDeviceId& id) const;
    TVector<TDeviceId> GetSuspendedDevices() const;

    NProto::TError CreateDiskFromDevices(
        TDiskRegistryDatabase& db,
        bool force,
        const TDiskId& diskId,
        ui32 blockSize,
        const TVector<NProto::TDeviceConfig>& devices);

    // for tests
    const TReplicaTable& GetReplicaTable() const
    {
        return ReplicaTable;
    }

private:
    void ProcessDisksToNotify(TVector<TString> disksToNotify);
    void ProcessErrorNotifications(TVector<TDiskId> errorNotifications);
    void ProcessConfig(const NProto::TDiskRegistryConfig& config);
    void ProcessDisks(TVector<NProto::TDiskConfig> disks);
    void ProcessPlacementGroups(TVector<NProto::TPlacementGroupConfig> placementGroups);
    void ProcessAgents();
    void ProcessDisksToCleanup(TVector<TString> diskIds);
    void ProcessOutdatedVolumeConfigs(TVector<TString> diskIds);

    void AddMigration(
        const TDiskState& disk,
        const TString& diskId,
        const TString& sourceDeviceId);
    void FillMigrations();

    TDiskState* FindDiskState(const TDiskId& diskId);

    template <typename T>
    bool RemoveAgent(
        TDiskRegistryDatabase& db,
        const T& id);

    void RemoveAgent(
        TDiskRegistryDatabase& db,
        const NProto::TAgentConfig& agent);

    void RemoveAgentFromNode(
        TDiskRegistryDatabase& db,
        NProto::TAgentConfig& agent,
        TInstant timestamp,
        TVector<TDiskStateUpdate>* affectedDisks,
        THashMap<TString, ui64>* notifiedDisks);

    [[nodiscard]] NProto::TError GetDiskDevices(
        const TString& diskId,
        const TDiskState& disk,
        TVector<NProto::TDeviceConfig>& devices) const;

    [[nodiscard]] NProto::TError GetDiskMigrations(
        const TDiskState& disk,
        TVector<NProto::TDeviceMigration>& migrations) const;

    void ValidateAgent(const NProto::TAgentConfig& agent);

    bool IsKnownDevice(const TDeviceId& uuid) const;

    NProto::TDiskConfig BuildDiskConfig(
        TDiskId diskId,
        const TDiskState& diskState) const;

    auto FindDeviceLocation(const TDeviceId& uuid)
        -> std::pair<NProto::TAgentConfig*, NProto::TDeviceConfig*>;

    auto FindDeviceLocation(const TDeviceId& uuid) const
        -> std::pair<const NProto::TAgentConfig*, const NProto::TDeviceConfig*>;

    const NProto::TDeviceConfig* FindDevice(
        const TString& agentId,
        const TString& path) const;

    NProto::EDiskState CalculateDiskState(const TDiskState& disk) const;

    TMaybe<TDiskStateUpdate> TryUpdateDiskState(
        TDiskRegistryDatabase& db,
        const TString& diskId,
        TInstant timestamp);

    TMaybe<TDiskStateUpdate> TryUpdateDiskState(
        TDiskRegistryDatabase& db,
        const TString& diskId,
        TDiskState& disk,
        TInstant timestamp);

    NProto::TPlacementGroupConfig::TDiskInfo* CollectRacks(
        const TString& diskId,
        NProto::TPlacementGroupConfig& placementGroup,
        THashSet<TString>* otherDiskRacks);

    void CollectRacks(
        const NProto::TPlacementGroupConfig& placementGroup,
        THashSet<TString>* otherDiskRacks);

    THashSet<TString> CollectOtherRacks(
        const TDiskId& diskId,
        const TDiskState& disk,
        TStringBuf callerName);

    void RebuildDiskPlacementInfo(
        const TDiskState& disk,
        NProto::TPlacementGroupConfig::TDiskInfo* diskInfo) const;

    bool UpdatePlacementGroup(
        TDiskRegistryDatabase& db,
        const TDiskId& diskId,
        const TDiskState& disk,
        TStringBuf callerName);

    void ApplyAgentStateChange(
        TDiskRegistryDatabase& db,
        const NProto::TAgentConfig& agent,
        TInstant timestamp,
        TVector<TDiskStateUpdate>& affectedDisks);

    bool HasDependentDisks(const NProto::TAgentConfig& agent) const;

    NProto::TError CheckAgentStateTransition(
        const TString& agentId,
        NProto::EAgentState newState,
        TInstant timestamp) const;

    NProto::TError CheckDeviceStateTransition(
        const TString& deviceId,
        NProto::EDeviceState newState,
        TInstant timestamp);

    void ApplyDeviceStateChange(
        TDiskRegistryDatabase& db,
        const NProto::TAgentConfig& agent,
        const NProto::TDeviceConfig& device,
        TInstant timestamp,
        TMaybe<TDiskStateUpdate>& affectedDisk);

    bool RestartDeviceMigration(
        TDiskRegistryDatabase& db,
        const TDiskId& diskId,
        TDiskState& disk,
        const TDeviceId& targetId);

    void CancelDeviceMigration(
        TDiskRegistryDatabase& db,
        const TDiskId& diskId,
        TDiskState& disk,
        const TDeviceId& sourceId);

    void DeleteDeviceMigration(
        const TDiskId& diskId,
        const TDeviceId& sourceId);

    void DeleteDeviceMigration(const TDiskId& diskId);

    void UpdateAndNotifyDisk(
        TDiskRegistryDatabase& db,
        const TString& diskId,
        TDiskState& disk);

    void AdjustDeviceIfNeeded(
        NProto::TDeviceConfig& device,
        TInstant timestamp);

    void AdjustDeviceBlockCount(
        TDiskRegistryDatabase& db,
        NProto::TDeviceConfig& device,
        ui64 newBlockCount);

    ui64 GetDeviceBlockCountWithOverrides(
        const TDiskId& diskId,
        const NProto::TDeviceConfig& device);

    void RemoveFinishedMigrations(
        TDiskRegistryDatabase& db,
        const TString& diskId,
        ui64 seqNo);

    NProto::TError ValidateUpdateDiskBlockSizeParams(
        const TDiskId& diskId,
        ui32 blockSize,
        bool force);

    NProto::TError ValidateUpdateDiskReplicaCountParams(
        const TDiskId& masterDiskId,
        ui32 replicaCount);

    bool DoesNewDiskBlockSizeBreakDevice(
        const TDiskId& diskId,
        const TDeviceId& deviceId,
        ui64 newLogicalBlockSize);

    NProto::TError ValidateDiskLocation(
        const TVector<NProto::TDeviceConfig>& diskDevices,
        const TAllocateDiskParams& params) const;

    TResultOrError<TDeviceList::TAllocationQuery> PrepareAllocationQuery(
        ui64 blocksToAllocate,
        THashSet<TString> otherDiskRacks,
        const TVector<NProto::TDeviceConfig>& diskDevices,
        const TAllocateDiskParams& params);

    void UpdateAgent(TDiskRegistryDatabase& db, const NProto::TAgentConfig& config);

    ui64 GetAllocationUnit(const TString& poolName) const;
    NProto::EDevicePoolKind GetDevicePoolKind(const TString& poolName) const;

    NProto::TError ValidateAllocateDiskParams(
        const TDiskState& disk,
        const TAllocateDiskParams& params) const;

    NProto::TError AllocateSimpleDisk(
        TInstant now,
        TDiskRegistryDatabase& db,
        const TAllocateDiskParams& params,
        TDiskState& disk,
        TAllocateDiskResult* result);

    void DeallocateSimpleDisk(
        TDiskRegistryDatabase& db,
        const TString& diskId,
        TDiskState& disk);

    void DeallocateSimpleDisk(
        TDiskRegistryDatabase& db,
        const TString& diskId,
        const TString& logPrefix);

    void DeleteDisk(TDiskRegistryDatabase& db, const TString& diskId);

    void AddToBrokenDisks(
        TInstant now,
        TDiskRegistryDatabase& db,
        const TString& diskId);

    bool TryUpdateDevice(
        TDiskRegistryDatabase& db,
        const TDeviceId& uuid);
};

}   // namespace NCloud::NBlockStore::NStorage
