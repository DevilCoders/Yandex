#include "part_mirror_state.h"

#include "config.h"

#include <cloud/blockstore/libs/storage/core/config.h>

#include <util/string/builder.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

////////////////////////////////////////////////////////////////////////////////

TMirrorPartitionState::TMirrorPartitionState(
        TStorageConfigPtr config,
        TString rwClientId,
        TNonreplicatedPartitionConfigPtr partConfig,
        TVector<TDevices> replicaDevices)
    : Config(std::move(config))
    , RWClientId(std::move(rwClientId))
{
    ReplicaInfos.push_back({partConfig->Fork(partConfig->GetDevices()), {}});
    for (auto& devices: replicaDevices) {
        ReplicaInfos.push_back({partConfig->Fork(std::move(devices)), {}});
    }
}

NProto::TError TMirrorPartitionState::Validate()
{
    for (auto& replicaInfo: ReplicaInfos) {
        const auto mainDeviceCount =
            ReplicaInfos.front().Config->GetDevices().size();
        const auto replicaDeviceCount =
            replicaInfo.Config->GetDevices().size();

        if (mainDeviceCount != replicaDeviceCount) {
            return MakeError(E_INVALID_STATE, TStringBuilder()
                << "bad replica device count, main config: " << mainDeviceCount
                << ", replica: " << replicaDeviceCount);
        }
    }

    return {};
}

NProto::TError TMirrorPartitionState::PrepareMigrationConfig()
{
    if (MigrationConfigPrepared) {
        return {};
    }

    auto* replicaInfo = FindIfPtr(ReplicaInfos, [] (const auto& info) {
        return !info.Config->GetFreshDeviceIds().empty();
    });

    if (!replicaInfo) {
        return {};
    }

    const auto& freshDevices = replicaInfo->Config->GetFreshDeviceIds();
    auto& devices = replicaInfo->Config->AccessDevices();

    // initializing (copying data via migration) one device at a time
    int deviceIdx = 0;
    while (deviceIdx < devices.size()) {
        if (freshDevices.contains(devices[deviceIdx].GetDeviceUUID())) {
            break;
        }

        ++deviceIdx;
    }

    if (deviceIdx == devices.size()) {
        TStringBuilder message;
        message << "fresh devices not empty but no device is fresh, devices:";
        for (const auto& device: devices) {
            message << " " << device.GetDeviceUUID();
        }

        message << ", fresh:";
        for (const auto& fd: freshDevices) {
            message << " " << fd;
        }

        return MakeError(E_INVALID_STATE, message);
    }

    // we need to find corresponding good device from some other replica
    for (auto& anotherReplica: ReplicaInfos) {
        auto& anotherFreshDevices =
            anotherReplica.Config->GetFreshDeviceIds();
        auto& anotherDevices = anotherReplica.Config->AccessDevices();
        auto& anotherDevice = anotherDevices[deviceIdx];
        const auto& uuid = anotherDevice.GetDeviceUUID();

        if (!anotherFreshDevices.contains(uuid)) {
            // we found a good device, lets build our migration config
            auto targetDevice = devices[deviceIdx];
            devices[deviceIdx] = anotherDevice;
            auto& migration = *replicaInfo->Migrations.Add();
            migration.SetSourceDeviceId(uuid);
            *migration.MutableTargetDevice() = std::move(targetDevice);

            // we need to replace anotherDevice with a dummy device
            // since now our migration actor will be responsible for
            // write request replication to this device
            anotherDevice.SetDeviceUUID({});

            break;
        }
    }

    MigrationConfigPrepared = true;

    return {};
}

NProto::TError TMirrorPartitionState::NextReadReplica(
    const TBlockRange64 readRange,
    NActors::TActorId* actorId)
{
    ui32 replicaIndex = 0;
    for (ui32 i = 0; i < ReplicaActors.size(); ++i) {
        replicaIndex = ReadReplicaIndex++ % ReplicaActors.size();
        const auto& replicaInfo = ReplicaInfos[replicaIndex];
        if (replicaInfo.Config->DevicesReadyForReading(readRange)) {
            *actorId = ReplicaActors[replicaIndex];
            return {};
        }
    }

    return MakeError(E_INVALID_STATE, TStringBuilder() << "range "
        << DescribeRange(readRange) << " targets only fresh/dummy devices");
}

}   // namespace NCloud::NBlockStore::NStorage
