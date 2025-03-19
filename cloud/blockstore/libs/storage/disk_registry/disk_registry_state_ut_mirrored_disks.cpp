#include "disk_registry_state.h"

#include "disk_registry_database.h"

#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/disk_registry/testlib/test_state.h>
#include <cloud/blockstore/libs/storage/testlib/test_executor.h>
#include <cloud/blockstore/libs/storage/testlib/ut_helpers.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/guid.h>
#include <util/generic/size_literals.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NDiskRegistryStateTest;

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TDiskRegistryStateMirroredDisksTest)
{
    Y_UNIT_TEST(ShouldAllocateDisks)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        auto agentConfig1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1"),
            Device("dev-3", "uuid-3", "rack-1"),
        });

        auto agentConfig2 = AgentConfig(2, {
            Device("dev-4", "uuid-4", "rack-2"),
            Device("dev-5", "uuid-5", "rack-2"),
            Device("dev-6", "uuid-6", "rack-2"),
        });

        auto agentConfig3 = AgentConfig(3, {
            Device("dev-7", "uuid-7", "rack-3"),
            Device("dev-8", "uuid-8", "rack-3"),
            Device("dev-9", "uuid-9", "rack-3"),
        });

        auto agentConfig4 = AgentConfig(4, {
            Device("dev-10", "uuid-10", "rack-4"),
            Device("dev-11", "uuid-11", "rack-4"),
            Device("dev-12", "uuid-12", "rack-4"),
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({
                agentConfig1,
                agentConfig2,
                agentConfig3,
                agentConfig4,
            })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            TVector<TVector<TDeviceConfig>> replicas;
            TVector<TString> deviceReplacementIds;
            auto error = AllocateMirroredDisk(
                db,
                state,
                "disk-1",
                {},
                10_GB,
                2,
                devices,
                replicas,
                deviceReplacementIds);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-1", devices[0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(2, replicas.size());
            UNIT_ASSERT_VALUES_EQUAL(1, replicas[0].size());
            UNIT_ASSERT_VALUES_EQUAL("dev-4", replicas[0][0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(1, replicas[1].size());
            UNIT_ASSERT_VALUES_EQUAL("dev-7", replicas[1][0].GetDeviceName());
            ASSERT_VECTORS_EQUAL(TVector<TString>{}, deviceReplacementIds);
        });

        auto* group = state.FindPlacementGroup("disk-1/g");
        UNIT_ASSERT(group);
        UNIT_ASSERT_VALUES_EQUAL("disk-1/g", group->GetGroupId());
        UNIT_ASSERT_VALUES_EQUAL(3, group->DisksSize());
        UNIT_ASSERT_VALUES_EQUAL("disk-1/0", group->GetDisks(0).GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL("disk-1/1", group->GetDisks(1).GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL("disk-1/2", group->GetDisks(2).GetDiskId());

        {
            TDiskInfo diskInfo;
            auto error = state.GetDiskInfo("disk-1", diskInfo);
            UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-1", diskInfo.Devices[0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(0, diskInfo.Migrations.size());
            UNIT_ASSERT_VALUES_EQUAL(2, diskInfo.Replicas.size());
            UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Replicas[0].size());
            UNIT_ASSERT_VALUES_EQUAL(
                "dev-4",
                diskInfo.Replicas[0][0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Replicas[1].size());
            UNIT_ASSERT_VALUES_EQUAL(
                "dev-7",
                diskInfo.Replicas[1][0].GetDeviceName());
        }

        {
            TDiskInfo diskInfo;
            auto error = state.StartAcquireDisk("disk-1", diskInfo);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-1", diskInfo.Devices[0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(0, diskInfo.Migrations.size());
            UNIT_ASSERT_VALUES_EQUAL(2, diskInfo.Replicas.size());
            UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Replicas[0].size());
            UNIT_ASSERT_VALUES_EQUAL(
                "dev-4",
                diskInfo.Replicas[0][0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Replicas[1].size());
            UNIT_ASSERT_VALUES_EQUAL(
                "dev-7",
                diskInfo.Replicas[1][0].GetDeviceName());
        }

        {
            TDiskInfo diskInfo;
            auto error = state.StartAcquireDisk("disk-1", diskInfo);
            UNIT_ASSERT(HasError(error));
            UNIT_ASSERT_VALUES_EQUAL(E_REJECTED, error.GetCode());
        }

        state.FinishAcquireDisk("disk-1");

        auto diskIds = state.GetDiskIds();
        UNIT_ASSERT_VALUES_EQUAL(4, diskIds.size());
        UNIT_ASSERT_VALUES_EQUAL("disk-1", diskIds[0]);
        UNIT_ASSERT_VALUES_EQUAL("disk-1/0", diskIds[1]);
        UNIT_ASSERT_VALUES_EQUAL("disk-1/1", diskIds[2]);
        UNIT_ASSERT_VALUES_EQUAL("disk-1/2", diskIds[3]);

        diskIds = state.GetMasterDiskIds();
        UNIT_ASSERT_VALUES_EQUAL(1, diskIds.size());
        UNIT_ASSERT_VALUES_EQUAL("disk-1", diskIds[0]);

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            state.DeallocateDisk(db, "disk-1");
        });

        group = state.FindPlacementGroup("disk-1/g");
        UNIT_ASSERT(!group);

        UNIT_ASSERT_VALUES_EQUAL(0, state.GetDiskIds().size());
        UNIT_ASSERT_VALUES_EQUAL(0, state.GetMasterDiskIds().size());
    }

    Y_UNIT_TEST(ShouldCleanupAfterAllocationFailure)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        auto agentConfig1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1"),
            Device("dev-3", "uuid-3", "rack-1"),
        });

        auto agentConfig2 = AgentConfig(2, {
            Device("dev-4", "uuid-4", "rack-2"),
            Device("dev-5", "uuid-5", "rack-2"),
            Device("dev-6", "uuid-6", "rack-2"),
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({
                agentConfig1,
                agentConfig2,
            })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            TVector<TVector<TDeviceConfig>> replicas;
            TVector<TString> deviceReplacementIds;
            auto error = AllocateMirroredDisk(
                db,
                state,
                "disk-1",
                {},
                30_GB,
                2,
                devices,
                replicas,
                deviceReplacementIds);
            UNIT_ASSERT_VALUES_EQUAL_C(
                E_BS_DISK_ALLOCATION_FAILED,
                error.GetCode(),
                error.GetMessage());
        });

        UNIT_ASSERT_VALUES_EQUAL(0, state.GetDiskIds().size());
        UNIT_ASSERT_VALUES_EQUAL(0, state.GetMasterDiskIds().size());

        auto* group = state.FindPlacementGroup("disk-1/g");
        UNIT_ASSERT(!group);

        {
            TDiskInfo diskInfo;
            auto error = state.GetDiskInfo("disk-1", diskInfo);
            UNIT_ASSERT_VALUES_EQUAL_C(
                E_NOT_FOUND,
                error.GetCode(),
                error.GetMessage());
        }

        UNIT_ASSERT_VALUES_EQUAL(1, state.GetBrokenDisks().size());
        UNIT_ASSERT_VALUES_EQUAL("disk-1", state.GetBrokenDisks()[0].DiskId);

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto dd = state.GetDirtyDevices();
            UNIT_ASSERT_VALUES_EQUAL(6, dd.size());
            for (const auto& device: dd) {
                const auto& uuid = device.GetDeviceUUID();
                UNIT_ASSERT(state.MarkDeviceAsClean(db, uuid));
            }
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            TVector<TVector<TDeviceConfig>> replicas;
            TVector<TString> deviceReplacementIds;
            auto error = AllocateMirroredDisk(
                db,
                state,
                "disk-2",
                {},
                30_GB,
                1,
                devices,
                replicas,
                deviceReplacementIds);
            UNIT_ASSERT_VALUES_EQUAL_C(
                S_OK,
                error.GetCode(),
                error.GetMessage());
            UNIT_ASSERT_VALUES_EQUAL(3, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-1", devices[0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-2", devices[1].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-3", devices[2].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(1, replicas.size());
            UNIT_ASSERT_VALUES_EQUAL(3, replicas[0].size());
            UNIT_ASSERT_VALUES_EQUAL("dev-4", replicas[0][0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-5", replicas[0][1].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-6", replicas[0][2].GetDeviceName());
            ASSERT_VECTORS_EQUAL(TVector<TString>{}, deviceReplacementIds);
        });

        group = state.FindPlacementGroup("disk-2/g");
        UNIT_ASSERT(group);
        UNIT_ASSERT_VALUES_EQUAL("disk-2/g", group->GetGroupId());
        UNIT_ASSERT_VALUES_EQUAL(2, group->DisksSize());
        UNIT_ASSERT_VALUES_EQUAL("disk-2/0", group->GetDisks(0).GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL("disk-2/1", group->GetDisks(1).GetDiskId());

        {
            TDiskInfo diskInfo;
            auto error = state.GetDiskInfo("disk-2", diskInfo);
            UNIT_ASSERT_VALUES_EQUAL(3, diskInfo.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-1", diskInfo.Devices[0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-2", diskInfo.Devices[1].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-3", diskInfo.Devices[2].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(0, diskInfo.Migrations.size());
            UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Replicas.size());
            UNIT_ASSERT_VALUES_EQUAL(3, diskInfo.Replicas[0].size());
            UNIT_ASSERT_VALUES_EQUAL(
                "dev-4",
                diskInfo.Replicas[0][0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(
                "dev-5",
                diskInfo.Replicas[0][1].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(
                "dev-6",
                diskInfo.Replicas[0][2].GetDeviceName());
        }

        auto diskIds = state.GetDiskIds();
        UNIT_ASSERT_VALUES_EQUAL(3, diskIds.size());
        UNIT_ASSERT_VALUES_EQUAL("disk-2", diskIds[0]);
        UNIT_ASSERT_VALUES_EQUAL("disk-2/0", diskIds[1]);
        UNIT_ASSERT_VALUES_EQUAL("disk-2/1", diskIds[2]);

        diskIds = state.GetMasterDiskIds();
        UNIT_ASSERT_VALUES_EQUAL(1, diskIds.size());
        UNIT_ASSERT_VALUES_EQUAL("disk-2", diskIds[0]);

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            state.DeallocateDisk(db, "disk-2");
        });

        group = state.FindPlacementGroup("disk-2/g");
        UNIT_ASSERT(!group);

        UNIT_ASSERT_VALUES_EQUAL(0, state.GetDiskIds().size());
        UNIT_ASSERT_VALUES_EQUAL(0, state.GetMasterDiskIds().size());
    }

    Y_UNIT_TEST(ShouldResizeDisk)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        auto agentConfig1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1"),
            Device("dev-3", "uuid-3", "rack-1"),
            Device("dev-4", "uuid-4", "rack-1"),
            Device("dev-5", "uuid-5", "rack-1"),
            Device("dev-6", "uuid-6", "rack-1"),
        });

        auto agentConfig2 = AgentConfig(2, {
            Device("dev-7", "uuid-7", "rack-2"),
            Device("dev-8", "uuid-8", "rack-2"),
            Device("dev-9", "uuid-9", "rack-2"),
        });

        auto agentConfig3 = AgentConfig(3, {
            Device("dev-10", "uuid-10", "rack-1"),
            Device("dev-11", "uuid-11", "rack-1"),
            Device("dev-12", "uuid-12", "rack-1"),
        });

        auto agentConfig4 = AgentConfig(4, {
            Device("dev-13", "uuid-13", "rack-3"),
            Device("dev-14", "uuid-14", "rack-3"),
            Device("dev-15", "uuid-15", "rack-3"),
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({
                agentConfig1,
                agentConfig2,
                agentConfig3,
                agentConfig4,
            })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            TVector<TVector<TDeviceConfig>> replicas;
            TVector<TString> deviceReplacementIds;
            auto error = AllocateMirroredDisk(
                db,
                state,
                "disk-1",
                {},
                30_GB,
                1,
                devices,
                replicas,
                deviceReplacementIds);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(3, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-1", devices[0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-2", devices[1].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-3", devices[2].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(1, replicas.size());
            UNIT_ASSERT_VALUES_EQUAL(3, replicas[0].size());
            UNIT_ASSERT_VALUES_EQUAL("dev-7", replicas[0][0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-8", replicas[0][1].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-9", replicas[0][2].GetDeviceName());
            ASSERT_VECTORS_EQUAL(TVector<TString>{}, deviceReplacementIds);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            TVector<TVector<TDeviceConfig>> replicas;
            TVector<TString> deviceReplacementIds;
            auto error = AllocateMirroredDisk(
                db,
                state,
                "disk-1",
                {},
                60_GB,
                1,
                devices,
                replicas,
                deviceReplacementIds);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(6, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-1", devices[0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-2", devices[1].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-3", devices[2].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-4", devices[3].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-5", devices[4].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-6", devices[5].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(1, replicas.size());
            UNIT_ASSERT_VALUES_EQUAL(6, replicas[0].size());
            UNIT_ASSERT_VALUES_EQUAL("dev-7", replicas[0][0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-8", replicas[0][1].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-9", replicas[0][2].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-13", replicas[0][3].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-14", replicas[0][4].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-15", replicas[0][5].GetDeviceName());
            ASSERT_VECTORS_EQUAL(TVector<TString>{}, deviceReplacementIds);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            state.DeallocateDisk(db, "disk-1");
        });
    }

    Y_UNIT_TEST(ShouldAllocateReplicas)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({
                AgentConfig(1, {
                    Device("dev-1", "uuid-1", "rack-1"),
                    Device("dev-2", "uuid-2", "rack-1"),
                    Device("dev-3", "uuid-3", "rack-1"),
                }),
                AgentConfig(2, {
                    Device("dev-4", "uuid-4", "rack-2"),
                    Device("dev-5", "uuid-5", "rack-2"),
                    Device("dev-6", "uuid-6", "rack-2"),
                }),
                AgentConfig(3, {
                    Device("dev-7", "uuid-7", "rack-3"),
                    Device("dev-8", "uuid-8", "rack-3"),
                    Device("dev-9", "uuid-9", "rack-3"),
                }),
                AgentConfig(4, {
                    Device("dev-10", "uuid-10", "rack-4"),
                    Device("dev-11", "uuid-11", "rack-4"),
                    Device("dev-12", "uuid-12", "rack-4"),
                }),
                AgentConfig(5, {
                    Device("dev-13", "uuid-13", "rack-5"),
                    Device("dev-14", "uuid-14", "rack-5"),
                    Device("dev-15", "uuid-15", "rack-5"),
                }),
            })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> d;
            TVector<TVector<TDeviceConfig>> addedReplicas;
            TVector<TString> deviceReplacementIds;
            auto error = AllocateMirroredDisk(db, state, "disk-1", {}, 20_GB, 1,
                d, addedReplicas, deviceReplacementIds);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL_C(2, addedReplicas[0].size(),
                "should allocate disk with 2 devices");

            const auto* group = state.FindPlacementGroup("disk-1/g");
            UNIT_ASSERT(group);
            UNIT_ASSERT_VALUES_EQUAL(2, group->DisksSize());

            error = state.AllocateDiskReplicas(Now(), db, "disk-1", 0);
            UNIT_ASSERT_EQUAL_C(E_ARGUMENT, error.GetCode(),
                "should not let to allocate zero replicas");

            error = state.AllocateDiskReplicas(Now(), db, "undefined", 1);
            UNIT_ASSERT_EQUAL_C(E_NOT_FOUND, error.GetCode(),
                "should not let to allocate replicas for undefined disk");

            TVector<TDeviceConfig> nd;
            error = AllocateDisk(db, state, "not-mirrored-disk", {}, 10_GB, nd);
            UNIT_ASSERT_SUCCESS(error);
            error = state.AllocateDiskReplicas(Now(), db, "not-mirrored-disk",
                1);
            UNIT_ASSERT_EQUAL_C(E_ARGUMENT, error.GetCode(),
                "should not let to allocate replicas for a not mirrored disk");

            UNIT_ASSERT_VALUES_EQUAL("disk-1/0",
                group->GetDisks(0).GetDiskId());
            error = state.AllocateDiskReplicas(Now(), db, "disk-1/0", 1);
            UNIT_ASSERT_EQUAL_C(E_ARGUMENT, error.GetCode(),
                "should not let to allocate replicas for a not master disk");

            error = state.AllocateDiskReplicas(Now(), db, "disk-1", 10);
            UNIT_ASSERT_EQUAL_C(E_ARGUMENT, error.GetCode(),
                "should not let to allocate a lot of replicas which is above "
                "the limit");

            error = state.AllocateDiskReplicas(Now(), db, "disk-1", 1);
            UNIT_ASSERT_EQUAL_C(S_OK, error.GetCode(),
                "should allocate one more replica");
            UNIT_ASSERT_VALUES_EQUAL_C(3, group->DisksSize(),
                "placement group size should be increased");
            TDiskInfo diskInfo;
            error = state.GetDiskInfo("disk-1", diskInfo);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL_C(2, diskInfo.Replicas.size(),
                "disk's replicaCount should be increased");

            const auto prototypeReplicaDevices = diskInfo.Replicas[0];
            const auto addedReplicaDevices = diskInfo.Replicas[1];
            UNIT_ASSERT_VALUES_EQUAL_C(2, prototypeReplicaDevices.size(),
                "should have 2 devices");
            UNIT_ASSERT_VALUES_EQUAL_C(prototypeReplicaDevices.size(),
                addedReplicaDevices.size(),
                "should allocate same amount of devices as prototype");
            for (size_t i = 0; i < prototypeReplicaDevices.size(); i++) {
                UNIT_ASSERT_VALUES_EQUAL_C(
                    prototypeReplicaDevices[i].GetBlockSize(),
                    addedReplicaDevices[i].GetBlockSize(),
                    "added replica device should have same block size as the "
                    "prototype replica");
                UNIT_ASSERT_VALUES_EQUAL_C(
                    prototypeReplicaDevices[i].GetBlocksCount(),
                    addedReplicaDevices[i].GetBlocksCount(),
                    "added replica device should have same blocks count as the "
                    "prototype replica");
            }
        });
    }

    Y_UNIT_TEST(ShouldDeallocateReplicas)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({
                AgentConfig(1, {
                    Device("dev-1", "uuid-1", "rack-1"),
                    Device("dev-2", "uuid-2", "rack-1"),
                    Device("dev-3", "uuid-3", "rack-1"),
                }),
                AgentConfig(2, {
                    Device("dev-4", "uuid-4", "rack-2"),
                    Device("dev-5", "uuid-5", "rack-2"),
                    Device("dev-6", "uuid-6", "rack-2"),
                }),
                AgentConfig(3, {
                    Device("dev-7", "uuid-7", "rack-3"),
                    Device("dev-8", "uuid-8", "rack-3"),
                    Device("dev-9", "uuid-9", "rack-3"),
                }),
                AgentConfig(4, {
                    Device("dev-10", "uuid-10", "rack-4"),
                    Device("dev-11", "uuid-11", "rack-4"),
                    Device("dev-12", "uuid-12", "rack-4"),
                }),
                AgentConfig(5, {
                    Device("dev-13", "uuid-13", "rack-5"),
                    Device("dev-14", "uuid-14", "rack-5"),
                    Device("dev-15", "uuid-15", "rack-5"),
                }),
            })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> d;
            TVector<TVector<TDeviceConfig>> m;
            TVector<TString> r;
            auto error = AllocateMirroredDisk(db, state, "disk-1", {}, 10_GB, 2,
                d, m, r);
            UNIT_ASSERT_SUCCESS(error);

            const auto* group = state.FindPlacementGroup("disk-1/g");
            UNIT_ASSERT(group);
            UNIT_ASSERT_VALUES_EQUAL(3, group->DisksSize());

            error = state.DeallocateDiskReplicas(db, "disk-1", 0);
            UNIT_ASSERT_EQUAL_C(E_ARGUMENT, error.GetCode(),
                "should not let to deallocate zero replicas");

            error = state.DeallocateDiskReplicas(db, "undefined", 1);
            UNIT_ASSERT_EQUAL_C(E_NOT_FOUND, error.GetCode(),
                "should not let to deallocate replicas for undefined disk");

            TVector<TDeviceConfig> nd;
            error = AllocateDisk(db, state, "not-mirrored-disk", {}, 10_GB, nd);
            UNIT_ASSERT_SUCCESS(error);
            error = state.DeallocateDiskReplicas(db, "not-mirrored-disk", 1);
            UNIT_ASSERT_EQUAL_C(E_ARGUMENT, error.GetCode(), "should not let "
                "to deallocate replicas for a not mirrored disk");

            UNIT_ASSERT_VALUES_EQUAL("disk-1/0",
                group->GetDisks(0).GetDiskId());
            error = state.DeallocateDiskReplicas(db, "disk-1/0", 1);
            UNIT_ASSERT_EQUAL_C(E_ARGUMENT, error.GetCode(),
                "should not let to deallocate replicas for a not master disk");

            error = state.DeallocateDiskReplicas(db, "disk-1", 10);
            UNIT_ASSERT_EQUAL_C(E_ARGUMENT, error.GetCode(), "should not let "
                "to deallocate a not sufficient amount of replicas");

            error = state.DeallocateDiskReplicas(db, "disk-1", 2);
            UNIT_ASSERT_EQUAL_C(E_ARGUMENT, error.GetCode(),
                "should not let to turn a mirrored disk to a simple one");

            error = state.DeallocateDiskReplicas(db, "disk-1", 1);
            UNIT_ASSERT_EQUAL_C(S_OK, error.GetCode(),
                "should deallocate one replica");
            UNIT_ASSERT_VALUES_EQUAL_C(2, group->DisksSize(),
                "placement group size should be decreased");
            TDiskInfo diskInfo;
            error = state.GetDiskInfo("disk-1", diskInfo);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL_C(1, diskInfo.Replicas.size(),
                "disk replicaCount should be decreased");
        });
    }

    Y_UNIT_TEST(ShouldUpdateReplicaCount)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({
                AgentConfig(1, {
                    Device("dev-1", "uuid-1", "rack-1"),
                    Device("dev-2", "uuid-2", "rack-1"),
                    Device("dev-3", "uuid-3", "rack-1"),
                }),
                AgentConfig(2, {
                    Device("dev-4", "uuid-4", "rack-2"),
                    Device("dev-5", "uuid-5", "rack-2"),
                    Device("dev-6", "uuid-6", "rack-2"),
                }),
                AgentConfig(3, {
                    Device("dev-7", "uuid-7", "rack-3"),
                    Device("dev-8", "uuid-8", "rack-3"),
                    Device("dev-9", "uuid-9", "rack-3"),
                }),
                AgentConfig(4, {
                    Device("dev-10", "uuid-10", "rack-4"),
                    Device("dev-11", "uuid-11", "rack-4"),
                    Device("dev-12", "uuid-12", "rack-4"),
                }),
                AgentConfig(5, {
                    Device("dev-13", "uuid-13", "rack-5"),
                    Device("dev-14", "uuid-14", "rack-5"),
                    Device("dev-15", "uuid-15", "rack-5"),
                }),
            })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TDiskInfo diskInfo;

            TVector<TDeviceConfig> d;
            TVector<TVector<TDeviceConfig>> m;
            TVector<TString> r;
            auto error = AllocateMirroredDisk(db, state, "disk-1", {}, 10_GB, 1,
                d, m, r);
            UNIT_ASSERT_SUCCESS(error);

            error = state.GetDiskInfo("disk-1", diskInfo);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Replicas.size());

            const auto* group = state.FindPlacementGroup("disk-1/g");
            UNIT_ASSERT(group);
            UNIT_ASSERT_VALUES_EQUAL(2, group->DisksSize());

            error = state.UpdateDiskReplicaCount(db, "undefined", 1);
            UNIT_ASSERT_EQUAL_C(E_NOT_FOUND, error.GetCode(),
                "should not let to deallocate replicas for undefined disk");

            TVector<TDeviceConfig> nd;
            error = AllocateDisk(db, state, "not-mirrored-disk", {}, 10_GB, nd);
            UNIT_ASSERT_SUCCESS(error);
            error = state.UpdateDiskReplicaCount(db, "not-mirrored-disk", 2);
            UNIT_ASSERT_EQUAL_C(E_ARGUMENT, error.GetCode(), "should not let "
                "to deallocate replicas for a not mirrored disk");

            UNIT_ASSERT_VALUES_EQUAL("disk-1/0",
                group->GetDisks(0).GetDiskId());
            error = state.UpdateDiskReplicaCount(db, "disk-1/0", 2);
            UNIT_ASSERT_EQUAL_C(E_ARGUMENT, error.GetCode(),
                "should not let to deallocate replicas for a not master disk");

            error = state.UpdateDiskReplicaCount(db, "disk-1", 1);
            UNIT_ASSERT_EQUAL_C(S_FALSE, error.GetCode(), "should not update "
                "the amount of replicas to the same one");

            error = state.UpdateDiskReplicaCount(db, "disk-1", 10);
            UNIT_ASSERT_EQUAL_C(E_ARGUMENT, error.GetCode(), "should not let "
                "to allocate a lot of replicas which is above the limit");

            error = state.UpdateDiskReplicaCount(db, "disk-1", 0);
            UNIT_ASSERT_EQUAL_C(E_ARGUMENT, error.GetCode(),
                "should not let to turn a mirrored disk to a simple one");

            error = state.UpdateDiskReplicaCount(db, "disk-1", 2);
            UNIT_ASSERT_EQUAL_C(S_OK, error.GetCode(),
                "should allocate one replica");
            UNIT_ASSERT_VALUES_EQUAL_C(3, group->DisksSize(),
                "placement group size should be increased");
            error = state.GetDiskInfo("disk-1", diskInfo);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL_C(2, diskInfo.Replicas.size(),
                "disk replicaCount should be updated");

            error = state.UpdateDiskReplicaCount(db, "disk-1", 1);
            UNIT_ASSERT_EQUAL_C(S_OK, error.GetCode(),
                "should deallocate one replica");
            UNIT_ASSERT_VALUES_EQUAL_C(2, group->DisksSize(),
                "placement group size should be decreased");
            error = state.GetDiskInfo("disk-1", diskInfo);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL_C(1, diskInfo.Replicas.size(),
                "disk replicaCount should be updated");
        });
    }

    Y_UNIT_TEST(ShouldReplaceBrokenDevices)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        auto agentConfig1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1"),
            Device("dev-3", "uuid-3", "rack-1"),
        });

        auto agentConfig2 = AgentConfig(2, {
            Device("dev-4", "uuid-4", "rack-2"),
            Device("dev-5", "uuid-5", "rack-2"),
            Device("dev-6", "uuid-6", "rack-2"),
        });

        auto agentConfig3 = AgentConfig(3, {
            Device("dev-7", "uuid-7", "rack-3"),
            Device("dev-8", "uuid-8", "rack-3"),
            Device("dev-9", "uuid-9", "rack-3"),
        });

        auto agentConfig4 = AgentConfig(4, {
            Device("dev-10", "uuid-10", "rack-4"),
            Device("dev-11", "uuid-11", "rack-4"),
            Device("dev-12", "uuid-12", "rack-4"),
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({
                agentConfig1,
                agentConfig2,
                agentConfig3,
                agentConfig4
            })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            TVector<TVector<TDeviceConfig>> replicas;
            TVector<TString> deviceReplacementIds;
            auto error = AllocateMirroredDisk(
                db,
                state,
                "disk-1",
                {},
                10_GB,
                2,
                devices,
                replicas,
                deviceReplacementIds);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-1", devices[0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(2, replicas.size());
            UNIT_ASSERT_VALUES_EQUAL(1, replicas[0].size());
            UNIT_ASSERT_VALUES_EQUAL("dev-4", replicas[0][0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(1, replicas[1].size());
            UNIT_ASSERT_VALUES_EQUAL("dev-7", replicas[1][0].GetDeviceName());
            ASSERT_VECTORS_EQUAL(TVector<TString>{}, deviceReplacementIds);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            // mirrored disk replicas should not delay host/device maintenance

            TVector<TDiskStateUpdate> affectedDisks;
            TDuration timeout;
            auto error = state.UpdateCmsHostState(
                db,
                agentConfig1.GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                Now(),
                false,
                affectedDisks,
                timeout);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(TDuration::Zero(), timeout);
            ASSERT_VECTORS_EQUAL(TVector<TDiskStateUpdate>(), affectedDisks);
        });

        TDiskInfo diskInfo;
        auto error = state.GetDiskInfo("disk-1", diskInfo);
        UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
        UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Devices.size());
        UNIT_ASSERT_VALUES_EQUAL("dev-1", diskInfo.Devices[0].GetDeviceName());
        UNIT_ASSERT_VALUES_EQUAL(0, diskInfo.Migrations.size());
        UNIT_ASSERT_VALUES_EQUAL(2, diskInfo.Replicas.size());
        UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Replicas[0].size());
        UNIT_ASSERT_VALUES_EQUAL(
            "dev-4",
            diskInfo.Replicas[0][0].GetDeviceName());
        UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Replicas[1].size());
        UNIT_ASSERT_VALUES_EQUAL(
            "dev-7",
            diskInfo.Replicas[1][0].GetDeviceName());
        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<ui32>(NProto::DISK_STATE_ONLINE),
            static_cast<ui32>(diskInfo.State));

        TDiskInfo replicaInfo;
        error = state.GetDiskInfo("disk-1/0", replicaInfo);
        UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<ui32>(NProto::DISK_STATE_ONLINE),
            static_cast<ui32>(replicaInfo.State));

        replicaInfo = {};
        error = state.GetDiskInfo("disk-1/1", replicaInfo);
        UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<ui32>(NProto::DISK_STATE_ONLINE),
            static_cast<ui32>(replicaInfo.State));

        replicaInfo = {};
        error = state.GetDiskInfo("disk-1/2", replicaInfo);
        UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<ui32>(NProto::DISK_STATE_ONLINE),
            static_cast<ui32>(replicaInfo.State));

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            TDuration timeout;
            auto error = state.UpdateAgentState(
                db,
                agentConfig1.GetAgentId(),
                NProto::AGENT_STATE_UNAVAILABLE,
                Now(),
                "unreachable",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(TDuration::Zero(), timeout);
            ASSERT_VECTORS_EQUAL(TVector<TDiskStateUpdate>(), affectedDisks);
        });

        TVector<TString> disksToNotify;
        for (const auto& x: state.GetDisksToNotify()) {
            disksToNotify.push_back(x.first);
        }
        ASSERT_VECTORS_EQUAL(TVector<TString>{"disk-1"}, disksToNotify);

        diskInfo = {};
        error = state.GetDiskInfo("disk-1", diskInfo);
        UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
        UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Devices.size());
        UNIT_ASSERT_VALUES_EQUAL("dev-10", diskInfo.Devices[0].GetDeviceName());
        UNIT_ASSERT_VALUES_EQUAL(0, diskInfo.Migrations.size());
        UNIT_ASSERT_VALUES_EQUAL(2, diskInfo.Replicas.size());
        UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Replicas[0].size());
        UNIT_ASSERT_VALUES_EQUAL(
            "dev-4",
            diskInfo.Replicas[0][0].GetDeviceName());
        UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Replicas[1].size());
        UNIT_ASSERT_VALUES_EQUAL(
            "dev-7",
            diskInfo.Replicas[1][0].GetDeviceName());
        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<ui32>(NProto::DISK_STATE_ONLINE),
            static_cast<ui32>(diskInfo.State));
        ASSERT_VECTORS_EQUAL(
            TVector<TString>{"uuid-10"},
            diskInfo.DeviceReplacementIds);

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            TVector<TVector<TDeviceConfig>> replicas;
            TVector<TString> deviceReplacementIds;
            auto error = AllocateMirroredDisk(
                db,
                state,
                "disk-1",
                {},
                10_GB,
                2,
                devices,
                replicas,
                deviceReplacementIds);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-10", devices[0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(2, replicas.size());
            UNIT_ASSERT_VALUES_EQUAL(1, replicas[0].size());
            UNIT_ASSERT_VALUES_EQUAL("dev-4", replicas[0][0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(1, replicas[1].size());
            UNIT_ASSERT_VALUES_EQUAL("dev-7", replicas[1][0].GetDeviceName());
            ASSERT_VECTORS_EQUAL(
                TVector<TString>{"uuid-10"},
                deviceReplacementIds);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            auto error =
                state.MarkReplacementDevice(db, "disk-1", "uuid-10", false);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());

            error =
                state.MarkReplacementDevice(db, "nonexistent-disk", "", false);
            UNIT_ASSERT_VALUES_EQUAL(E_NOT_FOUND, error.GetCode());

            error =
                state.MarkReplacementDevice(db, "disk-1", "nonexistent-device", false);
            UNIT_ASSERT_VALUES_EQUAL(S_ALREADY, error.GetCode());

            error = state.MarkReplacementDevice(db, "disk-1", "uuid-1", false);
            UNIT_ASSERT_VALUES_EQUAL(S_ALREADY, error.GetCode());

            error =
                state.MarkReplacementDevice(db, "disk-1", "uuid-10", false);
            UNIT_ASSERT_VALUES_EQUAL(S_ALREADY, error.GetCode());
        });

        diskInfo = {};
        error = state.GetDiskInfo("disk-1", diskInfo);
        UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
        UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Devices.size());
        UNIT_ASSERT_VALUES_EQUAL("dev-10", diskInfo.Devices[0].GetDeviceName());
        ASSERT_VECTORS_EQUAL(
            TVector<TString>{},
            diskInfo.DeviceReplacementIds);

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            TVector<TVector<TDeviceConfig>> replicas;
            TVector<TString> deviceReplacementIds;
            auto error = AllocateMirroredDisk(
                db,
                state,
                "disk-1",
                {},
                10_GB,
                2,
                devices,
                replicas,
                deviceReplacementIds);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-10", devices[0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(2, replicas.size());
            UNIT_ASSERT_VALUES_EQUAL(1, replicas[0].size());
            UNIT_ASSERT_VALUES_EQUAL("dev-4", replicas[0][0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(1, replicas[1].size());
            UNIT_ASSERT_VALUES_EQUAL("dev-7", replicas[1][0].GetDeviceName());
            ASSERT_VECTORS_EQUAL(TVector<TString>{}, deviceReplacementIds);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            state.DeallocateDisk(db, "disk-1");
        });
    }

    Y_UNIT_TEST(ShouldTakeReplicaAvailabilityIntoAccount)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        auto agentConfig1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1"),
            Device("dev-3", "uuid-3", "rack-1"),
        });

        auto agentConfig2 = AgentConfig(2, {
            Device("dev-4", "uuid-4", "rack-2"),
            Device("dev-5", "uuid-5", "rack-2"),
            Device("dev-6", "uuid-6", "rack-2"),
        });

        auto agentConfig3 = AgentConfig(3, {
            Device("dev-7", "uuid-7", "rack-3"),
            Device("dev-8", "uuid-8", "rack-3"),
            Device("dev-9", "uuid-9", "rack-3"),
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({
                agentConfig1,
                agentConfig2,
                agentConfig3,
            })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            TVector<TVector<TDeviceConfig>> replicas;
            TVector<TString> deviceReplacementIds;
            auto error = AllocateMirroredDisk(
                db,
                state,
                "disk-1",
                {},
                10_GB,
                1,
                devices,
                replicas,
                deviceReplacementIds);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-1", devices[0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(1, replicas.size());
            UNIT_ASSERT_VALUES_EQUAL(1, replicas[0].size());
            UNIT_ASSERT_VALUES_EQUAL("dev-4", replicas[0][0].GetDeviceName());
            ASSERT_VECTORS_EQUAL(TVector<TString>{}, deviceReplacementIds);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            TDuration timeout;
            auto error = state.UpdateAgentState(
                db,
                agentConfig1.GetAgentId(),
                NProto::AGENT_STATE_UNAVAILABLE,
                Now(),
                "unreachable",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(TDuration::Zero(), timeout);
            ASSERT_VECTORS_EQUAL(TVector<TDiskStateUpdate>(), affectedDisks);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            // maintenance should not be allowed - it will break the last
            // available replica

            TVector<TDiskStateUpdate> affectedDisks;
            TDuration timeout;
            auto error = state.UpdateCmsHostState(
                db,
                agentConfig2.GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                Now(),
                false,
                affectedDisks,
                timeout);

            UNIT_ASSERT_VALUES_EQUAL(E_TRY_AGAIN, error.GetCode());
            UNIT_ASSERT_VALUES_UNEQUAL(TDuration::Zero(), timeout);
            ASSERT_VECTORS_EQUAL(TVector<TDiskStateUpdate>(), affectedDisks);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            TDuration timeout;
            auto error = state.UpdateAgentState(
                db,
                agentConfig2.GetAgentId(),
                NProto::AGENT_STATE_UNAVAILABLE,
                Now(),
                "unreachable",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(TDuration::Zero(), timeout);
            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());
            UNIT_ASSERT_DISK_STATE(
                "disk-1/1",
                DISK_STATE_TEMPORARILY_UNAVAILABLE,
                affectedDisks[0]);
        });

        // agent2 unavailability should not have caused device replacement

        TDiskInfo diskInfo;
        auto error = state.GetDiskInfo("disk-1", diskInfo);
        UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
        UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Devices.size());
        UNIT_ASSERT_VALUES_EQUAL("dev-7", diskInfo.Devices[0].GetDeviceName());
        UNIT_ASSERT_VALUES_EQUAL(0, diskInfo.Migrations.size());
        UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Replicas.size());
        UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Replicas[0].size());
        UNIT_ASSERT_VALUES_EQUAL(
            "dev-4",
            diskInfo.Replicas[0][0].GetDeviceName());

        TDiskInfo replicaInfo;
        error = state.GetDiskInfo("disk-1/0", replicaInfo);
        UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<ui32>(NProto::DISK_STATE_ONLINE),
            static_cast<ui32>(replicaInfo.State));

        replicaInfo = {};
        error = state.GetDiskInfo("disk-1/1", replicaInfo);
        UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<ui32>(NProto::DISK_STATE_TEMPORARILY_UNAVAILABLE),
            static_cast<ui32>(replicaInfo.State));

        TVector<TString> disksToNotify;
        for (const auto& x: state.GetDisksToNotify()) {
            disksToNotify.push_back(x.first);
        }
        ASSERT_VECTORS_EQUAL(TVector<TString>{"disk-1"}, disksToNotify);

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            TVector<TVector<TDeviceConfig>> replicas;
            TVector<TString> deviceReplacementIds;
            auto error = AllocateMirroredDisk(
                db,
                state,
                "disk-1",
                {},
                10_GB,
                1,
                devices,
                replicas,
                deviceReplacementIds);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-7", devices[0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(1, replicas.size());
            UNIT_ASSERT_VALUES_EQUAL(1, replicas[0].size());
            UNIT_ASSERT_VALUES_EQUAL("dev-4", replicas[0][0].GetDeviceName());
            ASSERT_VECTORS_EQUAL(
                TVector<TString>{"uuid-7"},
                deviceReplacementIds);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            state.DeallocateDisk(db, "disk-1");
        });
    }

    Y_UNIT_TEST(ShouldBuildReplicaTableUponStateConstruction)
    {
        auto agentConfig1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1"),
            Device("dev-3", "uuid-3", "rack-1"),
        });

        auto agentConfig2 = AgentConfig(2, {
            Device("dev-4", "uuid-4", "rack-2"),
            Device("dev-5", "uuid-5", "rack-2"),
            Device("dev-6", "uuid-6", "rack-2"),
        });

        auto agentConfig3 = AgentConfig(3, {
            Device("dev-7", "uuid-7", "rack-3"),
            Device("dev-8", "uuid-8", "rack-3"),
            Device("dev-9", "uuid-9", "rack-3"),
        });

        TVector<NProto::TDiskConfig> disks = {
            Disk("disk-1/0", {"uuid-1", "uuid-2", "uuid-3"}, NProto::DISK_STATE_ONLINE),
            Disk("disk-1/1", {"uuid-4", "uuid-5", "uuid-6"}, NProto::DISK_STATE_ONLINE),
            Disk("disk-1/2", {"uuid-7", "uuid-8", "uuid-9"}, NProto::DISK_STATE_ONLINE),
        };

        for (auto& disk: disks) {
            disk.SetMasterDiskId("disk-1");
        }

        disks.push_back(Disk("disk-1", {}, NProto::DISK_STATE_ONLINE));
        disks.back().SetReplicaCount(2);
        *disks.back().AddDeviceReplacementUUIDs() = "uuid-1";
        *disks.back().AddDeviceReplacementUUIDs() = "uuid-6";

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({
                agentConfig1,
                agentConfig2,
                agentConfig3,
            })
            .WithDisks(disks)
            .Build();

        const auto& rt = state.GetReplicaTable();
        const auto m = rt.AsMatrix("disk-1");
        TStringBuilder sb;
        for (const auto& row: m) {
            sb << "|";
            for (const auto& d: row) {
                sb << d.Id;
                if (d.IsReplacement) {
                    sb << "*";
                }
                sb << "|";
            }
        }

        UNIT_ASSERT_VALUES_EQUAL(
            "|uuid-1*|uuid-4|uuid-7|"
            "|uuid-2|uuid-5|uuid-8|"
            "|uuid-3|uuid-6*|uuid-9|",
            sb);
    }
}

bool operator==(const TDiskStateUpdate& l, const TDiskStateUpdate& r)
{
    return l.State.SerializeAsString() == r.State.SerializeAsString()
        && l.SeqNo == r.SeqNo;
}

}   // namespace NCloud::NBlockStore::NStorage

template <>
inline void Out<NCloud::NBlockStore::NStorage::TDiskStateUpdate>(
    IOutputStream& out,
    const NCloud::NBlockStore::NStorage::TDiskStateUpdate& update)
{
    out << update.State.DebugString() << "\t" << update.SeqNo;
}
