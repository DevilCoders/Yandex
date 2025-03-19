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

Y_UNIT_TEST_SUITE(TDiskRegistryStateCreateTest)
{
    Y_UNIT_TEST(ShouldCreateDiskFromDevices)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

         const TVector agents {
            AgentConfig(1, {
                Device("dev-1", "uuid-1.1"),    // disk-1
                Device("dev-2", "uuid-1.2"),
                Device("dev-3", "uuid-1.3"),
            }),
            AgentConfig(2, {
                Device("dev-1", "uuid-2.1"),    // dirty
                Device("dev-2", "uuid-2.2"),
                Device("dev-3", "uuid-2.3"),
            })
        };

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents(agents)
            .WithDisks({ Disk("disk-1", {"uuid-1.1"}) })
            .WithDirtyDevices({"uuid-2.1"})
            .Build();

        auto device = [] (auto agentId, auto name) {
            NProto::TDeviceConfig config;
            config.SetAgentId(agentId);
            config.SetDeviceName(name);
            return config;
        };

        // unknown device
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto error = state.CreateDiskFromDevices(
                db,
                false,  // force
                "foo",
                4_KB,
                {
                    device("agent-2", "dev-3"),
                    device("agent-3", "foo-2"),
                }
            );

            UNIT_ASSERT_VALUES_EQUAL(E_ARGUMENT, error.GetCode());
            UNIT_ASSERT(error.GetMessage().Contains("not found"));

            UNIT_ASSERT(state.FindDisk("uuid-2.3").empty());
            UNIT_ASSERT(state.FindDisk("foo-2").empty());
        });

        // allocated device
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto error = state.CreateDiskFromDevices(
                db,
                true,  // force
                "foo",
                4_KB,
                {device("agent-1", "dev-1")}
            );

            UNIT_ASSERT_VALUES_EQUAL(E_ARGUMENT, error.GetCode());
            UNIT_ASSERT(error.GetMessage().Contains("is allocated for"));
        });

        // dirty device (!force)
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto error = state.CreateDiskFromDevices(
                db,
                false,  // force
                "foo",
                4_KB,
                {device("agent-2", "dev-1")}
            );

            UNIT_ASSERT_VALUES_EQUAL(E_ARGUMENT, error.GetCode());
            UNIT_ASSERT(error.GetMessage().Contains("is dirty"));
        });

        // disk already exists
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto error = state.CreateDiskFromDevices(
                db,
                true,  // force
                "disk-1",
                4_KB,
                {device("agent-2", "dev-2")}
            );

            UNIT_ASSERT_VALUES_EQUAL(E_ARGUMENT, error.GetCode());
            UNIT_ASSERT(error.GetMessage().Contains("already exists"));
        });

        // dirty device (force)
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto error = state.CreateDiskFromDevices(
                db,
                true,  // force
                "bar",
                4_KB,
                {device("agent-2", "dev-1")}
            );

            UNIT_ASSERT_VALUES_EQUAL_C(S_OK, error.GetCode(), error.GetMessage());
            UNIT_ASSERT_VALUES_EQUAL(0, state.GetDirtyDevices().size());

            TDiskInfo info;
            UNIT_ASSERT_SUCCESS(state.GetDiskInfo("bar", info));
                
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ONLINE, info.State);
            UNIT_ASSERT_VALUES_EQUAL(1, info.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", info.Devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(4096, info.LogicalBlockSize);
        });

        // regular
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto error = state.CreateDiskFromDevices(
                db,
                false,  // force
                "baz",
                4_KB,
                {
                    device("agent-1", "dev-2"),
                    device("agent-2", "dev-2")
                }
            );

            UNIT_ASSERT_VALUES_EQUAL_C(S_OK, error.GetCode(), error.GetMessage());

            TDiskInfo info;
            UNIT_ASSERT_SUCCESS(state.GetDiskInfo("baz", info));
                
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ONLINE, info.State);
            UNIT_ASSERT_VALUES_EQUAL(2, info.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.2", info.Devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.2", info.Devices[1].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(4096, info.LogicalBlockSize);
        });
    }
}

}   // namespace NCloud::NBlockStore::NStorage
