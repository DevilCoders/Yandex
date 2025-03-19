#include "disk_registry_state.h"

#include "disk_registry_schema.h"

#include <cloud/blockstore/libs/diagnostics/critical_events.h>
#include <cloud/blockstore/libs/kikimr/events.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/disk_validation.h>
#include <cloud/blockstore/libs/storage/disk_common/monitoring_utils.h>

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/format.h>

#include <library/cpp/monlib/service/pages/templates.h>

#include <util/generic/algorithm.h>
#include <util/generic/iterator_range.h>
#include <util/generic/size_literals.h>
#include <util/string/builder.h>
#include <util/string/join.h>

#include <tuple>

namespace NCloud::NBlockStore::NStorage {

namespace {

////////////////////////////////////////////////////////////////////////////////

static const TString DISK_STATE_MIGRATION_MESSAGE =
    "data migration in progress, slight performance decrease may be experienced";

////////////////////////////////////////////////////////////////////////////////

template<typename T>
struct TTableCount;

template<typename ... Ts>
struct TTableCount<NKikimr::NIceDb::Schema::SchemaTables<Ts...>>
{
    enum { value = sizeof...(Ts) };
};

////////////////////////////////////////////////////////////////////////////////

struct TByUUID
{
    template <typename T, typename U>
    bool operator () (const T& lhs, const U& rhs) const
    {
        return lhs.GetDeviceUUID() < rhs.GetDeviceUUID();
    }
};

NProto::EDiskState ToDiskState(NProto::EAgentState agentState)
{
    switch (agentState)
    {
    case NProto::AGENT_STATE_ONLINE:
        return NProto::DISK_STATE_ONLINE;
    case NProto::AGENT_STATE_WARNING:
        return NProto::DISK_STATE_MIGRATION;
    case NProto::AGENT_STATE_UNAVAILABLE:
        return NProto::DISK_STATE_TEMPORARILY_UNAVAILABLE;
    default:
        Y_FAIL("unknown agent state");
    }
}

NProto::EDiskState ToDiskState(NProto::EDeviceState deviceState)
{
    switch (deviceState)
    {
    case NProto::DEVICE_STATE_ONLINE:
        return NProto::DISK_STATE_ONLINE;
    case NProto::DEVICE_STATE_WARNING:
        return NProto::DISK_STATE_MIGRATION;
    case NProto::DEVICE_STATE_ERROR:
        return NProto::DISK_STATE_ERROR;
    default:
        Y_FAIL("unknown device state");
    }
}

////////////////////////////////////////////////////////////////////////////////

TString GetMirroredDiskGroupId(const TString& diskId)
{
    return diskId + "/g";
}

TString GetReplicaDiskId(const TString& diskId, ui32 i)
{
    return TStringBuilder() << diskId << "/" << i;
}

////////////////////////////////////////////////////////////////////////////////

TDuration GetInfraTimeout(
    const TStorageConfig& config,
    NProto::EAgentState agentState)
{
    if (agentState == NProto::AGENT_STATE_UNAVAILABLE) {
        return config.GetNonReplicatedInfraUnavailableAgentTimeout();
    }

    return config.GetNonReplicatedInfraTimeout();
}

////////////////////////////////////////////////////////////////////////////////

THashMap<TString, NProto::TDevicePoolConfig> CreateDevicePoolConfigs(
    const NProto::TDiskRegistryConfig& config,
    const TStorageConfig& storageConfig)
{
    NProto::TDevicePoolConfig nonrepl;
    nonrepl.SetAllocationUnit(
        storageConfig.GetAllocationUnitNonReplicatedSSD() * 1_GB);

    THashMap<TString, NProto::TDevicePoolConfig> result {
        { TString {}, nonrepl }
    };

    for (const auto& pool: config.GetDevicePoolConfigs()) {
        result[pool.GetName()] = pool;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////

bool CheckPlacementGroupRequest(
    const TString& groupId,
    const ui32 configVersion,
    const TPlacementGroupInfo* g,
    NProto::TError* error)
{
    if (!g) {
        *error = MakeError(
            E_NOT_FOUND,
            Sprintf("group does not exist: %s", groupId.c_str())
        );

        return false;
    }

    if (g->Config.GetConfigVersion() != configVersion) {
        *error = MakeError(
            E_CONFIG_VERSION_MISMATCH,
            Sprintf(
                "received version != expected version: %u != %u",
                configVersion,
                g->Config.GetConfigVersion()
            )
        );

        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

ui32 GetMaxDisksInPlacementGroup(
    const TStorageConfig& config,
    const NProto::TPlacementGroupConfig& g)
{
    if (g.GetSettings().GetMaxDisksInGroup()) {
        return g.GetSettings().GetMaxDisksInGroup();
    }

    return config.GetMaxDisksInPlacementGroup();
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TDiskRegistryState::TDiskRegistryState(
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
        TVector<TString> errorNotifications,
        TVector<TString> outdatedVolumeConfigs,
        TVector<TDeviceId> suspendedDevices)
    : StorageConfig(std::move(storageConfig))
    , Counters(counters)
    , AgentList(counters, std::move(agents))
    , DeviceList(std::move(dirtyDevices), std::move(suspendedDevices))
    , BrokenDisks(std::move(brokenDisks))
    , CurrentConfig(std::move(config))
    , DiskStateUpdates(std::move(diskStateUpdates))
    , DiskStateSeqNo(diskStateSeqNo)
    , DiskAllocationAllowed(diskAllocationAllowed)
{
    if (Counters) {
        SelfCounters.Init(Counters);
    }

    ProcessDisksToNotify(std::move(disksToNotify));
    ProcessErrorNotifications(std::move(errorNotifications));
    ProcessConfig(CurrentConfig);
    ProcessDisks(std::move(disks));
    ProcessPlacementGroups(std::move(placementGroups));
    ProcessAgents();
    ProcessDisksToCleanup(std::move(disksToCleanup));
    ProcessOutdatedVolumeConfigs(std::move(outdatedVolumeConfigs));

    FillMigrations();
}

void TDiskRegistryState::ProcessDisksToNotify(TVector<TString> disksToNotify)
{
    for (auto& diskId: disksToNotify) {
        DisksToNotify.emplace(std::move(diskId), DisksToNotifySeqNo++);
    }
}

void TDiskRegistryState::ProcessErrorNotifications(TVector<TDiskId> errorNotifications)
{
    for (auto& diskId: errorNotifications) {
        ErrorNotifications.insert(std::move(diskId));
    }
}

void TDiskRegistryState::ProcessDisks(TVector<NProto::TDiskConfig> configs)
{
    for (auto& config: configs) {
        const auto& diskId = config.GetDiskId();
        auto& disk = Disks[config.GetDiskId()];
        disk.LogicalBlockSize = config.GetBlockSize();
        disk.Devices.reserve(config.DeviceUUIDsSize());
        disk.State = config.GetState();
        disk.StateTs = TInstant::MicroSeconds(config.GetStateTs());
        disk.CloudId = config.GetCloudId();
        disk.FolderId = config.GetFolderId();
        disk.UserId = config.GetUserId();
        disk.ReplicaCount = config.GetReplicaCount();
        disk.MasterDiskId = config.GetMasterDiskId();

        // XXX temp hack
        if (auto i = diskId.find('/'); i != TString::npos) {
            disk.MasterDiskId = diskId.substr(0, i);
        }

        for (const auto& uuid: config.GetDeviceUUIDs()) {
            disk.Devices.push_back(uuid);

            DeviceList.MarkDeviceAllocated(diskId, uuid);
        }

        if (disk.MasterDiskId) {
            ReplicaTable.AddReplica(disk.MasterDiskId, disk.Devices);
        }

        for (const auto& id: config.GetDeviceReplacementUUIDs()) {
            disk.DeviceReplacementIds.push_back(id);
        }

        for (auto& m: config.GetMigrations()) {
            ++DeviceMigrationsInProgress;

            disk.MigrationTarget2Source.emplace(
                m.GetTargetDevice().GetDeviceUUID(),
                m.GetSourceDeviceId());

            disk.MigrationSource2Target.emplace(
                m.GetSourceDeviceId(),
                m.GetTargetDevice().GetDeviceUUID());

            DeviceList.MarkDeviceAllocated(
                diskId,
                m.GetTargetDevice().GetDeviceUUID());
        }

        if (!config.GetFinishedMigrations().empty()) {
            ui64 seqNo = DisksToNotify[diskId];

            if (seqNo == 0) {
                ReportDiskRegistryNoScheduledNotification(TStringBuilder()
                    << "No scheduled notification for disk " << diskId.Quote());

                seqNo = DisksToNotifySeqNo++;
                DisksToNotify[diskId] = seqNo;
            }

            for (auto& m: config.GetFinishedMigrations()) {
                const auto& uuid = m.GetDeviceId();

                DeviceList.MarkDeviceAllocated(diskId, uuid);

                disk.FinishedMigrations.push_back({
                    .DeviceId = uuid,
                    .SeqNo = seqNo
                });
            }
        }
    }

    for (const auto& x: Disks) {
        if (x.second.ReplicaCount) {
            for (const auto& id: x.second.DeviceReplacementIds) {
                ReplicaTable.MarkReplacementDevice(x.first, id, true);
            }
        }
    }
}

void TDiskRegistryState::AddMigration(
    const TDiskState& disk,
    const TString& diskId,
    const TString& sourceDeviceId)
{
    if (disk.MasterDiskId) {
        // mirrored disk replicas don't need migration
        return;
    }

    if (!StorageConfig->GetNonReplicatedMigrationStartAllowed()) {
        return;
    }

    Migrations.emplace(diskId, sourceDeviceId);
}

void TDiskRegistryState::FillMigrations()
{
    for (auto& [diskId, disk]: Disks) {
        if (disk.State == NProto::DISK_STATE_ONLINE) {
            continue;
        }

        for (const auto& uuid: disk.Devices) {
            if (disk.MigrationSource2Target.contains(uuid)) {
                continue; // migration in progress
            }

            auto [agent, device] = FindDeviceLocation(uuid);

            if (!device) {
                ReportDiskRegistryDeviceNotFound(
                    TStringBuilder() << "FillMigrations:DeviceId: " << uuid);

                continue;
            }

            if (!agent) {
                ReportDiskRegistryAgentNotFound(
                    TStringBuilder() << "FillMigrations:AgentId: "
                    << device->GetAgentId());

                continue;
            }

            if (device->GetState() == NProto::DEVICE_STATE_WARNING) {
                AddMigration(disk, diskId, uuid);

                continue;
            }

            if (device->GetState() == NProto::DEVICE_STATE_ERROR) {
                continue; // skip for broken devices
            }

            if (agent->GetState() == NProto::AGENT_STATE_WARNING) {
                AddMigration(disk, diskId, uuid);
            }
        }
    }
}

void TDiskRegistryState::ProcessPlacementGroups(
    TVector<NProto::TPlacementGroupConfig> configs)
{
    for (auto& config: configs) {
        auto groupId = config.GetGroupId();

        for (const auto& disk: config.GetDisks()) {
            auto* d = Disks.FindPtr(disk.GetDiskId());

            if (!d) {
                ReportDiskRegistryDiskNotFound(TStringBuilder()
                    << "ProcessPlacementGroups:DiskId: " << disk.GetDiskId());

                continue;
            }

            d->PlacementGroupId = groupId;
        }

        PlacementGroups[groupId] = std::move(config);
    }
}

void TDiskRegistryState::ProcessAgents()
{
    for (auto& agent: AgentList.GetAgents()) {
        DeviceList.UpdateDevices(agent);
    }
}

void TDiskRegistryState::ProcessDisksToCleanup(TVector<TString> disksToCleanup)
{
    for (auto& diskId: disksToCleanup) {
        DisksToCleanup.emplace(std::move(diskId));
    }
}

void TDiskRegistryState::ProcessOutdatedVolumeConfigs(TVector<TString> diskIds)
{
    for (auto& diskId: diskIds) {
        OutdatedVolumeConfigs.emplace(std::move(diskId), VolumeConfigSeqNo++);
    }
}

const TVector<NProto::TAgentConfig>& TDiskRegistryState::GetAgents() const
{
    return AgentList.GetAgents();
}

void TDiskRegistryState::ValidateAgent(const NProto::TAgentConfig& agent)
{
    const auto& agentId = agent.GetAgentId();

    if (agentId.empty()) {
        throw TServiceError(E_ARGUMENT) << "empty agent id";
    }

    if (agent.GetNodeId() == 0) {
        throw TServiceError(E_ARGUMENT) << "empty node id";
    }

    auto* buddy = AgentList.FindAgent(agent.GetNodeId());

    if (buddy
        && buddy->GetAgentId() != agentId
        && buddy->GetState() != NProto::AGENT_STATE_UNAVAILABLE)
    {
        throw TServiceError(E_INVALID_STATE)
            << "Agent " << buddy->GetAgentId().Quote()
            << " already registered at node #" << agent.GetNodeId();
    }

    buddy = AgentList.FindAgent(agentId);

    if (buddy
        && buddy->GetSeqNumber() > agent.GetSeqNumber()
        && buddy->GetState() != NProto::AGENT_STATE_UNAVAILABLE)
    {
        throw TServiceError(E_INVALID_STATE)
            << "Agent " << buddy->GetAgentId().Quote()
            << " already registered with a greater SeqNo "
            << "(" << buddy->GetSeqNumber() << " > " << agent.GetSeqNumber() << ")";
    }

    auto it = KnownAgents.find(agentId);

    if (it == KnownAgents.end()) {
        throw TServiceError(E_INVALID_STATE) << "agent not allowed";
    }

    const auto& allowedDevices = it->second;

    TString rack;
    if (agent.DevicesSize()) {
        rack = agent.GetDevices(0).GetRack();
    }

    for (const auto& device: agent.GetDevices()) {
        const auto& uuid = device.GetDeviceUUID();
        if (!allowedDevices.contains(uuid)) {
            throw TServiceError(E_INVALID_STATE)
                << "device " << uuid.Quote() << " not allowed";
        }

        // right now we suppose that each agent presents devices
        // from one rack only
        if (rack != device.GetRack()) {
            throw TServiceError(E_ARGUMENT)
                << "all agent devices should come from the same rack, mismatch: "
                << rack << " != " << device.GetRack();
        }
    }
}

TDiskRegistryState::TDiskId TDiskRegistryState::FindDisk(
    const TDeviceId& uuid) const
{
    return DeviceList.FindDiskId(uuid);
}

void TDiskRegistryState::AdjustDeviceIfNeeded(
    NProto::TDeviceConfig& device,
    TInstant timestamp)
{
    if (!device.GetUnadjustedBlockCount()) {
        device.SetUnadjustedBlockCount(device.GetBlocksCount());
    }

    const auto* poolConfig = DevicePoolConfigs.FindPtr(device.GetPoolName());
    if (!poolConfig) {
        device.SetState(NProto::DEVICE_STATE_ERROR);
        device.SetStateTs(timestamp.MicroSeconds());
        device.SetStateMessage(
            Sprintf("unknown pool: %s", device.GetPoolName().c_str()));

        return;
    }

    device.SetPoolKind(poolConfig->GetKind());

    const ui64 unit = poolConfig->GetAllocationUnit();
    Y_VERIFY_DEBUG(unit != 0);

    if (device.GetState() == NProto::DEVICE_STATE_ERROR ||
        !DeviceList.FindDiskId(device.GetDeviceUUID()).empty())
    {
        return;
    }

    const auto deviceSize = device.GetBlockSize() * device.GetBlocksCount();

    if (deviceSize < unit) {
        device.SetState(NProto::DEVICE_STATE_ERROR);
        device.SetStateTs(timestamp.MicroSeconds());
        device.SetStateMessage(TStringBuilder()
            << "device is too small: " << deviceSize);

        return;
    }

    if (device.GetBlockSize() == 0 || unit % device.GetBlockSize() != 0) {
        device.SetState(NProto::DEVICE_STATE_ERROR);
        device.SetStateTs(timestamp.MicroSeconds());
        device.SetStateMessage(TStringBuilder()
            << "bad block size: " << device.GetBlockSize());

        return;
    }

    if (deviceSize > unit) {
        device.SetBlocksCount(unit / device.GetBlockSize());
    }
}

void TDiskRegistryState::RemoveAgentFromNode(
    TDiskRegistryDatabase& db,
    NProto::TAgentConfig& agent,
    TInstant timestamp,
    TVector<TDiskStateUpdate>* affectedDisks,
    THashMap<TString, ui64>* notifiedDisks)
{
    Y_VERIFY_DEBUG(agent.GetState() == NProto::AGENT_STATE_UNAVAILABLE);

    const ui32 nodeId = agent.GetNodeId();

    agent.SetNodeId(0);
    agent.SetState(NProto::AGENT_STATE_UNAVAILABLE);
    agent.SetStateTs(timestamp.MicroSeconds());
    agent.SetStateMessage("lost");

    THashSet<TDiskId> diskIds;

    for (auto& d: *agent.MutableDevices()) {
        d.SetNodeId(0);

        const auto& uuid = d.GetDeviceUUID();
        auto diskId = DeviceList.FindDiskId(uuid);

        if (!diskId.empty()) {
            diskIds.emplace(std::move(diskId));
        }
    }

    AgentList.RemoveAgentFromNode(nodeId);
    DeviceList.UpdateDevices(agent, nodeId);

    for (const auto& id: diskIds) {
        notifiedDisks->emplace(id, AddDiskToNotify(db, id));
    }

    db.UpdateAgent(agent);
    db.DeleteOldAgent(nodeId);

    ApplyAgentStateChange(db, agent, timestamp, *affectedDisks);
}

NProto::TError TDiskRegistryState::RegisterAgent(
    TDiskRegistryDatabase& db,
    NProto::TAgentConfig config,
    TInstant timestamp,
    TVector<TDiskStateUpdate>* affectedDisks,
    THashMap<TString, ui64>* notifiedDisks)
{
    if (!DiskAllocationAllowed) {
        return MakeError(E_INVALID_STATE, "Operation disallowed");
    }

    try {
        ValidateAgent(config);

        if (auto* buddy = AgentList.FindAgent(config.GetNodeId());
                buddy && buddy->GetAgentId() != config.GetAgentId())
        {
            RemoveAgentFromNode(
                db,
                *buddy,
                timestamp,
                affectedDisks,
                notifiedDisks);
        }

        const auto prevNodeId = AgentList.FindNodeId(config.GetAgentId());

        THashSet<TDeviceId> newDevices;

        auto& agent = AgentList.RegisterAgent(
            std::move(config),
            timestamp,
            &newDevices);

        for (auto& d: *agent.MutableDevices()) {
            AdjustDeviceIfNeeded(d, timestamp);

            const auto& uuid = d.GetDeviceUUID();

            if (d.GetPoolKind() != NProto::DEVICE_POOL_KIND_DEFAULT
                    && newDevices.contains(uuid))
            {
                DeviceList.SuspendDevice(uuid);
                db.UpdateSuspendedDevice(uuid);
            }
        }

        DeviceList.UpdateDevices(agent, prevNodeId);

        for (const auto& uuid: newDevices) {
            if (!DeviceList.FindDiskId(uuid)) {
                DeviceList.MarkDeviceAsDirty(uuid);
                db.UpdateDirtyDevice(uuid);
            }
        }

        THashSet<TDiskId> diskIds;

        for (const auto& d: agent.GetDevices()) {
            const auto& uuid = d.GetDeviceUUID();
            auto diskId = DeviceList.FindDiskId(uuid);

            if (diskId.empty()) {
                continue;
            }

            if (d.GetState() == NProto::DEVICE_STATE_ERROR) {
                auto& disk = Disks[diskId];
                if (!RestartDeviceMigration(db, diskId, disk, uuid)) {
                    CancelDeviceMigration(db, diskId, disk, uuid);
                }
            }

            diskIds.emplace(std::move(diskId));
        }

        for (auto& id: diskIds) {
            TMaybe<TDiskStateUpdate> update = TryUpdateDiskState(db, id, timestamp);
            if (update) {
                affectedDisks->push_back(std::move(*update));
            }
        }

        if (prevNodeId != agent.GetNodeId()) {
            db.DeleteOldAgent(prevNodeId);

            for (const auto& id: diskIds) {
                notifiedDisks->emplace(id, AddDiskToNotify(db, id));
            }
        }

        if (agent.GetState() == NProto::AGENT_STATE_UNAVAILABLE) {
            agent.SetCmsTs(0);
            agent.SetState(NProto::AGENT_STATE_WARNING);
            agent.SetStateTs(timestamp.MicroSeconds());
            agent.SetStateMessage("back from unavailable");

            ApplyAgentStateChange(db, agent, timestamp, *affectedDisks);
        }

        UpdateAgent(db, agent);

    } catch (const TServiceError& e) {
        return MakeError(e.GetCode(), e.what());
    } catch (...) {
        return MakeError(E_FAIL, CurrentExceptionMessage());
    }

    return {};
}

NProto::TError TDiskRegistryState::UnregisterAgent(
    TDiskRegistryDatabase& db,
    ui32 nodeId)
{
    if (!RemoveAgent(db, nodeId)) {
        return MakeError(S_ALREADY, "agent not found");
    }

    return {};
}

void TDiskRegistryState::RebuildDiskPlacementInfo(
    const TDiskState& disk,
    NProto::TPlacementGroupConfig::TDiskInfo* diskInfo) const
{
    Y_VERIFY(diskInfo);

    THashSet<TString> racks;

    for (const auto& uuid: disk.Devices) {
        auto rack = DeviceList.FindRack(uuid);
        if (rack) {
            racks.insert(std::move(rack));
        }
    }

    for (auto& [targetId, sourceId]: disk.MigrationTarget2Source) {
        auto rack = DeviceList.FindRack(targetId);
        if (rack) {
            racks.insert(std::move(rack));
        }
    }

    diskInfo->MutableDeviceRacks()->Assign(
        std::make_move_iterator(racks.begin()),
        std::make_move_iterator(racks.end())
    );
}

NProto::TError TDiskRegistryState::ReplaceDevice(
    TDiskRegistryDatabase& db,
    const TString& diskId,
    const TString& deviceId,
    TInstant timestamp,
    TMaybe<TDiskStateUpdate>* update)
{
    try {
        if (!diskId) {
            return MakeError(E_ARGUMENT, "empty disk id");
        }

        if (!deviceId) {
            return MakeError(E_ARGUMENT, "empty device id");
        }

        if (DeviceList.FindDiskId(deviceId) != diskId) {
            return MakeError(E_ARGUMENT, TStringBuilder()
                << "device does not belong to disk " << diskId.Quote());
        }

        if (!Disks.contains(diskId)) {
            return MakeError(E_ARGUMENT, TStringBuilder()
                << "unknown disk: " << diskId.Quote());
        }

        TDiskState& disk = Disks[diskId];

        auto it = Find(disk.Devices, deviceId);
        if (it == disk.Devices.end()) {
            auto message = ReportDiskRegistryDeviceNotFound(
                TStringBuilder() << "ReplaceDevice:DiskId: " << diskId
                << ", DeviceId: " << deviceId);

            return MakeError(E_FAIL, message);
        }

        auto [agentPtr, devicePtr] = FindDeviceLocation(deviceId);
        if (!agentPtr || !devicePtr) {
            return MakeError(E_INVALID_STATE, "can't find device");
        }

        const ui64 logicalBlockCount = devicePtr->GetBlockSize() * devicePtr->GetBlocksCount()
            / disk.LogicalBlockSize;

        TDeviceList::TAllocationQuery query {
            .OtherRacks = CollectOtherRacks(diskId, disk, "ReplaceDevice"),
            .LogicalBlockSize = disk.LogicalBlockSize,
            .BlockCount = logicalBlockCount,
            .PoolName = devicePtr->GetPoolName(),
            .PoolKind = GetDevicePoolKind(devicePtr->GetPoolName())
        };

        if (query.PoolKind == NProto::DEVICE_POOL_KIND_LOCAL) {
            query.NodeIds = { devicePtr->GetNodeId() };
        }

        auto targetDevice = DeviceList.AllocateDevice(diskId, query);

        if (targetDevice.GetDeviceUUID().empty()) {
            return MakeError(E_BS_DISK_ALLOCATION_FAILED, "can't allocate device");
        }

        AdjustDeviceBlockCount(
            db,
            targetDevice,
            logicalBlockCount * disk.LogicalBlockSize / targetDevice.GetBlockSize()
        );

        if (disk.MasterDiskId) {
            auto* masterDisk = Disks.FindPtr(disk.MasterDiskId);
            Y_VERIFY_DEBUG(masterDisk);
            if (masterDisk) {
                masterDisk->DeviceReplacementIds.push_back(
                    targetDevice.GetDeviceUUID());

                db.UpdateDisk(BuildDiskConfig(disk.MasterDiskId, *masterDisk));

                const bool replaced = ReplicaTable.ReplaceDevice(
                    disk.MasterDiskId,
                    deviceId,
                    targetDevice.GetDeviceUUID());

                Y_VERIFY_DEBUG(replaced);
            }
        }

        devicePtr->SetState(NProto::DEVICE_STATE_ERROR);
        devicePtr->SetStateMessage("replaced");
        devicePtr->SetStateTs(timestamp.MicroSeconds());

        DeviceList.UpdateDevices(*agentPtr);
        DeviceList.ReleaseDevice(deviceId);
        db.UpdateDirtyDevice(deviceId);

        CancelDeviceMigration(db, diskId, disk, deviceId);

        *it = targetDevice.GetDeviceUUID();

        *update = TryUpdateDiskState(db, diskId, disk, timestamp);

        UpdateAgent(db, *agentPtr);

        UpdatePlacementGroup(db, diskId, disk, "ReplaceDevice");
        UpdateAndNotifyDisk(db, diskId, disk);

    } catch (const TServiceError& e) {
        return MakeError(e.GetCode(), e.what());
    }

    return {};
}

void TDiskRegistryState::AdjustDeviceBlockCount(
    TDiskRegistryDatabase& db,
    NProto::TDeviceConfig& device,
    ui64 newBlockCount)
{
    if (newBlockCount > device.GetUnadjustedBlockCount()) {
        ReportDiskRegistryBadDeviceSizeAdjustment(
            TStringBuilder() << "AdjustDeviceBlockCount:DeviceId: "
            << device.GetDeviceUUID()
            << ", UnadjustedBlockCount: " << device.GetUnadjustedBlockCount()
            << ", newBlockCount: " << newBlockCount);

        return;
    }

    if (newBlockCount == device.GetBlocksCount()) {
        return;
    }

    auto [agent, source] = FindDeviceLocation(device.GetDeviceUUID());
    if (!agent || !source) {
        ReportDiskRegistryBadDeviceSizeAdjustment(
            TStringBuilder() << "AdjustDeviceBlockCount:DeviceId: "
            << device.GetDeviceUUID()
            << ", agent?: " << !!agent
            << ", source?: " << !!source);

        return;
    }

    source->SetBlocksCount(newBlockCount);

    UpdateAgent(db, *agent);
    DeviceList.UpdateDevices(*agent);

    device = *source;
}

ui64 TDiskRegistryState::GetDeviceBlockCountWithOverrides(
    const TDiskId& diskId,
    const NProto::TDeviceConfig& device)
{
    auto deviceBlockCount = device.GetBlocksCount();

    if (const auto* overrides = DeviceOverrides.FindPtr(diskId)) {
        const auto* overriddenBlockCount =
            overrides->Device2BlockCount.FindPtr(device.GetDeviceUUID());
        if (overriddenBlockCount) {
            deviceBlockCount = *overriddenBlockCount;
        }
    }

    return deviceBlockCount;
}

bool TDiskRegistryState::UpdatePlacementGroup(
    TDiskRegistryDatabase& db,
    const TDiskId& diskId,
    const TDiskState& disk,
    TStringBuf callerName)
{
    if (disk.PlacementGroupId.empty()) {
        return false;
    }

    auto* pg = PlacementGroups.FindPtr(disk.PlacementGroupId);
    if (!pg) {
        ReportDiskRegistryPlacementGroupNotFound(
            TStringBuilder() << callerName << ":DiskId: " << diskId
                << ", PlacementGroupId: " << disk.PlacementGroupId);
        return false;
    }

    auto* diskInfo = FindIfPtr(*pg->Config.MutableDisks(), [&] (auto& disk) {
        return disk.GetDiskId() == diskId;
    });

    if (!diskInfo) {
        ReportDiskRegistryPlacementGroupDiskNotFound(
            TStringBuilder() << callerName << ":DiskId: " << diskId
                << ", PlacementGroupId: " << disk.PlacementGroupId);

        return false;
    }

    RebuildDiskPlacementInfo(disk, diskInfo);

    pg->Config.SetConfigVersion(pg->Config.GetConfigVersion() + 1);
    db.UpdatePlacementGroup(pg->Config);

    return true;
}

TResultOrError<NProto::TDeviceConfig> TDiskRegistryState::StartDeviceMigration(
    TDiskRegistryDatabase& db,
    const TString& diskId,
    const TString& deviceId)
{
    try {
        if (!Disks.contains(diskId)) {
            return MakeError(E_NOT_FOUND, TStringBuilder() <<
                "disk " << diskId.Quote() << " not found");
        }

        if (DeviceList.FindDiskId(deviceId) != diskId) {
            ReportDiskRegistryDeviceDoesNotBelongToDisk(
                TStringBuilder() << "StartDeviceMigration:DiskId: "
                << diskId << ", DeviceId: " << deviceId);

            return MakeError(E_ARGUMENT, TStringBuilder() <<
                "device " << deviceId.Quote() << " does not belong to "
                    << diskId.Quote());
        }

        auto* device = DeviceList.FindDevice(deviceId);
        if (!device) {
            return MakeError(E_NOT_FOUND, TStringBuilder() <<
                "device " << deviceId.Quote() << " not found");
        }

        TDiskState& disk = Disks[diskId];

        const auto logicalBlockCount =
            GetDeviceBlockCountWithOverrides(diskId, *device)
            * device->GetBlockSize() / disk.LogicalBlockSize;

        TDeviceList::TAllocationQuery query {
            .OtherRacks = CollectOtherRacks(diskId, disk, "StartDeviceMigration"),
            .LogicalBlockSize = disk.LogicalBlockSize,
            .BlockCount = logicalBlockCount,
            .PoolName = device->GetPoolName(),
            .PoolKind = GetDevicePoolKind(device->GetPoolName())
        };

        if (query.PoolKind == NProto::DEVICE_POOL_KIND_LOCAL) {
            query.NodeIds = { device->GetNodeId() };
        }

        auto targetDevice = DeviceList.AllocateDevice(diskId, query);

        const auto& targetId = targetDevice.GetDeviceUUID();

        if (targetId.empty()) {
            return MakeError(E_BS_DISK_ALLOCATION_FAILED, TStringBuilder() <<
                "can't allocate target for " << deviceId.Quote());
        }

        AdjustDeviceBlockCount(
            db,
            targetDevice,
            logicalBlockCount * disk.LogicalBlockSize / targetDevice.GetBlockSize()
        );

        disk.MigrationTarget2Source[targetId] = deviceId;
        disk.MigrationSource2Target[deviceId] = targetId;
        ++DeviceMigrationsInProgress;

        DeviceList.MarkDeviceAllocated(diskId, targetId);

        DeleteDeviceMigration(diskId, deviceId);

        UpdatePlacementGroup(db, diskId, disk, "StartDeviceMigration");
        UpdateAndNotifyDisk(db, diskId, disk);

        return targetDevice;

    } catch (const TServiceError& e) {
        return MakeError(e.GetCode(), e.what());
    }
}

TDeque<TRackInfo> TDiskRegistryState::GatherRacksInfo() const
{
    TDeque<TRackInfo> racks;
    THashMap<TString, TRackInfo*> m;

    for (const auto& agent: AgentList.GetAgents()) {
        TRackInfo::TAgentInfo* agentInfo = nullptr;

        for (const auto& device: agent.GetDevices()) {
            auto& rackPtr = m[device.GetRack()];
            if (!rackPtr) {
                racks.emplace_back(device.GetRack());
                rackPtr = &racks.back();
            }

            if (!agentInfo) {
                agentInfo = FindIfPtr(
                    rackPtr->AgentInfos,
                    [&] (const TRackInfo::TAgentInfo& info) {
                        return info.AgentId == agent.GetAgentId();
                    });
            }

            if (!agentInfo) {
                agentInfo = &rackPtr->AgentInfos.emplace_back(
                    agent.GetAgentId(),
                    agent.GetNodeId()
                );
            }

            ++agentInfo->TotalDevices;

            if (device.GetState() == NProto::DEVICE_STATE_ERROR) {
                ++agentInfo->BrokenDevices;
            }

            if (auto diskId = DeviceList.FindDiskId(device.GetDeviceUUID())) {
                ++agentInfo->AllocatedDevices;

                auto* disk = Disks.FindPtr(diskId);
                if (disk) {
                    if (disk->PlacementGroupId) {
                        rackPtr->PlacementGroups.insert(disk->PlacementGroupId);
                    }
                } else {
                    ReportDiskRegistryDeviceListReferencesNonexistentDisk(
                        TStringBuilder() << "GatherRacksInfo:DiskId: " << diskId
                        << ", DeviceId: " << device.GetDeviceUUID());
                }
            } else if (device.GetState() == NProto::DEVICE_STATE_ONLINE) {
                switch (agent.GetState()) {
                    case NProto::AGENT_STATE_ONLINE: {
                        if (DeviceList.IsDirtyDevice(device.GetDeviceUUID())) {
                            ++agentInfo->DirtyDevices;
                        } else {
                            ++agentInfo->FreeDevices;
                            rackPtr->FreeBytes +=
                                device.GetBlockSize() * device.GetBlocksCount();
                        }

                        break;
                    }

                    case NProto::AGENT_STATE_WARNING: {
                        ++agentInfo->WarningDevices;
                        rackPtr->WarningBytes +=
                            device.GetBlockSize() * device.GetBlocksCount();

                        break;
                    }

                    case NProto::AGENT_STATE_UNAVAILABLE: {
                        ++agentInfo->UnavailableDevices;

                        break;
                    }

                    default: {}
                }
            }

            rackPtr->TotalBytes += device.GetBlockSize() * device.GetBlocksCount();
        }
    }

    for (auto& rack: racks) {
        SortBy(rack.AgentInfos, [] (const TRackInfo::TAgentInfo& x) {
            return x.AgentId;
        });
    }

    SortBy(racks, [] (const TRackInfo& x) {
        return x.Name;
    });

    return racks;
}

THashSet<TString> TDiskRegistryState::CollectOtherRacks(
    const TDiskId& diskId,
    const TDiskState& disk,
    TStringBuf callerName)
{
    THashSet<TString> otherDiskRacks;

    if (disk.PlacementGroupId.empty()) {
        return otherDiskRacks;
    }

    auto* pg = PlacementGroups.FindPtr(disk.PlacementGroupId);
    if (!pg) {
        auto message = ReportDiskRegistryPlacementGroupNotFound(
            TStringBuilder() << callerName << ":DiskId: " << diskId
            << ", PlacementGroupId: " << disk.PlacementGroupId);

        ythrow TServiceError(E_FAIL) << message;
    }

    auto* thisDisk = CollectRacks(diskId, pg->Config, &otherDiskRacks);

    if (!thisDisk) {
        auto message = ReportDiskRegistryPlacementGroupDiskNotFound(
            TStringBuilder() << callerName << ":DiskId: " << diskId
            << ", PlacementGroupId: " << disk.PlacementGroupId);

        ythrow TServiceError(E_FAIL) << message;
    }

    return otherDiskRacks;
}

NProto::TPlacementGroupConfig::TDiskInfo* TDiskRegistryState::CollectRacks(
    const TString& diskId,
    NProto::TPlacementGroupConfig& placementGroup,
    THashSet<TString>* otherDiskRacks)
{
    NProto::TPlacementGroupConfig::TDiskInfo* thisDisk = nullptr;

    for (auto& disk: *placementGroup.MutableDisks()) {
        if (disk.GetDiskId() == diskId) {
            if (thisDisk) {
                ReportDiskRegistryDuplicateDiskInPlacementGroup(
                    TStringBuilder() << "CollectRacks:PlacementGroupId: "
                    << placementGroup.GetGroupId()
                    << ", DiskId: " << diskId);
            }

            thisDisk = &disk;
            continue;
        }
        for (const auto& rack: disk.GetDeviceRacks()) {
            otherDiskRacks->insert(rack);
        }
    }

    return thisDisk;
}

void TDiskRegistryState::CollectRacks(
    const NProto::TPlacementGroupConfig& placementGroup,
    THashSet<TString>* otherDiskRacks)
{
    for (auto& disk: placementGroup.GetDisks()) {
        for (const auto& rack: disk.GetDeviceRacks()) {
            otherDiskRacks->insert(rack);
        }
    }
}

NProto::TError TDiskRegistryState::ValidateDiskLocation(
    const TVector<NProto::TDeviceConfig>& diskDevices,
    const TAllocateDiskParams& params) const
{
    if (params.MediaKind != NProto::STORAGE_MEDIA_SSD_LOCAL || diskDevices.empty()) {
        return {};
    }

    if (!params.AgentIds.empty()) {
        const THashSet<TString> agents(
            params.AgentIds.begin(),
            params.AgentIds.end());

        for (const auto& device: diskDevices) {
            if (!agents.contains(device.GetAgentId())) {
                return MakeError(E_ARGUMENT, TStringBuilder() <<
                    "disk " << params.DiskId.Quote() << " already allocated at "
                        << device.GetAgentId());
            }
        }
    }

    return {};
}

TResultOrError<TDeviceList::TAllocationQuery> TDiskRegistryState::PrepareAllocationQuery(
    ui64 blocksToAllocate,
    THashSet<TString> otherDiskRacks,
    const TVector<NProto::TDeviceConfig>& diskDevices,
    const TAllocateDiskParams& params)
{
    THashSet<ui32> nodeIds;
    TVector<TString> unknownAgents;

    for (const auto& id: params.AgentIds) {
        const ui32 nodeId = AgentList.FindNodeId(id);
        if (!nodeId) {
            unknownAgents.push_back(id);
        } else {
            nodeIds.insert(nodeId);
        }
    }

    if (!unknownAgents.empty()) {
        return MakeError(E_ARGUMENT, TStringBuilder() <<
            "unknown agents: " << JoinSeq(", ", unknownAgents));
    }

    NProto::EDevicePoolKind poolKind = params.PoolName.empty()
        ? NProto::DEVICE_POOL_KIND_DEFAULT
        : NProto::DEVICE_POOL_KIND_GLOBAL;

    if (params.MediaKind == NProto::STORAGE_MEDIA_SSD_LOCAL) {
        poolKind = NProto::DEVICE_POOL_KIND_LOCAL;
    }

    if (poolKind == NProto::DEVICE_POOL_KIND_LOCAL && !diskDevices.empty()) {
        const auto nodeId = diskDevices[0].GetNodeId();

        if (!nodeIds.empty() && !nodeIds.contains(nodeId)) {
            return MakeError(E_ARGUMENT, TStringBuilder()
                << "disk " << params.DiskId << " already allocated on "
                << diskDevices[0].GetAgentId());
        }

        nodeIds = { nodeId };
    }

    return TDeviceList::TAllocationQuery {
        .OtherRacks = std::move(otherDiskRacks),
        .LogicalBlockSize = params.BlockSize,
        .BlockCount = blocksToAllocate,
        .PoolName = params.PoolName,
        .PoolKind = poolKind,
        .NodeIds = std::move(nodeIds),
    };
}

NProto::TError TDiskRegistryState::ValidateAllocateDiskParams(
    const TDiskState& disk,
    const TAllocateDiskParams& params) const
{
    if (disk.ReplicaCount && !params.ReplicaCount) {
        return MakeError(
            E_INVALID_STATE,
            "attempt to reallocate mirrored disk as nonrepl");
    }

    if (disk.LogicalBlockSize && disk.LogicalBlockSize != params.BlockSize) {
        return MakeError(
            E_ARGUMENT,
            TStringBuilder() << "attempt to change LogicalBlockSize: "
                << disk.LogicalBlockSize << " -> " << params.BlockSize);
    }

    return {};
}

NProto::TError TDiskRegistryState::AllocateDisk(
    TInstant now,
    TDiskRegistryDatabase& db,
    const TAllocateDiskParams& params,
    TAllocateDiskResult* result)
{
    const auto& diskId = params.DiskId;
    auto& disk = Disks[diskId];
    const bool isNewDisk = disk.Devices.empty() && disk.ReplicaCount == 0;
    const auto groupId = GetMirroredDiskGroupId(diskId);

    auto onError = [&] () {
        if (isNewDisk) {
            Disks.erase(diskId);

            if (params.ReplicaCount) {
                TVector<TString> affectedDisks;
                DestroyPlacementGroup(db, groupId, affectedDisks);

                if (affectedDisks.size()) {
                    ReportMirroredDiskAllocationPlacementGroupCleanupFailure(
                        TStringBuilder()
                            << "AllocateDisk:PlacementGroupCleanupFailure:DiskId: "
                            << diskId);
                }
            }

            AddToBrokenDisks(now, db, diskId);
        }
    };

    if (auto error = ValidateAllocateDiskParams(disk, params); HasError(error)) {
        onError();

        return error;
    }

    if (params.ReplicaCount) {
        ui32 code = S_OK;

        if (disk.ReplicaCount) {
            const auto diskId0 = GetReplicaDiskId(diskId, 0);
            const auto* disk0 = Disks.FindPtr(diskId0);
            if (disk0) {
                ui64 size = 0;
                for (const auto& id: disk0->Devices) {
                    const auto* device = DeviceList.FindDevice(id);
                    if (device) {
                        size += device->GetBlockSize() * device->GetBlocksCount();
                    }
                }

                if (size >= params.BlocksCount * params.BlockSize) {
                    code = S_ALREADY;
                }
            }
        } else if (disk.Devices) {
            return MakeError(
                E_INVALID_STATE,
                "attempt to reallocate nonrepl disk as mirrored");
        }

        NProto::TError error;
        if (isNewDisk) {
            error = CreatePlacementGroup(db, groupId);
            if (HasError(error)) {
                onError();

                return error;
            }
        }

        auto subParams = params;
        subParams.ReplicaCount = 0;
        subParams.DiskId = GetReplicaDiskId(diskId, 0);
        subParams.PlacementGroupId = groupId;
        subParams.MasterDiskId = diskId;

        TAllocateDiskResult subResult;
        error = AllocateSimpleDisk(
            now,
            db,
            subParams,
            Disks[subParams.DiskId],
            &subResult);

        if (HasError(error)) {
            onError();

            return error;
        }

        TVector<TString> allocatedReplicaIds;
        allocatedReplicaIds.push_back(subParams.DiskId);

        result->Devices = std::move(subResult.Devices);

        for (ui32 i = 0; i < params.ReplicaCount; ++i) {
            subParams.DiskId = GetReplicaDiskId(diskId, i + 1);

            error = AllocateSimpleDisk(
                now,
                db,
                subParams,
                Disks[subParams.DiskId],
                &subResult);

            if (HasError(error)) {
                if (isNewDisk) {
                    for (const auto& replicaId: allocatedReplicaIds) {
                        DeallocateSimpleDisk(
                            db,
                            replicaId,
                            "AllocateDiskReplicas:Cleanup");
                    }
                } else {
                    // TODO (https://st.yandex-team.ru/NBS-3419):
                    // support automatic cleanup after a failed resize
                    ReportMirroredDiskAllocationCleanupFailure(TStringBuilder()
                        << "AllocateDisk:ResizeCleanupFailure:DiskId: "
                        << diskId);
                }

                onError();

                return error;
            }

            result->Replicas.push_back(std::move(subResult.Devices));
            allocatedReplicaIds.push_back(subParams.DiskId);
        }

        result->DeviceReplacementIds = disk.DeviceReplacementIds;

        disk.CloudId = params.CloudId;
        disk.FolderId = params.FolderId;
        disk.LogicalBlockSize = params.BlockSize;
        disk.StateTs = now;
        disk.ReplicaCount = params.ReplicaCount;
        db.UpdateDisk(BuildDiskConfig(diskId, disk));

        TVector<TDeviceId> deviceIds;
        for (const auto& d: result->Devices) {
            deviceIds.push_back(d.GetDeviceUUID());
        }

        ReplicaTable.UpdateReplica(diskId, 0, deviceIds);

        for (ui32 i = 0; i < result->Replicas.size(); ++i) {
            deviceIds.clear();
            for (const auto& d: result->Replicas[i]) {
                deviceIds.push_back(d.GetDeviceUUID());
            }

            ReplicaTable.UpdateReplica(diskId, i + 1, deviceIds);
        }

        for (const auto& deviceId: result->DeviceReplacementIds) {
            ReplicaTable.MarkReplacementDevice(diskId, deviceId, true);
        }

        return MakeError(code);
    }

    return AllocateSimpleDisk(now, db, params, disk, result);
}

NProto::TError TDiskRegistryState::AllocateSimpleDisk(
    TInstant now,
    TDiskRegistryDatabase& db,
    const TAllocateDiskParams& params,
    TDiskState& disk,
    TAllocateDiskResult* result)
{
    auto onError = [&] () {
        const bool isNewDisk = disk.Devices.empty();

        if (isNewDisk) {
            Disks.erase(params.DiskId);

            if (!params.MasterDiskId) {
                // failed to allocate storage for the new volume, need to
                // destroy this volume
                AddToBrokenDisks(now, db, params.DiskId);
            }
        }
    };

    Y_VERIFY(disk.ReplicaCount == 0);
    Y_VERIFY(params.ReplicaCount == 0);

    if (!disk.StateTs) {
        disk.StateTs = now;
    }

    result->IOModeTs = disk.StateTs;
    result->IOMode = disk.State < NProto::DISK_STATE_ERROR
        ? NProto::VOLUME_IO_OK
        : NProto::VOLUME_IO_ERROR_READ_ONLY;

    result->MuteIOErrors =
        disk.State >= NProto::DISK_STATE_TEMPORARILY_UNAVAILABLE;

    const auto& placementGroupId = (disk.Devices && disk.PlacementGroupId)
        ? disk.PlacementGroupId
        : params.PlacementGroupId;

    NProto::TPlacementGroupConfig* placementGroup = nullptr;
    if (placementGroupId) {
        auto* g = PlacementGroups.FindPtr(placementGroupId);
        if (!g) {
            onError();

            return MakeError(
                E_NOT_FOUND,
                TStringBuilder() << "placement group " << placementGroupId
                    << " not found"
            );
        }

        placementGroup = &g->Config;
    }

    auto& output = result->Devices;

    if (auto error = GetDiskMigrations(disk, result->Migrations); HasError(error)) {
        onError();

        return error;
    }

    if (auto error = GetDiskDevices(params.DiskId, disk, output); HasError(error)) {
        onError();

        return error;
    }

    if (auto error = ValidateDiskLocation(output, params); HasError(error)) {
        onError();

        return error;
    }

    ui64 currentSize = 0;
    for (const auto& d: output) {
        currentSize += d.GetBlockSize() * d.GetBlocksCount();
    }

    ui64 requestedSize = params.BlockSize * params.BlocksCount;

    if (requestedSize <= currentSize) {
        if (disk.CloudId.empty() && !params.CloudId.empty()) {
            disk.CloudId = params.CloudId;
            disk.FolderId = params.FolderId;
            db.UpdateDisk(BuildDiskConfig(params.DiskId, disk));
        }

        return MakeError(S_ALREADY, TStringBuilder() <<
            "disk " << params.DiskId.Quote() << " already exists");
    }

    if (!DiskAllocationAllowed) {
        SelfCounters.RejectedAllocations.Increment(1);

        onError();

        return MakeError(E_BS_DISK_ALLOCATION_FAILED, "Operation disallowed");
    }

    ui64 blocksToAllocate =
        ceil(double(requestedSize - currentSize) / params.BlockSize);

    if (placementGroup
            && output.empty()
            && placementGroup->DisksSize()
                >= GetMaxDisksInPlacementGroup(*StorageConfig, *placementGroup))
    {
        onError();

        return MakeError(E_BS_RESOURCE_EXHAUSTED, TStringBuilder() <<
            "max disk count in group exceeded, max: "
            << GetMaxDisksInPlacementGroup(*StorageConfig, *placementGroup));
    }

    THashSet<TString> otherDiskRacks;
    NProto::TPlacementGroupConfig::TDiskInfo* thisDisk = placementGroup
        ? CollectRacks(params.DiskId, *placementGroup, &otherDiskRacks)
        : nullptr;

    auto [query, error] = PrepareAllocationQuery(
        blocksToAllocate,
        std::move(otherDiskRacks),
        output,
        params);

    if (HasError(error)) {
        onError();

        return error;
    }

    // TODO (https://st.yandex-team.ru/NBS-1278):
    // try to allocate space close to the already allocated devices
    auto allocatedDevices = DeviceList.AllocateDevices(params.DiskId, query);

    if (!allocatedDevices) {
        onError();

        return MakeError(E_BS_DISK_ALLOCATION_FAILED, TStringBuilder() <<
            "can't allocate disk with " << blocksToAllocate << " blocks x " <<
            params.BlockSize << " bytes");
    }

    for (const auto& device: allocatedDevices) {
        disk.Devices.push_back(device.GetDeviceUUID());
    }

    output.insert(
        output.end(),
        std::make_move_iterator(allocatedDevices.begin()),
        std::make_move_iterator(allocatedDevices.end()));

    disk.LogicalBlockSize = params.BlockSize;
    disk.CloudId = params.CloudId;
    disk.FolderId = params.FolderId;
    disk.MasterDiskId = params.MasterDiskId;

    db.UpdateDisk(BuildDiskConfig(params.DiskId, disk));

    if (placementGroup) {
        if (!thisDisk) {
            thisDisk = placementGroup->AddDisks();
            thisDisk->SetDiskId(params.DiskId);
        }

        RebuildDiskPlacementInfo(disk, thisDisk);
        placementGroup->SetConfigVersion(placementGroup->GetConfigVersion() + 1);
        db.UpdatePlacementGroup(*placementGroup);
        disk.PlacementGroupId = placementGroupId;
    }

    return {};
}

NProto::TError TDiskRegistryState::DeallocateDisk(
    TDiskRegistryDatabase& db,
    const TString& diskId)
{
    auto it = Disks.find(diskId);
    if (it == Disks.end()) {
        return MakeError(E_NOT_FOUND, TStringBuilder() <<
            "disk " << diskId.Quote() << " not found");
    }

    auto& disk = it->second;

    if (disk.ReplicaCount) {
        const TString groupId = GetMirroredDiskGroupId(diskId);
        TVector<TString> affectedDisks;
        auto error = DestroyPlacementGroup(db, groupId, affectedDisks);
        if (HasError(error)) {
            return error;
        }

        for (const auto& affectedDiskId: affectedDisks) {
            Y_VERIFY_DEBUG(affectedDiskId.StartsWith(diskId + "/"));
            DeallocateSimpleDisk(db, affectedDiskId, "DeallocateDisk:Replica");
        }

        DeleteDisk(db, diskId);
        ReplicaTable.RemoveMirroredDisk(diskId);

        return {};
    }

    DeallocateSimpleDisk(db, diskId, disk);

    return {};
}

void TDiskRegistryState::DeallocateSimpleDisk(
    TDiskRegistryDatabase& db,
    const TString& diskId,
    const TString& parentMethodName)
{
    auto it = Disks.find(diskId);
    if (it == Disks.end()) {
        ReportDiskRegistryDiskNotFound(
            TStringBuilder() << parentMethodName << ":DiskId: "
            << diskId);

        return;
    }

    DeallocateSimpleDisk(db, diskId, it->second);
}

void TDiskRegistryState::DeallocateSimpleDisk(
    TDiskRegistryDatabase& db,
    const TString& diskId,
    TDiskState& disk)
{
    Y_VERIFY(disk.ReplicaCount == 0);

    for (const auto& uuid: disk.Devices) {
        if (DeviceList.ReleaseDevice(uuid)) {
            db.UpdateDirtyDevice(uuid);
        }
    }

    if (disk.PlacementGroupId) {
        auto* pg = PlacementGroups.FindPtr(disk.PlacementGroupId);

        if (pg) {
            auto& config = pg->Config;

            auto end = std::remove_if(
                config.MutableDisks()->begin(),
                config.MutableDisks()->end(),
                [&] (const NProto::TPlacementGroupConfig::TDiskInfo& d) {
                    return diskId == d.GetDiskId();
                }
            );

            while (config.MutableDisks()->end() > end) {
                config.MutableDisks()->RemoveLast();
            }

            config.SetConfigVersion(config.GetConfigVersion() + 1);
            db.UpdatePlacementGroup(config);
        } else {
            ReportDiskRegistryPlacementGroupNotFound(
                TStringBuilder() << "DeallocateDisk:DiskId: " << diskId
                << ", PlacementGroupId: " << disk.PlacementGroupId);
        }
    }

    for (const auto& [targetId, sourceId]: disk.MigrationTarget2Source) {
        Y_UNUSED(sourceId);

        if (DeviceList.ReleaseDevice(targetId)) {
            db.UpdateDirtyDevice(targetId);
        }
    }

    for (const auto& [uuid, seqNo]: disk.FinishedMigrations) {
        Y_UNUSED(seqNo);

        if (DeviceList.ReleaseDevice(uuid)) {
            db.UpdateDirtyDevice(uuid);
        }
    }

    DeleteDeviceMigration(diskId);

    DeleteDisk(db, diskId);
}

void TDiskRegistryState::DeleteDisk(
    TDiskRegistryDatabase& db,
    const TString& diskId)
{
    Disks.erase(diskId);
    DisksToCleanup.erase(diskId);
    DisksToNotify.erase(diskId);
    OutdatedVolumeConfigs.erase(diskId);

    db.DeleteDisk(diskId);
    db.DeleteDiskToCleanup(diskId);
    db.DeleteDiskToNotify(diskId);
    db.DeleteOutdatedVolumeConfig(diskId);
}

void TDiskRegistryState::AddToBrokenDisks(
    TInstant now,
    TDiskRegistryDatabase& db,
    const TString& diskId)
{
    TBrokenDiskInfo brokenDiskInfo{
        diskId,
        now + StorageConfig->GetBrokenDiskDestructionDelay()
    };
    db.AddBrokenDisk(brokenDiskInfo);
    BrokenDisks.push_back(brokenDiskInfo);
}

NProto::TDeviceConfig TDiskRegistryState::GetDevice(const TString& id) const
{
    const auto* device = DeviceList.FindDevice(id);

    if (device) {
        return *device;
    }

    return {};
}

const NProto::TDeviceConfig* TDiskRegistryState::FindDevice(
    const TString& agentId,
    const TString& path) const
{
    const auto* agent = AgentList.FindAgent(agentId);
    if (!agent) {
        return nullptr;
    }

    return FindIfPtr(agent->GetDevices(), [&] (const auto& x) {
        return x.GetDeviceName() == path;
    });
}

TString TDiskRegistryState::GetDeviceId(
    const TString& agentId,
    const TString& path) const
{
    auto* device = FindDevice(agentId, path);
    if (device) {
        return device->GetDeviceUUID();
    }
    return {};
}

NProto::TError TDiskRegistryState::GetDependentDisks(
    const TString& agentId,
    const TString& path,
    TVector<TDiskId>* diskIds) const
{
    auto* agent = AgentList.FindAgent(agentId);
    if (!agent) {
        return MakeError(E_NOT_FOUND, agentId);
    }

    for (const auto& d: agent->GetDevices()) {
        if (path && d.GetDeviceName() != path) {
            continue;
        }

        auto diskId = FindDisk(d.GetDeviceUUID());

        if (!diskId) {
            continue;
        }

        diskIds->push_back(std::move(diskId));
    }

    return {};
}

NProto::TError TDiskRegistryState::GetDiskDevices(
    const TString& diskId,
    TVector<NProto::TDeviceConfig>& devices) const
{
    auto it = Disks.find(diskId);

    if (it == Disks.end()) {
        return MakeError(E_NOT_FOUND, TStringBuilder() <<
            "disk " << diskId.Quote() << " not found");
    }

    return GetDiskDevices(diskId, it->second, devices);
}

NProto::TError TDiskRegistryState::GetDiskDevices(
    const TString& diskId,
    const TDiskState& disk,
    TVector<NProto::TDeviceConfig>& devices) const
{
    auto* overrides = DeviceOverrides.FindPtr(diskId);

    for (const auto& uuid: disk.Devices) {
        const auto* device = DeviceList.FindDevice(uuid);

        if (!device) {
            return MakeError(E_NOT_FOUND, TStringBuilder() <<
                "device " << uuid.Quote() << " not found");
        }

        devices.emplace_back(*device);
        if (overrides) {
            auto* blocksCount = overrides->Device2BlockCount.FindPtr(uuid);
            if (blocksCount) {
                devices.back().SetBlocksCount(*blocksCount);
            }
        }
    }

    return {};
}

NProto::TError TDiskRegistryState::GetDiskMigrations(
    const TDiskState& disk,
    TVector<NProto::TDeviceMigration>& migrations) const
{
    migrations.reserve(disk.MigrationTarget2Source.size());

    for (const auto& [targetId, sourceId]: disk.MigrationTarget2Source) {
        NProto::TDeviceMigration m;
        m.SetSourceDeviceId(sourceId);
        *m.MutableTargetDevice() = GetDevice(targetId);

        const auto& uuid = m.GetTargetDevice().GetDeviceUUID();

        if (uuid.empty()) {
            return MakeError(E_NOT_FOUND, TStringBuilder() <<
                "device " << uuid.Quote() << " not found");
        }

        migrations.push_back(std::move(m));
    }

    return {};
}

NProto::TError TDiskRegistryState::GetDiskInfo(
    const TString& diskId,
    TDiskInfo& diskInfo) const
{
    auto it = Disks.find(diskId);

    if (it == Disks.end()) {
        return MakeError(E_NOT_FOUND, TStringBuilder() <<
            "disk " << diskId.Quote() << " not found");
    }

    const auto& disk = it->second;

    diskInfo.CloudId = disk.CloudId;
    diskInfo.FolderId = disk.FolderId;
    diskInfo.UserId = disk.UserId;
    diskInfo.LogicalBlockSize = disk.LogicalBlockSize;
    diskInfo.State = disk.State;
    diskInfo.StateTs = disk.StateTs;
    diskInfo.PlacementGroupId = disk.PlacementGroupId;
    diskInfo.FinishedMigrations = disk.FinishedMigrations;
    diskInfo.DeviceReplacementIds = disk.DeviceReplacementIds;

    NProto::TError error;

    if (disk.ReplicaCount) {
        error = GetDiskDevices(GetReplicaDiskId(diskId, 0), diskInfo.Devices);
        diskInfo.Replicas.resize(disk.ReplicaCount);
        for (ui32 i = 0; i < disk.ReplicaCount; ++i) {
            if (!HasError(error)) {
                error = GetDiskDevices(
                    GetReplicaDiskId(diskId, i + 1),
                    diskInfo.Replicas[i]);
            }
        }
    } else {
        error = GetDiskDevices(diskId, disk, diskInfo.Devices);
    }

    if (!HasError(error)) {
        error = GetDiskMigrations(disk, diskInfo.Migrations);
    }

    if (error.GetCode() == E_NOT_FOUND) {
        return MakeError(E_INVALID_STATE, error.GetMessage());
    }

    return error;
}

bool TDiskRegistryState::FilterDevicesAtUnavailableAgents(TDiskInfo& diskInfo) const
{
    auto isUnavailable = [&] (const auto& d) {
        const auto* agent = AgentList.FindAgent(d.GetAgentId());

        return !agent || agent->GetState() == NProto::AGENT_STATE_UNAVAILABLE;
    };

    EraseIf(diskInfo.Devices, isUnavailable);
    EraseIf(diskInfo.Migrations, [&] (const auto& d) {
        return isUnavailable(d.GetTargetDevice());
    });

    return diskInfo.Devices.size() || diskInfo.Migrations.size();
}

NProto::TError TDiskRegistryState::StartAcquireDisk(
    const TString& diskId,
    TDiskInfo& diskInfo)
{
    auto it = Disks.find(diskId);

    if (it == Disks.end()) {
        return MakeError(E_NOT_FOUND, TStringBuilder() <<
            "disk " << diskId.Quote() << " not found");
    }

    auto& disk = it->second;

    if (disk.AcquireInProgress) {
        return MakeError(E_REJECTED, TStringBuilder() <<
            "disk " << diskId.Quote() << " acquire in progress");
    }

    NProto::TError error;
    if (disk.ReplicaCount) {
        error = GetDiskDevices(GetReplicaDiskId(diskId, 0), diskInfo.Devices);
        diskInfo.Replicas.resize(disk.ReplicaCount);
        for (ui32 i = 0; i < disk.ReplicaCount; ++i) {
            if (!HasError(error)) {
                error = GetDiskDevices(
                    GetReplicaDiskId(diskId, i + 1),
                    diskInfo.Replicas[i]);
            }
        }
    } else {
        error = GetDiskDevices(diskId, disk, diskInfo.Devices);
    }

    if (HasError(error)) {
        return error;
    }

    error = GetDiskMigrations(disk, diskInfo.Migrations);

    if (HasError(error)) {
        return error;
    }

    disk.AcquireInProgress = true;

    diskInfo.LogicalBlockSize = disk.LogicalBlockSize;

    return {};
}

void TDiskRegistryState::FinishAcquireDisk(const TString& diskId)
{
    auto it = Disks.find(diskId);

    if (it == Disks.end()) {
        return;
    }

    auto& disk = it->second;

    disk.AcquireInProgress = false;
}

bool TDiskRegistryState::IsAcquireInProgress(const TString& diskId) const
{
    auto it = Disks.find(diskId);

    if (it == Disks.end()) {
        return false;
    }

    return it->second.AcquireInProgress;
}

ui32 TDiskRegistryState::GetConfigVersion() const
{
    return CurrentConfig.GetVersion();
}

NProto::TError TDiskRegistryState::UpdateConfig(
    TDiskRegistryDatabase& db,
    NProto::TDiskRegistryConfig newConfig,
    bool ignoreVersion,
    TVector<TString>& affectedDisks)
{
    if (!ignoreVersion && newConfig.GetVersion() != CurrentConfig.GetVersion()) {
        return MakeError(E_CONFIG_VERSION_MISMATCH, "Wrong config version");
    }

    for (const auto& pool: newConfig.GetDevicePoolConfigs()) {
        if (pool.GetName().empty()
                && pool.GetKind() != NProto::DEVICE_POOL_KIND_DEFAULT)
        {
            return MakeError(E_ARGUMENT, "non default pool with empty name");
        }

        if (!pool.GetName().empty()
                && pool.GetKind() == NProto::DEVICE_POOL_KIND_DEFAULT)
        {
            return MakeError(E_ARGUMENT, "default pool with non empty name");
        }
    }

    THashSet<TString> allDevices;
    TKnownAgents newKnownAgents;

    for (const auto& agent: newConfig.GetKnownAgents()) {
        if (newKnownAgents.contains(agent.GetAgentId())) {
            return MakeError(E_ARGUMENT, "bad config");
        }

        auto& uuids = newKnownAgents[agent.GetAgentId()];

        for (const auto& device: agent.GetDevices()) {
            uuids.insert(device.GetDeviceUUID());
            auto [it, ok] = allDevices.insert(device.GetDeviceUUID());
            if (!ok) {
                return MakeError(E_ARGUMENT, "bad config");
            }
        }
    }

    TVector<TString> removedDevices;
    TVector<TString> removedAgents;

    for (const auto& [id, uuids]: KnownAgents) {
        for (const auto& uuid: uuids) {
            if (!allDevices.contains(uuid)) {
                removedDevices.push_back(uuid);
            }
        }

        if (!newKnownAgents.contains(id)) {
            removedAgents.push_back(id);
        }
    }

    THashSet<TString> diskIds;

    for (const auto& uuid: removedDevices) {
        auto diskId = DeviceList.FindDiskId(uuid);
        if (diskId) {
            diskIds.emplace(std::move(diskId));
        }
    }

    affectedDisks.assign(
        std::make_move_iterator(diskIds.begin()),
        std::make_move_iterator(diskIds.end()));

    Sort(affectedDisks);

    if (!affectedDisks.empty()) {
        return MakeError(E_INVALID_STATE, "Destructive configuration change");
    }

    newConfig.SetVersion(CurrentConfig.GetVersion() + 1);
    ProcessConfig(newConfig);

    for (const auto& id: removedAgents) {
        RemoveAgent(db, id);
    }

    db.WriteDiskRegistryConfig(newConfig);
    CurrentConfig = std::move(newConfig);

    return {};
}

void TDiskRegistryState::RemoveAgent(
    TDiskRegistryDatabase& db,
    const NProto::TAgentConfig& agent)
{
    const auto nodeId = agent.GetNodeId();
    const auto agentId = agent.GetAgentId();

    DeviceList.RemoveDevices(agent);
    AgentList.RemoveAgent(nodeId);

    db.DeleteOldAgent(nodeId);
    db.DeleteAgent(agentId);
}

template <typename T>
bool TDiskRegistryState::RemoveAgent(
    TDiskRegistryDatabase& db,
    const T& id)
{
    auto* agent = AgentList.FindAgent(id);
    if (!agent) {
        return false;
    }

    RemoveAgent(db, *agent);

    return true;
}

void TDiskRegistryState::ProcessConfig(const NProto::TDiskRegistryConfig& config)
{
    TKnownAgents newKnownAgents;

    for (const auto& agent: config.GetKnownAgents()) {
        auto& uuids = newKnownAgents[agent.GetAgentId()];

        for (const auto& device: agent.GetDevices()) {
            uuids.insert(device.GetDeviceUUID());
        }
    }

    newKnownAgents.swap(KnownAgents);

    TDeviceOverrides newDeviceOverrides;

    for (const auto& deviceOverride: config.GetDeviceOverrides()) {
        auto& diskOverrides = newDeviceOverrides[deviceOverride.GetDiskId()];
        diskOverrides.Device2BlockCount[deviceOverride.GetDevice()] =
            deviceOverride.GetBlocksCount();
    }

    newDeviceOverrides.swap(DeviceOverrides);

    DevicePoolConfigs = CreateDevicePoolConfigs(config, *StorageConfig);
}

const NProto::TDiskRegistryConfig& TDiskRegistryState::GetConfig() const
{
    return CurrentConfig;
}

TVector<TString> TDiskRegistryState::GetDiskIds() const
{
    TVector<TString> ids(Reserve(Disks.size()));

    for (auto& kv: Disks) {
        ids.push_back(kv.first);
    }

    Sort(ids);

    return ids;
}

TVector<TString> TDiskRegistryState::GetMasterDiskIds() const
{
    TVector<TString> ids(Reserve(Disks.size()));

    for (auto& kv: Disks) {
        if (!kv.second.MasterDiskId) {
            ids.push_back(kv.first);
        }
    }

    Sort(ids);

    return ids;
}

bool TDiskRegistryState::IsMasterDisk(const TString& diskId) const
{
    const auto* disk = Disks.FindPtr(diskId);
    return disk && disk->ReplicaCount;
}

TVector<NProto::TDeviceConfig> TDiskRegistryState::GetDirtyDevices() const
{
    return DeviceList.GetDirtyDevices();
}

bool TDiskRegistryState::IsKnownDevice(const TString& uuid) const
{
    for (auto& [id, devices]: KnownAgents) {
        if (devices.contains(uuid)) {
            return true;
        }
    }

    return false;
}

NProto::TError TDiskRegistryState::MarkDiskForCleanup(
    TDiskRegistryDatabase& db,
    const TString& diskId)
{
    if (!Disks.contains(diskId)) {
        return MakeError(E_NOT_FOUND, TStringBuilder() << "disk " <<
            diskId.Quote() << " not found");
    }

    db.AddDiskToCleanup(diskId);
    DisksToCleanup.insert(diskId);

    return {};
}

bool TDiskRegistryState::MarkDeviceAsDirty(
    TDiskRegistryDatabase& db,
    const TDeviceId& uuid)
{
    if (!IsKnownDevice(uuid)) {
        return false;
    }

    DeviceList.MarkDeviceAsDirty(uuid);
    db.UpdateDirtyDevice(uuid);

    return true;
}

bool TDiskRegistryState::MarkDeviceAsClean(
    TDiskRegistryDatabase& db,
    const TDeviceId& uuid)
{
    DeviceList.MarkDeviceAsClean(uuid);
    db.DeleteDirtyDevice(uuid);

    return TryUpdateDevice(db, uuid);
}

bool TDiskRegistryState::TryUpdateDevice(
    TDiskRegistryDatabase& db,
    const TDeviceId& uuid)
{
    auto [agent, device] = FindDeviceLocation(uuid);
    if (!agent || !device) {
        return false;
    }

    AdjustDeviceIfNeeded(*device, {});

    UpdateAgent(db, *agent);
    DeviceList.UpdateDevices(*agent);

    return true;
}

TVector<TString> TDiskRegistryState::CollectBrokenDevices(
    const NProto::TAgentStats& stats) const
{
    TVector<TString> uuids;

    for (const auto& ds: stats.GetDeviceStats()) {
        if (!ds.GetErrors()) {
            continue;
        }

        const auto& uuid = ds.GetDeviceUUID();

        const auto* device = DeviceList.FindDevice(uuid);
        if (!device) {
            continue;
        }

        Y_VERIFY_DEBUG(device->GetNodeId() == stats.GetNodeId());

        if (device->GetState() != NProto::DEVICE_STATE_ERROR) {
            uuids.push_back(uuid);
        }
    }

    return uuids;
}

NProto::TError TDiskRegistryState::UpdateAgentCounters(
    const NProto::TAgentStats& stats)
{
    // TODO (https://st.yandex-team.ru/NBS-3280): add AgentId to TAgentStats
    const auto* agent = AgentList.FindAgent(stats.GetNodeId());

    if (!agent) {
        return MakeError(E_NOT_FOUND, "agent not found");
    }

    for (const auto& device: stats.GetDeviceStats()) {
        const auto& uuid = device.GetDeviceUUID();

        if (stats.GetNodeId() != DeviceList.FindNodeId(uuid)) {
            return MakeError(E_ARGUMENT, "unexpected device");
        }
    }

    AgentList.UpdateCounters(stats);

    return {};
}

THashMap<TString, TBrokenGroupInfo> TDiskRegistryState::GatherBrokenGroupsInfo(
    TInstant now,
    TDuration period) const
{
    THashMap<TString, TBrokenGroupInfo> groups;

    for (const auto& x: Disks) {
        if (x.second.State != NProto::DISK_STATE_TEMPORARILY_UNAVAILABLE &&
            x.second.State != NProto::DISK_STATE_ERROR)
        {
            continue;
        }

        if (x.second.PlacementGroupId.empty()) {
            continue;
        }

        TBrokenGroupInfo& info = groups[x.second.PlacementGroupId];

        ++info.TotalBrokenDiskCount;
        if (now - period < x.second.StateTs) {
            ++info.RecentlyBrokenDiskCount;
        }
    }

    return groups;
}

void TDiskRegistryState::PublishCounters(TInstant now)
{
    if (!Counters) {
        return;
    }

    AgentList.PublishCounters(now);

    ui64 freeBytes = 0;
    ui64 totalBytes = 0;
    ui32 allocatedDisks = 0;
    ui32 allocatedDevices = 0;
    ui32 dirtyDevices = 0;
    ui32 devicesInOnlineState = 0;
    ui32 devicesInWarningState = 0;
    ui32 devicesInErrorState = 0;
    ui32 agentsInOnlineState = 0;
    ui32 agentsInWarningState = 0;
    ui32 agentsInUnavailableState = 0;
    ui32 disksInOnlineState = 0;
    ui32 disksInMigrationState = 0;
    ui32 disksInTemporarilyUnavailableState = 0;
    ui32 disksInErrorState = 0;
    ui32 placementGroups = 0;
    ui32 fullPlacementGroups = 0;
    ui32 allocatedDisksInGroups = 0;

    for (const auto& agent: AgentList.GetAgents()) {
        switch (agent.GetState()) {
            case NProto::AGENT_STATE_ONLINE: {
                ++agentsInOnlineState;
                break;
            }
            case NProto::AGENT_STATE_WARNING: {
                ++agentsInWarningState;
                break;
            }
            case NProto::AGENT_STATE_UNAVAILABLE: {
                ++agentsInUnavailableState;
                break;
            }
            default: {}
        }

        for (const auto& device: agent.GetDevices()) {
            const auto deviceState = device.GetState();
            const auto deviceBytes = device.GetBlockSize() * device.GetBlocksCount();
            const bool allocated = !DeviceList.FindDiskId(device.GetDeviceUUID()).empty();
            const bool dirty = DeviceList.IsDirtyDevice(device.GetDeviceUUID());

            totalBytes += deviceBytes;

            if (allocated) {
                ++allocatedDevices;
            }

            if (dirty) {
                ++dirtyDevices;
            }

            switch (deviceState) {
                case NProto::DEVICE_STATE_ONLINE: {
                    if (!allocated && !dirty && agent.GetState() == NProto::AGENT_STATE_ONLINE) {
                        freeBytes += deviceBytes;
                    }

                    ++devicesInOnlineState;
                    break;
                }
                case NProto::DEVICE_STATE_WARNING: {
                    ++devicesInWarningState;
                    break;
                }
                case NProto::DEVICE_STATE_ERROR: {
                    ++devicesInErrorState;
                    break;
                }
                default: {}
            }
        }
    }

    placementGroups = PlacementGroups.size();

    for (auto& x: PlacementGroups) {
        allocatedDisksInGroups += x.second.Config.DisksSize();

        const auto limit =
            GetMaxDisksInPlacementGroup(*StorageConfig, x.second.Config);
        if (x.second.Config.DisksSize() >= limit
                || x.second.Config.DisksSize() == 0)
        {
            continue;
        }

        x.second.BiggestDiskId = {};
        x.second.BiggestDiskSize = 0;
        x.second.Full = false;
        ui32 logicalBlockSize = 0;
        for (const auto& diskInfo: x.second.Config.GetDisks()) {
            auto* disk = Disks.FindPtr(diskInfo.GetDiskId());

            if (!disk) {
                ReportDiskRegistryDiskNotFound(
                    TStringBuilder() << "PublishCounters:DiskId: "
                    << diskInfo.GetDiskId());

                continue;
            }

            ui64 diskSize = 0;
            for (const auto& deviceId: disk->Devices) {
                const auto& device = DeviceList.FindDevice(deviceId);

                if (!device) {
                    ReportDiskRegistryDeviceNotFound(
                        TStringBuilder() << "PublishCounters:DiskId: "
                        << diskInfo.GetDiskId()
                        << ", DeviceId: " << deviceId);

                    continue;
                }

                diskSize += device->GetBlockSize() * device->GetBlocksCount();
            }

            if (diskSize > x.second.BiggestDiskSize) {
                logicalBlockSize = disk->LogicalBlockSize;
                x.second.BiggestDiskId = diskInfo.GetDiskId();
                x.second.BiggestDiskSize = diskSize;
            }
        }

        if (!logicalBlockSize) {
            continue;
        }

        THashSet<TString> otherDiskRacks;
        CollectRacks(x.second.Config, &otherDiskRacks);

        const TDeviceList::TAllocationQuery query {
            .OtherRacks = std::move(otherDiskRacks),
            .LogicalBlockSize = logicalBlockSize,
            .BlockCount = x.second.BiggestDiskSize / logicalBlockSize,
            .PoolName = {},
            // TODO (https://st.yandex-team.ru/NBS-3392): handle other kinds
            .PoolKind = NProto::DEVICE_POOL_KIND_DEFAULT,
            .NodeIds = {}
        };

        if (!DeviceList.CanAllocateDevices(query)) {
            ++fullPlacementGroups;
            x.second.Full = true;
        }
    }

    allocatedDisks = Disks.size();

    TDuration maxMigrationTime;

    for (const auto& x: Disks) {
        switch (x.second.State) {
            case NProto::DISK_STATE_ONLINE: {
                ++disksInOnlineState;
                break;
            }
            case NProto::DISK_STATE_MIGRATION: {
                ++disksInMigrationState;

                maxMigrationTime = std::max(maxMigrationTime, now - x.second.StateTs);

                break;
            }
            case NProto::DISK_STATE_TEMPORARILY_UNAVAILABLE: {
                ++disksInTemporarilyUnavailableState;
                break;
            }
            case NProto::DISK_STATE_ERROR: {
                ++disksInErrorState;
                break;
            }
            default: {}
        }
    }

    SelfCounters.FreeBytes->Set(freeBytes);
    SelfCounters.TotalBytes->Set(totalBytes);
    SelfCounters.AllocatedDisks->Set(allocatedDisks);
    SelfCounters.AllocatedDevices->Set(allocatedDevices);
    SelfCounters.DirtyDevices->Set(dirtyDevices);
    SelfCounters.DevicesInOnlineState->Set(devicesInOnlineState);
    SelfCounters.DevicesInWarningState->Set(devicesInWarningState);
    SelfCounters.DevicesInErrorState->Set(devicesInErrorState);
    SelfCounters.AgentsInOnlineState->Set(agentsInOnlineState);
    SelfCounters.AgentsInWarningState->Set(agentsInWarningState);
    SelfCounters.AgentsInUnavailableState->Set(agentsInUnavailableState);
    SelfCounters.DisksInOnlineState->Set(disksInOnlineState);
    SelfCounters.DisksInMigrationState->Set(disksInMigrationState);
    SelfCounters.DevicesInMigrationState->Set(
        DeviceMigrationsInProgress + Migrations.size());
    SelfCounters.DisksInTemporarilyUnavailableState->Set(
        disksInTemporarilyUnavailableState);
    SelfCounters.DisksInErrorState->Set(disksInErrorState);
    SelfCounters.PlacementGroups->Set(placementGroups);
    SelfCounters.FullPlacementGroups->Set(fullPlacementGroups);
    SelfCounters.AllocatedDisksInGroups->Set(allocatedDisksInGroups);
    SelfCounters.MaxMigrationTime->Set(maxMigrationTime.Seconds());

    ui32 placementGroupsWithRecentlyBrokenSingleDisk = 0;
    ui32 placementGroupsWithRecentlyBrokenTwoOrMoreDisks = 0;
    ui32 placementGroupsWithBrokenSingleDisk = 0;
    ui32 placementGroupsWithBrokenTwoOrMoreDisks = 0;

    auto brokenGroups = GatherBrokenGroupsInfo(
        now,
        StorageConfig->GetPlacementGroupAlertPeriod());

    for (const auto& kv: brokenGroups) {
        const auto [total, recently] = kv.second;

        placementGroupsWithRecentlyBrokenSingleDisk += recently == 1;
        placementGroupsWithRecentlyBrokenTwoOrMoreDisks += recently > 1;
        placementGroupsWithBrokenSingleDisk += total == 1;
        placementGroupsWithBrokenTwoOrMoreDisks += total > 1;
    }

    SelfCounters.PlacementGroupsWithRecentlyBrokenSingleDisk->Set(
        placementGroupsWithRecentlyBrokenSingleDisk);

    SelfCounters.PlacementGroupsWithRecentlyBrokenTwoOrMoreDisks->Set(
        placementGroupsWithRecentlyBrokenTwoOrMoreDisks);

    SelfCounters.PlacementGroupsWithBrokenSingleDisk->Set(
        placementGroupsWithBrokenSingleDisk);

    SelfCounters.PlacementGroupsWithBrokenTwoOrMoreDisks->Set(
        placementGroupsWithBrokenTwoOrMoreDisks);

    SelfCounters.RejectedAllocations.Publish(now);
    SelfCounters.RejectedAllocations.Reset();

    SelfCounters.QueryAvailableStorageErrors.Publish(now);
    SelfCounters.QueryAvailableStorageErrors.Reset();
}

NProto::TError TDiskRegistryState::CreatePlacementGroup(
    TDiskRegistryDatabase& db,
    const TString& groupId)
{
    if (PlacementGroups.contains(groupId)) {
        return MakeError(S_ALREADY);
    }

    auto& g = PlacementGroups[groupId].Config;
    g.SetGroupId(groupId);
    g.SetConfigVersion(1);
    db.UpdatePlacementGroup(g);

    return {};
}

NProto::TError TDiskRegistryState::UpdatePlacementGroupSettings(
    TDiskRegistryDatabase& db,
    const TString& groupId,
    ui32 configVersion,
    NProto::TPlacementGroupSettings settings)
{
    auto* g = PlacementGroups.FindPtr(groupId);
    NProto::TError error;
    if (!CheckPlacementGroupRequest(groupId, configVersion, g, &error)) {
        return error;
    }

    g->Config.SetConfigVersion(configVersion + 1);
    *g->Config.MutableSettings() = std::move(settings);
    db.UpdatePlacementGroup(g->Config);

    return {};
}

NProto::TError TDiskRegistryState::DestroyPlacementGroup(
    TDiskRegistryDatabase& db,
    const TString& groupId,
    TVector<TString>& affectedDisks)
{
    auto* g = FindPlacementGroup(groupId);
    if (!g) {
        return MakeError(S_ALREADY);
    }

    for (const auto& diskInfo: g->GetDisks()) {
        auto& diskId = diskInfo.GetDiskId();
        auto* d = Disks.FindPtr(diskId);
        if (!d) {
            ReportDiskRegistryDiskNotFound(TStringBuilder()
                << "DestroyPlacementGroup:DiskId: " << diskId
                << ", PlacementGroupId: " << groupId);

            continue;
        }

        d->PlacementGroupId.clear();

        OutdatedVolumeConfigs[diskId] = VolumeConfigSeqNo++;
        db.AddOutdatedVolumeConfig(diskId);
        affectedDisks.emplace_back(diskId);
    }

    db.DeletePlacementGroup(g->GetGroupId());
    PlacementGroups.erase(g->GetGroupId());

    return {};
}

NProto::TError TDiskRegistryState::AlterPlacementGroupMembership(
    TDiskRegistryDatabase& db,
    const TString& groupId,
    ui32 configVersion,
    TVector<TString>& disksToAdd,
    const TVector<TString>& disksToRemove)
{
    const auto* g = PlacementGroups.FindPtr(groupId);
    NProto::TError error;
    if (!CheckPlacementGroupRequest(groupId, configVersion, g, &error)) {
        return error;
    }

    auto newG = g->Config;

    if (disksToRemove) {
        auto end = std::remove_if(
            newG.MutableDisks()->begin(),
            newG.MutableDisks()->end(),
            [&] (const NProto::TPlacementGroupConfig::TDiskInfo& d) {
                return Find(disksToRemove.begin(), disksToRemove.end(), d.GetDiskId())
                    != disksToRemove.end();
            }
        );

        while (newG.MutableDisks()->end() > end) {
            newG.MutableDisks()->RemoveLast();
        }
    }

    TSet<TString> groupRacks;
    for (const auto& disk: newG.GetDisks()) {
        for (const auto& rack: disk.GetDeviceRacks()) {
            groupRacks.insert(rack);
        }
    }

    TVector<TString> failedToAdd;
    THashMap<TString, TVector<TString>> disk2racks;
    for (const auto& diskId: disksToAdd) {
        const auto it = FindIf(
            newG.GetDisks(),
            [&] (const NProto::TPlacementGroupConfig::TDiskInfo& d) {
                return d.GetDiskId() == diskId;
            }
        );

        if (it != newG.GetDisks().end()) {
            continue;
        }

        const auto* disk = Disks.FindPtr(diskId);

        if (!disk) {
            return MakeError(
                E_ARGUMENT,
                TStringBuilder() << "no such nonreplicated disk: " << diskId
                    << " - wrong media kind specified during disk creation?"
            );
        }

        if (disk->PlacementGroupId) {
            Y_VERIFY_DEBUG(disk->PlacementGroupId != groupId);

            failedToAdd.push_back(diskId);
            continue;
        }

        auto& racks = disk2racks[diskId];
        bool canAdd = true;

        for (const auto& deviceUuid: disk->Devices) {
            const auto rack = DeviceList.FindRack(deviceUuid);
            if (groupRacks.contains(rack)) {
                canAdd = false;
                break;
            }
        }

        if (!canAdd) {
            failedToAdd.push_back(diskId);
            continue;
        }

        for (const auto& deviceUuid: disk->Devices) {
            const auto rack = DeviceList.FindRack(deviceUuid);
            if (!FindPtr(racks, rack)) {
                racks.push_back(rack);
                groupRacks.insert(rack);
            }
        }
    }

    if (failedToAdd.size()) {
        disksToAdd = std::move(failedToAdd);
        return MakeError(E_PRECONDITION_FAILED, "failed to add some disks");
    }

    if (newG.DisksSize() + disk2racks.size()
            > GetMaxDisksInPlacementGroup(*StorageConfig, newG))
    {
        return MakeError(E_BS_RESOURCE_EXHAUSTED, TStringBuilder() <<
            "max disk count in group exceeded, max: "
            << GetMaxDisksInPlacementGroup(*StorageConfig, newG));
    }

    for (const auto& [diskId, racks]: disk2racks) {
        auto& d = *newG.AddDisks();
        d.SetDiskId(diskId);
        for (const auto& rack: racks) {
            *d.AddDeviceRacks() = rack;
        }
    }
    newG.SetConfigVersion(configVersion + 1);
    db.UpdatePlacementGroup(newG);
    PlacementGroups[groupId] = std::move(newG);

    for (const auto& diskId: disksToAdd) {
        auto* d = Disks.FindPtr(diskId);

        if (!d) {
            ReportDiskRegistryDiskNotFound(TStringBuilder()
                << "AlterPlacementGroupMembership:DiskId: " << diskId
                << ", PlacementGroupId: " << groupId);

            continue;
        }

        d->PlacementGroupId = groupId;

        OutdatedVolumeConfigs[diskId] = VolumeConfigSeqNo++;
        db.AddOutdatedVolumeConfig(diskId);
    }

    for (const auto& diskId: disksToRemove) {
        const auto it = Find(
            disksToAdd.begin(),
            disksToAdd.end(),
            diskId
        );

        if (it == disksToAdd.end()) {
            if (auto* d = Disks.FindPtr(diskId)) {
                d->PlacementGroupId.clear();

                OutdatedVolumeConfigs[diskId] = VolumeConfigSeqNo++;
                db.AddOutdatedVolumeConfig(diskId);
            }
        }
    }

    disksToAdd.clear();

    return {};
}

const NProto::TPlacementGroupConfig* TDiskRegistryState::FindPlacementGroup(
    const TString& groupId) const
{
    if (auto g = PlacementGroups.FindPtr(groupId)) {
        return &g->Config;
    }

    return nullptr;
}

void TDiskRegistryState::DeleteBrokenDisks(TDiskRegistryDatabase& db)
{
    for (const auto& x: BrokenDisks) {
        db.DeleteBrokenDisk(x.DiskId);
    }

    BrokenDisks.clear();
}

void TDiskRegistryState::UpdateAndNotifyDisk(
    TDiskRegistryDatabase& db,
    const TString& diskId,
    TDiskState& disk)
{
    db.UpdateDisk(BuildDiskConfig(diskId, disk));
    AddDiskToNotify(db, diskId);
}

ui64 TDiskRegistryState::AddDiskToNotify(
    TDiskRegistryDatabase& db,
    TString diskId)
{
    const auto* disk = Disks.FindPtr(diskId);
    Y_VERIFY_DEBUG(disk, "unknown disk: %s", diskId.c_str());

    if (disk && disk->MasterDiskId) {
        diskId = disk->MasterDiskId;
    }

    db.AddDiskToNotify(diskId);

    const auto seqNo = DisksToNotifySeqNo++;

    DisksToNotify[diskId] = seqNo;

    return seqNo;
}

auto TDiskRegistryState::FindDiskState(const TDiskId& diskId) -> TDiskState*
{
    auto it = Disks.find(diskId);
    if (it == Disks.end()) {
        return nullptr;
    }
    return &it->second;
}

void TDiskRegistryState::RemoveFinishedMigrations(
    TDiskRegistryDatabase& db,
    const TString& diskId,
    ui64 seqNo)
{
    auto* disk = FindDiskState(diskId);
    if (!disk) {
        return;
    }

    auto& migrations = disk->FinishedMigrations;

    auto it = std::remove_if(
        migrations.begin(),
        migrations.end(),
        [&] (const auto& m) {
            if (m.SeqNo > seqNo) {
                return false;
            }

            DeviceList.ReleaseDevice(m.DeviceId);
            db.UpdateDirtyDevice(m.DeviceId);

            return true;
        }
    );

    if (it != migrations.end()) {
        migrations.erase(it, migrations.end());
        db.UpdateDisk(BuildDiskConfig(diskId, *disk));
    }
}

void TDiskRegistryState::DeleteDiskToNotify(
    TDiskRegistryDatabase& db,
    const TString& diskId,
    ui64 seqNo)
{
    auto it = DisksToNotify.find(diskId);
    if (it != DisksToNotify.end() && it->second == seqNo) {
        DisksToNotify.erase(it);
        db.DeleteDiskToNotify(diskId);
    }

    RemoveFinishedMigrations(db, diskId, seqNo);
}

void TDiskRegistryState::AddErrorNotification(TDiskRegistryDatabase& db, TString diskId)
{
    db.AddErrorNotification(diskId);

    ErrorNotifications.emplace(std::move(diskId));
}

void TDiskRegistryState::DeleteErrorNotification(
    TDiskRegistryDatabase& db,
    const TString& diskId)
{
    ErrorNotifications.erase(diskId);
    db.DeleteErrorNotification(diskId);
}

NProto::TDiskConfig TDiskRegistryState::BuildDiskConfig(
    TDiskId diskId,
    const TDiskState& diskState) const
{
    NProto::TDiskConfig config;

    config.SetDiskId(std::move(diskId));
    config.SetBlockSize(diskState.LogicalBlockSize);
    config.SetState(diskState.State);
    config.SetStateTs(diskState.StateTs.MicroSeconds());
    config.SetCloudId(diskState.CloudId);
    config.SetFolderId(diskState.FolderId);
    config.SetUserId(diskState.UserId);
    config.SetReplicaCount(diskState.ReplicaCount);
    config.SetMasterDiskId(diskState.MasterDiskId);

    for (const auto& [uuid, seqNo]: diskState.FinishedMigrations) {
        Y_UNUSED(seqNo);
        auto& m = *config.AddFinishedMigrations();
        m.SetDeviceId(uuid);
    }

    for (const auto& id: diskState.DeviceReplacementIds) {
        *config.AddDeviceReplacementUUIDs() = id;
    }

    for (const auto& uuid: diskState.Devices) {
        *config.AddDeviceUUIDs() = uuid;
    }

    for (const auto& [targetId, sourceId]: diskState.MigrationTarget2Source) {
        auto& m = *config.AddMigrations();
        m.SetSourceDeviceId(sourceId);
        m.MutableTargetDevice()->SetDeviceUUID(targetId);
    }

    return config;
}

void TDiskRegistryState::DeleteDiskStateChanges(
    TDiskRegistryDatabase& db,
    const TString& diskId,
    ui64 seqNo)
{
    db.DeleteDiskStateChanges(diskId, seqNo);
}

NProto::TError TDiskRegistryState::CheckAgentStateTransition(
    const TString& agentId,
    NProto::EAgentState newState,
    TInstant timestamp) const
{
    const auto* agent = AgentList.FindAgent(agentId);

    if (!agent) {
        return MakeError(E_NOT_FOUND, "agent not found");
    }

    if (agent->GetState() == newState) {
        return MakeError(S_ALREADY);
    }

    if (agent->GetStateTs() > timestamp.MicroSeconds()) {
        return MakeError(E_INVALID_STATE, "out of order");
    }

    return {};
}

NProto::TError TDiskRegistryState::UpdateAgentState(
    TDiskRegistryDatabase& db,
    TString agentId,
    NProto::EAgentState newState,
    TInstant timestamp,
    TString reason,
    TVector<TDiskStateUpdate>& affectedDisks)
{
    auto error = CheckAgentStateTransition(agentId, newState, timestamp);
    if (FAILED(error.GetCode())) {
        return error;
    }

    auto* agent = AgentList.FindAgent(agentId);
    if (!agent) {
        auto message = ReportDiskRegistryAgentNotFound(
            TStringBuilder() << "UpdateAgentState:AgentId: " << agentId);

        return MakeError(E_FAIL, agentId);
    }

    const auto cmsTs = TInstant::MicroSeconds(agent->GetCmsTs());
    const auto oldState = agent->GetState();
    const auto cmsDeadline = cmsTs + GetInfraTimeout(*StorageConfig, oldState);
    const auto cmsRequestActive = cmsTs && cmsDeadline > timestamp;

    if (!cmsRequestActive) {
        agent->SetCmsTs(0);
    }

    // when newState is less than AGENT_STATE_WARNING
    // check if agent is not scheduled for shutdown by cms
    if ((newState < NProto::EAgentState::AGENT_STATE_WARNING) && cmsRequestActive) {
        newState = oldState;
    }

    agent->SetState(newState);
    agent->SetStateTs(timestamp.MicroSeconds());
    agent->SetStateMessage(std::move(reason));

    ApplyAgentStateChange(db, *agent, timestamp, affectedDisks);

    return error;
}

void TDiskRegistryState::ApplyAgentStateChange(
    TDiskRegistryDatabase& db,
    const NProto::TAgentConfig& agent,
    TInstant timestamp,
    TVector<TDiskStateUpdate>& affectedDisks)
{
    UpdateAgent(db, agent);
    DeviceList.UpdateDevices(agent);

    THashSet<TString> diskIds;

    for (const auto& d: agent.GetDevices()) {
        const auto& deviceId = d.GetDeviceUUID();
        auto diskId = DeviceList.FindDiskId(deviceId);

        if (diskId.empty()) {
            continue;
        }

        auto& disk = Disks[diskId];

        // check if deviceId is target for migration
        if (RestartDeviceMigration(db, diskId, disk, deviceId)) {
            continue;
        }

        if (agent.GetState() == NProto::AGENT_STATE_WARNING) {
            if (disk.MigrationSource2Target.contains(deviceId)) {
                // migration already started
                continue;
            }

            AddMigration(disk, diskId, deviceId);
        } else {
            if (agent.GetState() == NProto::AGENT_STATE_UNAVAILABLE
                    && disk.MasterDiskId)
            {
                const bool canReplaceDevice = ReplicaTable.IsReplacementAllowed(
                    disk.MasterDiskId,
                    deviceId);

                if (canReplaceDevice) {
                    TMaybe<TDiskStateUpdate> update;
                    ReplaceDevice(db, diskId, deviceId, timestamp, &update);
                } else {
                    ReportMirroredDiskDeviceReplacementForbidden();
                }
            }

            CancelDeviceMigration(db, diskId, disk, deviceId);
        }

        diskIds.emplace(std::move(diskId));
    }

    for (auto& id: diskIds) {
        TMaybe<TDiskStateUpdate> update = TryUpdateDiskState(db, id, timestamp);
        if (update) {
            affectedDisks.push_back(std::move(*update));
        }
    }
}

bool TDiskRegistryState::HasDependentDisks(const NProto::TAgentConfig& agent) const
{
    for (const auto& d: agent.GetDevices()) {
        if (d.GetState() >= NProto::DEVICE_STATE_ERROR) {
            continue;
        }

        const auto diskId = FindDisk(d.GetDeviceUUID());

        if (!diskId) {
            continue;
        }

        const auto* disk = Disks.FindPtr(diskId);
        if (!disk) {
            ReportDiskRegistryDiskNotFound(
                TStringBuilder() << "HasDependentDisks:DiskId: " << diskId);
            continue;
        }

        if (!disk->MasterDiskId) {
            return true;
        }

        const bool canReplaceDevice = ReplicaTable.IsReplacementAllowed(
            disk->MasterDiskId,
            d.GetDeviceUUID());

        // mirrored disk replicas should not delay host/device maintenance
        // unless it is the last available replica
        if (!canReplaceDevice) {
            return true;
        }
    }

    return false;
}

NProto::TError TDiskRegistryState::UpdateCmsHostState(
    TDiskRegistryDatabase& db,
    TString agentId,
    NProto::EAgentState newState,
    TInstant now,
    bool dryRun,
    TVector<TDiskStateUpdate>& affectedDisks,
    TDuration& timeout)
{
    auto error = CheckAgentStateTransition(agentId, newState, now);
    if (FAILED(error.GetCode())) {
        return error;
    }

    auto* agent = AgentList.FindAgent(agentId);
    if (!agent) {
        auto message = ReportDiskRegistryAgentNotFound(
            TStringBuilder() << "UpdateCmsHostState:AgentId: " << agentId);

        return MakeError(E_FAIL, agentId);
    }

    TInstant cmsTs = TInstant::MicroSeconds(agent->GetCmsTs());
    if (cmsTs == TInstant::Zero()) {
        cmsTs = now;
    }

    const auto infraTimeout = GetInfraTimeout(*StorageConfig, agent->GetState());

    if (cmsTs + infraTimeout <= now
            && agent->GetState() < NProto::AGENT_STATE_UNAVAILABLE)
    {
        // restart timer
        cmsTs = now;
    }

    timeout = cmsTs + infraTimeout - now;

    const bool hasDependentDisks = HasDependentDisks(*agent);
    if (!hasDependentDisks) {
        // no dependent disks => we can return this host immediately
        timeout = TDuration::Zero();
    }

    NProto::TError result = MakeError(E_TRY_AGAIN);
    if (timeout == TDuration::Zero()) {
        result = MakeError(S_OK);
        cmsTs = TInstant::Zero();
    }

    if (dryRun) {
        return result;
    }

    if (agent->GetState() <= newState) {
        agent->SetState(newState);
        agent->SetStateMessage("cms action");
    }

    agent->SetStateTs(now.MicroSeconds());
    agent->SetCmsTs(cmsTs.MicroSeconds());

    ApplyAgentStateChange(db, *agent, now, affectedDisks);

    return result;
}

TMaybe<NProto::EAgentState> TDiskRegistryState::GetAgentState(
    const TString& agentId) const
{
    const auto* agent = AgentList.FindAgent(agentId);
    if (agent) {
        return agent->GetState();
    }

    return {};
}

TMaybe<TInstant> TDiskRegistryState::GetAgentCmsTs(
    const TString& agentId) const
{
    const auto* agent = AgentList.FindAgent(agentId);
    if (agent) {
        return TInstant::MicroSeconds(agent->GetCmsTs());
    }

    return {};
}

TMaybe<TDiskStateUpdate> TDiskRegistryState::TryUpdateDiskState(
    TDiskRegistryDatabase& db,
    const TString& diskId,
    TInstant timestamp)
{
    auto* d = Disks.FindPtr(diskId);

    if (!d) {
        ReportDiskRegistryDiskNotFound(TStringBuilder()
            << "TryUpdateDiskState:DiskId: " << diskId);

        return {};
    }

    return TryUpdateDiskState(
        db,
        diskId,
        *d,
        timestamp);
}

TMaybe<TDiskStateUpdate> TDiskRegistryState::TryUpdateDiskState(
    TDiskRegistryDatabase& db,
    const TString& diskId,
    TDiskState& disk,
    TInstant timestamp)
{
    const auto newState = CalculateDiskState(disk);
    const auto oldState = disk.State;
    if (oldState == newState) {
        return {};
    }

    disk.State = newState;
    disk.StateTs = timestamp;

    NProto::TDiskState diskState;
    diskState.SetDiskId(diskId);
    diskState.SetState(disk.State);

    if (newState == NProto::DISK_STATE_MIGRATION) {
        diskState.SetStateMessage(DISK_STATE_MIGRATION_MESSAGE);
    }

    const auto seqNo = DiskStateSeqNo++;

    db.UpdateDiskState(diskState, seqNo);
    db.WriteLastDiskStateSeqNo(DiskStateSeqNo);

    DiskStateUpdates.emplace_back(std::move(diskState), seqNo);

    UpdateAndNotifyDisk(db, diskId, disk);

    if (newState >= NProto::DISK_STATE_TEMPORARILY_UNAVAILABLE) {
        AddErrorNotification(db, diskId);
    }

    return DiskStateUpdates.back();
}

void TDiskRegistryState::DeleteDiskStateUpdate(
    TDiskRegistryDatabase& db,
    ui64 maxSeqNo)
{
    auto begin = DiskStateUpdates.cbegin();
    auto it = begin;

    for (; it != DiskStateUpdates.cend(); ++it) {
        if (it->SeqNo > maxSeqNo) {
            break;
        }

        db.DeleteDiskStateChanges(it->State.GetDiskId(), it->SeqNo);
    }

    DiskStateUpdates.erase(begin, it);
}

NProto::EDiskState TDiskRegistryState::CalculateDiskState(
    const TDiskState& disk) const
{
    NProto::EDiskState state = NProto::DISK_STATE_ONLINE;

    for (const auto& uuid: disk.Devices) {
        const auto* device = DeviceList.FindDevice(uuid);

        if (!device) {
            return NProto::DISK_STATE_ERROR;
        }

        const auto* agent = AgentList.FindAgent(device->GetAgentId());
        if (!agent) {
            return NProto::DISK_STATE_ERROR;
        }

        state = std::max(state, ToDiskState(agent->GetState()));
        state = std::max(state, ToDiskState(device->GetState()));

        if (disk.MasterDiskId && state == NProto::DISK_STATE_MIGRATION) {
            // mirrored disk replicas don't need migration
            state = NProto::DISK_STATE_ONLINE;
        }

        if (state == NProto::DISK_STATE_ERROR) {
            break;
        }
    }

    return state;
}

auto TDiskRegistryState::FindDeviceLocation(const TDeviceId& deviceId) const
    -> std::pair<const NProto::TAgentConfig*, const NProto::TDeviceConfig*>
{
    return const_cast<TDiskRegistryState*>(this)->FindDeviceLocation(deviceId);
}

auto TDiskRegistryState::FindDeviceLocation(const TDeviceId& deviceId)
    -> std::pair<NProto::TAgentConfig*, NProto::TDeviceConfig*>
{
    const auto agentId = DeviceList.FindAgentId(deviceId);
    if (agentId.empty()) {
        return {};
    }

    auto* agent = AgentList.FindAgent(agentId);
    if (!agent) {
        return {};
    }

    auto* device = FindIfPtr(*agent->MutableDevices(), [&] (const auto& x) {
        return x.GetDeviceUUID() == deviceId;
    });

    if (!device) {
        return {};
    }

    return {agent, device};
}

NProto::TError TDiskRegistryState::UpdateDeviceState(
    TDiskRegistryDatabase& db,
    const TString& deviceId,
    NProto::EDeviceState newState,
    TInstant now,
    TString reason,
    TMaybe<TDiskStateUpdate>& affectedDisk)
{
    auto error = CheckDeviceStateTransition(deviceId, newState, now);
    if (FAILED(error.GetCode())) {
        return error;
    }

    auto [agentPtr, devicePtr] = FindDeviceLocation(deviceId);
    if (!agentPtr || !devicePtr) {
        auto message = ReportDiskRegistryDeviceLocationNotFound(
            TStringBuilder() << "UpdateDeviceState:DeviceId: " << deviceId
            << ", agentPtr?: " << !!agentPtr
            << ", devicePtr?: " << !!devicePtr);

        return MakeError(E_FAIL, message);
    }

    const auto cmsTs = TInstant::MicroSeconds(devicePtr->GetCmsTs());
    const auto cmsDeadline = cmsTs + StorageConfig->GetNonReplicatedInfraTimeout();
    const auto cmsRequestActive = cmsTs && cmsDeadline > now;
    const auto oldState = devicePtr->GetState();

    if (!cmsRequestActive) {
        devicePtr->SetCmsTs(0);
    }

    // when newState is less than DEVICE_STATE_WARNING
    // check if device is not scheduled for shutdown by cms
    if (newState < NProto::DEVICE_STATE_WARNING && cmsRequestActive) {
        newState = oldState;
    }

    devicePtr->SetState(newState);
    devicePtr->SetStateTs(now.MicroSeconds());
    devicePtr->SetStateMessage(std::move(reason));

    ApplyDeviceStateChange(db, *agentPtr, *devicePtr, now, affectedDisk);

    return error;
}

NProto::TError TDiskRegistryState::UpdateCmsDeviceState(
    TDiskRegistryDatabase& db,
    const TString& deviceId,
    NProto::EDeviceState newState,
    TInstant now,
    bool dryRun,
    TMaybe<TDiskStateUpdate>& affectedDisk,
    TDuration& timeout)
{
    auto error = CheckDeviceStateTransition(deviceId, newState, now);
    if (FAILED(error.GetCode())) {
        return error;
    }

    auto [agentPtr, devicePtr] = FindDeviceLocation(deviceId);
    if (!agentPtr || !devicePtr) {
        auto message = ReportDiskRegistryDeviceLocationNotFound(
            TStringBuilder() << "UpdateCmsDeviceState:DeviceId: " << deviceId
            << ", agentPtr?: " << !!agentPtr
            << ", devicePtr?: " << !!devicePtr);

        return MakeError(E_FAIL, message);
    }

    NProto::TError result = MakeError(S_OK);
    TInstant cmsTs;
    timeout = TDuration::Zero();

    const bool hasDependentDisk = devicePtr->GetState() < NProto::DEVICE_STATE_ERROR
        && FindDisk(devicePtr->GetDeviceUUID());

    if (hasDependentDisk) {
        result = MakeError(E_TRY_AGAIN);

        cmsTs = TInstant::MicroSeconds(devicePtr->GetCmsTs());
        if (cmsTs == TInstant::Zero()
                || cmsTs + StorageConfig->GetNonReplicatedInfraTimeout() <= now)
        {
            // restart timer
            cmsTs = now;
        }

        timeout = cmsTs + StorageConfig->GetNonReplicatedInfraTimeout() - now;
    }

    if (dryRun) {
        return result;
    }

    if (devicePtr->GetState() <= newState) {
        devicePtr->SetState(newState);
        devicePtr->SetStateMessage("cms action");
    }
    devicePtr->SetStateTs(now.MicroSeconds());
    devicePtr->SetCmsTs(cmsTs.MicroSeconds());

    ApplyDeviceStateChange(db, *agentPtr, *devicePtr, now, affectedDisk);

    return result;
}

void TDiskRegistryState::ApplyDeviceStateChange(
    TDiskRegistryDatabase& db,
    const NProto::TAgentConfig& agent,
    const NProto::TDeviceConfig& device,
    TInstant now,
    TMaybe<TDiskStateUpdate>& affectedDisk)
{
    UpdateAgent(db, agent);
    DeviceList.UpdateDevices(agent);

    const auto& uuid = device.GetDeviceUUID();
    auto diskId = DeviceList.FindDiskId(uuid);

    if (diskId.empty()) {
        return;
    }

    auto* disk = Disks.FindPtr(diskId);

    if (!disk) {
        ReportDiskRegistryDiskNotFound(TStringBuilder()
            << "ApplyDeviceStateChange:DiskId: " << diskId);

        return;
    }

    // check if uuid is target for migration
    if (RestartDeviceMigration(db, diskId, *disk, uuid)) {
        return;
    }

    affectedDisk = TryUpdateDiskState(db, diskId, *disk, now);

    if (device.GetState() != NProto::DEVICE_STATE_WARNING) {
        CancelDeviceMigration(db, diskId, *disk, uuid);
        return;
    }

    if (!disk->MigrationSource2Target.contains(uuid)) {
        AddMigration(*disk, diskId, uuid);
    }
}

bool TDiskRegistryState::RestartDeviceMigration(
    TDiskRegistryDatabase& db,
    const TDiskId& diskId,
    TDiskState& disk,
    const TDeviceId& targetId)
{
    auto it = disk.MigrationTarget2Source.find(targetId);

    if (it == disk.MigrationTarget2Source.end()) {
        return false;
    }

    TDeviceId sourceId = it->second;

    CancelDeviceMigration(db, diskId, disk, sourceId);

    AddMigration(disk, diskId, sourceId);

    return true;
}

void TDiskRegistryState::DeleteDeviceMigration(const TDiskId& diskId)
{
    const TDeviceMigration key{diskId, TString()};

    Migrations.erase(
        Migrations.lower_bound(key),
        Migrations.upper_bound(key)
    );
}

void TDiskRegistryState::DeleteDeviceMigration(
    const TDiskId& diskId,
    const TDeviceId& sourceId)
{
    Migrations.erase({ diskId, sourceId });
}

void TDiskRegistryState::CancelDeviceMigration(
    TDiskRegistryDatabase& db,
    const TDiskId& diskId,
    TDiskState& disk,
    const TDeviceId& sourceId)
{
    Migrations.erase(TDeviceMigration(diskId, sourceId));

    auto it = disk.MigrationSource2Target.find(sourceId);

    if (it == disk.MigrationSource2Target.end()) {
        return;
    }

    const TDeviceId targetId = it->second;

    disk.MigrationTarget2Source.erase(targetId);
    disk.MigrationSource2Target.erase(it);
    --DeviceMigrationsInProgress;

    const ui64 seqNo = AddDiskToNotify(db, diskId);

    disk.FinishedMigrations.push_back({
        .DeviceId = targetId,
        .SeqNo = seqNo
    });

    db.UpdateDisk(BuildDiskConfig(diskId, disk));

    UpdatePlacementGroup(db, diskId, disk, "CancelDeviceMigration");
}

NProto::TError TDiskRegistryState::FinishDeviceMigration(
    TDiskRegistryDatabase& db,
    const TDiskId& diskId,
    const TDeviceId& sourceId,
    const TDeviceId& targetId,
    TInstant timestamp,
    TMaybe<TDiskStateUpdate>* affectedDisk)
{
    if (!Disks.contains(diskId)) {
        return MakeError(E_NOT_FOUND, TStringBuilder() <<
            "disk " << diskId.Quote() << " not found");
    }

    TDiskState& disk = Disks[diskId];

    auto devIt = Find(disk.Devices, sourceId);

    if (devIt == disk.Devices.end()) {
        return MakeError(E_INVALID_STATE, TStringBuilder() <<
            "device " << sourceId.Quote() << " not found");
    }

    if (auto it = disk.MigrationTarget2Source.find(targetId);
        it == disk.MigrationTarget2Source.end() || it->second != sourceId)
    {
        return MakeError(E_ARGUMENT, "invalid migration");
    } else {
        disk.MigrationTarget2Source.erase(it);
        disk.MigrationSource2Target.erase(sourceId);
        --DeviceMigrationsInProgress;
    }

    *devIt = targetId;

    const ui64 seqNo = AddDiskToNotify(db, diskId);
    disk.FinishedMigrations.push_back({
        .DeviceId = sourceId,
        .SeqNo = seqNo
    });

    *affectedDisk = TryUpdateDiskState(db, diskId, disk, timestamp);

    db.UpdateDisk(BuildDiskConfig(diskId, disk));

    UpdatePlacementGroup(db, diskId, disk, "FinishDeviceMigration");

    return {};
}

TVector<TDeviceMigration> TDiskRegistryState::BuildMigrationList() const
{
    size_t budget = StorageConfig->GetMaxNonReplicatedDeviceMigrationsInProgress();
    // can be true if we decrease the limit in our storage config while there
    // are more migrations in progress than our new limit
    if (budget <= DeviceMigrationsInProgress) {
        return {};
    }

    budget -= DeviceMigrationsInProgress;

    const size_t limit = std::min(Migrations.size(), budget);

    TVector<TDeviceMigration> result;
    result.reserve(limit);

    for (const auto& m: Migrations) {
        auto [agentPtr, devicePtr] = FindDeviceLocation(m.SourceDeviceId);
        if (!agentPtr || !devicePtr) {
            continue;
        }

        if (agentPtr->GetState() > NProto::AGENT_STATE_WARNING) {
            // skip migration from unavailable agents
            continue;
        }

        if (devicePtr->GetState() > NProto::DEVICE_STATE_WARNING) {
            // skip migration from broken devices
            continue;
        }

        result.push_back(m);

        if (result.size() == limit) {
            break;
        }
    }

    return result;
}

NProto::TError TDiskRegistryState::CheckDeviceStateTransition(
    const TString& deviceId,
    NProto::EDeviceState newState,
    TInstant timestamp)
{
    auto [agentPtr, devicePtr] = FindDeviceLocation(deviceId);
    if (!agentPtr || !devicePtr) {
        return MakeError(E_NOT_FOUND, TStringBuilder() <<
            "device " << deviceId.Quote() << " not found");
    }

    if (devicePtr->GetState() == newState) {
        return MakeError(S_ALREADY);
    }

    if (TInstant::MicroSeconds(devicePtr->GetStateTs()) > timestamp) {
        return MakeError(E_INVALID_STATE, "out of order");
    }

    return {};
}

TString TDiskRegistryState::GetAgentId(TNodeId nodeId) const
{
    const auto* agent = AgentList.FindAgent(nodeId);

    return agent
        ? agent->GetAgentId()
        : TString();
}

NProto::TDiskRegistryStateBackup TDiskRegistryState::BackupState() const
{
    static_assert(
        TTableCount<TDiskRegistrySchema::TTables>::value == 13,
        "not all fields are processed"
    );

    NProto::TDiskRegistryStateBackup backup;

    auto transform = [] (const auto& src, auto* dst, auto func) {
        dst->Reserve(src.size());
        std::transform(
            src.cbegin(),
            src.cend(),
            RepeatedFieldBackInserter(dst),
            func
        );
    };

    auto copy = [] (const auto& src, auto* dst) {
        dst->Reserve(src.size());
        std::copy(
            src.cbegin(),
            src.cend(),
            RepeatedFieldBackInserter(dst)
        );
    };

    transform(Disks, backup.MutableDisks(), [this] (const auto& kv) {
        return BuildDiskConfig(kv.first, kv.second);
    });

    transform(PlacementGroups, backup.MutablePlacementGroups(), [] (const auto& kv) {
        return kv.second.Config;
    });

    transform(GetDirtyDevices(), backup.MutableDirtyDevices(), [] (auto& x) {
        return x.GetDeviceUUID();
    });

    transform(BrokenDisks, backup.MutableBrokenDisks(), [] (auto& x) {
        NProto::TDiskRegistryStateBackup::TBrokenDiskInfo info;
        info.SetDiskId(x.DiskId);
        info.SetTsToDestroy(x.TsToDestroy.MicroSeconds());

        return info;
    });

    transform(DisksToNotify, backup.MutableDisksToNotify(), [] (auto& kv) {
        return kv.first;
    });

    transform(DiskStateUpdates, backup.MutableDiskStateChanges(), [] (auto& x) {
        NProto::TDiskRegistryStateBackup::TDiskStateUpdate update;

        update.MutableState()->CopyFrom(x.State);
        update.SetSeqNo(x.SeqNo);

        return update;
    });

    copy(AgentList.GetAgents(), backup.MutableAgents());
    copy(DisksToCleanup, backup.MutableDisksToCleanup());
    copy(ErrorNotifications, backup.MutableErrorNotifications());

    transform(OutdatedVolumeConfigs, backup.MutableOutdatedVolumeConfigs(), [] (auto& kv) {
        return kv.first;
    });

    copy(GetSuspendedDevices(), backup.MutableSuspendedDevices());

    auto config = GetConfig();
    config.SetLastDiskStateSeqNo(DiskStateSeqNo);
    config.SetDiskAllocationAllowed(DiskAllocationAllowed);

    backup.MutableConfig()->Swap(&config);

    return backup;
}

NProto::TError TDiskRegistryState::AllowDiskAllocation(
    TDiskRegistryDatabase& db,
    bool allow)
{
    db.WriteDiskAllocationAllowed(allow);

    DiskAllocationAllowed = allow;

    return {};
}

bool TDiskRegistryState::IsDiskAllocationAllowed() const
{
    return DiskAllocationAllowed;
}

bool TDiskRegistryState::IsReadyForCleanup(const TDiskId& diskId) const
{
    return DisksToCleanup.contains(diskId);
}

std::pair<TVolumeConfig, ui64> TDiskRegistryState::GetVolumeConfigUpdate(
    const TDiskId& diskId) const
{
    std::pair<TVolumeConfig, ui64> result;
    auto& [config, seqNo] = result;

    auto update = OutdatedVolumeConfigs.find(diskId);
    auto disk = Disks.find(diskId);

    if (update != OutdatedVolumeConfigs.end() && disk != Disks.end()) {
        config.SetDiskId(diskId);
        config.SetPlacementGroupId(disk->second.PlacementGroupId);
        seqNo = update->second;
    }

    return result;
}

TVector<TDiskRegistryState::TDiskId> TDiskRegistryState::GetOutdatedVolumeConfigs() const
{
    TVector<TDiskId> diskIds;

    for (auto& kv: OutdatedVolumeConfigs) {
        diskIds.emplace_back(kv.first);
    }

    return diskIds;
}

void TDiskRegistryState::DeleteOutdatedVolumeConfig(
    TDiskRegistryDatabase& db,
    const TDiskId& diskId)
{
    db.DeleteOutdatedVolumeConfig(diskId);
    OutdatedVolumeConfigs.erase(diskId);
}

NProto::TError TDiskRegistryState::SetUserId(
    TDiskRegistryDatabase& db,
    const TDiskId& diskId,
    const TString& userId)
{
    auto it = Disks.find(diskId);
    if (it == Disks.end()) {
        return MakeError(E_NOT_FOUND, TStringBuilder() <<
            "disk " << diskId.Quote() << " not found");
    }

    auto& disk = it->second;
    disk.UserId = userId;

    db.UpdateDisk(BuildDiskConfig(diskId, disk));

    return {};
}

bool TDiskRegistryState::DoesNewDiskBlockSizeBreakDevice(
    const TDiskId& diskId,
    const TDeviceId& deviceId,
    ui64 newLogicalBlockSize)
{
    const auto& disk = Disks[diskId];
    const auto device = GetDevice(deviceId);

    const auto deviceSize = device.GetBlockSize() *
        GetDeviceBlockCountWithOverrides(diskId, device);

    const ui64 oldLogicalSize = deviceSize / disk.LogicalBlockSize
        * disk.LogicalBlockSize;
    const ui64 newLogicalSize = deviceSize / newLogicalBlockSize
        * newLogicalBlockSize;

    return oldLogicalSize != newLogicalSize;
}

NProto::TError TDiskRegistryState::ValidateUpdateDiskBlockSizeParams(
    const TDiskId& diskId,
    ui32 blockSize,
    bool force)
{
    if (diskId.empty()) {
        return MakeError(E_ARGUMENT, TStringBuilder() << "diskId is required");
    }
    if (blockSize == 0) {
        return MakeError(E_ARGUMENT, TStringBuilder()
            << "blockSize is required");
    }

    if (!Disks.contains(diskId)) {
        return MakeError(E_NOT_FOUND, TStringBuilder() <<
            "disk " << diskId.Quote() << " not found");
    }

    const auto& disk = Disks[diskId];

    if (blockSize == disk.LogicalBlockSize) {
        return MakeError(S_FALSE, TStringBuilder()
            << "disk " << diskId.Quote() << " already has block size equal to "
            << disk.LogicalBlockSize);
    }

    if (disk.Devices.empty()) {
        return MakeError(E_INVALID_STATE, "disk without devices");
    }

    if (force) {
        return {};
    }

    const auto forceNotice = "(use force flag to bypass this restriction)";

    ui64 devicesSize = 0;
    for (const auto& id: disk.Devices) {
        const auto device = GetDevice(id);
        if (device.GetDeviceUUID().empty()) {
            return MakeError(E_INVALID_STATE,
                TStringBuilder() << "one of the disk devices cannot be "
                "found " << forceNotice);
        }

        devicesSize += device.GetBlocksCount() * device.GetBlockSize();
    }

    const auto error = ValidateBlockSize(blockSize);

    if (HasError(error)) {
        return MakeError(error.GetCode(), TStringBuilder()
            << error.GetMessage() << " " << forceNotice);
    }

    const ui64 volumeSize = devicesSize / disk.LogicalBlockSize * blockSize;
    const ui64 allocationUnit = GetAllocationUnit(
        GetDevice(disk.Devices[0]).GetPoolName());

    if (volumeSize % allocationUnit != 0) {
        return MakeError(E_ARGUMENT, TStringBuilder()
            << "volume size should be divisible by " << allocationUnit);
    }

    for (const auto& deviceId: disk.Devices) {
        if (DoesNewDiskBlockSizeBreakDevice(diskId, deviceId, blockSize)) {
            return MakeError(E_ARGUMENT, TStringBuilder()
                << "Device " << deviceId.Quote()
                << " logical size " << disk.LogicalBlockSize
                << " is not equal to new logical size " << blockSize
                << ", that breaks disk " << forceNotice);
        }
    }

    return {};
}

NProto::TError TDiskRegistryState::UpdateDiskBlockSize(
    TDiskRegistryDatabase& db,
    const TDiskId& diskId,
    ui32 blockSize,
    bool force)
{
    const auto validateError = ValidateUpdateDiskBlockSizeParams(diskId,
        blockSize, force);
    if (HasError(validateError) || validateError.GetCode() == S_FALSE) {
        return validateError;
    }

    auto& disk = Disks[diskId];

    for (const auto& deviceId: disk.Devices) {
        if (DoesNewDiskBlockSizeBreakDevice(diskId, deviceId, blockSize)) {
            auto device = GetDevice(deviceId);

            const auto newBlocksCount = device.GetBlockSize()
                * GetDeviceBlockCountWithOverrides(diskId, device)
                / disk.LogicalBlockSize * disk.LogicalBlockSize
                / device.GetBlockSize();
            AdjustDeviceBlockCount(db, device, newBlocksCount);
        }
    }

    disk.LogicalBlockSize = blockSize;
    UpdateAndNotifyDisk(db, diskId, disk);

    return {};
}

auto TDiskRegistryState::QueryAvailableStorage(
    const TString& agentId,
    const TString& poolName,
    NProto::EDevicePoolKind poolKind) const
        -> TResultOrError<TVector<TAgentStorageInfo>>
{
    if (!poolName.empty()) {
        auto* poolConfig = DevicePoolConfigs.FindPtr(poolName);
        if (!poolConfig) {
            return TVector<TAgentStorageInfo> {};
        }

        if (poolConfig->GetKind() != poolKind) {
            SelfCounters.QueryAvailableStorageErrors.Increment(1);

            return MakeError(
                E_ARGUMENT,
                Sprintf(
                    "Unexpected device pool kind (actual: %d, expected: %d) "
                    "for the device pool %s.",
                    poolKind,
                    poolConfig->GetKind(),
                    poolName.Quote().c_str()
                )
            );
        }
    }

    auto* agent = AgentList.FindAgent(agentId);
    if (!agent) {
        SelfCounters.QueryAvailableStorageErrors.Increment(1);

        return MakeError(E_NOT_FOUND, TStringBuilder{} <<
            "agent " << agentId.Quote() << " not found");
    }

    if (agent->GetState() != NProto::AGENT_STATE_ONLINE) {
        return TVector<TAgentStorageInfo> {};
    }

    THashMap<ui64, ui32> chunks;

    for (const auto& device: agent->GetDevices()) {
        if (device.GetPoolKind() != poolKind) {
            continue;
        }

        if (poolName && device.GetPoolName() != poolName) {
            continue;
        }

        if (device.GetState() != NProto::DEVICE_STATE_ONLINE) {
            continue;
        }

        if (DeviceList.IsSuspendedDevice(device.GetDeviceUUID())) {
            continue;
        }

        const ui64 size { device.GetBlockSize() * device.GetBlocksCount() };

        ++chunks[size];
    }

    TVector<TAgentStorageInfo> infos;
    infos.reserve(chunks.size());

    for (auto [size, count]: chunks) {
        infos.push_back({ size, count });
    }

    return infos;
}

NProto::TError TDiskRegistryState::AllocateDiskReplicas(
    TInstant now,
    TDiskRegistryDatabase& db,
    const TDiskId& masterDiskId,
    ui32 replicaCount)
{
    if (replicaCount == 0) {
        return MakeError(E_ARGUMENT, "replica count can't be zero");
    }

    auto* masterDisk = Disks.FindPtr(masterDiskId);
    if (!masterDisk) {
        return MakeError(E_NOT_FOUND, TStringBuilder()
            << "disk " << masterDiskId.Quote() << " is not found");
    }

    if (masterDisk->ReplicaCount == 0) {
        return MakeError(E_ARGUMENT, "unable to allocate disk replica for not "
            "a master disk");
    }

    const auto newReplicaCount = masterDisk->ReplicaCount + replicaCount;

    const auto maxReplicaCount =
        StorageConfig->GetMaxDisksInPlacementGroup() - 1;
    if (newReplicaCount > maxReplicaCount) {
        return MakeError(E_ARGUMENT, TStringBuilder()
            << "mirrored disks can have maximum of " << maxReplicaCount
            << " replicas, and you are asking for " << newReplicaCount);
    }

    const auto firstReplicaDiskId = GetReplicaDiskId(masterDiskId, 0);
    const auto& firstReplica = Disks[firstReplicaDiskId];

    ui64 blocksCount = 0;
    for (const auto& deviceId: firstReplica.Devices) {
        const auto device = GetDevice(deviceId);
        blocksCount += device.GetBlockSize() * device.GetBlocksCount()
            / firstReplica.LogicalBlockSize;
    }

    auto allocateParams = TDiskRegistryState::TAllocateDiskParams {
        .CloudId = firstReplica.CloudId,
        .FolderId = firstReplica.FolderId,
        .PlacementGroupId = firstReplica.PlacementGroupId,
        .BlockSize = firstReplica.LogicalBlockSize,
        .BlocksCount = blocksCount,
        .MasterDiskId = firstReplica.MasterDiskId,
    };

    TVector<TString> allocatedReplicaIds;

    for (size_t i = 0; i < replicaCount; ++i) {
        allocateParams.DiskId = GetReplicaDiskId(masterDiskId,
            masterDisk->ReplicaCount + i + 1);

        TAllocateDiskResult r;
        const auto error = AllocateDisk(now, db, allocateParams, &r);
        if (HasError(error)) {
            for (const auto& replicaId: allocatedReplicaIds) {
                DeallocateSimpleDisk(
                    db,
                    replicaId,
                    "AllocateDiskReplicas:Cleanup");
            }

            return error;
        }

        allocatedReplicaIds.push_back(allocateParams.DiskId);

        // TODO (https://st.yandex-team.ru/NBS-3418):
        // * add device ids for new replicas to device replacements
        // * update ReplicaTable
    }

    masterDisk->ReplicaCount = newReplicaCount;
    UpdateAndNotifyDisk(db, masterDiskId, *masterDisk);

    return {};
}

NProto::TError TDiskRegistryState::DeallocateDiskReplicas(
    TDiskRegistryDatabase& db,
    const TDiskId& masterDiskId,
    ui32 replicaCount)
{
    if (replicaCount == 0) {
        return MakeError(E_ARGUMENT, "replica count can't be zero");
    }

    auto* masterDisk = Disks.FindPtr(masterDiskId);
    if (!masterDisk) {
        return MakeError(E_NOT_FOUND, TStringBuilder()
            << "disk " << masterDiskId.Quote() << " is not found");
    }

    if (masterDisk->ReplicaCount == 0) {
        return MakeError(E_ARGUMENT, "unable to deallocate disk replica for "
            "not a master disk");
    }

    if (replicaCount > masterDisk->ReplicaCount) {
        return MakeError(E_ARGUMENT, TStringBuilder() << "deallocating "
            << replicaCount << "replicas is impossible, because disk "
            "only has " << masterDisk->ReplicaCount << " replicas");
    }
    if (replicaCount == masterDisk->ReplicaCount) {
        return MakeError(E_ARGUMENT, TStringBuilder() << "deallocating "
            << replicaCount << "replicas will make disk non mirrored, "
            "which is not supported yet");
    }

    const auto newReplicaCount = masterDisk->ReplicaCount - replicaCount;

    for (size_t i = masterDisk->ReplicaCount; i >= newReplicaCount + 1; --i) {
        const auto replicaDiskId = GetReplicaDiskId(masterDiskId, i);
        DeallocateSimpleDisk(db, replicaDiskId, "DeallocateDiskReplicas");

        // TODO (https://st.yandex-team.ru/NBS-3418): update ReplicaTable
    }

    masterDisk->ReplicaCount = newReplicaCount;
    UpdateAndNotifyDisk(db, masterDiskId, *masterDisk);

    return {};
}

NProto::TError TDiskRegistryState::UpdateDiskReplicaCount(
    TDiskRegistryDatabase& db,
    const TDiskId& masterDiskId,
    ui32 replicaCount)
{
    const auto validateError = ValidateUpdateDiskReplicaCountParams(
        masterDiskId, replicaCount);
    if (HasError(validateError) || validateError.GetCode() == S_FALSE) {
        return validateError;
    }

    const auto& masterDisk = Disks[masterDiskId];

    if (replicaCount > masterDisk.ReplicaCount) {
        return AllocateDiskReplicas(Now(), db, masterDiskId,
            replicaCount - masterDisk.ReplicaCount);
    } else {
        return DeallocateDiskReplicas(db, masterDiskId,
            masterDisk.ReplicaCount - replicaCount);
    }
}

NProto::TError TDiskRegistryState::ValidateUpdateDiskReplicaCountParams(
    const TDiskId& masterDiskId,
    ui32 replicaCount)
{
    if (masterDiskId.empty()) {
        return MakeError(E_ARGUMENT, "disk id is required");
    }
    if (replicaCount == 0) {
        return MakeError(E_ARGUMENT,
            "unable to turn mirrored disk to a simple one");
    }

    const auto& masterDisk = Disks.FindPtr(masterDiskId);
    if (!masterDisk) {
        return MakeError(E_NOT_FOUND, TStringBuilder()
            << "disk " << masterDiskId.Quote() << " is not found");
    }

    const auto onlyMasterNotice = "The method only works with master disks";
    if (!masterDisk->MasterDiskId.empty()) {
        return MakeError(E_ARGUMENT, TStringBuilder()
            << "disk " << masterDiskId.Quote() << " is not a master, it's "
            "a slave of the master disk " << masterDisk->MasterDiskId.Quote() << ". "
            << onlyMasterNotice);
    }
    if (masterDisk->ReplicaCount == 0) {
        return MakeError(E_ARGUMENT, TStringBuilder()
            << "disk " << masterDiskId.Quote() << " is not a master. "
            << onlyMasterNotice);
    }

    if (replicaCount == masterDisk->ReplicaCount) {
        return MakeError(S_FALSE, TStringBuilder()
            << "disk " << masterDiskId.Quote() << " already has replicaCount "
            "equal to " << masterDisk->ReplicaCount << ", no changes will be made");
    }

    return {};
}

NProto::TError TDiskRegistryState::MarkReplacementDevice(
    TDiskRegistryDatabase& db,
    const TDiskId& diskId,
    const TDeviceId& deviceId,
    bool isReplacement)
{
    auto* disk = Disks.FindPtr(diskId);

    if (!disk) {
        return MakeError(
            E_NOT_FOUND,
            TStringBuilder() << "Disk " << diskId << " not found");
    }

    auto it = Find(disk->DeviceReplacementIds, deviceId);

    if (isReplacement) {
        if (it != disk->DeviceReplacementIds.end()) {
            return MakeError(
                S_ALREADY,
                TStringBuilder() << "Device " << deviceId
                    << " already in replacement list for disk " << diskId);
        }

        disk->DeviceReplacementIds.push_back(deviceId);
    } else {
        if (it == disk->DeviceReplacementIds.end()) {
            return MakeError(
                S_ALREADY,
                TStringBuilder() << "Device " << deviceId
                    << " not found in replacement list for disk " << diskId);
        }

        disk->DeviceReplacementIds.erase(it);
    }

    db.UpdateDisk(BuildDiskConfig(diskId, *disk));
    ReplicaTable.MarkReplacementDevice(diskId, deviceId, isReplacement);

    return {};
}

void TDiskRegistryState::UpdateAgent(
    TDiskRegistryDatabase& db,
    const NProto::TAgentConfig& config)
{
    if (config.GetNodeId() != 0) {
        db.UpdateOldAgent(config);
    }

    db.UpdateAgent(config);
}

ui64 TDiskRegistryState::GetAllocationUnit(const TString& poolName) const
{
    const auto* config = DevicePoolConfigs.FindPtr(poolName);
    if (!config) {
        return 0;
    }

    return config->GetAllocationUnit();
}

NProto::EDevicePoolKind TDiskRegistryState::GetDevicePoolKind(
    const TString& poolName) const
{
    const auto* config = DevicePoolConfigs.FindPtr(poolName);
    if (!config) {
        return NProto::DEVICE_POOL_KIND_DEFAULT;
    }

    return config->GetKind();
}

NProto::TError TDiskRegistryState::SuspendDevice(
    TDiskRegistryDatabase& db,
    const TDeviceId& id)
{
    if (id.empty()) {
        return MakeError(E_ARGUMENT, "empty device id");
    }

    DeviceList.SuspendDevice(id);
    db.UpdateSuspendedDevice(id);

    return {};
}

NProto::TError TDiskRegistryState::ResumeDevice(
    TDiskRegistryDatabase& db,
    const TDeviceId& id)
{
    if (id.empty()) {
        return MakeError(E_ARGUMENT, "empty device id");
    }

    DeviceList.ResumeDevice(id);
    db.DeleteSuspendedDevice(id);

    if (!DeviceList.IsDirtyDevice(id)) {
        TryUpdateDevice(db, id);
    }

    return {};
}

bool TDiskRegistryState::IsSuspendedDevice(const TDeviceId& id) const
{
    return DeviceList.IsSuspendedDevice(id);
}

auto TDiskRegistryState::GetSuspendedDevices() const -> TVector<TDeviceId>
{
    return DeviceList.GetSuspendedDevices();
}

NProto::TError TDiskRegistryState::CreateDiskFromDevices(
    TDiskRegistryDatabase& db,
    bool force,
    const TDiskId& diskId,
    ui32 blockSize,
    const TVector<NProto::TDeviceConfig>& devices)
{
    if (devices.empty()) {
        return MakeError(E_ARGUMENT, "empty device list");
    }

    TVector<TString> deviceIds;
    int poolKind = -1;

    for (auto& device: devices) {
        const auto& agentId = device.GetAgentId();
        const auto& name = device.GetDeviceName();

        auto* config = FindDevice(agentId, name);
        if (!config) {
            return MakeError(E_ARGUMENT, TStringBuilder() <<
                "device " << agentId << ":" << name << " not found");
        }

        const auto& uuid = config->GetDeviceUUID();

        if (poolKind == -1) {
            poolKind = static_cast<int>(config->GetPoolKind());
        }

        if (static_cast<int>(config->GetPoolKind()) != poolKind) {
            return MakeError(E_ARGUMENT, TStringBuilder() <<
                "several device pool kinds for one disk: " <<
                static_cast<int>(config->GetPoolKind()) << " and " <<
                poolKind);
        }

        if (!force
                && DeviceList.IsDirtyDevice(uuid)
                && !DeviceList.IsSuspendedDevice(uuid))
        {
            return MakeError(E_ARGUMENT, TStringBuilder() <<
                "device " << uuid.Quote() << " is dirty");
        }

        const auto otherDiskId = FindDisk(uuid);

        if (!otherDiskId.empty() && diskId != otherDiskId) {
            return MakeError(E_ARGUMENT, TStringBuilder() <<
                "device " << uuid.Quote() << " is allocated for disk "
                << otherDiskId.Quote());
        }

        deviceIds.push_back(std::move(uuid));
    }

    auto& disk = Disks[diskId];

    if (!disk.Devices.empty()) {
        if (disk.LogicalBlockSize == blockSize && disk.Devices == deviceIds) {
            return MakeError(S_ALREADY);
        }

        return MakeError(E_ARGUMENT, TStringBuilder() <<
            "disk " << diskId.Quote() << " already exists");
    }

    disk.Devices = deviceIds;
    disk.LogicalBlockSize = blockSize;

    for (auto& uuid: deviceIds) {
        DeviceList.MarkDeviceAllocated(diskId, uuid);
        DeviceList.MarkDeviceAsClean(uuid);
        db.DeleteDirtyDevice(uuid);

        DeviceList.ResumeDevice(uuid);
        db.DeleteSuspendedDevice(uuid);
    }

    db.UpdateDisk(BuildDiskConfig(diskId, disk));

    return {};
}

}   // namespace NCloud::NBlockStore::NStorage
