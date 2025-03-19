#include "disk_registry_database.h"

#include "disk_registry_schema.h"

namespace NCloud::NBlockStore::NStorage {

using namespace NKikimr;

////////////////////////////////////////////////////////////////////////////////

enum EDiskRegistryConfigKey
{
    DISK_REGISTRY_CONFIG = 1,
    LAST_DISK_STATE_SEQ_NO = 2,
    DISK_ALLOCATION_ALLOWED = 3,
};

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryDatabase::InitSchema()
{
    Materialize<TDiskRegistrySchema>();

    TSchemaInitializer<TDiskRegistrySchema::TTables>::InitStorage(
        Database.Alter());
}

template <typename TTable>
bool TDiskRegistryDatabase::LoadConfigs(
    TVector<typename TTable::Config::Type>& configs)
{
    auto it = Table<TTable>()
        .Range()
        .template Select<typename TTable::TColumns>();

    if (!it.IsReady()) {
        return false;   // not ready
    }

    while (it.IsValid()) {
        configs.emplace_back(it.template GetValue<typename TTable::Config>());

        if (!it.Next()) {
            return false;   // not ready
        }
    }

    return true;
}

bool TDiskRegistryDatabase::ReadDirtyDevices(TVector<TString>& dirtyDevices)
{
    using TTable = TDiskRegistrySchema::DirtyDevices;

    auto it = Table<TTable>()
        .Range()
        .Select<TTable::TColumns>();

    if (!it.IsReady()) {
        return false;   // not ready
    }

    while (it.IsValid()) {
        dirtyDevices.emplace_back(it.GetValue<TTable::Id>());

        if (!it.Next()) {
            return false;   // not ready
        }
    }

    return true;
}

bool TDiskRegistryDatabase::ReadOldAgents(TVector<NProto::TAgentConfig>& agents)
{
    return LoadConfigs<TDiskRegistrySchema::Agents>(agents);
}

bool TDiskRegistryDatabase::ReadAgents(TVector<NProto::TAgentConfig>& agents)
{
    return LoadConfigs<TDiskRegistrySchema::AgentById>(agents);
}

bool TDiskRegistryDatabase::ReadDisks(TVector<NProto::TDiskConfig>& disks)
{
    return LoadConfigs<TDiskRegistrySchema::Disks>(disks);
}

bool TDiskRegistryDatabase::ReadPlacementGroups(TVector<NProto::TPlacementGroupConfig>& groups)
{
    return LoadConfigs<TDiskRegistrySchema::PlacementGroups>(groups);
}

bool TDiskRegistryDatabase::ReadDiskRegistryConfig(NProto::TDiskRegistryConfig& config)
{
    using TTable = TDiskRegistrySchema::DiskRegistryConfig;

    auto it = Table<TTable>()
        .Key(DISK_REGISTRY_CONFIG)
        .Select();

    if (!it.IsReady()) {
        return false;   // not ready
    }

    if (it.IsValid()) {
        config = it.GetValue<TTable::Config>();
    }

    return true;
}

bool TDiskRegistryDatabase::ReadLastDiskStateSeqNo(ui64& lastSeqNo)
{
    using TTable = TDiskRegistrySchema::DiskRegistryConfig;

    auto it = Table<TTable>()
        .Key(LAST_DISK_STATE_SEQ_NO)
        .Select();

    if (!it.IsReady()) {
        return false;   // not ready
    }

    lastSeqNo = it.IsValid()
        ? it.GetValue<TTable::Config>().GetLastDiskStateSeqNo()
        : 0;

    return true;
}

void TDiskRegistryDatabase::WriteDiskRegistryConfig(
    const NProto::TDiskRegistryConfig& config)
{
    using TTable = TDiskRegistrySchema::DiskRegistryConfig;

    Table<TTable>()
        .Key(DISK_REGISTRY_CONFIG)
        .Update<TTable::Config>(config);
}

void TDiskRegistryDatabase::WriteLastDiskStateSeqNo(ui64 lastSeqNo)
{
    using TTable = TDiskRegistrySchema::DiskRegistryConfig;

    NProto::TDiskRegistryConfig config;
    config.SetLastDiskStateSeqNo(lastSeqNo);

    Table<TTable>()
        .Key(LAST_DISK_STATE_SEQ_NO)
        .Update<TTable::Config>(config);
}

void TDiskRegistryDatabase::UpdateDisk(const NProto::TDiskConfig& config)
{
    using TTable = TDiskRegistrySchema::Disks;
    Table<TTable>()
        .Key(config.GetDiskId())
        .Update<TTable::Config>(config);
}

void TDiskRegistryDatabase::DeleteDisk(const TString& diskId)
{
    using TTable = TDiskRegistrySchema::Disks;
    Table<TTable>()
        .Key(diskId)
        .Delete();
}

void TDiskRegistryDatabase::UpdateOldAgent(const NProto::TAgentConfig& config)
{
    Y_VERIFY_DEBUG(config.GetNodeId() != 0);

    using TTable = TDiskRegistrySchema::Agents;
    Table<TTable>()
        .Key(config.GetNodeId())
        .Update<TTable::Config>(config);
}

void TDiskRegistryDatabase::UpdateAgent(const NProto::TAgentConfig& config)
{
    using TTable = TDiskRegistrySchema::AgentById;
    Table<TTable>()
        .Key(config.GetAgentId())
        .Update<TTable::Config>(config);
}

void TDiskRegistryDatabase::DeleteOldAgent(ui32 nodeId)
{
    using TTable = TDiskRegistrySchema::Agents;
    Table<TTable>()
        .Key(nodeId)
        .Delete();
}

void TDiskRegistryDatabase::DeleteAgent(const TString& id)
{
    using TTable = TDiskRegistrySchema::AgentById;
    Table<TTable>()
        .Key(id)
        .Delete();
}

void TDiskRegistryDatabase::UpdateDirtyDevice(const TString& uuid)
{
    using TTable = TDiskRegistrySchema::DirtyDevices;

    Table<TTable>()
        .Key(uuid)
        .Update();
}

void TDiskRegistryDatabase::DeleteDirtyDevice(const TString& uuid)
{
    using TTable = TDiskRegistrySchema::DirtyDevices;

    Table<TTable>()
        .Key(uuid)
        .Delete();
}

void TDiskRegistryDatabase::UpdatePlacementGroup(const NProto::TPlacementGroupConfig& config)
{
    using TTable = TDiskRegistrySchema::PlacementGroups;
    Table<TTable>()
        .Key(config.GetGroupId())
        .Update<TTable::Config>(config);
}

void TDiskRegistryDatabase::DeletePlacementGroup(const TString& groupId)
{
    using TTable = TDiskRegistrySchema::PlacementGroups;
    Table<TTable>()
        .Key(groupId)
        .Delete();
}

void TDiskRegistryDatabase::UpdateDiskState(
    const NProto::TDiskState& state,
    ui64 seqNo)
{
    using TTable = TDiskRegistrySchema::DiskStateChanges;
    Table<TTable>()
        .Key(seqNo, state.GetDiskId())
        .Update<TTable::State>(state);
}

bool TDiskRegistryDatabase::ReadDiskStateChanges(
    TVector<TDiskStateUpdate>& changes)
{
    using TTable = TDiskRegistrySchema::DiskStateChanges;

    auto it = Table<TTable>()
        .Range()
        .template Select<typename TTable::TColumns>();

    if (!it.IsReady()) {
        return false;   // not ready
    }

    while (it.IsValid()) {
        changes.emplace_back(
            it.GetValue<TTable::State>(),
            it.GetValue<TTable::SeqNo>());

        if (!it.Next()) {
            return false;   // not ready
        }
    }

    return true;
}

void TDiskRegistryDatabase::DeleteDiskStateChanges(const TString& diskId, ui64 seqNo)
{
    using TTable = TDiskRegistrySchema::DiskStateChanges;
    Table<TTable>()
        .Key(seqNo, diskId)
        .Delete();
}

void TDiskRegistryDatabase::AddBrokenDisk(const TBrokenDiskInfo& diskInfo)
{
    using TTable = TDiskRegistrySchema::BrokenDisks;
    Table<TTable>()
        .Key(diskInfo.DiskId)
        .Update<TTable::TsToDestroy>(diskInfo.TsToDestroy.MicroSeconds());
}

bool TDiskRegistryDatabase::ReadBrokenDisks(TVector<TBrokenDiskInfo>& diskInfos)
{
    using TTable = TDiskRegistrySchema::BrokenDisks;

    auto it = Table<TTable>()
        .Range()
        .template Select<typename TTable::TColumns>();

    if (!it.IsReady()) {
        return false;   // not ready
    }

    while (it.IsValid()) {
        diskInfos.push_back({
            it.GetValue<TTable::Id>(),
            TInstant::MicroSeconds(it.GetValue<TTable::TsToDestroy>())
        });

        if (!it.Next()) {
            return false;   // not ready
        }
    }

    Sort(
        diskInfos.begin(),
        diskInfos.end(),
        [] (const TBrokenDiskInfo& l, const TBrokenDiskInfo& r) {
            return l.TsToDestroy < r.TsToDestroy;
        }
    );

    return true;
}

void TDiskRegistryDatabase::DeleteBrokenDisk(const TString& diskId)
{
    using TTable = TDiskRegistrySchema::BrokenDisks;
    Table<TTable>()
        .Key(diskId)
        .Delete();
}

void TDiskRegistryDatabase::AddDiskToNotify(const TString& diskId)
{
    using TTable = TDiskRegistrySchema::DisksToNotify;
    Table<TTable>()
        .Key(diskId)
        .Update();
}

bool TDiskRegistryDatabase::ReadDisksToNotify(TVector<TString>& diskIds)
{
    using TTable = TDiskRegistrySchema::DisksToNotify;

    auto it = Table<TTable>()
        .Range()
        .template Select<typename TTable::TColumns>();

    if (!it.IsReady()) {
        return false;   // not ready
    }

    while (it.IsValid()) {
        diskIds.push_back(it.GetValue<TTable::Id>());

        if (!it.Next()) {
            return false;   // not ready
        }
    }

    return true;
}

void TDiskRegistryDatabase::DeleteDiskToNotify(const TString& diskId)
{
    using TTable = TDiskRegistrySchema::DisksToNotify;
    Table<TTable>()
        .Key(diskId)
        .Delete();
}

void TDiskRegistryDatabase::AddDiskToCleanup(const TString& diskId)
{
    using TTable = TDiskRegistrySchema::DisksToCleanup;
    Table<TTable>()
        .Key(diskId)
        .Update();
}

void TDiskRegistryDatabase::DeleteDiskToCleanup(const TString& diskId)
{
    using TTable = TDiskRegistrySchema::DisksToCleanup;
    Table<TTable>()
        .Key(diskId)
        .Delete();
}

bool TDiskRegistryDatabase::ReadDisksToCleanup(TVector<TString>& diskIds)
{
    using TTable = TDiskRegistrySchema::DisksToCleanup;
    auto it = Table<TTable>()
        .Range()
        .template Select<typename TTable::TColumns>();

    if (!it.IsReady()) {
        return false;   // not ready
    }

    while (it.IsValid()) {
        diskIds.push_back(it.GetValue<TTable::Id>());

        if (!it.Next()) {
            return false;   // not ready
        }
    }

    return true;
}

bool TDiskRegistryDatabase::ReadDiskAllocationAllowed(bool& allowed)
{
    using TTable = TDiskRegistrySchema::DiskRegistryConfig;

    auto it = Table<TTable>()
        .Key(DISK_ALLOCATION_ALLOWED)
        .Select();

    if (!it.IsReady()) {
        return false;   // not ready
    }

    allowed = it.IsValid()
        ? it.GetValue<TTable::Config>().GetDiskAllocationAllowed()
        : false;

    return true;
}

void TDiskRegistryDatabase::WriteDiskAllocationAllowed(bool allowed)
{
    using TTable = TDiskRegistrySchema::DiskRegistryConfig;

    NProto::TDiskRegistryConfig config;
    config.SetDiskAllocationAllowed(allowed);

    Table<TTable>()
        .Key(DISK_ALLOCATION_ALLOWED)
        .Update<TTable::Config>(config);
}

void TDiskRegistryDatabase::AddErrorNotification(const TString& diskId)
{
    using TTable = TDiskRegistrySchema::ErrorNotifications;

    Table<TTable>()
        .Key(diskId)
        .Update();
}

void TDiskRegistryDatabase::DeleteErrorNotification(const TString& diskId)
{
    using TTable = TDiskRegistrySchema::ErrorNotifications;

    Table<TTable>()
        .Key(diskId)
        .Delete();
}

bool TDiskRegistryDatabase::ReadErrorNotifications(TVector<TString>& diskIds)
{
    using TTable = TDiskRegistrySchema::ErrorNotifications;

    auto it = Table<TTable>()
        .Range()
        .template Select<typename TTable::TColumns>();

    if (!it.IsReady()) {
        return false;   // not ready
    }

    while (it.IsValid()) {
        diskIds.push_back(it.GetValue<TTable::Id>());

        if (!it.Next()) {
            return false;   // not ready
        }
    }

    return true;
}

void TDiskRegistryDatabase::AddOutdatedVolumeConfig(const TString& diskId)
{
    using TTable = TDiskRegistrySchema::OutdatedVolumeConfigs;

    Table<TTable>()
        .Key(diskId)
        .Update();
}

void TDiskRegistryDatabase::DeleteOutdatedVolumeConfig(const TString& diskId)
{
    using TTable = TDiskRegistrySchema::OutdatedVolumeConfigs;

    Table<TTable>()
        .Key(diskId)
        .Delete();
}

bool TDiskRegistryDatabase::ReadOutdatedVolumeConfigs(TVector<TString>& diskIds)
{
    using TTable = TDiskRegistrySchema::OutdatedVolumeConfigs;

    auto it = Table<TTable>()
        .Range()
        .template Select<typename TTable::TColumns>();

    if (!it.IsReady()) {
        return false;   // not ready
    }

    while (it.IsValid()) {
        diskIds.push_back(it.GetValue<TTable::Id>());

        if (!it.Next()) {
            return false;   // not ready
        }
    }

    return true;
}

bool TDiskRegistryDatabase::ReadSuspendedDevices(TVector<TString>& suspendedDevices)
{
    using TTable = TDiskRegistrySchema::SuspendedDevices;

    auto it = Table<TTable>()
        .Range()
        .Select<TTable::TColumns>();

    if (!it.IsReady()) {
        return false;   // not ready
    }

    while (it.IsValid()) {
        suspendedDevices.emplace_back(it.GetValue<TTable::Id>());

        if (!it.Next()) {
            return false;   // not ready
        }
    }

    return true;
}

void TDiskRegistryDatabase::UpdateSuspendedDevice(const TString& uuid)
{
    using TTable = TDiskRegistrySchema::SuspendedDevices;

    Table<TTable>()
        .Key(uuid)
        .Update();
}

void TDiskRegistryDatabase::DeleteSuspendedDevice(const TString& uuid)
{
    using TTable = TDiskRegistrySchema::SuspendedDevices;

    Table<TTable>()
        .Key(uuid)
        .Delete();
}

}   // namespace NCloud::NBlockStore::NStorage
