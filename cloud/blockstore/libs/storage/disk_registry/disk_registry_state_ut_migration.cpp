#include "disk_registry_state.h"

#include "disk_registry_database.h"

#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/disk_registry/testlib/test_state.h>
#include <cloud/blockstore/libs/storage/testlib/test_executor.h>
#include <cloud/blockstore/libs/storage/testlib/ut_helpers.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NDiskRegistryStateTest;

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TDiskRegistryStateMigrationTest)
{
    Y_UNIT_TEST(ShouldRespectPlacementGroups)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const TVector agents {
            AgentConfig(1, {
                Device("dev-1", "uuid-1.1", "rack-1"),
                Device("dev-1", "uuid-1.2", "rack-1")
            }),
            AgentConfig(2, { Device("dev-1", "uuid-2.1", "rack-2") }),
            AgentConfig(3, { Device("dev-1", "uuid-3.1", "rack-1") }),
            AgentConfig(4, {
                Device("dev-1", "uuid-4.1", "rack-3"),
                Device("dev-2", "uuid-4.2", "rack-3"),
                Device("dev-3", "uuid-4.3", "rack-3")
            })
        };

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents(agents)
            .WithDisks({
                Disk("foo", { "uuid-1.1", "uuid-1.2" }),    // rack-1
                Disk("bar", { "uuid-2.1" })                 // rack-2
            })
            .WithDirtyDevices({"uuid-4.1", "uuid-4.2", "uuid-4.3"})
            .Build();

        auto changeAgentState = [&] (
            auto db,
            const auto& config,
            NProto::EAgentState newState) mutable
        {
            TVector<TDiskStateUpdate> affectedDisks;

            auto error = state.UpdateAgentState(
                db,
                config.GetAgentId(),
                newState,
                TInstant::Now(),
                "test",
                affectedDisks);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());

            return affectedDisks;
        };

        UNIT_ASSERT(state.IsMigrationListEmpty());

        // create & initialize `pg`
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            UNIT_ASSERT_SUCCESS(state.CreatePlacementGroup(db, "pg"));

            TVector<TString> disksToAdd {"foo", "bar"};
            UNIT_ASSERT_SUCCESS(state.AlterPlacementGroupMembership(
                db, "pg", 1, disksToAdd, {}
            ));
        });

        {
            TDiskInfo info;
            UNIT_ASSERT_SUCCESS(state.GetDiskInfo("foo", info));
            UNIT_ASSERT_VALUES_EQUAL(2, info.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL("rack-1", info.Devices[0].GetRack());
            UNIT_ASSERT_VALUES_EQUAL("rack-1", info.Devices[1].GetRack());
            UNIT_ASSERT_VALUES_EQUAL("pg", info.PlacementGroupId);
        }

        {
            TDiskInfo info;
            UNIT_ASSERT_SUCCESS(state.GetDiskInfo("bar", info));
            UNIT_ASSERT_VALUES_EQUAL(1, info.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL("rack-2", info.Devices[0].GetRack());
            UNIT_ASSERT_VALUES_EQUAL("pg", info.PlacementGroupId);
        }

        // enable migrations of disk-1 & disk-2
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            const TString diskIds[] { "foo", "bar" };
            for (const int i: {0, 1}) {
                auto affectedDisks = changeAgentState(
                    db, agents[i], NProto::AGENT_STATE_WARNING);

                UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());

                auto& [d, seqNo] = affectedDisks[0];

                UNIT_ASSERT_VALUES_EQUAL(diskIds[i], d.GetDiskId());
                UNIT_ASSERT_EQUAL(NProto::DISK_STATE_MIGRATION, d.GetState());
            }
        });

        {
            auto migrations = state.BuildMigrationList();
            UNIT_ASSERT_VALUES_EQUAL(3, migrations.size());

            SortBy(migrations, [] (auto& m) {
                return std::tie(m.DiskId, m.SourceDeviceId);
            });

            UNIT_ASSERT_VALUES_EQUAL("bar", migrations[0].DiskId);
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", migrations[0].SourceDeviceId);

            UNIT_ASSERT_VALUES_EQUAL("foo", migrations[1].DiskId);
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", migrations[1].SourceDeviceId);
            UNIT_ASSERT_VALUES_EQUAL("foo", migrations[2].DiskId);
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.2", migrations[2].SourceDeviceId);
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            auto [device, error] = state.StartDeviceMigration(db, "bar", "uuid-2.1");
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
        });

        // start migration for foo:uuid-1.1 -> uuid-3.1
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            auto [device, error] = state.StartDeviceMigration(db, "foo", "uuid-1.1");
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());

            UNIT_ASSERT_VALUES_EQUAL("uuid-3.1", device.GetDeviceUUID());
        });

        // cleanup dirty device
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            auto dirtyDevices = state.GetDirtyDevices();
            UNIT_ASSERT_VALUES_EQUAL(3, dirtyDevices.size());

            SortBy(dirtyDevices, [] (const auto& d) {
                return d.GetDeviceUUID();
            });

            for (int i = 0; i != 3; ++i) {
                const auto& d = dirtyDevices[i];
                UNIT_ASSERT_VALUES_EQUAL(Sprintf("uuid-4.%d", i + 1), d.GetDeviceUUID());
                UNIT_ASSERT_VALUES_EQUAL("rack-3", d.GetRack());
                UNIT_ASSERT(state.MarkDeviceAsClean(db, d.GetDeviceUUID()));
            }
        });

        UNIT_ASSERT_VALUES_EQUAL(0, state.GetDirtyDevices().size());

        // start migration for foo:uuid-1.2 -> uuid-4.X
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            auto [device, error] = state.StartDeviceMigration(db, "foo", "uuid-1.2");
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL("rack-3", device.GetRack());
            UNIT_ASSERT(device.GetDeviceUUID().StartsWith("uuid-4."));
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            auto [device, error] = state.StartDeviceMigration(db, "bar", "uuid-2.1");
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
        });

        // finish migration for foo:uuid-1.1 -> uuid-3.1
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            auto error = state.FinishDeviceMigration(
                db,
                "foo",
                "uuid-1.1",
                "uuid-3.1",
                TInstant::Now(),
                &affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(!affectedDisk.Defined());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            auto [device, error] = state.StartDeviceMigration(db, "bar", "uuid-2.1");
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
        });

        // cancel migration for foo:uuid-1.2 -> uuid-4.X
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            auto affectedDisks = changeAgentState(
                db, agents[0], NProto::AGENT_STATE_ONLINE);

            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());

            auto& [d, seqNo] = affectedDisks[0];

            UNIT_ASSERT_VALUES_EQUAL("foo", d.GetDiskId());
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ONLINE, d.GetState());
        });

        // start migration for bar:uuid-2.1 -> uuid-4.X
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            auto [device, error] = state.StartDeviceMigration(db, "bar", "uuid-2.1");
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL("rack-3", device.GetRack());
            UNIT_ASSERT(device.GetDeviceUUID().StartsWith("uuid-4."));
        });
    }
}

}   // namespace NCloud::NBlockStore::NStorage
