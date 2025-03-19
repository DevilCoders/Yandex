#include "disk_registry_state.h"

#include "disk_registry_database.h"

#include <cloud/blockstore/libs/diagnostics/critical_events.h>
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

Y_UNIT_TEST_SUITE(TDiskRegistryStateTest)
{
    Y_UNIT_TEST(ShouldRegisterAgent)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const ui32 agentId = 42;

        const auto partial = AgentConfig(agentId, {
            Device("dev-1", "uuid-1", "rack-1"),
        });

        const auto complete = AgentConfig(agentId, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1")
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder().Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UpdateConfig(state, db, { complete });
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, partial));
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, complete));

            UNIT_ASSERT_VALUES_EQUAL(S_OK,
                state.UnregisterAgent(db, agentId).GetCode());

            UNIT_ASSERT_VALUES_EQUAL(S_ALREADY,
                state.UnregisterAgent(db, agentId).GetCode());
        });
    }

    Y_UNIT_TEST(ShouldRejectUnallowedAgent)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto config1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
        });

        const auto config2 = AgentConfig(2, {
            Device("dev-2", "uuid-2", "rack-1")
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder().Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UpdateConfig(state, db, { config1 });
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, config1));

            auto error = RegisterAgent(state, db, config2);

            UNIT_ASSERT_VALUES_EQUAL(error.GetCode(), E_INVALID_STATE);
        });
    }

    Y_UNIT_TEST(ShouldRejectAgentWithUnallowedDevice)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto config1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
        });

        const auto config2 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1")
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder().Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UpdateConfig(state, db, { config1 });
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto error = RegisterAgent(state, db, config2);

            UNIT_ASSERT_VALUES_EQUAL(error.GetCode(), E_INVALID_STATE);
            UNIT_ASSERT(error.GetMessage().Contains("uuid-2"));
        });
    }

    Y_UNIT_TEST(ShouldUpdateKnownAgents)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto config1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1")
        });

        const auto config2 = AgentConfig(1, {
            Device("dev-3", "uuid-3", "rack-1")
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder().Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UpdateConfig(state, db, { config1 });

            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, config1));

            UpdateConfig(state, db, { config2 });

            auto error = RegisterAgent(state, db, config1);
            UNIT_ASSERT_VALUES_EQUAL(error.GetCode(), E_INVALID_STATE);

            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, config2));
        });

        const auto current = state.GetConfig();
        UNIT_ASSERT_VALUES_EQUAL(current.KnownAgentsSize(), 1);

        const auto& agents = current.GetKnownAgents(0);

        UNIT_ASSERT_VALUES_EQUAL(agents.GetAgentId(), "agent-1");
        UNIT_ASSERT_VALUES_EQUAL(agents.DevicesSize(), 1);
        UNIT_ASSERT_VALUES_EQUAL(agents.GetDevices(0).GetDeviceUUID(), "uuid-3");
    }

    Y_UNIT_TEST(ShouldCorrectlyReRegisterAgent)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto config1 = AgentConfig(1, {
            Device(
                "dev-1",
                "uuid-1",
                "rack-1",
                DefaultBlockSize,
                11_GB
            ),
            Device(
                "dev-2",
                "uuid-2",
                "rack-1",
                DefaultBlockSize,
                10_GB
            ),
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder().Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UpdateConfig(state, db, { config1 });

            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, config1));
            state.MarkDeviceAsClean(db, "uuid-1");
            state.MarkDeviceAsClean(db, "uuid-2");

            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", {}, 20_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(2, devices.size());
            SortBy(devices, [&] (const auto& d) {
                return d.GetDeviceUUID();
            });
            UNIT_ASSERT_VALUES_EQUAL("uuid-1", devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultBlockSize,
                devices[0].GetBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL("uuid-2", devices[1].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultBlockSize,
                devices[1].GetBlocksCount()
            );
        });

        // simply repeating agent registration and disk allocation - device
        // sizes should not change
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, config1));

            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", {}, 20_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(S_ALREADY, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(2, devices.size());
            SortBy(devices, [&] (const auto& d) {
                return d.GetDeviceUUID();
            });
            UNIT_ASSERT_VALUES_EQUAL("uuid-1", devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultBlockSize,
                devices[0].GetBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL("uuid-2", devices[1].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultBlockSize,
                devices[1].GetBlocksCount()
            );
        });
    }

    Y_UNIT_TEST(ShouldRejectDestructiveConfig)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1.1", "rack-1"),
            Device("dev-2", "uuid-1.2", "rack-1")
        });

        const auto agent2a = AgentConfig(2, {
            Device("dev-1", "uuid-2.1", "rack-2"),
            Device("dev-2", "uuid-2.2", "rack-2")
        });

        const auto agent2b = AgentConfig(2, {
            Device("dev-1", "uuid-2.1", "rack-2"),
            Device("dev-3", "uuid-2.3", "rack-2"),
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1, agent2a })
            .WithDisks({
                Disk("disk-1", {"uuid-1.1", "uuid-2.1"}),
                Disk("disk-2", {"uuid-1.2", "uuid-2.2"})
            })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> affectedDisks;
            const auto error = state.UpdateConfig(
                db,
                MakeConfig({ agent1, agent2b }),
                true,   // ignoreVersion
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(E_INVALID_STATE, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());
            UNIT_ASSERT_VALUES_EQUAL("disk-2", affectedDisks[0]);

            UNIT_ASSERT_VALUES_EQUAL(0, state.GetDiskStateUpdates().size());
        });
    }

    Y_UNIT_TEST(ShouldRejectConfigWithWrongVersion)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto agent1a = AgentConfig(1, {
            Device("dev-1", "uuid-1.1", "rack-1"),
            Device("dev-2", "uuid-1.2", "rack-1")
        });

        const auto agent1b = AgentConfig(1, {
            Device("dev-1", "uuid-1.1", "rack-1"),
            Device("dev-2", "uuid-1.2", "rack-1"),
            Device("dev-3", "uuid-1.3", "rack-1")
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithConfig(1, { agent1a })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> affectedDisks;
            const auto error = state.UpdateConfig(
                db,
                MakeConfig(42, { agent1b }),
                false,  // ignoreVersion
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(E_CONFIG_VERSION_MISMATCH, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> affectedDisks;
            const auto error = state.UpdateConfig(
                db,
                MakeConfig(1, { agent1b }),
                false,  // ignoreVersion
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> affectedDisks;
            const auto error = state.UpdateConfig(
                db,
                MakeConfig(2, { agent1a }),
                false,  // ignoreVersion
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });
    }

    Y_UNIT_TEST(ShouldRemoveDiscardedAgent)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1.1", "rack-1"),
            Device("dev-2", "uuid-1.2", "rack-1")
        });

        const auto agent2 = AgentConfig(2, {
            Device("dev-1", "uuid-2.1", "rack-2"),
            Device("dev-2", "uuid-2.2", "rack-2")
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.UpdateAgent(agent1);
            db.UpdateAgent(agent2);
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1, agent2 })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> affectedDisks;
            const auto error = state.UpdateConfig(
                db,
                MakeConfig(0, { agent1 }),
                false,  // ignoreVersion
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        executor.ReadTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<NProto::TAgentConfig> agents;

            db.ReadAgents(agents);
            UNIT_ASSERT_VALUES_EQUAL(1, agents.size());
            UNIT_ASSERT_VALUES_EQUAL(1, agents[0].GetNodeId());
        });
    }

    Y_UNIT_TEST(AllocateDiskWithDifferentBlockSize)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto config1 = AgentConfig(42, {
            Device("dev-1", "uuid-1", "rack-1", 512, 1_TB)
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ config1 })
            .Build();

        TVector<TDeviceConfig> devices;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto error = AllocateDisk(db, state, "disk-1", {}, 10_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL(42, devices.front().GetNodeId());
        });
    }

    Y_UNIT_TEST(ShouldNotReturnErrorForAlreadyAllocatedDisks)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto config1 = AgentConfig(42, {
            Device("dev-1", "uuid-1", "rack-1")
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ config1 })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", {}, 10_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL(42, devices.front().GetNodeId());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", {}, 9_GB, devices);
            UNIT_ASSERT_VALUES_EQUAL(S_ALREADY, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL(42, devices.front().GetNodeId());
        });
    }

    Y_UNIT_TEST(AllocateDiskOnSingleDevice)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto config1 = AgentConfig(42, {
            Device("dev-1", "uuid-1", "rack-1", 512, 1_TB)
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ config1 })
            .Build();

        TVector<TDeviceConfig> devices;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto error = AllocateDisk(db, state, "disk-1", {}, 10_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL(42, devices.front().GetNodeId());
        });

        {
            TDiskInfo diskInfo;
            auto error = state.GetDiskInfo("disk-1", diskInfo);
            auto& devices = diskInfo.Devices;
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL(42, devices.front().GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1", devices.front().GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(DefaultLogicalBlockSize, diskInfo.LogicalBlockSize);
        }
    }

    Y_UNIT_TEST(AllocateDiskOnFewDevices)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto config1 = AgentConfig(100, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1"),
            Device("dev-3", "uuid-3", "rack-1")
        });

        const auto config2 = AgentConfig(200, {
            Device("dev-1", "uuid-4", "rack-1"),
            Device("dev-2", "uuid-5", "rack-1"),
            Device("dev-3", "uuid-6", "rack-1"),
            Device("dev-4", "uuid-7", "rack-1")
        });

        const auto config3 = AgentConfig(300, {
            Device("dev-1", "uuid-8", "rack-1"),
            Device("dev-2", "uuid-9", "rack-1"),
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ config1, config2, config3 })
            .Build();

        TVector<TDeviceConfig> expected;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto error = AllocateDisk(db, state, "disk-id", {}, 90_GB, expected);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(9, expected.size());
        });

        {
            TDiskInfo diskInfo;
            auto error = state.GetDiskInfo("disk-id", diskInfo);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(
                expected.size(),
                diskInfo.Devices.size());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                diskInfo.LogicalBlockSize);

            for (size_t i = 0; i != expected.size(); ++i) {
                auto& device = diskInfo.Devices[i];

                UNIT_ASSERT_VALUES_EQUAL(
                    expected[i].GetDeviceUUID(),
                    device.GetDeviceUUID());

                UNIT_ASSERT_VALUES_EQUAL(
                    expected[i].GetBlockSize(),
                    device.GetBlockSize());

                UNIT_ASSERT_VALUES_EQUAL(
                    expected[i].GetBlocksCount(),
                    device.GetBlocksCount());
            }
        }
    }

    Y_UNIT_TEST(ShouldTakeDeviceOverridesIntoAccount)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto config1 = AgentConfig(100, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1"),
            Device("dev-3", "uuid-3", "rack-1")
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ config1 })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", {}, 28_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(3, devices.size());
            SortBy(devices, [] (const auto& d) {
                return d.GetDeviceUUID();
            });

            UNIT_ASSERT_VALUES_EQUAL("uuid-1", devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultBlockSize,
                devices[0].GetBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL("uuid-2", devices[1].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultBlockSize,
                devices[1].GetBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL("uuid-3", devices[2].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultBlockSize,
                devices[2].GetBlocksCount()
            );
        });

        TVector<NProto::TDeviceOverride> deviceOverrides;
        deviceOverrides.emplace_back();
        deviceOverrides.back().SetDiskId("disk-1");
        deviceOverrides.back().SetDevice("uuid-1");
        deviceOverrides.back().SetBlocksCount(9_GB / DefaultBlockSize);
        deviceOverrides.emplace_back();
        deviceOverrides.back().SetDiskId("disk-1");
        deviceOverrides.back().SetDevice("uuid-2");
        deviceOverrides.back().SetBlocksCount(9_GB / DefaultBlockSize);

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UpdateConfig(state, db, { config1 }, deviceOverrides);
        });

        // overrides should affect this AllocateDisk call
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", {}, 28_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(S_ALREADY, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(3, devices.size());
            SortBy(devices, [] (const auto& d) {
                return d.GetDeviceUUID();
            });

            UNIT_ASSERT_VALUES_EQUAL("uuid-1", devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                9_GB / DefaultBlockSize,
                devices[0].GetBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL("uuid-2", devices[1].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                9_GB / DefaultBlockSize,
                devices[1].GetBlocksCount()
            );
            UNIT_ASSERT_VALUES_EQUAL("uuid-3", devices[2].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultBlockSize,
                devices[2].GetBlocksCount()
            );
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(state.DeallocateDisk(db, "disk-1"));
            CleanDevices(state, db);
        });

        // overrides should only affect disk-1
        for (ui32 i = 0; i < 3; ++i) {
            executor.WriteTx([&] (TDiskRegistryDatabase db) {
                TVector<TDeviceConfig> devices;
                auto error = AllocateDisk(db, state, "disk-2", {}, 28_GB, devices);
                UNIT_ASSERT_SUCCESS(error);
                UNIT_ASSERT_VALUES_EQUAL(i ? S_ALREADY : S_OK, error.GetCode());
                UNIT_ASSERT_VALUES_EQUAL(3, devices.size());
                SortBy(devices, [] (const auto& d) {
                    return d.GetDeviceUUID();
                });

                UNIT_ASSERT_VALUES_EQUAL("uuid-1", devices[0].GetDeviceUUID());
                UNIT_ASSERT_VALUES_EQUAL(
                    10_GB / DefaultBlockSize,
                    devices[0].GetBlocksCount()
                );
                UNIT_ASSERT_VALUES_EQUAL("uuid-2", devices[1].GetDeviceUUID());
                UNIT_ASSERT_VALUES_EQUAL(
                    10_GB / DefaultBlockSize,
                    devices[1].GetBlocksCount()
                );
                UNIT_ASSERT_VALUES_EQUAL("uuid-3", devices[2].GetDeviceUUID());
                UNIT_ASSERT_VALUES_EQUAL(
                    10_GB / DefaultBlockSize,
                    devices[2].GetBlocksCount()
                );
            });
        }
    }

    Y_UNIT_TEST(CanNotAllocateTooBigDisk)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto config1 = AgentConfig(1000, {
            Device("dev-1", "uuid-1", "rack-1")
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ config1 })
            .Build();

        TVector<TDeviceConfig> devices;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto error = AllocateDisk(db, state, "disk-id", {}, 50_GB, devices);

            UNIT_ASSERT(HasError(error));
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
            UNIT_ASSERT(devices.empty());
        });
    }

    Y_UNIT_TEST(ReAllocateDisk)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto config1 = AgentConfig(42, {
            Device("dev-1", "uuid-1", "rack-1")
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ config1 })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", {}, 10_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-2", {}, 10_GB, devices);
            UNIT_ASSERT(HasError(error));
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
            UNIT_ASSERT(devices.empty());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(state.DeallocateDisk(db, "disk-1"));
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> dirtyDevices;
            db.ReadDirtyDevices(dirtyDevices);

            for (const auto& uuid: dirtyDevices) {
                state.MarkDeviceAsClean(db, uuid);
            }
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-2", {}, 10_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
        });
    }

    Y_UNIT_TEST(ResizeDisk)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto config1 = AgentConfig(42, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1")
        });

        const auto config2 = AgentConfig(43, {
            Device("dev-1", "uuid-3", "rack-1"),
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ config1, config2 })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", {}, 10_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", {}, 10_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(S_ALREADY, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", {}, 30_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());

            UNIT_ASSERT_VALUES_EQUAL(3, devices.size());

            Sort(devices, TByDeviceUUID());

            UNIT_ASSERT_VALUES_EQUAL("uuid-1", devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-2", devices[1].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-3", devices[2].GetDeviceUUID());
        });
    }

    Y_UNIT_TEST(AllocateWithBlinkingAgent)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto stage1 = AgentConfig(1000, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1"),
            Device("dev-3", "uuid-3", "rack-1")
        });

        const auto stage2 = AgentConfig(1000, {
            Device("dev-2", "uuid-2", "rack-1")
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder().Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UpdateConfig(state, db, { stage1 });
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, stage1));
            CleanDevices(state, db);

            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", {}, 30_GB, devices); // use all devices
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(stage1.DevicesSize(), devices.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDiskStateUpdate> affectedDisks;
            THashMap<TString, ui64> disksToNotify;

            // lost dev-1 & dev-3
            UNIT_ASSERT_SUCCESS(state.RegisterAgent(
                db,
                stage2,
                Now(),
                &affectedDisks,
                &disksToNotify
            ));

            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());
            const auto& diskState = affectedDisks[0].State;
            UNIT_ASSERT_VALUES_EQUAL("disk-1", diskState.GetDiskId());
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ERROR, diskState.GetState());

            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-2", {}, 10_GB, devices);
            UNIT_ASSERT(HasError(error));
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
            UNIT_ASSERT(devices.empty());

            UNIT_ASSERT_VALUES_EQUAL(1, state.GetDiskStateUpdates().size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDiskStateUpdate> affectedDisks;
            THashMap<TString, ui64> disksToNotify;

            // restore dev-1 & dev-3 but with error state
            UNIT_ASSERT_SUCCESS(state.RegisterAgent(
                db,
                stage1,
                Now(),
                &affectedDisks,
                &disksToNotify
            ));

            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
            UNIT_ASSERT_VALUES_EQUAL(1, state.GetDiskStateUpdates().size());
        });

        // restore dev-1 to online state
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TMaybe<TDiskStateUpdate> affectedDisk;
            UNIT_ASSERT_SUCCESS(state.UpdateDeviceState(
                db,
                "uuid-1",
                NProto::DEVICE_STATE_ONLINE,
                Now(),
                "test",
                affectedDisk));

            UNIT_ASSERT(!affectedDisk);
            UNIT_ASSERT_VALUES_EQUAL(1, state.GetDiskStateUpdates().size());
        });

        // restore dev-3 to online state
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TMaybe<TDiskStateUpdate> affectedDisk;
            UNIT_ASSERT_SUCCESS(state.UpdateDeviceState(
                db,
                "uuid-3",
                NProto::DEVICE_STATE_ONLINE,
                Now(),
                "test",
                affectedDisk));

            UNIT_ASSERT(affectedDisk);

            const auto& diskState = affectedDisk->State;
            UNIT_ASSERT_VALUES_EQUAL("disk-1", diskState.GetDiskId());
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ONLINE, diskState.GetState());
            UNIT_ASSERT(diskState.GetStateMessage().empty());

            UNIT_ASSERT_VALUES_EQUAL(2, state.GetDiskStateUpdates().size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-2", {}, 20_GB, devices);
            UNIT_ASSERT(HasError(error));
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode()); // can't use dev-1 nor dev-3
            UNIT_ASSERT(devices.empty());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-2", {}, 10_GB, devices);
            UNIT_ASSERT(HasError(error));
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode()); // can't use dev-1 nor dev-3
            UNIT_ASSERT(devices.empty());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDiskStateUpdate> affectedDisks;
            THashMap<TString, ui64> disksToNotify;

            // lost dev-1 & dev-3
            UNIT_ASSERT_SUCCESS(state.RegisterAgent(
                db,
                stage2,
                Now(),
                &affectedDisks,
                &disksToNotify
            ));

            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());
            const auto& diskState = affectedDisks[0].State;
            UNIT_ASSERT_VALUES_EQUAL("disk-1", diskState.GetDiskId());
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ERROR, diskState.GetState());

            UNIT_ASSERT_SUCCESS(state.DeallocateDisk(db, "disk-1"));

            UNIT_ASSERT_VALUES_EQUAL(3, state.GetDiskStateUpdates().size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-2", {}, 30_GB, devices);
            // not enough online devices
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
        });

        // restore uuid-1 & uuid-3
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, stage1));

            {
                TMaybe<TDiskStateUpdate> affectedDisk;
                UNIT_ASSERT_SUCCESS(state.UpdateDeviceState(
                    db,
                    "uuid-1",
                    NProto::DEVICE_STATE_ONLINE,
                    Now(),
                    "test",
                    affectedDisk));
            }

            {
                TMaybe<TDiskStateUpdate> affectedDisk;
                UNIT_ASSERT_SUCCESS(state.UpdateDeviceState(
                    db,
                    "uuid-3",
                    NProto::DEVICE_STATE_ONLINE,
                    Now(),
                    "test",
                    affectedDisk));
            }
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-2", {}, 30_GB, devices);
            // not enough clean devices
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> dirtyDevices;
            db.ReadDirtyDevices(dirtyDevices);

            UNIT_ASSERT_VALUES_EQUAL(3, dirtyDevices.size());
        });

        UNIT_ASSERT_VALUES_EQUAL(3, state.GetDirtyDevices().size());

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, stage1)); // restore dev-1 & dev-3
            UNIT_ASSERT_VALUES_EQUAL(3, state.GetDirtyDevices().size());
            CleanDevices(state, db);

            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-2", {}, 30_GB, devices);
            UNIT_ASSERT_SUCCESS(error); // use all devices
            UNIT_ASSERT_VALUES_EQUAL(stage1.DevicesSize(), devices.size());
        });
    }

    Y_UNIT_TEST(AcquireDisk)
    {
        const auto config1 = AgentConfig(1, { Device("dev-1", "uuid-1", "rack-1")});

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ config1 })
            .WithDisks({ Disk("disk-1", {"uuid-1"}) })
            .Build();

        {
            TDiskInfo diskInfo;
            auto error = state.StartAcquireDisk("disk-1", diskInfo);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Devices.size());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                diskInfo.LogicalBlockSize);
        }

        {
            TDiskInfo diskInfo;
            auto error = state.StartAcquireDisk("disk-1", diskInfo);
            UNIT_ASSERT(HasError(error));
            UNIT_ASSERT_VALUES_EQUAL(E_REJECTED, error.GetCode());
        }

        state.FinishAcquireDisk("disk-1");

        {
            TDiskInfo diskInfo;
            auto error = state.StartAcquireDisk("disk-1", diskInfo);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, diskInfo.Devices.size());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                diskInfo.LogicalBlockSize);
        }

        {
            TDiskInfo diskInfo;
            auto error = state.StartAcquireDisk("disk-2", diskInfo);
            UNIT_ASSERT(HasError(error));
            UNIT_ASSERT_VALUES_EQUAL(E_NOT_FOUND, error.GetCode());
        }

        // TODO: test Acquire with migrations
    }

    Y_UNIT_TEST(ShouldGetDiskIds)
    {
        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithDisks({
                Disk("disk-1", {}),
                Disk("disk-2", {}),
                Disk("disk-3", {})
            })
            .Build();

        auto ids = state.GetDiskIds();

        UNIT_ASSERT_VALUES_EQUAL(ids.size(), 3);
        UNIT_ASSERT_VALUES_EQUAL(ids[0], "disk-1");
        UNIT_ASSERT_VALUES_EQUAL(ids[1], "disk-2");
        UNIT_ASSERT_VALUES_EQUAL(ids[2], "disk-3");
    }

    Y_UNIT_TEST(ShouldNotAllocateDirtyDevices)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1"),
            Device("dev-3", "uuid-3", "rack-1"),
            Device("dev-4", "uuid-4", "rack-1")
        });

        const auto agent2 = AgentConfig(2, {
            Device("dev-1", "uuid-5", "rack-1"),
            Device("dev-2", "uuid-6", "rack-1"),
            Device("dev-3", "uuid-7", "rack-1")
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1, agent2 })
            .WithDisks({ Disk("disk-1", {"uuid-1"}) })
            .WithDirtyDevices({ "uuid-4", "uuid-7" })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-2", {}, 50_GB, devices);
            // no memory: uuid-1 allocated for disk-1
            UNIT_ASSERT(HasError(error));
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(state.DeallocateDisk(db, "disk-1"));
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-2", {}, 50_GB, devices);
            // still not enough memory: uuid-1 marked as dirty
            UNIT_ASSERT(HasError(error));
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> dirtyDevices;
            db.ReadDirtyDevices(dirtyDevices);

            for (const auto& uuid: dirtyDevices) {
                state.MarkDeviceAsClean(db, uuid);
            }
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-2", {}, 50_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(5, devices.size());
        });
    }

    Y_UNIT_TEST(ShouldPreserveDirtyDevicesAfterRegistration)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1"),
            Device("dev-3", "uuid-3", "rack-1"),
        });

        // same agent but without uuid-2
        const auto agent2 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-3", "uuid-3", "rack-1"),
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1 })
            .WithDisks({ Disk("disk-1", {"uuid-1", "uuid-2"}) })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(state.DeallocateDisk(db, "disk-1"));
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> dirtyDevices;
            db.ReadDirtyDevices(dirtyDevices);

            UNIT_ASSERT_VALUES_EQUAL(2, dirtyDevices.size());
            Sort(dirtyDevices);
            UNIT_ASSERT_VALUES_EQUAL("uuid-1", dirtyDevices[0]);
            UNIT_ASSERT_VALUES_EQUAL("uuid-2", dirtyDevices[1]);
        });

        {
            auto dirtyDevices = state.GetDirtyDevices();

            UNIT_ASSERT_VALUES_EQUAL(2, dirtyDevices.size());
            Sort(dirtyDevices, TByDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1", dirtyDevices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-2", dirtyDevices[1].GetDeviceUUID());
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agent2));
        });

        {
            auto dirtyDevices = state.GetDirtyDevices();

            UNIT_ASSERT_VALUES_EQUAL(2, dirtyDevices.size());
            Sort(dirtyDevices, TByDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1", dirtyDevices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-2", dirtyDevices[1].GetDeviceUUID());
        }
    }

    Y_UNIT_TEST(ShouldGetDirtyDevices)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto agent1 = AgentConfig(1, { Device("dev-1", "uuid-1", "rack-1") });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1 })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT(state.MarkDeviceAsDirty(db, "uuid-1"));
        });

        {
            auto dirtyDevices = state.GetDirtyDevices();
            UNIT_ASSERT_VALUES_EQUAL(dirtyDevices.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(dirtyDevices[0].GetDeviceUUID(), "uuid-1");
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            state.MarkDeviceAsClean(db, "uuid-1");
        });

        UNIT_ASSERT_VALUES_EQUAL(state.GetDirtyDevices().size(), 0);
    }

    Y_UNIT_TEST(ShouldUpdateCounters)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto agent1 = AgentConfig(1000, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1") });

        const auto agent2 = AgentConfig(2000, {
            Device("dev-1", "uuid-3", "rack-2"),
            Device("dev-2", "uuid-4", "rack-2") });

        auto monitoring = CreateMonitoringServiceStub();
        auto diskRegistryGroup = monitoring->GetCounters()
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "disk_registry");

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .With(diskRegistryGroup)
            .WithKnownAgents({ agent1, agent2 })
            .Build();

        auto freeBytes = diskRegistryGroup->GetCounter("FreeBytes");
        auto totalBytes = diskRegistryGroup->GetCounter("TotalBytes");
        auto allocatedDisks = diskRegistryGroup->GetCounter("AllocatedDisks");
        auto allocatedDevices = diskRegistryGroup->GetCounter("AllocatedDevices");
        auto dirtyDevices = diskRegistryGroup->GetCounter("DirtyDevices");
        auto devicesInOnlineState = diskRegistryGroup->GetCounter("DevicesInOnlineState");
        auto devicesInWarningState = diskRegistryGroup->GetCounter("DevicesInWarningState");
        auto devicesInErrorState = diskRegistryGroup->GetCounter("DevicesInErrorState");
        auto agentsInOnlineState = diskRegistryGroup->GetCounter("AgentsInOnlineState");
        auto agentsInWarningState = diskRegistryGroup->GetCounter("AgentsInWarningState");
        auto agentsInUnavailableState = diskRegistryGroup->GetCounter("AgentsInUnavailableState");
        auto disksInOnlineState = diskRegistryGroup->GetCounter("DisksInOnlineState");
        auto disksInMigrationState = diskRegistryGroup->GetCounter("DisksInMigrationState");
        auto disksInTemporarilyUnavailableState =
            diskRegistryGroup->GetCounter("DisksInTemporarilyUnavailableState");
        auto disksInErrorState = diskRegistryGroup->GetCounter("DisksInErrorState");
        auto placementGroups = diskRegistryGroup->GetCounter("PlacementGroups");
        auto fullPlacementGroups = diskRegistryGroup->GetCounter("FullPlacementGroups");
        auto allocatedDisksInGroups = diskRegistryGroup->GetCounter("AllocatedDisksInGroups");

        auto agentCounters = diskRegistryGroup
            ->GetSubgroup("agent", "agent-1000");

        auto totalReadCount = agentCounters->GetCounter("ReadCount");
        auto totalReadBytes = agentCounters->GetCounter("ReadBytes");
        auto totalWriteCount = agentCounters->GetCounter("WriteCount");
        auto totalWriteBytes = agentCounters->GetCounter("WriteBytes");
        auto totalZeroCount = agentCounters->GetCounter("ZeroCount");
        auto totalZeroBytes = agentCounters->GetCounter("ZeroBytes");

        auto device = agentCounters->GetSubgroup("device", "uuid-1");

        auto timePercentiles = device->GetSubgroup("percentiles", "Time");
        auto p90 = timePercentiles->GetCounter("90");

        auto readCount = device->GetCounter("ReadCount");
        auto readBytes = device->GetCounter("ReadBytes");
        auto writeCount = device->GetCounter("WriteCount");
        auto writeBytes = device->GetCounter("WriteBytes");
        auto zeroCount = device->GetCounter("ZeroCount");
        auto zeroBytes = device->GetCounter("ZeroBytes");

        UNIT_ASSERT_VALUES_EQUAL(p90->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(totalReadCount->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(totalReadBytes->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(totalWriteCount->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(totalWriteBytes->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(totalZeroCount->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(totalZeroBytes->Val(), 0);

        UNIT_ASSERT_VALUES_EQUAL(readCount->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(readBytes->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(writeCount->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(writeBytes->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(zeroCount->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(zeroBytes->Val(), 0);

        auto makeDeviceStats = [] (
            const TString& uuid,
            std::pair<ui64, ui64> r,
            std::pair<ui64, ui64> w,
            std::pair<ui64, ui64> z)
        {
            NProto::TDeviceStats stats;
            stats.SetDeviceUUID(uuid);

            stats.SetNumReadOps(r.first);
            stats.SetBytesRead(r.second);

            stats.SetNumWriteOps(w.first);
            stats.SetBytesWritten(w.second);

            stats.SetNumZeroOps(z.first);
            stats.SetBytesZeroed(z.second);

            {
                auto& bucket = *stats.AddHistogramBuckets();
                bucket.SetValue(1000);
                bucket.SetCount(90);
            }

            {
                auto& bucket = *stats.AddHistogramBuckets();
                bucket.SetValue(10000);
                bucket.SetCount(4);
            }

            {
                auto& bucket = *stats.AddHistogramBuckets();
                bucket.SetValue(100000);
                bucket.SetCount(5);
            }

            {
                auto& bucket = *stats.AddHistogramBuckets();
                bucket.SetValue(10000000);
                bucket.SetCount(1);
            }

            return stats;
        };

        {
            NProto::TAgentStats agentStats;
            agentStats.SetNodeId(1000);

            *agentStats.AddDeviceStats() = makeDeviceStats(
                "uuid-1", { 200, 10000 }, { 100, 5000 }, {10, 1000});

            *agentStats.AddDeviceStats() = makeDeviceStats(
                "uuid-2", { 100, 40000 }, { 20, 1000 }, {20, 2000});

            state.UpdateAgentCounters(agentStats);
        }

        UNIT_ASSERT_VALUES_EQUAL(p90->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(totalReadCount->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(totalReadBytes->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(totalWriteCount->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(totalWriteBytes->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(totalZeroCount->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(totalZeroBytes->Val(), 0);

        UNIT_ASSERT_VALUES_EQUAL(readCount->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(readBytes->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(writeCount->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(writeBytes->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(zeroCount->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(zeroBytes->Val(), 0);

        state.PublishCounters(Now());

        UNIT_ASSERT_VALUES_EQUAL(p90->Val(), 1000);
        UNIT_ASSERT_VALUES_EQUAL(totalReadCount->Val(), 300);
        UNIT_ASSERT_VALUES_EQUAL(totalReadBytes->Val(), 50000);
        UNIT_ASSERT_VALUES_EQUAL(totalWriteCount->Val(), 120);
        UNIT_ASSERT_VALUES_EQUAL(totalWriteBytes->Val(), 6000);
        UNIT_ASSERT_VALUES_EQUAL(totalZeroCount->Val(), 30);
        UNIT_ASSERT_VALUES_EQUAL(totalZeroBytes->Val(), 3000);

        UNIT_ASSERT_VALUES_EQUAL(readCount->Val(), 200);
        UNIT_ASSERT_VALUES_EQUAL(readBytes->Val(), 10000);
        UNIT_ASSERT_VALUES_EQUAL(writeCount->Val(), 100);
        UNIT_ASSERT_VALUES_EQUAL(writeBytes->Val(), 5000);
        UNIT_ASSERT_VALUES_EQUAL(zeroCount->Val(), 10);
        UNIT_ASSERT_VALUES_EQUAL(zeroBytes->Val(), 1000);

        UNIT_ASSERT_VALUES_EQUAL(dirtyDevices->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(freeBytes->Val(), 40_GB);
        UNIT_ASSERT_VALUES_EQUAL(totalBytes->Val(), 40_GB);
        UNIT_ASSERT_VALUES_EQUAL(allocatedDisks->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(allocatedDevices->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(devicesInOnlineState->Val(), 4);
        UNIT_ASSERT_VALUES_EQUAL(devicesInWarningState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(devicesInErrorState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(agentsInOnlineState->Val(), 2);
        UNIT_ASSERT_VALUES_EQUAL(agentsInWarningState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(agentsInUnavailableState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(disksInOnlineState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(disksInMigrationState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(disksInTemporarilyUnavailableState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(disksInErrorState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(placementGroups->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(fullPlacementGroups->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(allocatedDisksInGroups->Val(), 0);

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(state.CreatePlacementGroup(db, "group-1"));
        });

        TString deviceToBreak;
        TString agentToBreak;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", "group-1", 20_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(2, devices.size());
            deviceToBreak = devices[0].GetDeviceUUID();
            agentToBreak = devices[0].GetNodeId() == 1000 ? "agent-2000" : "agent-1000";
        });

        state.PublishCounters(Now());

        UNIT_ASSERT_VALUES_EQUAL(freeBytes->Val(), 20_GB);
        UNIT_ASSERT_VALUES_EQUAL(totalBytes->Val(), 40_GB);
        UNIT_ASSERT_VALUES_EQUAL(allocatedDisks->Val(), 1);
        UNIT_ASSERT_VALUES_EQUAL(allocatedDevices->Val(), 2);
        UNIT_ASSERT_VALUES_EQUAL(devicesInOnlineState->Val(), 4);
        UNIT_ASSERT_VALUES_EQUAL(devicesInWarningState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(devicesInErrorState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(agentsInOnlineState->Val(), 2);
        UNIT_ASSERT_VALUES_EQUAL(agentsInWarningState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(agentsInUnavailableState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(disksInOnlineState->Val(), 1);
        UNIT_ASSERT_VALUES_EQUAL(disksInMigrationState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(disksInTemporarilyUnavailableState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(disksInErrorState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(placementGroups->Val(), 1);
        UNIT_ASSERT_VALUES_EQUAL(fullPlacementGroups->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(allocatedDisksInGroups->Val(), 1);

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-2", {}, 10_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
        });

        state.PublishCounters(Now());

        UNIT_ASSERT_VALUES_EQUAL(freeBytes->Val(), 10_GB);
        UNIT_ASSERT_VALUES_EQUAL(totalBytes->Val(), 40_GB);
        UNIT_ASSERT_VALUES_EQUAL(allocatedDisks->Val(), 2);
        UNIT_ASSERT_VALUES_EQUAL(allocatedDevices->Val(), 3);
        UNIT_ASSERT_VALUES_EQUAL(devicesInOnlineState->Val(), 4);
        UNIT_ASSERT_VALUES_EQUAL(devicesInWarningState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(devicesInErrorState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(agentsInOnlineState->Val(), 2);
        UNIT_ASSERT_VALUES_EQUAL(agentsInWarningState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(agentsInUnavailableState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(disksInOnlineState->Val(), 2);
        UNIT_ASSERT_VALUES_EQUAL(disksInMigrationState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(disksInTemporarilyUnavailableState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(disksInErrorState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(placementGroups->Val(), 1);
        UNIT_ASSERT_VALUES_EQUAL(fullPlacementGroups->Val(), 1);
        UNIT_ASSERT_VALUES_EQUAL(allocatedDisksInGroups->Val(), 1);

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            auto error = state.UpdateDeviceState(
                db,
                deviceToBreak,
                NProto::DEVICE_STATE_ERROR,
                Now(),
                "test",
                affectedDisk
            );

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());

            UpdateAgentState(state, db, agentToBreak, NProto::AGENT_STATE_WARNING);
        });

        state.PublishCounters(Now());

        UNIT_ASSERT_VALUES_EQUAL(freeBytes->Val(), 0); // agent in warning state
        UNIT_ASSERT_VALUES_EQUAL(totalBytes->Val(), 40_GB);
        UNIT_ASSERT_VALUES_EQUAL(allocatedDisks->Val(), 2);
        UNIT_ASSERT_VALUES_EQUAL(allocatedDevices->Val(), 3);
        UNIT_ASSERT_VALUES_EQUAL(devicesInOnlineState->Val(), 3);
        UNIT_ASSERT_VALUES_EQUAL(devicesInWarningState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(devicesInErrorState->Val(), 1);
        UNIT_ASSERT_VALUES_EQUAL(agentsInOnlineState->Val(), 1);
        UNIT_ASSERT_VALUES_EQUAL(agentsInWarningState->Val(), 1);
        UNIT_ASSERT_VALUES_EQUAL(agentsInUnavailableState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(disksInOnlineState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(disksInMigrationState->Val(), 1);
        UNIT_ASSERT_VALUES_EQUAL(disksInTemporarilyUnavailableState->Val(), 0);
        UNIT_ASSERT_VALUES_EQUAL(disksInErrorState->Val(), 1);
        UNIT_ASSERT_VALUES_EQUAL(placementGroups->Val(), 1);
        UNIT_ASSERT_VALUES_EQUAL(fullPlacementGroups->Val(), 1);
        UNIT_ASSERT_VALUES_EQUAL(allocatedDisksInGroups->Val(), 1);
    }

    Y_UNIT_TEST(ShouldRejectBrokenStats)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        auto config1 = AgentConfig(1000, {
            Device("dev-1", "uuid-1", "rack-1"),
        });

        auto monitoring = CreateMonitoringServiceStub();
        auto diskRegistryGroup = monitoring->GetCounters()
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "disk_registry");

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .With(diskRegistryGroup)
            .WithConfig({ config1 })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, config1));
        });

        state.PublishCounters(Now());

        {
            NProto::TAgentStats stats;
            stats.SetNodeId(1000);
            auto* d = stats.AddDeviceStats();
            d->SetDeviceUUID("garbage");

            auto error = state.UpdateAgentCounters(stats);
            UNIT_ASSERT_VALUES_EQUAL(E_ARGUMENT, error.GetCode());
        }

        state.PublishCounters(Now());
    }

    Y_UNIT_TEST(ShouldRemoveAgentWithSameId)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        auto config1 = AgentConfig(1121, {
            Device("dev-1", "uuid-1", "rack-1"),
        });

        auto config2 = config1;
        config2.SetNodeId(1400);

        TDiskRegistryState state = TDiskRegistryStateBuilder().Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UpdateConfig(state, db, { config1 });
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, config1));
        });

        {
            const auto agents = state.GetAgents();

            UNIT_ASSERT_VALUES_EQUAL(1, agents.size());
            UNIT_ASSERT_VALUES_EQUAL(1121, agents[0].GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL("agent-1121", agents[0].GetAgentId());
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, config2));
        });

        {
            const auto agents = state.GetAgents();

            UNIT_ASSERT_VALUES_EQUAL(1, agents.size());
            UNIT_ASSERT_VALUES_EQUAL(1400, agents[0].GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL("agent-1121", agents[0].GetAgentId());
        }

        {
            const auto device = state.GetDevice("uuid-1");

            UNIT_ASSERT_VALUES_EQUAL("uuid-1", device.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(1400, device.GetNodeId());
        }
    }

    Y_UNIT_TEST(ShouldRejectUnknownDevices)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        auto agentConfig = AgentConfig(1121, {
            Device("dev-1", "uuid-1", "rack-1"),
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UpdateConfig(state, db, { agentConfig });

            UNIT_ASSERT(state.MarkDeviceAsDirty(db, "uuid-1"));
            UNIT_ASSERT(!state.MarkDeviceAsDirty(db, "unknown"));
        });

        UNIT_ASSERT(state.GetDirtyDevices().empty());

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            RegisterAgent(state, db, agentConfig);
        });

        const auto& devices = state.GetDirtyDevices();

        UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
        UNIT_ASSERT_VALUES_EQUAL("uuid-1", devices[0].GetDeviceUUID());
        UNIT_ASSERT_VALUES_EQUAL(1121, devices[0].GetNodeId());
    }

    Y_UNIT_TEST(ShouldSupportPlacementGroups)
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
            UNIT_ASSERT_SUCCESS(state.CreatePlacementGroup(db, "group-1"));
        });

        const auto& groups = state.GetPlacementGroups();
        UNIT_ASSERT_VALUES_EQUAL(1, groups.size());
        UNIT_ASSERT_VALUES_EQUAL("group-1", groups.begin()->first);

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", {}, 10_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-1", devices[0].GetDeviceName());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-2", {}, 20_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(2, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-4", devices[0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-5", devices[1].GetDeviceName());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-3", {}, 10_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-7", devices[0].GetDeviceName());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-4", {}, 30_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(3, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-10", devices[0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-11", devices[1].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-12", devices[2].GetDeviceName());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-5", {}, 20_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(2, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-2", devices[0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-3", devices[1].GetDeviceName());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-6", {}, 30_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(3, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-8", devices[0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-9", devices[1].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-6", devices[2].GetDeviceName());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> disksToAdd = {"disk-1", "disk-5", "disk-3"};
            auto error = state.AlterPlacementGroupMembership(
                db,
                "group-1",
                1,
                disksToAdd,
                {}
            );
            UNIT_ASSERT_VALUES_EQUAL(E_PRECONDITION_FAILED, error.GetCode());
            ASSERT_VECTORS_EQUAL(
                TVector<TString>{"disk-5"},
                disksToAdd
            );

            disksToAdd = {"disk-1"};
            error = state.AlterPlacementGroupMembership(
                db,
                "group-1",
                1,
                disksToAdd,
                {}
            );
            UNIT_ASSERT_SUCCESS(error);
            ASSERT_VECTORS_EQUAL(TVector<TString>{}, disksToAdd);

            disksToAdd = {"disk-5"};
            error = state.AlterPlacementGroupMembership(
                db,
                "group-1",
                2,
                disksToAdd,
                {}
            );
            UNIT_ASSERT_VALUES_EQUAL(E_PRECONDITION_FAILED, error.GetCode());
            ASSERT_VECTORS_EQUAL(
                TVector<TString>{"disk-5"},
                disksToAdd
            );

            disksToAdd = {"disk-3"};
            error = state.AlterPlacementGroupMembership(
                db,
                "group-1",
                1,
                disksToAdd,
                {}
            );
            UNIT_ASSERT_VALUES_EQUAL(E_CONFIG_VERSION_MISMATCH, error.GetCode());

            disksToAdd = {"disk-3"};
            error = state.AlterPlacementGroupMembership(
                db,
                "group-1",
                2,
                disksToAdd,
                {}
            );
            UNIT_ASSERT_SUCCESS(error);
            ASSERT_VECTORS_EQUAL(TVector<TString>{}, disksToAdd);

            disksToAdd = {"disk-4", "disk-5"};
            error = state.AlterPlacementGroupMembership(
                db,
                "group-1",
                3,
                disksToAdd,
                {}
            );
            UNIT_ASSERT_VALUES_EQUAL_C(
                E_PRECONDITION_FAILED,
                error.GetCode(),
                error.GetMessage()
            );
            ASSERT_VECTORS_EQUAL(
                TVector<TString>{"disk-5"},
                disksToAdd
            );

            disksToAdd = {"disk-4"};
            error = state.AlterPlacementGroupMembership(
                db,
                "group-1",
                3,
                disksToAdd,
                {}
            );
            UNIT_ASSERT_SUCCESS(error);
            ASSERT_VECTORS_EQUAL(TVector<TString>{}, disksToAdd);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> disksToAdd = {"disk-2"};
            auto error = state.AlterPlacementGroupMembership(
                db,
                "group-1",
                4,
                disksToAdd,
                {}
            );
            UNIT_ASSERT_VALUES_EQUAL(E_BS_RESOURCE_EXHAUSTED, error.GetCode());
            ASSERT_VECTORS_EQUAL(
                TVector<TString>{"disk-2"},
                disksToAdd
            );

            UNIT_ASSERT_VALUES_EQUAL(0, state.GetBrokenDisks().size());
        });

        auto* group = state.FindPlacementGroup("group-1");
        UNIT_ASSERT(group);
        UNIT_ASSERT_VALUES_EQUAL("group-1", group->GetGroupId());
        UNIT_ASSERT_VALUES_EQUAL(3, group->DisksSize());
        UNIT_ASSERT_VALUES_EQUAL("disk-1", group->GetDisks(0).GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL("disk-3", group->GetDisks(1).GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL("disk-4", group->GetDisks(2).GetDiskId());

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> disksToAdd = {"disk-2"};
            auto error = state.AlterPlacementGroupMembership(
                db,
                "group-1",
                4,
                disksToAdd,
                {"disk-3"}
            );
            UNIT_ASSERT_SUCCESS(error);
            ASSERT_VECTORS_EQUAL(TVector<TString>{}, disksToAdd);
        });

        group = state.FindPlacementGroup("group-1");
        UNIT_ASSERT(group);
        UNIT_ASSERT_VALUES_EQUAL("group-1", group->GetGroupId());
        UNIT_ASSERT_VALUES_EQUAL(3, group->DisksSize());
        UNIT_ASSERT_VALUES_EQUAL("disk-1", group->GetDisks(0).GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL("disk-4", group->GetDisks(1).GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL("disk-2", group->GetDisks(2).GetDiskId());

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            state.DeallocateDisk(db, "disk-4");
        });

        group = state.FindPlacementGroup("group-1");
        UNIT_ASSERT(group);
        UNIT_ASSERT_VALUES_EQUAL("group-1", group->GetGroupId());
        UNIT_ASSERT_VALUES_EQUAL(2, group->DisksSize());
        UNIT_ASSERT_VALUES_EQUAL("disk-1", group->GetDisks(0).GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL("disk-2", group->GetDisks(1).GetDiskId());

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> disksToAdd = {};
            auto error = state.AlterPlacementGroupMembership(
                db,
                "group-1",
                6,
                disksToAdd,
                {"disk-1"}
            );
            group = state.FindPlacementGroup("group-1");
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, group->DisksSize());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> disksToAdd = {};
            auto error = state.AlterPlacementGroupMembership(
                db,
                "group-1",
                7,
                disksToAdd,
                {"disk-2"}
            );
            group = state.FindPlacementGroup("group-1");
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(0, group->DisksSize());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> affectedDisks;
            UNIT_ASSERT_SUCCESS(state.DestroyPlacementGroup(db, "group-1", affectedDisks));
        });

        UNIT_ASSERT_VALUES_EQUAL(0, groups.size());

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            state.DeallocateDisk(db, "disk-1");
        });
    }

    Y_UNIT_TEST(ShouldAllocateInPlacementGroup)
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
                agentConfig3
            })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(state.CreatePlacementGroup(db, "group-1"));
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", "group-1", 10_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-1", devices[0].GetDeviceName());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-2", "group-1", 20_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(2, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-4", devices[0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-5", devices[1].GetDeviceName());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-3", "group-1", 10_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-7", devices[0].GetDeviceName());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-4", "group-1", 10_GB, devices);
            UNIT_ASSERT_VALUES_EQUAL(E_BS_RESOURCE_EXHAUSTED, error.GetCode());
        });

        auto* group = state.FindPlacementGroup("group-1");
        UNIT_ASSERT(group);
        UNIT_ASSERT_VALUES_EQUAL("group-1", group->GetGroupId());
        UNIT_ASSERT_VALUES_EQUAL(3, group->DisksSize());
        UNIT_ASSERT_VALUES_EQUAL("disk-1", group->GetDisks(0).GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL("disk-2", group->GetDisks(1).GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL("disk-3", group->GetDisks(2).GetDiskId());

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-3", "group-1", 40_GB, devices);
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            // group hint should be ignored since we already know this disk's
            // actual group
            auto error = AllocateDisk(db, state, "disk-3", "group-2", 30_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(3, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-7", devices[0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-8", devices[1].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-9", devices[2].GetDeviceName());
        });

        group = state.FindPlacementGroup("group-2");
        UNIT_ASSERT(!group);

        group = state.FindPlacementGroup("group-1");
        UNIT_ASSERT(group);
        UNIT_ASSERT_VALUES_EQUAL("group-1", group->GetGroupId());
        UNIT_ASSERT_VALUES_EQUAL(3, group->DisksSize());
        UNIT_ASSERT_VALUES_EQUAL("disk-1", group->GetDisks(0).GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL("disk-2", group->GetDisks(1).GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL("disk-3", group->GetDisks(2).GetDiskId());

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto error = state.DeallocateDisk(db, "disk-3");
            UNIT_ASSERT_SUCCESS(error);

            for (const auto& d: state.GetDirtyDevices()) {
                state.MarkDeviceAsClean(db, d.GetDeviceUUID());
            }
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-4", "group-1", 40_GB, devices);
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-4", "group-1", 30_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(3, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-7", devices[0].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-8", devices[1].GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-9", devices[2].GetDeviceName());
        });

        group = state.FindPlacementGroup("group-1");
        UNIT_ASSERT(group);
        UNIT_ASSERT_VALUES_EQUAL("group-1", group->GetGroupId());
        UNIT_ASSERT_VALUES_EQUAL(3, group->DisksSize());
        UNIT_ASSERT_VALUES_EQUAL("disk-1", group->GetDisks(0).GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL("disk-2", group->GetDisks(1).GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL("disk-4", group->GetDisks(2).GetDiskId());

        auto racksInfo = state.GatherRacksInfo();
        UNIT_ASSERT_VALUES_EQUAL(3, racksInfo.size());

        UNIT_ASSERT_VALUES_EQUAL(1, racksInfo[0].AgentInfos.size());
        UNIT_ASSERT_VALUES_EQUAL(
            agentConfig1.GetAgentId(),
            racksInfo[0].AgentInfos[0].AgentId
        );
        UNIT_ASSERT_VALUES_EQUAL(2, racksInfo[0].AgentInfos[0].FreeDevices);
        UNIT_ASSERT_VALUES_EQUAL(1, racksInfo[0].AgentInfos[0].AllocatedDevices);
        UNIT_ASSERT_VALUES_EQUAL(0, racksInfo[0].AgentInfos[0].BrokenDevices);

        UNIT_ASSERT_VALUES_EQUAL(1, racksInfo[1].AgentInfos.size());
        UNIT_ASSERT_VALUES_EQUAL(
            agentConfig2.GetAgentId(),
            racksInfo[1].AgentInfos[0].AgentId
        );
        UNIT_ASSERT_VALUES_EQUAL(1, racksInfo[1].AgentInfos[0].FreeDevices);
        UNIT_ASSERT_VALUES_EQUAL(2, racksInfo[1].AgentInfos[0].AllocatedDevices);
        UNIT_ASSERT_VALUES_EQUAL(0, racksInfo[1].AgentInfos[0].BrokenDevices);

        UNIT_ASSERT_VALUES_EQUAL(1, racksInfo[2].AgentInfos.size());
        UNIT_ASSERT_VALUES_EQUAL(
            agentConfig3.GetAgentId(),
            racksInfo[2].AgentInfos[0].AgentId
        );
        UNIT_ASSERT_VALUES_EQUAL(0, racksInfo[2].AgentInfos[0].FreeDevices);
        UNIT_ASSERT_VALUES_EQUAL(3, racksInfo[2].AgentInfos[0].AllocatedDevices);
        UNIT_ASSERT_VALUES_EQUAL(0, racksInfo[2].AgentInfos[0].BrokenDevices);
    }

    Y_UNIT_TEST(ShouldNotAllocateMoreThanLimitInPlacementGroup)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        auto agentConfig1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
        });

        auto agentConfig2 = AgentConfig(2, {
            Device("dev-2", "uuid-2", "rack-2"),
        });

        auto agentConfig3 = AgentConfig(3, {
            Device("dev-3", "uuid-3", "rack-3"),
        });

        auto agentConfig4 = AgentConfig(4, {
            Device("dev-4", "uuid-4", "rack-4"),
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
            UNIT_ASSERT_SUCCESS(state.CreatePlacementGroup(db, "group-1"));
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", "group-1", 10_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-1", devices[0].GetDeviceName());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-2", "group-1", 10_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-2", devices[0].GetDeviceName());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-3", "group-1", 10_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-3", devices[0].GetDeviceName());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(
                db,
                state,
                "disk-4",
                "group-1",
                10_GB,
                devices,
                TInstant::Seconds(100)
            );
            UNIT_ASSERT_VALUES_EQUAL(E_BS_RESOURCE_EXHAUSTED, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(1, state.GetBrokenDisks().size());
            UNIT_ASSERT_VALUES_EQUAL("disk-4", state.GetBrokenDisks()[0].DiskId);
            UNIT_ASSERT_VALUES_EQUAL(
                TInstant::Seconds(105),
                state.GetBrokenDisks()[0].TsToDestroy
            );

            error = AllocateDisk(
                db,
                state,
                "disk-5",
                "group-1",
                10_GB,
                devices,
                TInstant::Seconds(101)
            );
            UNIT_ASSERT_VALUES_EQUAL(E_BS_RESOURCE_EXHAUSTED, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(2, state.GetBrokenDisks().size());
            UNIT_ASSERT_VALUES_EQUAL("disk-4", state.GetBrokenDisks()[0].DiskId);
            UNIT_ASSERT_VALUES_EQUAL(
                TInstant::Seconds(105),
                state.GetBrokenDisks()[0].TsToDestroy
            );
            UNIT_ASSERT_VALUES_EQUAL("disk-5", state.GetBrokenDisks()[1].DiskId);
            UNIT_ASSERT_VALUES_EQUAL(
                TInstant::Seconds(106),
                state.GetBrokenDisks()[1].TsToDestroy
            );
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TBrokenDiskInfo> diskInfos;
            UNIT_ASSERT(db.ReadBrokenDisks(diskInfos));
            UNIT_ASSERT_VALUES_EQUAL(2, diskInfos.size());
            UNIT_ASSERT_VALUES_EQUAL("disk-4", diskInfos[0].DiskId);
            UNIT_ASSERT_VALUES_EQUAL("disk-5", diskInfos[1].DiskId);

            state.DeleteBrokenDisks(db);
            UNIT_ASSERT_VALUES_EQUAL(0, state.GetBrokenDisks().size());
        });

        executor.ReadTx([&] (TDiskRegistryDatabase db) {
            TVector<TBrokenDiskInfo> diskInfos;
            UNIT_ASSERT(db.ReadBrokenDisks(diskInfos));
            UNIT_ASSERT_VALUES_EQUAL(0, diskInfos.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-4", "", 10_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-4", devices[0].GetDeviceName());
        });

        auto* group = state.FindPlacementGroup("group-1");
        UNIT_ASSERT(group);
        UNIT_ASSERT_VALUES_EQUAL("group-1", group->GetGroupId());
        UNIT_ASSERT_VALUES_EQUAL(3, group->DisksSize());
        UNIT_ASSERT_VALUES_EQUAL("disk-1", group->GetDisks(0).GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL("disk-2", group->GetDisks(1).GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL("disk-3", group->GetDisks(2).GetDiskId());

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(state.DeallocateDisk(db, "disk-4"));
            CleanDevices(state, db);

            NProto::TPlacementGroupSettings settings;
            settings.SetMaxDisksInGroup(4);
            state.UpdatePlacementGroupSettings(db, "group-1", 4, std::move(settings));

            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(
                db,
                state,
                "disk-4",
                "group-1",
                10_GB,
                devices,
                TInstant::Seconds(102)
            );

            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-4", devices[0].GetDeviceName());

            error = AllocateDisk(
                db,
                state,
                "disk-5",
                "group-1",
                10_GB,
                devices,
                TInstant::Seconds(103)
            );
            UNIT_ASSERT_VALUES_EQUAL(E_BS_RESOURCE_EXHAUSTED, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(1, state.GetBrokenDisks().size());
            UNIT_ASSERT_VALUES_EQUAL("disk-5", state.GetBrokenDisks()[0].DiskId);
            UNIT_ASSERT_VALUES_EQUAL(
                TInstant::Seconds(108),
                state.GetBrokenDisks()[0].TsToDestroy
            );
        });

        UNIT_ASSERT_VALUES_EQUAL("group-1", group->GetGroupId());
        UNIT_ASSERT_VALUES_EQUAL(4, group->DisksSize());
        UNIT_ASSERT_VALUES_EQUAL("disk-1", group->GetDisks(0).GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL("disk-2", group->GetDisks(1).GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL("disk-3", group->GetDisks(2).GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL("disk-4", group->GetDisks(3).GetDiskId());
    }

    Y_UNIT_TEST(ShouldNotAddOneDiskToTwoGroups)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        auto agentConfig1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agentConfig1 })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(state.CreatePlacementGroup(db, "group-1"));
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(state.CreatePlacementGroup(db, "group-2"));
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", "group-1", 10_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("dev-1", devices[0].GetDeviceName());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> disksToAdd = {"disk-1"};
            auto error = state.AlterPlacementGroupMembership(
                db,
                "group-1",
                2,
                disksToAdd,
                {}
            );
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> disksToAdd = {"disk-1"};
            auto error = state.AlterPlacementGroupMembership(
                db,
                "group-2",
                1,
                disksToAdd,
                {}
            );
            UNIT_ASSERT_VALUES_EQUAL(E_PRECONDITION_FAILED, error.GetCode());
        });
    }

    Y_UNIT_TEST(ShouldUpdateAgentState)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1")
        });

        const auto agent2 = AgentConfig(2, {
            Device("dev-1", "uuid-3", "rack-2"),
            Device("dev-2", "uuid-4", "rack-2"),
            Device("dev-3", "uuid-5", "rack-2")
        });

        const auto agent3 = AgentConfig(3, {
            Device("dev-1", "uuid-6", "rack-3"),
            Device("dev-2", "uuid-7", "rack-3")
        });

        ui64 lastSeqNo = 0;

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1, agent2, agent3 })
            .WithDisks({
                Disk("disk-1", {"uuid-1"}),
                Disk("disk-2", {"uuid-3"}),
                Disk("disk-3", {"uuid-2", "uuid-4"}),
            })
            .With(lastSeqNo)
            .Build();

        // #1 : online -> warning
        // disk-1 : online -> migration
        // disk-3 : online -> migration
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                Now(),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());

            UNIT_ASSERT_VALUES_EQUAL(2, affectedDisks.size());
            Sort(affectedDisks, TByDiskId());

            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_MIGRATION, affectedDisks[0]);
            UNIT_ASSERT_DISK_STATE("disk-3", DISK_STATE_MIGRATION, affectedDisks[1]);

            UNIT_ASSERT_VALUES_UNEQUAL(affectedDisks[0].SeqNo, affectedDisks[1].SeqNo);
            lastSeqNo = std::max(affectedDisks[0].SeqNo, affectedDisks[1].SeqNo);

            UNIT_ASSERT_VALUES_EQUAL(2, state.GetDiskStateUpdates().size());
        });

        UNIT_ASSERT_VALUES_EQUAL(1, lastSeqNo);

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                Now(),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_ALREADY, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        // #1 : warning -> unavailable
        // disk-1 : online -> unavailable
        // disk-3 : online -> unavailable
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_UNAVAILABLE,
                Now(),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());

            UNIT_ASSERT_VALUES_EQUAL(2, affectedDisks.size());
            Sort(affectedDisks, TByDiskId());

            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_TEMPORARILY_UNAVAILABLE, affectedDisks[0]);
            UNIT_ASSERT_DISK_STATE("disk-3", DISK_STATE_TEMPORARILY_UNAVAILABLE, affectedDisks[1]);

            UNIT_ASSERT_GT(affectedDisks[0].SeqNo, lastSeqNo);
            UNIT_ASSERT_VALUES_UNEQUAL(affectedDisks[0].SeqNo, affectedDisks[1].SeqNo);
            lastSeqNo = std::max(affectedDisks[0].SeqNo, affectedDisks[1].SeqNo);

            UNIT_ASSERT_VALUES_EQUAL(4, state.GetDiskStateUpdates().size());
        });

        UNIT_ASSERT_VALUES_EQUAL(3, lastSeqNo);

        // #3 : online -> unavailable
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent3.GetAgentId(),
                NProto::AGENT_STATE_UNAVAILABLE,
                Now(),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        // #2 : online -> online
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent2.GetAgentId(),
                NProto::AGENT_STATE_ONLINE,
                Now(),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_ALREADY, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        // #2 : online -> unavailable
        // disk-2 : online -> unavailable
        // disk-3 : already in unavailable
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent2.GetAgentId(),
                NProto::AGENT_STATE_UNAVAILABLE,
                Now(),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());

            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());

            UNIT_ASSERT_DISK_STATE("disk-2", DISK_STATE_TEMPORARILY_UNAVAILABLE, affectedDisks[0]);
            lastSeqNo = affectedDisks[0].SeqNo;

            UNIT_ASSERT_VALUES_EQUAL(5, state.GetDiskStateUpdates().size());
        });

        UNIT_ASSERT_VALUES_EQUAL(4, lastSeqNo);

        // #1 : unavailable -> online
        // disk-1 : unavailable -> online
        // disk-3 : #2 still in unavailable
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_ONLINE,
                Now(),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());

            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_ONLINE, affectedDisks[0]);
            lastSeqNo = affectedDisks[0].SeqNo;

            UNIT_ASSERT_VALUES_EQUAL(6, state.GetDiskStateUpdates().size());
        });

        UNIT_ASSERT_VALUES_EQUAL(5, lastSeqNo);

        // #2 : unavailable -> online
        // disk-2 : unavailable -> online
        // disk-3 : unavailable -> online
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent2.GetAgentId(),
                NProto::AGENT_STATE_ONLINE,
                Now(),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(2, affectedDisks.size());
            Sort(affectedDisks, TByDiskId());

            UNIT_ASSERT_DISK_STATE("disk-2", DISK_STATE_ONLINE, affectedDisks[0]);
            UNIT_ASSERT_DISK_STATE("disk-3", DISK_STATE_ONLINE, affectedDisks[1]);

            UNIT_ASSERT_GT(affectedDisks[0].SeqNo, lastSeqNo);
            UNIT_ASSERT_VALUES_UNEQUAL(affectedDisks[0].SeqNo, affectedDisks[1].SeqNo);
            lastSeqNo = std::max(affectedDisks[0].SeqNo, affectedDisks[1].SeqNo);

            UNIT_ASSERT_VALUES_EQUAL(8, state.GetDiskStateUpdates().size());
        });

        UNIT_ASSERT_VALUES_EQUAL(7, lastSeqNo);

        executor.ReadTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<NProto::TAgentConfig> agents;
            TVector<NProto::TDiskConfig> disks;
            ui64 seqNo = 0;

            db.ReadLastDiskStateSeqNo(seqNo);
            UNIT_ASSERT_VALUES_EQUAL(lastSeqNo + 1, seqNo);

            db.ReadAgents(agents);
            UNIT_ASSERT_VALUES_EQUAL(3, agents.size());
            Sort(agents, TByNodeId());

            UNIT_ASSERT_VALUES_EQUAL(1, agents[0].GetNodeId());
            UNIT_ASSERT_EQUAL(NProto::AGENT_STATE_ONLINE, agents[0].GetState());

            UNIT_ASSERT_VALUES_EQUAL(2, agents[1].GetNodeId());
            UNIT_ASSERT_EQUAL(NProto::AGENT_STATE_ONLINE, agents[1].GetState());

            UNIT_ASSERT_VALUES_EQUAL(3, agents[2].GetNodeId());
            UNIT_ASSERT_EQUAL(NProto::AGENT_STATE_UNAVAILABLE, agents[2].GetState());

            db.ReadDisks(disks);
            UNIT_ASSERT_VALUES_EQUAL(3, disks.size());
            Sort(disks, TByDiskId());

            UNIT_ASSERT_VALUES_EQUAL("disk-1", disks[0].GetDiskId());
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ONLINE, disks[0].GetState());

            UNIT_ASSERT_VALUES_EQUAL("disk-2", disks[1].GetDiskId());
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ONLINE, disks[1].GetState());

            UNIT_ASSERT_VALUES_EQUAL("disk-3", disks[2].GetDiskId());
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ONLINE, disks[2].GetState());
        });
    }

    Y_UNIT_TEST(ShouldUpdateDeviceState)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1")
        });

        const auto agent2 = AgentConfig(2, {
            Device("dev-1", "uuid-3", "rack-2"),
            Device("dev-2", "uuid-4", "rack-2"),
            Device("dev-3", "uuid-5", "rack-2")
        });

        const auto agent3 = AgentConfig(3, {
            Device("dev-1", "uuid-6", "rack-3"),
            Device("dev-2", "uuid-7", "rack-3"),
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1, agent2, agent3 })
            .WithDisks({
                Disk("disk-1", {"uuid-1"}),
                Disk("disk-2", {"uuid-3"}),
                Disk("disk-3", {"uuid-2", "uuid-4"})
            })
            .Build();

        // uuid-1 : online -> warning
        // disk-1 : online -> migration
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            const auto error = state.UpdateDeviceState(
                db,
                "uuid-1",
                NProto::DEVICE_STATE_WARNING,
                Now(),
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(affectedDisk.Defined());

            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_MIGRATION, *affectedDisk);
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisk->SeqNo);

            UNIT_ASSERT_VALUES_EQUAL(1, state.GetDiskStateUpdates().size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            const auto error = state.UpdateDeviceState(
                db,
                "uuid-1",
                NProto::DEVICE_STATE_WARNING,
                Now(),
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_ALREADY, error.GetCode());
            UNIT_ASSERT(affectedDisk.Empty());
        });

        // uuid-5 : online -> warning

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            const auto error = state.UpdateDeviceState(
                db,
                "uuid-5",
                NProto::DEVICE_STATE_WARNING,
                Now(),
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(affectedDisk.Empty());
        });

        // uuid-7 : online -> warning

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            const auto error = state.UpdateDeviceState(
                db,
                "uuid-7",
                NProto::DEVICE_STATE_WARNING,
                Now(),
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(affectedDisk.Empty());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            const auto error = state.UpdateDeviceState(
                db,
                "uuid-7",
                NProto::DEVICE_STATE_WARNING,
                Now(),
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_ALREADY, error.GetCode());
            UNIT_ASSERT(affectedDisk.Empty());
        });

        // uuid-2 : online -> warning
        // disk-3 : online -> migration

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            const auto error = state.UpdateDeviceState(
                db,
                "uuid-2",
                NProto::DEVICE_STATE_WARNING,
                Now(),
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(affectedDisk.Defined());

            UNIT_ASSERT_DISK_STATE("disk-3", DISK_STATE_MIGRATION, *affectedDisk);
            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisk->SeqNo);

            UNIT_ASSERT_VALUES_EQUAL(2, state.GetDiskStateUpdates().size());
        });

        // uuid-4 : online -> warning
        // disk-3 : already in migration

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            const auto error = state.UpdateDeviceState(
                db,
                "uuid-4",
                NProto::DEVICE_STATE_WARNING,
                Now(),
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(affectedDisk.Empty());
        });

        // uuid-4 : warning -> error
        // disk-3 : migration -> error

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            const auto error = state.UpdateDeviceState(
                db,
                "uuid-4",
                NProto::DEVICE_STATE_ERROR,
                Now(),
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(affectedDisk.Defined());

            UNIT_ASSERT_DISK_STATE("disk-3", DISK_STATE_ERROR, *affectedDisk);
            UNIT_ASSERT_VALUES_EQUAL(2, affectedDisk->SeqNo);

            UNIT_ASSERT_VALUES_EQUAL(3, state.GetDiskStateUpdates().size());
        });

        // uuid-4 : error -> online
        // disk-3 : error -> migration

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            const auto error = state.UpdateDeviceState(
                db,
                "uuid-4",
                NProto::DEVICE_STATE_ONLINE,
                Now(),
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(affectedDisk.Defined());

            UNIT_ASSERT_DISK_STATE("disk-3", DISK_STATE_MIGRATION, *affectedDisk);
            UNIT_ASSERT_VALUES_EQUAL(3, affectedDisk->SeqNo);

            UNIT_ASSERT_VALUES_EQUAL(4, state.GetDiskStateUpdates().size());
        });

        // uuid-2 : warning -> online
        // disk-3 : migration -> online

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            const auto error = state.UpdateDeviceState(
                db,
                "uuid-2",
                NProto::DEVICE_STATE_ONLINE,
                Now(),
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(affectedDisk.Defined());

            UNIT_ASSERT_DISK_STATE("disk-3", DISK_STATE_ONLINE, *affectedDisk);
            UNIT_ASSERT_VALUES_EQUAL(4, affectedDisk->SeqNo);

            UNIT_ASSERT_VALUES_EQUAL(5, state.GetDiskStateUpdates().size());
        });
    }

    Y_UNIT_TEST(ShouldNotAllocateDiskWithBrokenDevices)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1")
        });

        const auto agent2 = AgentConfig(2, {
            Device("dev-1", "uuid-3", "rack-2"),
            Device("dev-2", "uuid-4", "rack-2"),
            Device("dev-3", "uuid-5", "rack-2")
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1, agent2 })
            .Build();

        // #2 : online -> warning
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent2.GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                Now(),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        // uuid-1 : online -> warning
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            const auto error = state.UpdateDeviceState(
                db,
                "uuid-1",
                NProto::DEVICE_STATE_WARNING,
                Now(),
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(affectedDisk.Empty());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;

            auto error = AllocateDisk(db, state, "disk-1", {}, 30_GB, devices);
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
        });

        // #2 : warning -> online
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent2.GetAgentId(),
                NProto::AGENT_STATE_ONLINE,
                Now(),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;

            auto error = AllocateDisk(db, state, "disk-1", {}, 30_GB, devices);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(3, devices.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;

            auto error = AllocateDisk(db, state, "disk-2", {}, 20_GB, devices);
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(0, devices.size());
        });

        // uuid-1 : warning -> online
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            const auto error = state.UpdateDeviceState(
                db,
                "uuid-1",
                NProto::DEVICE_STATE_ONLINE,
                Now(),
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(affectedDisk.Empty());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;

            auto error = AllocateDisk(db, state, "disk-2", {}, 20_GB, devices);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(2, devices.size());
        });
    }

    Y_UNIT_TEST(ShouldUpdateDeviceAndAgentState)
    {
        const auto genAgentConfig = [](ui32 nodeId, int deviceCount) {
            NProto::TAgentConfig agent;
            agent.SetNodeId(nodeId);
            agent.SetAgentId("agent-" + ToString(nodeId));

            auto& devices = *agent.MutableDevices();
            devices.Reserve(deviceCount);

            for (int i = 0; i != deviceCount; ++i) {
                *devices.Add() = Device(
                    Sprintf("dev-%d", i + 1),
                    Sprintf("uuid-%d.%d", nodeId, i + 1),
                    Sprintf("rack-%d", nodeId));
            }

            return agent;
        };

        const auto agent1 = genAgentConfig(1, 3);
        const auto agent2 = genAgentConfig(2, 3);

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1, agent2 })
            .WithDisks({
                Disk("disk-1", {"uuid-1.1"}),
                Disk("disk-2", {"uuid-2.1"}),
                Disk("disk-3", {"uuid-1.2", "uuid-2.2"}),
                Disk("disk-4", {"uuid-1.3", "uuid-2.3"})
            })
            .Build();

        // #2 : online -> warning
        // disk-2 : online -> migration
        // disk-3 : online -> migration
        // disk-4 : online -> migration
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent2.GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                Now(),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(3, affectedDisks.size());
            Sort(affectedDisks, TByDiskId());

            UNIT_ASSERT_DISK_STATE("disk-2", DISK_STATE_MIGRATION, affectedDisks[0]);
            UNIT_ASSERT_DISK_STATE("disk-3", DISK_STATE_MIGRATION, affectedDisks[1]);
            UNIT_ASSERT_DISK_STATE("disk-4", DISK_STATE_MIGRATION, affectedDisks[2]);
        });

        // uuid-2.2 : online -> error
        // disk-3 : migration -> error
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            const auto error = state.UpdateDeviceState(
                db,
                "uuid-2.2",
                NProto::DEVICE_STATE_ERROR,
                Now(),
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(affectedDisk.Defined());

            UNIT_ASSERT_DISK_STATE("disk-3", DISK_STATE_ERROR, *affectedDisk);
        });

        // uuid-1.3 : online -> error
        // disk-4 : migration -> error
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            const auto error = state.UpdateDeviceState(
                db,
                "uuid-1.3",
                NProto::DEVICE_STATE_ERROR,
                Now(),
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(affectedDisk.Defined());

            UNIT_ASSERT_DISK_STATE("disk-4", DISK_STATE_ERROR, *affectedDisk);
        });

        // #2 : warning -> online
        // disk-2 : migration -> online
        // disk-3 : still in error state
        // disk-4 : still in error state
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent2.GetAgentId(),
                NProto::AGENT_STATE_ONLINE,
                Now(),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());
            Sort(affectedDisks, TByDiskId());

            UNIT_ASSERT_DISK_STATE("disk-2", DISK_STATE_ONLINE, affectedDisks[0]);
        });
    }

    Y_UNIT_TEST(ShouldUpdateDiskAtAgentRegistration)
    {
        const auto agent1a = AgentConfig(1, "host-1.cloud.yandex.net", {
            Device("dev-1", "uuid-1.1", "rack-1"),
            Device("dev-2", "uuid-1.2", "rack-1"),
            Device("dev-3", "uuid-1.3", "rack-1"),
        });

        const auto agent1b = AgentConfig(1, "host-1.cloud.yandex.net", {
            Device("dev-2", "uuid-1.2", "rack-1"),
            Device("dev-3", "uuid-1.3", "rack-1"),
        });

        const auto agent2a = AgentConfig(2, "host-2.cloud.yandex.net", {
            Device("dev-1", "uuid-2.1", "rack-2"),
            Device("dev-2", "uuid-2.2", "rack-2"),
            Device("dev-3", "uuid-2.3", "rack-2"),
        });

        const auto agent2b = AgentConfig(2, "host-2.cloud.yandex.net", {
            Device("dev-1", "uuid-2.1", "rack-2"),
        });

        const auto agent2c = AgentConfig(42, "host-2.cloud.yandex.net", {
            Device("dev-1", "uuid-2.1", "rack-2"),
            Device("dev-2", "uuid-2.2", "rack-2"),
            Device("dev-3", "uuid-2.3", "rack-2"),
        });

        const auto agent3a = AgentConfig(3, "host-3.cloud.yandex.net", {
            Device("dev-1", "uuid-3.1", "rack-3"),
            Device("dev-2", "uuid-3.2", "rack-3"),
            Device("dev-3", "uuid-3.3", "rack-3"),
        });

        const auto agent4a = AgentConfig(4, "host-4.cloud.yandex.net", {
            Device("dev-1", "uuid-4.1", "rack-4"),
            Device("dev-2", "uuid-4.2", "rack-4"),
            Device("dev-3", "uuid-4.3", "rack-4"),
            Device("dev-4", "uuid-4.4", "rack-4"),
            Device("dev-5", "uuid-4.5", "rack-4"),
            Device("dev-6", "uuid-4.6", "rack-4"),
        });

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1a, agent2a, agent3a, agent4a })
            .WithDisks({
                Disk("disk-1", {"uuid-1.1"}),
                Disk("disk-2", {"uuid-2.1"}),
                Disk("disk-3", {"uuid-1.2", "uuid-2.2"}),
                Disk("disk-4", {"uuid-1.3", "uuid-2.3"})
            })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            THashMap<TString, ui64> disksToNotify;

            // drop uuid-1.1
            UNIT_ASSERT_SUCCESS(state.RegisterAgent(
                db,
                agent1b,
                Now(),
                &affectedDisks,
                &disksToNotify
            ));

            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());
            const auto& diskState = affectedDisks[0].State;
            UNIT_ASSERT_VALUES_EQUAL("disk-1", diskState.GetDiskId());
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ERROR, diskState.GetState());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            THashMap<TString, ui64> disksToNotify;

            // drop uuid-2.2 & uuid-2.3
            UNIT_ASSERT_SUCCESS(state.RegisterAgent(
                db,
                agent2b,
                Now(),
                &affectedDisks,
                &disksToNotify
            ));

            UNIT_ASSERT_VALUES_EQUAL(2, affectedDisks.size());
            SortBy(affectedDisks, [] (const auto& update) {
                return update.State.GetDiskId();
            });

            {
                const auto& diskState = affectedDisks[0].State;
                UNIT_ASSERT_VALUES_EQUAL("disk-3", diskState.GetDiskId());
                UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ERROR, diskState.GetState());
            }
            {
                const auto& diskState = affectedDisks[1].State;
                UNIT_ASSERT_VALUES_EQUAL("disk-4", diskState.GetDiskId());
                UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ERROR, diskState.GetState());
            }
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            THashMap<TString, ui64> disksToNotify;

            // restore uuid-2.2 & uuid-2.3 but with error state
            UNIT_ASSERT_SUCCESS(state.RegisterAgent(
                db,
                agent2c,
                Now(),
                &affectedDisks,
                &disksToNotify
            ));

            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        // restore uuid-2.2 to online
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
                TMaybe<TDiskStateUpdate> affectedDisk;

                UNIT_ASSERT_SUCCESS(state.UpdateDeviceState(
                    db,
                    "uuid-2.2",
                    NProto::DEVICE_STATE_ONLINE,
                    Now(),
                    "test",
                    affectedDisk));

                UNIT_ASSERT(affectedDisk);

                const auto& diskState = affectedDisk->State;
                UNIT_ASSERT_VALUES_EQUAL("disk-3", diskState.GetDiskId());
                UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ONLINE, diskState.GetState());
                UNIT_ASSERT(diskState.GetStateMessage().empty());
        });

        // restore uuid-2.3 to online
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;

            UNIT_ASSERT_SUCCESS(state.UpdateDeviceState(
                db,
                "uuid-2.3",
                NProto::DEVICE_STATE_ONLINE,
                Now(),
                "test",
                affectedDisk));

            const auto& diskState = affectedDisk->State;
            UNIT_ASSERT_VALUES_EQUAL("disk-4", diskState.GetDiskId());
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ONLINE, diskState.GetState());
            UNIT_ASSERT(diskState.GetStateMessage().empty());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            THashMap<TString, ui64> disksToNotify;
            UNIT_ASSERT_SUCCESS(state.RegisterAgent(
                db,
                agent3a,
                Now(),
                &affectedDisks,
                &disksToNotify
            ));

            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            THashMap<TString, ui64> disksToNotify;
            UNIT_ASSERT_SUCCESS(state.RegisterAgent(
                db,
                agent4a,
                Now(),
                &affectedDisks,
                &disksToNotify
            ));

            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        // agent2 -> unavailable

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            UNIT_ASSERT_SUCCESS(state.UpdateAgentState(
                db,
                agent2c.GetAgentId(),
                NProto::AGENT_STATE_UNAVAILABLE,
                Now(),
                "state message",
                affectedDisks));

            UNIT_ASSERT_VALUES_EQUAL(3, affectedDisks.size());

            SortBy(affectedDisks, [] (const auto& update) {
                return update.State.GetDiskId();
            });

            {
                const auto& diskState = affectedDisks[0].State;
                UNIT_ASSERT_VALUES_EQUAL("disk-2", diskState.GetDiskId());
                UNIT_ASSERT_EQUAL(
                    NProto::DISK_STATE_TEMPORARILY_UNAVAILABLE,
                    diskState.GetState());
            }

            {
                const auto& diskState = affectedDisks[1].State;
                UNIT_ASSERT_VALUES_EQUAL("disk-3", diskState.GetDiskId());
                UNIT_ASSERT_EQUAL(
                    NProto::DISK_STATE_TEMPORARILY_UNAVAILABLE,
                    diskState.GetState());
            }

            {
                const auto& diskState = affectedDisks[2].State;
                UNIT_ASSERT_VALUES_EQUAL("disk-4", diskState.GetDiskId());
                UNIT_ASSERT_EQUAL(
                    NProto::DISK_STATE_TEMPORARILY_UNAVAILABLE,
                    diskState.GetState());
            }
        });

        // agent2 -> ...

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            THashMap<TString, ui64> disksToNotify;
            UNIT_ASSERT_SUCCESS(state.RegisterAgent(
                db,
                agent2a,
                Now(),
                &affectedDisks,
                &disksToNotify
            ));

            UNIT_ASSERT_VALUES_EQUAL(3, affectedDisks.size());

            SortBy(affectedDisks, [] (const auto& update) {
                return update.State.GetDiskId();
            });

            {
                const auto& diskState = affectedDisks[0].State;
                UNIT_ASSERT_VALUES_EQUAL("disk-2", diskState.GetDiskId());
                UNIT_ASSERT_EQUAL(
                    NProto::DISK_STATE_MIGRATION,
                    diskState.GetState());
            }

            {
                const auto& diskState = affectedDisks[1].State;
                UNIT_ASSERT_VALUES_EQUAL("disk-3", diskState.GetDiskId());
                UNIT_ASSERT_EQUAL(
                    NProto::DISK_STATE_MIGRATION,
                    diskState.GetState());
            }

            {
                const auto& diskState = affectedDisks[2].State;
                UNIT_ASSERT_VALUES_EQUAL("disk-4", diskState.GetDiskId());
                UNIT_ASSERT_EQUAL(
                    NProto::DISK_STATE_MIGRATION,
                    diskState.GetState());
            }
        });

        {
            auto migrations = state.BuildMigrationList();
            UNIT_ASSERT_VALUES_EQUAL(3, migrations.size());
            UNIT_ASSERT_VALUES_EQUAL("disk-2", migrations[0].DiskId);
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", migrations[0].SourceDeviceId);
            UNIT_ASSERT_VALUES_EQUAL("disk-3", migrations[1].DiskId);
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.2", migrations[1].SourceDeviceId);
            UNIT_ASSERT_VALUES_EQUAL("disk-4", migrations[2].DiskId);
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.3", migrations[2].SourceDeviceId);
        }

        // agent2 -> online

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent2a.GetAgentId(),
                NProto::AGENT_STATE_ONLINE,
                Now(),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(3, affectedDisks.size());

            SortBy(affectedDisks, [] (const auto& update) {
                return update.State.GetDiskId();
            });

            {
                const auto& diskState = affectedDisks[0].State;
                UNIT_ASSERT_VALUES_EQUAL("disk-2", diskState.GetDiskId());
                UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ONLINE, diskState.GetState());
                UNIT_ASSERT(diskState.GetStateMessage().empty());
            }

            {
                const auto& diskState = affectedDisks[1].State;
                UNIT_ASSERT_VALUES_EQUAL("disk-3", diskState.GetDiskId());
                UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ONLINE, diskState.GetState());
                UNIT_ASSERT(diskState.GetStateMessage().empty());
            }

            {
                const auto& diskState = affectedDisks[2].State;
                UNIT_ASSERT_VALUES_EQUAL("disk-4", diskState.GetDiskId());
                UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ONLINE, diskState.GetState());
                UNIT_ASSERT(diskState.GetStateMessage().empty());
            }
        });

        {
            auto migrations = state.BuildMigrationList();
            UNIT_ASSERT_VALUES_EQUAL(0, migrations.size());
        }

        executor.ReadTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<NProto::TAgentConfig> agents;
            db.ReadAgents(agents);

            UNIT_ASSERT_VALUES_EQUAL(4, agents.size());
        });
    }

    Y_UNIT_TEST(ShouldUpdateDeviceConfigAtAgentRegistration)
    {
        const auto agent1a = AgentConfig(1, "host-1.cloud.yandex.net", {
            Device("dev-1", "uuid-1.1", "rack-1", DefaultBlockSize, 10_GB),
            Device("dev-2", "uuid-1.2", "rack-1", DefaultBlockSize, 10_GB, "#1"),
            Device("dev-3", "uuid-1.3", "rack-1", DefaultBlockSize, 10_GB)
        });

        const auto agent1b = AgentConfig(1, "host-1.cloud.yandex.net", {
            Device("dev-2", "uuid-1.2", "rack-1", DefaultBlockSize, 10_GB, "#2"),
            Device("dev-3", "uuid-1.3", "rack-1", DefaultBlockSize, 20_GB)
        });

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1a })
            .Build();

        {
            const auto dev1 = state.GetDevice("uuid-1.1");
            UNIT_ASSERT_VALUES_EQUAL(10_GB / DefaultBlockSize, dev1.GetBlocksCount());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, dev1.GetState());

            const auto dev2 = state.GetDevice("uuid-1.2");
            UNIT_ASSERT_VALUES_EQUAL(10_GB / DefaultBlockSize, dev2.GetBlocksCount());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, dev2.GetState());
            UNIT_ASSERT_EQUAL("#1", dev2.GetTransportId());

            const auto dev3 = state.GetDevice("uuid-1.3");
            UNIT_ASSERT_VALUES_EQUAL(10_GB / DefaultBlockSize, dev3.GetBlocksCount());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, dev3.GetState());
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            THashMap<TString, ui64> disksToNotify;
            UNIT_ASSERT_SUCCESS(state.RegisterAgent(
                db,
                agent1b,
                Now(),
                &affectedDisks,
                &disksToNotify
            ));

            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        {
            const auto dev1 = state.GetDevice("uuid-1.1");
            UNIT_ASSERT_VALUES_EQUAL(10_GB / DefaultBlockSize, dev1.GetBlocksCount());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ERROR, dev1.GetState());

            const auto dev2 = state.GetDevice("uuid-1.2");
            UNIT_ASSERT_VALUES_EQUAL(10_GB / DefaultBlockSize, dev2.GetBlocksCount());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, dev2.GetState());
            UNIT_ASSERT_EQUAL("#2", dev2.GetTransportId());

            const auto dev3 = state.GetDevice("uuid-1.3");
            UNIT_ASSERT_VALUES_EQUAL(10_GB / DefaultBlockSize, dev3.GetBlocksCount());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ERROR, dev3.GetState());
        }
    }

    Y_UNIT_TEST(ShouldRestoreDevicePool)
    {
        const auto agent = AgentConfig(1, "host.cloud.yandex.net", {
            Device("dev-1", "uuid-1.1", "rack-1"),
            Device("dev-2", "uuid-1.2", "rack-1"),
            Device("dev-3", "uuid-1.3", "rack-1")
        });

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDiskStateUpdate> affectedDisks;
            UNIT_ASSERT_SUCCESS(state.UpdateAgentState(
                db,
                agent.GetAgentId(),
                NProto::AGENT_STATE_UNAVAILABLE,
                Now(),
                "state message",
                affectedDisks));
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", {}, 30_GB, devices);
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(0, devices.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agent));
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", {}, 30_GB, devices);
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent.GetAgentId(),
                NProto::AGENT_STATE_ONLINE,
                Now(),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", {}, 30_GB, devices);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(3, devices.size());
        });
    }

    Y_UNIT_TEST(ShouldFindDisksToNotify)
    {
        const auto agent1 = AgentConfig(1, "host-1.cloud.yandex.net", {
            Device("dev-1", "uuid-1", "rack-1", DefaultBlockSize, 10_GB),
            Device("dev-2", "uuid-2", "rack-1", DefaultBlockSize, 10_GB),
            Device("dev-3", "uuid-3", "rack-1", DefaultBlockSize, 10_GB)
        });

        const auto agent2 = AgentConfig(2, "host-2.cloud.yandex.net", {
            Device("dev-1", "uuid-4", "rack-2", DefaultBlockSize, 10_GB),
            Device("dev-2", "uuid-5", "rack-2", DefaultBlockSize, 10_GB),
            Device("dev-3", "uuid-6", "rack-2", DefaultBlockSize, 10_GB)
        });

        const auto agent3 = AgentConfig(3, "host-3.cloud.yandex.net", {
            Device("dev-1", "uuid-7", "rack-3", DefaultBlockSize, 10_GB),
            Device("dev-2", "uuid-8", "rack-3", DefaultBlockSize, 10_GB),
            Device("dev-3", "uuid-9", "rack-3", DefaultBlockSize, 10_GB)
        });

        const auto agent1a = AgentConfig(4, "host-1.cloud.yandex.net", {
            Device("dev-1", "uuid-1", "rack-2", DefaultBlockSize, 10_GB),
            Device("dev-2", "uuid-2", "rack-2", DefaultBlockSize, 10_GB),
            Device("dev-3", "uuid-3", "rack-2", DefaultBlockSize, 10_GB)
        });

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1, agent2, agent3 })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", {}, 10_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1", devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(1, devices[0].GetNodeId());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-2", {}, 30_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(3, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-4", devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(2, devices[0].GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL("uuid-5", devices[1].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(2, devices[1].GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL("uuid-6", devices[2].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(2, devices[2].GetNodeId());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-3", {}, 30_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(3, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-7", devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(3, devices[0].GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL("uuid-8", devices[1].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(3, devices[1].GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL("uuid-9", devices[2].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(3, devices[2].GetNodeId());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-4", {}, 20_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(2, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-2", devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(1, devices[0].GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL("uuid-3", devices[1].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(1, devices[1].GetNodeId());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            THashMap<TString, ui64> disksToNotify;
            UNIT_ASSERT_SUCCESS(state.RegisterAgent(
                db,
                agent1a,
                Now(),
                &affectedDisks,
                &disksToNotify
            ));

            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
            UNIT_ASSERT_VALUES_EQUAL(2, disksToNotify.size());
            auto it = disksToNotify.begin();
            UNIT_ASSERT_VALUES_EQUAL("disk-4", it->first);
            ++it;
            UNIT_ASSERT_VALUES_EQUAL("disk-1", it->first);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-4", {}, 20_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(2, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-2", devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(4, devices[0].GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL("uuid-3", devices[1].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(4, devices[1].GetNodeId());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", {}, 10_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1", devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(4, devices[0].GetNodeId());
        });
    }

    Y_UNIT_TEST(ShouldReplaceDevice)
    {
        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1.1", "rack-1"),
            Device("dev-2", "uuid-1.2", "rack-1")
        });

        const auto agent2 = AgentConfig(2, {
            Device("dev-1", "uuid-2.1", "rack-2")
        });

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1, agent2 })
            .WithDisks({ Disk("disk-1", { "uuid-1.1", "uuid-2.1" }) })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            const auto error = state.UpdateDeviceState(
                db,
                "uuid-2.1",
                NProto::DEVICE_STATE_ERROR,
                Now(),
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(affectedDisk.Defined());

            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_ERROR, *affectedDisk);
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisk->SeqNo);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> update;
            const auto error = state.ReplaceDevice(
                db,
                "disk-1",
                "uuid-2.1",
                Now(),
                &update);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(update.Defined());
            UNIT_ASSERT_VALUES_EQUAL("disk-1", update->State.GetDiskId());
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ONLINE, update->State.GetState());
            UNIT_ASSERT_VALUES_EQUAL(1, update->SeqNo);
        });

        {
            TVector<TDeviceConfig> devices;
            const auto error = state.GetDiskDevices("disk-1", devices);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(2, devices.size());
            Sort(devices, TByDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.2", devices[1].GetDeviceUUID());

            TDiskInfo info;
            UNIT_ASSERT_SUCCESS(state.GetDiskInfo("disk-1", info));
            // device replacement list should be empty for non-mirrored disks
            ASSERT_VECTORS_EQUAL(TVector<TString>{}, info.DeviceReplacementIds);
        }

        {
            const auto device = state.GetDevice("uuid-2.1");
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ERROR, device.GetState());
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-2", {}, 10_GB, devices);

            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(0, devices.size());
        });
    }

    Y_UNIT_TEST(ShouldReplaceDeviceInPlacementGroup)
    {
        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1.1", "rack-1")
        });

        const auto agent2 = AgentConfig(2, {
            Device("dev-1", "uuid-2.1", "rack-2"),
            Device("dev-2", "uuid-2.2", "rack-2")
        });

        const auto agent3 = AgentConfig(3, {
            Device("dev-1", "uuid-3.1", "rack-3")
        });

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1, agent2 })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            UNIT_ASSERT_SUCCESS(state.CreatePlacementGroup(db, "group-1"));
        });

        TString device;

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDeviceConfig> devices;

            UNIT_ASSERT_SUCCESS(AllocateDisk(
                db, state, "disk-1", "group-1", 10_GB, devices));
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("rack-2", devices[0].GetRack());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDeviceConfig> devices;

            UNIT_ASSERT_SUCCESS(AllocateDisk(
                db, state, "disk-2", "group-1", 10_GB, devices));
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("rack-1", devices[0].GetRack());

            device = devices[0].GetDeviceUUID();
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> update;
            const auto error = state.ReplaceDevice(
                db,
                "disk-2",
                device,
                Now(),
                &update);

            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
            UNIT_ASSERT(update.Empty());
        });

        UNIT_ASSERT_VALUES_EQUAL(0, state.GetDirtyDevices().size());

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UpdateConfig(state, db, { agent1, agent2, agent3 });
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agent3));
            CleanDevices(state, db);

            TMaybe<TDiskStateUpdate> update;
            const auto error = state.ReplaceDevice(
                db,
                "disk-2",
                device,
                Now(),
                &update);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
        });

        {
            auto* group = state.FindPlacementGroup("group-1");
            UNIT_ASSERT(group);

            for (auto& info: group->GetDisks()) {
                if (info.GetDiskId() == "disk-2") {
                    UNIT_ASSERT_VALUES_EQUAL(1, info.DeviceRacksSize());
                    UNIT_ASSERT_VALUES_EQUAL("rack-3", info.GetDeviceRacks(0));
                }
            }
        }

        executor.ReadTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<NProto::TPlacementGroupConfig> groups;

            db.ReadPlacementGroups(groups);
            UNIT_ASSERT_VALUES_EQUAL(1, groups.size());

            for (auto& group: groups) {
                UNIT_ASSERT_VALUES_EQUAL(2, group.DisksSize());
                for (auto& info: group.GetDisks()) {
                    if (info.GetDiskId() == "disk-1") {
                        UNIT_ASSERT_VALUES_EQUAL(1, info.DeviceRacksSize());
                        UNIT_ASSERT_VALUES_EQUAL("rack-2", info.GetDeviceRacks(0));
                    }
                    if (info.GetDiskId() == "disk-2") {
                        UNIT_ASSERT_VALUES_EQUAL(1, info.DeviceRacksSize());
                        UNIT_ASSERT_VALUES_EQUAL("rack-3", info.GetDeviceRacks(0));
                    }
                }
            }
        });
    }

    Y_UNIT_TEST(ShouldNotReplaceWithSeveralDevices)
    {
        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1.1", "rack-1", DefaultBlockSize, 40_GB),
            Device("dev-2", "uuid-1.2", "rack-1", DefaultBlockSize, 40_GB),
            Device("dev-3", "uuid-1.3", "rack-1", DefaultBlockSize, 40_GB)
        });

        const auto agent2 = AgentConfig(2, {
            Device("dev-1", "uuid-2.1", "rack-2", DefaultBlockSize, 10_GB),
            Device("dev-2", "uuid-2.2", "rack-2", DefaultBlockSize, 10_GB),
            Device("dev-3", "uuid-2.3", "rack-2", DefaultBlockSize, 10_GB),
            Device("dev-4", "uuid-2.4", "rack-2", DefaultBlockSize, 10_GB)
        });

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1, agent2 })
            .WithDisks({
                Disk("disk-1", { "uuid-1.1", "uuid-1.2", "uuid-1.3" })
            })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> update;
            const auto error = state.UpdateDeviceState(
                db,
                "uuid-1.2",
                NProto::DEVICE_STATE_ERROR,
                Now(),
                "test",
                update);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(update.Defined());
            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_ERROR, *update);
            UNIT_ASSERT_VALUES_EQUAL(0, update->SeqNo);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> update;
            const auto error = state.ReplaceDevice(
                db,
                "disk-1",
                "uuid-1.2",
                Now(),
                &update);

            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
        });
    }

    Y_UNIT_TEST(ShouldNotReplaceDeviceWithWrongBlockCount)
    {
        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1.1", "rack-1", DefaultBlockSize, 10_GB),
            Device("dev-2", "uuid-1.2", "rack-1", DefaultBlockSize, 10_GB),
            Device("dev-3", "uuid-1.3", "rack-1", DefaultBlockSize, 10_GB)
        });

        const auto agent2 = AgentConfig(2, {
            Device("dev-1", "uuid-2.1", "rack-2", DefaultBlockSize, 9_GB),
            Device("dev-2", "uuid-2.2", "rack-2", DefaultBlockSize, 9_GB)
        });

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1, agent2 })
            .WithDisks({
                Disk("disk-1", { "uuid-1.1", "uuid-1.2", "uuid-1.3" })
            })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> update;
            const auto error = state.ReplaceDevice(
                db,
                "disk-1",
                "uuid-1.2",
                Now(),
                &update);

            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
            UNIT_ASSERT(update.Empty());
        });
    }

    Y_UNIT_TEST(ShouldUpdateDeviceStateAtRegistration)
    {
        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1.1", "rack-1", DefaultBlockSize, 10_GB),
            Device("dev-2", "uuid-1.2", "rack-1", DefaultBlockSize, 10_GB),
            Device("dev-3", "uuid-1.3", "rack-1", DefaultBlockSize, 10_GB)
        });

        // uuid-1.1 -> online
        // uuid-1.2 -> online
        // uuid-1.3 -> N/A
        const auto agent1a = AgentConfig(1, {
            Device("dev-1", "uuid-1.1", "rack-1", DefaultBlockSize, 10_GB),
            Device("dev-2", "uuid-1.2", "rack-1", DefaultBlockSize, 10_GB)
        });

        // uuid-1.1 -> error
        // uuid-1.2 -> online
        // uuid-1.3 -> online
        const auto agent1b = AgentConfig(1, {
            Device("dev-2", "uuid-1.2", "rack-1", DefaultBlockSize, 10_GB),
            Device("dev-3", "uuid-1.3", "rack-1", DefaultBlockSize, 10_GB)
        });

        const auto ts1 = TInstant::MicroSeconds(10);
        const auto ts2 = TInstant::MicroSeconds(20);
        const auto warningTs = TInstant::MicroSeconds(30);
        const auto ts3 = TInstant::MicroSeconds(40);

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder().Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UpdateConfig(state, db, { agent1 });
        });

        // uuid-1.1 : online
        // uuid-1.2 : online
        // uuid-1.3 : N/A
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agent1a, ts1));

            const auto& agents = state.GetAgents();
            UNIT_ASSERT_VALUES_EQUAL(1, agents.size());
            UNIT_ASSERT_EQUAL(NProto::AGENT_STATE_ONLINE, agents[0].GetState());
            UNIT_ASSERT_VALUES_EQUAL(ts1.MicroSeconds(), agents[0].GetStateTs());

            auto dev1 = state.GetDevice("uuid-1.1");
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, dev1.GetState());
            UNIT_ASSERT_VALUES_EQUAL(ts1.MicroSeconds(), dev1.GetStateTs());

            auto dev2 = state.GetDevice("uuid-1.2");
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, dev2.GetState());
            UNIT_ASSERT_VALUES_EQUAL(ts1.MicroSeconds(), dev2.GetStateTs());

            auto dev3 = state.GetDevice("uuid-1.3");
            // N/A
            UNIT_ASSERT_VALUES_EQUAL(0, dev3.GetStateTs());
        });

        // uuid-1.1 : error
        // uuid-1.2 : online
        // uuid-1.3 : online
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agent1b, ts2));

            const auto& agents = state.GetAgents();
            UNIT_ASSERT_VALUES_EQUAL(1, agents.size());
            UNIT_ASSERT_EQUAL(NProto::AGENT_STATE_ONLINE, agents[0].GetState());
            UNIT_ASSERT_VALUES_EQUAL(ts1.MicroSeconds(), agents[0].GetStateTs());

            auto dev1 = state.GetDevice("uuid-1.1");
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ERROR, dev1.GetState());
            UNIT_ASSERT_VALUES_EQUAL(ts2.MicroSeconds(), dev1.GetStateTs());

            auto dev2 = state.GetDevice("uuid-1.2");
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, dev2.GetState());
            UNIT_ASSERT_VALUES_EQUAL(ts1.MicroSeconds(), dev2.GetStateTs());

            auto dev3 = state.GetDevice("uuid-1.3");
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, dev3.GetState());
            UNIT_ASSERT_VALUES_EQUAL(ts2.MicroSeconds(), dev3.GetStateTs());
        });

        // uuid-1.2 -> warn
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            const auto error = state.UpdateDeviceState(
                db,
                "uuid-1.2",
                NProto::DEVICE_STATE_WARNING,
                warningTs,
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(affectedDisk.Empty());
        });

        // uuid-1.1 : error
        // uuid-1.2 : warn
        // uuid-1.3 : online
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agent1b, ts3));

            const auto& agents = state.GetAgents();
            UNIT_ASSERT_VALUES_EQUAL(1, agents.size());
            UNIT_ASSERT_EQUAL(NProto::AGENT_STATE_ONLINE, agents[0].GetState());
            UNIT_ASSERT_VALUES_EQUAL(ts1.MicroSeconds(), agents[0].GetStateTs());

            auto dev1 = state.GetDevice("uuid-1.1");
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ERROR, dev1.GetState());
            UNIT_ASSERT_VALUES_EQUAL(ts2.MicroSeconds(), dev1.GetStateTs());

            auto dev2 = state.GetDevice("uuid-1.2");
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_WARNING, dev2.GetState());
            UNIT_ASSERT_VALUES_EQUAL(warningTs.MicroSeconds(), dev2.GetStateTs());

            auto dev3 = state.GetDevice("uuid-1.3");
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, dev3.GetState());
            UNIT_ASSERT_VALUES_EQUAL(ts2.MicroSeconds(), dev3.GetStateTs());
        });
    }

    Y_UNIT_TEST(ShouldNotAllocateDiskAtAgentWithWarningState)
    {
        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1.1", "rack-1", DefaultBlockSize, 10_GB)
        });

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder().Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UpdateConfig(state, db, { agent1 });
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agent1));
            CleanDevices(state, db);

            TVector<TDeviceConfig> devices;
            UNIT_ASSERT_SUCCESS(AllocateDisk(db, state, "disk-1", {}, 10_GB, devices));
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_SUCCESS(state.DeallocateDisk(db, "disk-1"));
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                Now(),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            const auto error = AllocateDisk(db, state, "disk-1", {}, 10_GB, devices);
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
        });
    }

    Y_UNIT_TEST(ShouldPreserveDiskState)
    {
        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1.1", "rack-1", DefaultBlockSize, 10_GB),
            Device("dev-2", "uuid-1.2", "rack-1", DefaultBlockSize, 10_GB)
        });

        const auto registerTs = TInstant::MicroSeconds(5);
        const auto createTs = TInstant::MicroSeconds(10);
        const auto resizeTs = TInstant::MicroSeconds(20);
        const auto errorTs = TInstant::MicroSeconds(30);
        const auto updateTs = TInstant::MicroSeconds(40);

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder().Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UpdateConfig(state, db, { agent1 });
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agent1, registerTs));
            CleanDevices(state, db);
        });

        // create
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TDiskRegistryState::TAllocateDiskResult result {};

            UNIT_ASSERT_SUCCESS(state.AllocateDisk(
                createTs,
                db,
                TDiskRegistryState::TAllocateDiskParams {
                    .DiskId = "disk-1",
                    .BlockSize = DefaultLogicalBlockSize,
                    .BlocksCount = 10_GB / DefaultLogicalBlockSize
                },
                &result));

            UNIT_ASSERT_VALUES_EQUAL(1, result.Devices.size());
            UNIT_ASSERT_EQUAL(NProto::VOLUME_IO_OK, result.IOMode);
            UNIT_ASSERT_UNEQUAL(TInstant::Zero(), result.IOModeTs);
            UNIT_ASSERT_VALUES_EQUAL(0, result.Migrations.size());
        });

        {
            TDiskInfo info;
            UNIT_ASSERT_SUCCESS(state.GetDiskInfo("disk-1", info));

            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ONLINE, info.State);
            UNIT_ASSERT_VALUES_EQUAL(createTs, info.StateTs);
            UNIT_ASSERT_VALUES_EQUAL(1, info.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL(DefaultLogicalBlockSize, info.LogicalBlockSize);
        }

        executor.ReadTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<NProto::TDiskConfig> disks;

            db.ReadDisks(disks);
            UNIT_ASSERT_VALUES_EQUAL(1, disks.size());
            const auto& disk = disks[0];
            UNIT_ASSERT_VALUES_EQUAL("disk-1", disk.GetDiskId());
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ONLINE, disk.GetState());
            UNIT_ASSERT_VALUES_EQUAL(createTs.MicroSeconds(), disk.GetStateTs());
        });

        // resize
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TDiskRegistryState::TAllocateDiskResult result {};

            UNIT_ASSERT_SUCCESS(state.AllocateDisk(
                resizeTs,
                db,
                TDiskRegistryState::TAllocateDiskParams {
                    .DiskId = "disk-1",
                    .BlockSize = DefaultLogicalBlockSize,
                    .BlocksCount = 20_GB / DefaultLogicalBlockSize
                },
                &result));

            UNIT_ASSERT_VALUES_EQUAL(2, result.Devices.size());
            UNIT_ASSERT_EQUAL(NProto::VOLUME_IO_OK, result.IOMode);
            UNIT_ASSERT_UNEQUAL(TInstant::Zero(), result.IOModeTs);
            UNIT_ASSERT_VALUES_EQUAL(0, result.Migrations.size());
        });

        {
            TDiskInfo info;
            UNIT_ASSERT_SUCCESS(state.GetDiskInfo("disk-1", info));

            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ONLINE, info.State);
            UNIT_ASSERT_VALUES_EQUAL(createTs, info.StateTs);
            UNIT_ASSERT_VALUES_EQUAL(2, info.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL(DefaultLogicalBlockSize, info.LogicalBlockSize);
        }

        executor.ReadTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<NProto::TDiskConfig> disks;

            db.ReadDisks(disks);
            UNIT_ASSERT_VALUES_EQUAL(1, disks.size());
            const auto& disk = disks[0];
            UNIT_ASSERT_VALUES_EQUAL("disk-1", disk.GetDiskId());
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ONLINE, disk.GetState());
            UNIT_ASSERT_VALUES_EQUAL(createTs.MicroSeconds(), disk.GetStateTs());
        });

        // uuid-1.1 : online -> error
        // disk-1   : online -> error
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            UNIT_ASSERT_SUCCESS(state.UpdateDeviceState(
                db,
                "uuid-1.1",
                NProto::DEVICE_STATE_ERROR,
                errorTs,
                "test",
                affectedDisk));

            UNIT_ASSERT(affectedDisk.Defined());
            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_ERROR, *affectedDisk);
        });

        {
            TDiskInfo info;
            UNIT_ASSERT_SUCCESS(state.GetDiskInfo("disk-1", info));

            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ERROR, info.State);
            UNIT_ASSERT_VALUES_EQUAL(errorTs, info.StateTs);
        }

        executor.ReadTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<NProto::TDiskConfig> disks;

            db.ReadDisks(disks);
            UNIT_ASSERT_VALUES_EQUAL(1, disks.size());
            const auto& disk = disks[0];
            UNIT_ASSERT_VALUES_EQUAL("disk-1", disk.GetDiskId());
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ERROR, disk.GetState());
            UNIT_ASSERT_VALUES_EQUAL(errorTs.MicroSeconds(), disk.GetStateTs());
        });

        // update
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TDiskRegistryState::TAllocateDiskResult result {};

            UNIT_ASSERT_SUCCESS(state.AllocateDisk(
                updateTs,
                db,
                TDiskRegistryState::TAllocateDiskParams {
                    .DiskId = "disk-1",
                    .BlockSize = DefaultLogicalBlockSize,
                    .BlocksCount = 20_GB / DefaultLogicalBlockSize
                },
                &result));

            UNIT_ASSERT_VALUES_EQUAL(2, result.Devices.size());
            UNIT_ASSERT_EQUAL(NProto::VOLUME_IO_ERROR_READ_ONLY, result.IOMode);
            UNIT_ASSERT_UNEQUAL(TInstant::Zero(), result.IOModeTs);
            UNIT_ASSERT_VALUES_EQUAL(0, result.Migrations.size());
        });

        {
            TDiskInfo info;
            UNIT_ASSERT_SUCCESS(state.GetDiskInfo("disk-1", info));

            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ERROR, info.State);
            UNIT_ASSERT_VALUES_EQUAL(errorTs, info.StateTs);
        }

        executor.ReadTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<NProto::TDiskConfig> disks;

            db.ReadDisks(disks);
            UNIT_ASSERT_VALUES_EQUAL(1, disks.size());
            const auto& disk = disks[0];
            UNIT_ASSERT_VALUES_EQUAL("disk-1", disk.GetDiskId());
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ERROR, disk.GetState());
            UNIT_ASSERT_VALUES_EQUAL(errorTs.MicroSeconds(), disk.GetStateTs());
        });
    }

    Y_UNIT_TEST(ShouldNotifyDisks)
    {
        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1.1", "rack-1", DefaultBlockSize, 10_GB),
        });

        const auto agent2a = AgentConfig(2, {
            Device("dev-1", "uuid-2.1", "rack-2", DefaultBlockSize, 10_GB),
            Device("dev-2", "uuid-2.2", "rack-2", DefaultBlockSize, 10_GB)
        });

        const auto agent2b = [&] {
            NProto::TAgentConfig config = agent2a;
            config.SetNodeId(42);

            return config;
        }();

        const TInstant errorTs = TInstant::MicroSeconds(10);
        const TInstant regTs = TInstant::MicroSeconds(20);

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1, agent2a })
            .WithDisks({
                Disk("disk-1", { "uuid-1.1", "uuid-2.1" })
            })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TMaybe<TDiskStateUpdate> update;
            UNIT_ASSERT_SUCCESS(state.UpdateDeviceState(
                db,
                "uuid-1.1",
                NProto::DEVICE_STATE_ERROR,
                errorTs,
                "test",
                update));

            UNIT_ASSERT(update.Defined());
            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_ERROR, *update);
        });

        ui64 seqNo1 = 0;
        ui64 seqNo2 = 0;

        {
            const auto& disks = state.GetDisksToNotify();
            UNIT_ASSERT_VALUES_EQUAL(1, disks.size());

            seqNo1 = disks.at("disk-1");
            UNIT_ASSERT_VALUES_UNEQUAL(0, seqNo1);
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDiskStateUpdate> updates;
            THashMap<TString, ui64> notifiedDisks;
            UNIT_ASSERT_SUCCESS(state.RegisterAgent(
                db,
                agent2b,
                regTs,
                &updates,
                &notifiedDisks));

            UNIT_ASSERT_VALUES_EQUAL(0, updates.size());
            UNIT_ASSERT_VALUES_EQUAL(1, notifiedDisks.size());

            seqNo2 = notifiedDisks["disk-1"];

            UNIT_ASSERT_VALUES_UNEQUAL(0, seqNo2);
            UNIT_ASSERT_VALUES_UNEQUAL(seqNo1, seqNo2);
        });

        {
            const auto& disks = state.GetDisksToNotify();
            UNIT_ASSERT_VALUES_EQUAL(1, disks.size());
            UNIT_ASSERT_VALUES_EQUAL(seqNo2, disks.at("disk-1"));
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            state.DeleteDiskToNotify(db, "disk-1", seqNo1);
        });

        {
            const auto& disks = state.GetDisksToNotify();
            UNIT_ASSERT_VALUES_EQUAL(1, disks.size());
            UNIT_ASSERT_VALUES_EQUAL(seqNo2, disks.at("disk-1"));
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            state.DeleteDiskToNotify(db, "disk-1", seqNo2);
        });

        {
            const auto& disks = state.GetDisksToNotify();
            UNIT_ASSERT_VALUES_EQUAL(0, disks.size());
        }
    }

    Y_UNIT_TEST(ShouldHandleCmsRequestsForHosts)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1")
        });

        ui64 lastSeqNo = 0;
        NProto::TStorageServiceConfig proto;
        proto.SetNonReplicatedInfraTimeout(TDuration::Days(1).MilliSeconds());
        proto.SetNonReplicatedInfraUnavailableAgentTimeout(
            TDuration::Hours(1).MilliSeconds());
        auto storageConfig = CreateStorageConfig(proto);

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .With(storageConfig)
            .WithKnownAgents({ agent1 })
            .WithDisks({
                Disk("disk-1", {"uuid-1"}),
                Disk("disk-3", {"uuid-2"}),
            })
            .With(lastSeqNo)
            .Build();

        auto ts = Now();
        TDuration cmsTimeout;

        // agent-1: online -> warning
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateCmsHostState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                ts,
                false, // dryRun
                affectedDisks,
                cmsTimeout);

            UNIT_ASSERT_VALUES_EQUAL(E_TRY_AGAIN, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(
                storageConfig->GetNonReplicatedInfraTimeout(),
                cmsTimeout);

            UNIT_ASSERT_VALUES_EQUAL(2, affectedDisks.size());
            Sort(affectedDisks, TByDiskId());

            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_MIGRATION, affectedDisks[0]);
            UNIT_ASSERT_DISK_STATE("disk-3", DISK_STATE_MIGRATION, affectedDisks[1]);

            UNIT_ASSERT_VALUES_UNEQUAL(affectedDisks[0].SeqNo, affectedDisks[1].SeqNo);
            lastSeqNo = std::max(affectedDisks[0].SeqNo, affectedDisks[1].SeqNo);
        });

        UNIT_ASSERT_VALUES_EQUAL(1, lastSeqNo);

        // agent-1: warning -> warning
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                ts + TDuration::Seconds(10),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_ALREADY, error.GetCode());
            {
                auto res = state.GetAgentCmsTs(agent1.GetAgentId());
                UNIT_ASSERT_VALUES_EQUAL(true, res.Defined());
                UNIT_ASSERT_VALUES_EQUAL(ts, *res);
            }

            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        // agent-1: warning -> unavailable
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_UNAVAILABLE,
                ts + TDuration::Seconds(20),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            {
                auto res = state.GetAgentCmsTs(agent1.GetAgentId());
                UNIT_ASSERT_VALUES_EQUAL(true, res.Defined());
                UNIT_ASSERT_VALUES_EQUAL(ts, *res);
            }
        });

        ts += storageConfig->GetNonReplicatedInfraUnavailableAgentTimeout() / 2;

        // cms comes and requests host removal
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            TDuration timeout;
            auto error = state.UpdateCmsHostState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                ts + TDuration::Seconds(10),
                false, // dryRun
                affectedDisks,
                timeout);

            UNIT_ASSERT_VALUES_EQUAL(E_TRY_AGAIN, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(
                storageConfig->GetNonReplicatedInfraUnavailableAgentTimeout() / 2
                    - TDuration::Seconds(10),
                timeout);
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        ts += storageConfig->GetNonReplicatedInfraUnavailableAgentTimeout() / 2
            + TDuration::Seconds(1);

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            TDuration timeout;
            auto error = state.UpdateCmsHostState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                ts + TDuration::Seconds(10),
                false, // dryRun
                affectedDisks,
                timeout);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(TDuration(), timeout);
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        // mark agent as unavailable
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_UNAVAILABLE,
                ts + TDuration::Seconds(20),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_ALREADY, error.GetCode());
            {
                auto res = state.GetAgentCmsTs(agent1.GetAgentId());
                UNIT_ASSERT_VALUES_EQUAL(true, res.Defined());
                UNIT_ASSERT_VALUES_EQUAL(TInstant(), *res);
            }

            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        // mark agent as online
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_ONLINE,
                ts + TDuration::Seconds(30),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            {
                auto res = state.GetAgentCmsTs(agent1.GetAgentId());
                UNIT_ASSERT_VALUES_EQUAL(true, res.Defined());
                UNIT_ASSERT_VALUES_EQUAL(TInstant(), *res);
            }

            UNIT_ASSERT_VALUES_EQUAL(2, affectedDisks.size());
            Sort(affectedDisks, TByDiskId());

            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_ONLINE, affectedDisks[0]);
            UNIT_ASSERT_DISK_STATE("disk-3", DISK_STATE_ONLINE, affectedDisks[1]);
        });

        ts += TDuration::Seconds(40);

        // cms comes and requests host removal
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            TDuration timeout;
            auto error = state.UpdateCmsHostState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                ts,
                false, // dryRun
                affectedDisks,
                timeout);

            UNIT_ASSERT_VALUES_EQUAL(E_TRY_AGAIN, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(
                storageConfig->GetNonReplicatedInfraTimeout(),
                timeout);
            UNIT_ASSERT_VALUES_EQUAL(2, affectedDisks.size());
            Sort(affectedDisks, TByDiskId());

            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_MIGRATION, affectedDisks[0]);
            UNIT_ASSERT_DISK_STATE("disk-3", DISK_STATE_MIGRATION, affectedDisks[1]);
        });

        ts += storageConfig->GetNonReplicatedInfraTimeout() / 2;

        // cms comes and requests host removal
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            TDuration timeout;
            auto error = state.UpdateCmsHostState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                ts,
                false, // dryRun
                affectedDisks,
                timeout);

            UNIT_ASSERT_VALUES_EQUAL(E_TRY_AGAIN, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(
                storageConfig->GetNonReplicatedInfraTimeout() / 2,
                timeout);
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        ts += storageConfig->GetNonReplicatedInfraTimeout() / 2
            + TDuration::Seconds(1);

        // timer should be restarted
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            TDuration timeout;
            auto error = state.UpdateCmsHostState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                ts + TDuration::Seconds(10),
                false, // dryRun
                affectedDisks,
                timeout);

            UNIT_ASSERT_VALUES_EQUAL(E_TRY_AGAIN, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(
                storageConfig->GetNonReplicatedInfraTimeout(),
                timeout);
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });
    }

    Y_UNIT_TEST(ShouldHandleCmsRequestsForDevices)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1")
        });

        ui64 lastSeqNo = 0;
        auto storageConfig = CreateStorageConfig();

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .With(storageConfig)
            .WithKnownAgents({ agent1 })
            .WithDisks({
                Disk("disk-1", {"uuid-1"}),
                Disk("disk-3", {"uuid-2"}),
            })
            .With(lastSeqNo)
            .Build();

        auto ts = Now();

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            TDuration timeout;
            auto error = state.UpdateCmsDeviceState(
                db,
                "uuid-2",
                NProto::DEVICE_STATE_WARNING,
                ts,
                false,
                affectedDisk,
                timeout);

            UNIT_ASSERT_VALUES_EQUAL(E_TRY_AGAIN, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(storageConfig->GetNonReplicatedInfraTimeout(), timeout);
            UNIT_ASSERT(affectedDisk.Defined());

            UNIT_ASSERT_DISK_STATE("disk-3", DISK_STATE_MIGRATION, *affectedDisk);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            auto error = state.UpdateDeviceState(
                db,
                "uuid-2",
                NProto::DEVICE_STATE_WARNING,
                ts,
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_ALREADY, error.GetCode());
            UNIT_ASSERT(affectedDisk.Empty());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            auto error = state.UpdateDeviceState(
                db,
                "uuid-2",
                NProto::DEVICE_STATE_ERROR,
                ts,
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(affectedDisk.Defined());

            UNIT_ASSERT_DISK_STATE("disk-3", DISK_STATE_ERROR, *affectedDisk);
        });

        ts = ts + storageConfig->GetNonReplicatedInfraTimeout();

        // cms comes and request host removal
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            TDuration timeout;
            auto error = state.UpdateCmsDeviceState(
                db,
                "uuid-2",
                NProto::DEVICE_STATE_WARNING,
                ts + TDuration::Seconds(10),
                false,
                affectedDisk,
                timeout);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(
                TDuration(),
                timeout);
            UNIT_ASSERT(affectedDisk.Empty());
        });

        // mark agent is unavailable
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            auto error = state.UpdateDeviceState(
                db,
                "uuid-2",
                NProto::DEVICE_STATE_ERROR,
                ts + TDuration::Seconds(20),
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_ALREADY, error.GetCode());
            UNIT_ASSERT(affectedDisk.Empty());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            auto error = state.UpdateDeviceState(
                db,
                "uuid-2",
                NProto::DEVICE_STATE_ONLINE,
                ts + TDuration::Seconds(20),
                "test",
                affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
        });
    }

    Y_UNIT_TEST(ShouldNotRestoreOnlineStateAutomatically)
    {
        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1.1", "rack-1", DefaultBlockSize, 10_GB),
        });

        const TInstant errorTs = TInstant::MicroSeconds(10);
        const TInstant regTs = TInstant::MicroSeconds(20);
        const TInstant errorTs2 = TInstant::MicroSeconds(30);

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1 })
            .Build();

        {
            auto agents = state.GetAgents();
            UNIT_ASSERT_VALUES_EQUAL(1, agents.size());
            UNIT_ASSERT_EQUAL(NProto::AGENT_STATE_ONLINE, agents[0].GetState());
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_UNAVAILABLE,
                errorTs,
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
        });

        {
            auto agents = state.GetAgents();
            UNIT_ASSERT_VALUES_EQUAL(1, agents.size());
            UNIT_ASSERT_EQUAL(
                NProto::AGENT_STATE_UNAVAILABLE,
                agents[0].GetState()
            );
        }

        executor.ReadTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<NProto::TAgentConfig> agents;
            db.ReadAgents(agents);
            UNIT_ASSERT_VALUES_EQUAL(1, agents.size());
            UNIT_ASSERT_EQUAL(
                NProto::AGENT_STATE_UNAVAILABLE,
                agents[0].GetState()
            );
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDiskStateUpdate> updates;
            THashMap<TString, ui64> notifiedDisks;
            UNIT_ASSERT_SUCCESS(state.RegisterAgent(
                db,
                agent1,
                regTs,
                &updates,
                &notifiedDisks));

            UNIT_ASSERT_VALUES_EQUAL(0, updates.size());
            UNIT_ASSERT_VALUES_EQUAL(0, notifiedDisks.size());
        });

        {
            auto agents = state.GetAgents();
            UNIT_ASSERT_VALUES_EQUAL(1, agents.size());
            UNIT_ASSERT_EQUAL(
                NProto::AGENT_STATE_WARNING,
                agents[0].GetState()
            );
        }

        executor.ReadTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<NProto::TAgentConfig> agents;
            db.ReadAgents(agents);
            UNIT_ASSERT_VALUES_EQUAL(1, agents.size());
            UNIT_ASSERT_EQUAL(
                NProto::AGENT_STATE_WARNING,
                agents[0].GetState()
            );
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_ONLINE,
                errorTs2,
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL_C(
                S_OK,
                error.GetCode(),
                error.GetMessage()
            );
        });

        {
            auto agents = state.GetAgents();
            UNIT_ASSERT_VALUES_EQUAL(1, agents.size());
            UNIT_ASSERT_EQUAL(
                NProto::AGENT_STATE_ONLINE,
                agents[0].GetState()
            );
        }

        executor.ReadTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<NProto::TAgentConfig> agents;
            db.ReadAgents(agents);
            UNIT_ASSERT_VALUES_EQUAL(1, agents.size());
            UNIT_ASSERT_EQUAL(
                NProto::AGENT_STATE_ONLINE,
                agents[0].GetState()
            );
        });
    }

    Y_UNIT_TEST(ShouldHandleDeviceSizeMismatch)
    {
        const auto agent1a = AgentConfig(1, {
            Device("dev-1", "uuid-1.1", "rack-1", DefaultBlockSize, 10_GB),
            Device("dev-2", "uuid-1.2", "rack-1", DefaultBlockSize, 10_GB),
            Device("dev-3", "uuid-1.3", "rack-1", DefaultBlockSize, 10_GB)
        });

        const auto agent1b = AgentConfig(1, {
            Device("dev-1", "uuid-1.1", "rack-1", 4_KB, 16_GB),
            Device("dev-2", "uuid-1.2", "rack-1", DefaultBlockSize, 10_GB),
            Device("dev-3", "uuid-1.3", "rack-1", 4_KB, 8_GB)
        });

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1a })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;

            auto error = AllocateDisk(db, state, "disk-1", {}, 30_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(3, devices.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDiskStateUpdate> affectedDisks;
            THashMap<TString, ui64> disksToNotify;

            UNIT_ASSERT_SUCCESS(state.RegisterAgent(
                db,
                agent1b,
                Now(),
                &affectedDisks,
                &disksToNotify
            ));

            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());
            const auto& diskState = affectedDisks[0].State;
            UNIT_ASSERT_VALUES_EQUAL("disk-1", diskState.GetDiskId());
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ERROR, diskState.GetState());

            UNIT_ASSERT_EQUAL(
                NProto::DEVICE_STATE_ERROR,
                state.GetDevice("uuid-1.1").GetState()
            );

            UNIT_ASSERT_EQUAL(
                NProto::DEVICE_STATE_ONLINE,
                state.GetDevice("uuid-1.2").GetState()
            );

            UNIT_ASSERT_EQUAL(
                NProto::DEVICE_STATE_ERROR,
                state.GetDevice("uuid-1.3").GetState()
            );
        });
    }

    Y_UNIT_TEST(ShouldVerifyConfig)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> affectedDisks;

            NProto::TDiskRegistryConfig config;
            auto& agent = *config.AddKnownAgents();
            agent.SetAgentId("foo");
            agent.AddDevices()->SetDeviceUUID("uuid-1");
            agent.AddDevices()->SetDeviceUUID("uuid-2");
            agent.AddDevices()->SetDeviceUUID("uuid-2");

            auto error = state.UpdateConfig(
                db,
                config,
                false, // ignoreVersion
                affectedDisks
            );

            UNIT_ASSERT_VALUES_EQUAL(E_ARGUMENT, error.GetCode());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> affectedDisks;

            NProto::TDiskRegistryConfig config;
            {
                auto& agent = *config.AddKnownAgents();
                agent.SetAgentId("foo");
                agent.AddDevices()->SetDeviceUUID("uuid-1");
                agent.AddDevices()->SetDeviceUUID("uuid-2");
            }

            {
                auto& agent = *config.AddKnownAgents();
                agent.SetAgentId("bar");
                agent.AddDevices()->SetDeviceUUID("uuid-3");
                agent.AddDevices()->SetDeviceUUID("uuid-2");
            }

            auto error = state.UpdateConfig(
                db,
                config,
                false, // ignoreVersion
                affectedDisks
            );

            UNIT_ASSERT_VALUES_EQUAL(E_ARGUMENT, error.GetCode());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> affectedDisks;

            NProto::TDiskRegistryConfig config;
            {
                auto& agent = *config.AddKnownAgents();
                agent.SetAgentId("foo");
                agent.AddDevices()->SetDeviceUUID("uuid-1");
                agent.AddDevices()->SetDeviceUUID("uuid-2");
            }

            {
                auto& agent = *config.AddKnownAgents();
                agent.SetAgentId("foo");
                agent.AddDevices()->SetDeviceUUID("uuid-3");
                agent.AddDevices()->SetDeviceUUID("uuid-4");
            }

            auto error = state.UpdateConfig(
                db,
                config,
                false, // ignoreVersion
                affectedDisks
            );

            UNIT_ASSERT_VALUES_EQUAL(E_ARGUMENT, error.GetCode());
        });
    }

    Y_UNIT_TEST(ShouldReturnOkForDeviceCmsRequestIfNoUserDisks)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1")
        });

        ui64 lastSeqNo = 0;
        auto storageConfig = CreateStorageConfig();

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .With(storageConfig)
            .WithKnownAgents({ agent1 })
            .With(lastSeqNo)
            .Build();

        auto ts = Now();

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            TDuration timeout;
            auto error = state.UpdateCmsDeviceState(
                db,
                "uuid-2",
                NProto::DEVICE_STATE_WARNING,
                ts,
                false,
                affectedDisk,
                timeout);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(TDuration::Seconds(0), timeout);
            UNIT_ASSERT(!affectedDisk.Defined());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            TDuration timeout;
            auto error = state.UpdateCmsDeviceState(
                db,
                "uuid-2",
                NProto::DEVICE_STATE_WARNING,
                ts + TDuration::Seconds(10),
                true,
                affectedDisk,
                timeout);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(TDuration::Seconds(0), timeout);
            UNIT_ASSERT(!affectedDisk.Defined());
        });
    }

    Y_UNIT_TEST(ShouldReturnOkForAgentCmsRequestIfNoUserDisks)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1")
        });

        ui64 lastSeqNo = 0;
        auto storageConfig = CreateStorageConfig();

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .With(storageConfig)
            .WithKnownAgents({ agent1 })
            .With(lastSeqNo)
            .Build();

        auto ts = Now();

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            TDuration timeout;
            auto error = state.UpdateCmsHostState(
                db,
                "agent-1",
                NProto::AGENT_STATE_WARNING,
                ts,
                false,
                affectedDisks,
                timeout);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(TDuration::Seconds(0), timeout);
            UNIT_ASSERT(!affectedDisks.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            TDuration timeout;
            auto error = state.UpdateCmsHostState(
                db,
                "agent-1",
                NProto::AGENT_STATE_WARNING,
                ts + TDuration::Seconds(10),
                true,
                affectedDisks,
                timeout);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(TDuration::Seconds(0), timeout);
            UNIT_ASSERT(!affectedDisks.size());
        });
    }

    Y_UNIT_TEST(ShouldRemoveOutdatedAgent)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        auto agent1 = AgentConfig(1000, "agent-1", {
            Device("dev-1", "uuid-1", "rack-1"),
        });

        auto agent2 = AgentConfig(1000, "agent-2", {
            Device("dev-1", "uuid-2", "rack-2"),
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UpdateConfig(state, db, {agent1, agent2});
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agent1));
        });

        {
            const auto agents = state.GetAgents();

            UNIT_ASSERT_VALUES_EQUAL(1, agents.size());
            UNIT_ASSERT_VALUES_EQUAL(1000, agents[0].GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL("agent-1", agents[0].GetAgentId());

            const auto device = state.GetDevice("uuid-1");
            UNIT_ASSERT_VALUES_EQUAL("uuid-1", device.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(1000, device.GetNodeId());
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto error = RegisterAgent(state, db, agent2);
            UNIT_ASSERT_VALUES_EQUAL(
                E_INVALID_STATE,
                RegisterAgent(state, db, agent2).GetCode());

            TVector<TDiskStateUpdate> affectedDisks;
            UNIT_ASSERT_SUCCESS(
                state.UpdateAgentState(
                    db,
                    "agent-1",
                    NProto::AGENT_STATE_UNAVAILABLE,
                    Now(),
                    "state message",
                    affectedDisks));

            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agent2));
        });

        {
            auto agents = state.GetAgents();
            Sort(agents, TByNodeId());

            UNIT_ASSERT_VALUES_EQUAL(2, agents.size());
            UNIT_ASSERT_VALUES_EQUAL(0, agents[0].GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL("agent-1", agents[0].GetAgentId());

            UNIT_ASSERT_VALUES_EQUAL(1000, agents[1].GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL("agent-2", agents[1].GetAgentId());

            auto d1 = state.GetDevice("uuid-1");
            UNIT_ASSERT_VALUES_EQUAL("uuid-1", d1.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(0, d1.GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL("agent-1", d1.GetAgentId());

            auto d2 = state.GetDevice("uuid-2");
            UNIT_ASSERT_VALUES_EQUAL("uuid-2", d2.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(1000, d2.GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL("agent-2", d2.GetAgentId());
        }

        executor.ReadTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<NProto::TAgentConfig> agents;

            db.ReadAgents(agents);
            Sort(agents, TByNodeId());

            UNIT_ASSERT_VALUES_EQUAL(2, agents.size());

            UNIT_ASSERT_VALUES_EQUAL(0, agents[0].GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL("agent-1", agents[0].GetAgentId());

            UNIT_ASSERT_VALUES_EQUAL(1000, agents[1].GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL("agent-2", agents[1].GetAgentId());
        });

        executor.ReadTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<NProto::TAgentConfig> agents;

            db.ReadOldAgents(agents);

            UNIT_ASSERT_VALUES_EQUAL(1, agents.size());

            UNIT_ASSERT_VALUES_EQUAL(1000, agents[0].GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL("agent-2", agents[0].GetAgentId());
        });
    }

    Y_UNIT_TEST(ShouldBackupState)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({
                AgentConfig(1, {
                    Device("dev-1", "uuid-1.1", "rack-1"),
                    Device("dev-2", "uuid-1.2", "rack-1"),
                    Device("dev-3", "uuid-1.3", "rack-1"),
                    Device("dev-4", "uuid-1.4", "rack-1"),
                }),
                AgentConfig(2, {
                    Device("dev-1", "uuid-2.1", "rack-2"),
                    Device("dev-2", "uuid-2.2", "rack-2"),
                    Device("dev-3", "uuid-2.3", "rack-2"),
                    Device("dev-4", "uuid-2.4", "rack-2")
                }),
                AgentConfig(3, {
                    Device("dev-1", "uuid-3.1", "rack-3"),
                    Device("dev-2", "uuid-3.2", "rack-3"),
                    Device("dev-3", "uuid-3.3", "rack-3"),
                    Device("dev-4", "uuid-3.4", "rack-3")
                }),
                AgentConfig(4, {
                    Device("dev-1", "uuid-4.1", "rack-4"),
                    Device("dev-2", "uuid-4.2", "rack-4"),
                    Device("dev-3", "uuid-4.3", "rack-4"),
                    Device("dev-4", "uuid-4.4", "rack-4")
                })
            })
            .Build();

        TVector<TVector<TDeviceConfig>> diskDevices(5);

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(state.CreatePlacementGroup(db, "group-1"));
            UNIT_ASSERT_SUCCESS(state.CreatePlacementGroup(db, "group-2"));
            UNIT_ASSERT_SUCCESS(
                AllocateDisk(db, state, "disk-1", "group-1", 20_GB, diskDevices[0]));
            UNIT_ASSERT_SUCCESS(
                AllocateDisk(db, state, "disk-2", "group-1", 20_GB, diskDevices[1]));
            UNIT_ASSERT_SUCCESS(
                AllocateDisk(db, state, "disk-3", "group-2", 20_GB, diskDevices[2]));
            UNIT_ASSERT_SUCCESS(
                AllocateDisk(db, state, "disk-4", "group-2", 20_GB, diskDevices[3]));
            UNIT_ASSERT_SUCCESS(
                AllocateDisk(db, state, "disk-5", {}, 40_GB, diskDevices[4]));
        });

        auto snapshot = state.BackupState();
        UNIT_ASSERT_VALUES_EQUAL(5, snapshot.DisksSize());
        UNIT_ASSERT_VALUES_EQUAL(2, snapshot.PlacementGroupsSize());
        UNIT_ASSERT_VALUES_EQUAL(4, snapshot.AgentsSize());

        SortBy(*snapshot.MutableDisks(), [](auto& x) {
            return x.GetDiskId();
        });

        SortBy(*snapshot.MutablePlacementGroups(), [](auto& x) {
            return x.GetGroupId();
        });

        auto equalDevices = [] (auto& disk, const auto& devices) {
            UNIT_ASSERT(std::equal(
                disk.GetDeviceUUIDs().begin(),
                disk.GetDeviceUUIDs().end(),
                devices.begin(),
                devices.end(),
                [] (auto& x, auto& y) {
                    return x == y.GetDeviceUUID();
                }));
        };

        for (size_t i = 0; i != snapshot.DisksSize(); ++i) {
            auto& disk = snapshot.GetDisks(i);
            UNIT_ASSERT_VALUES_EQUAL("disk-" + ToString(i + 1), disk.GetDiskId());
            equalDevices(disk, diskDevices[i]);
        }

        {
            auto& pg = *snapshot.MutablePlacementGroups(0);
            UNIT_ASSERT_VALUES_EQUAL("group-1", pg.GetGroupId());
            UNIT_ASSERT_VALUES_EQUAL(2, pg.DisksSize());
            UNIT_ASSERT_VALUES_EQUAL("disk-1", pg.GetDisks(0).GetDiskId());
            UNIT_ASSERT_VALUES_EQUAL(1, pg.GetDisks(0).DeviceRacksSize());
            UNIT_ASSERT_VALUES_EQUAL("disk-2", pg.GetDisks(1).GetDiskId());
            UNIT_ASSERT_VALUES_EQUAL(1, pg.GetDisks(1).DeviceRacksSize());
        }

        {
            auto& pg = *snapshot.MutablePlacementGroups(1);
            UNIT_ASSERT_VALUES_EQUAL("group-2", pg.GetGroupId());
            UNIT_ASSERT_VALUES_EQUAL(2, pg.DisksSize());
            UNIT_ASSERT_VALUES_EQUAL("disk-3", pg.GetDisks(0).GetDiskId());
            UNIT_ASSERT_VALUES_EQUAL(1, pg.GetDisks(0).DeviceRacksSize());
            UNIT_ASSERT_VALUES_EQUAL("disk-4", pg.GetDisks(1).GetDiskId());
            UNIT_ASSERT_VALUES_EQUAL(1, pg.GetDisks(1).DeviceRacksSize());
        }
    }

    Y_UNIT_TEST(ShouldMigrateDiskDevice)
    {
        const TVector agents {
            AgentConfig(1, {
                Device("dev-1", "uuid-1.1", "rack-1"),
                Device("dev-2", "uuid-1.2", "rack-1")
            }),
            AgentConfig(2, {
                Device("dev-1", "uuid-2.1", "rack-2")
            })
        };

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents(agents)
            .WithDisks({Disk("disk-1", {"uuid-2.1", "uuid-1.1"})})
            .Build();

        UNIT_ASSERT_VALUES_EQUAL(0, state.BuildMigrationList().size());
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TDiskRegistryState::TAllocateDiskResult result {};

            UNIT_ASSERT_SUCCESS(state.AllocateDisk(
                Now(),
                db,
                TDiskRegistryState::TAllocateDiskParams {
                    .DiskId = "disk-1",
                    .BlockSize = DefaultLogicalBlockSize,
                    .BlocksCount = 20_GB / DefaultLogicalBlockSize
                },
                &result));

            UNIT_ASSERT_VALUES_EQUAL(2, result.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", result.Devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", result.Devices[1].GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::VOLUME_IO_OK, result.IOMode);
            UNIT_ASSERT_UNEQUAL(TInstant::Zero(), result.IOModeTs);
            UNIT_ASSERT_VALUES_EQUAL(0, result.Migrations.size());
        });

        UNIT_ASSERT_VALUES_EQUAL(0, state.BuildMigrationList().size());
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            const auto error = state.UpdateDeviceState(
                db,
                "uuid-2.1",
                NProto::DEVICE_STATE_WARNING,
                Now(),
                "test",
                affectedDisk);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(affectedDisk.Defined());
            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_MIGRATION, *affectedDisk);
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisk->SeqNo);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TDiskRegistryState::TAllocateDiskResult result {};

            UNIT_ASSERT_SUCCESS(state.AllocateDisk(
                Now(),
                db,
                TDiskRegistryState::TAllocateDiskParams {
                    .DiskId = "disk-1",
                    .BlockSize = DefaultLogicalBlockSize,
                    .BlocksCount = 20_GB / DefaultLogicalBlockSize
                },
                &result));

            UNIT_ASSERT_VALUES_EQUAL(2, result.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", result.Devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", result.Devices[1].GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::VOLUME_IO_OK, result.IOMode);
            UNIT_ASSERT_UNEQUAL(TInstant::Zero(), result.IOModeTs);
            UNIT_ASSERT_VALUES_EQUAL(0, result.Migrations.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            const auto m = state.BuildMigrationList();
            UNIT_ASSERT_VALUES_EQUAL(1, m.size());
            auto it = m.begin();
            UNIT_ASSERT_VALUES_EQUAL("disk-1", it->DiskId);
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", it->SourceDeviceId);
            auto target = state.StartDeviceMigration(db, it->DiskId, it->SourceDeviceId);
            UNIT_ASSERT_SUCCESS(target.GetError());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.2", target.GetResult().GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(0, state.BuildMigrationList().size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TDiskRegistryState::TAllocateDiskResult result {};

            UNIT_ASSERT_SUCCESS(state.AllocateDisk(
                Now(),
                db,
                TDiskRegistryState::TAllocateDiskParams {
                    .DiskId = "disk-1",
                    .BlockSize = DefaultLogicalBlockSize,
                    .BlocksCount = 20_GB / DefaultLogicalBlockSize
                },
                &result));

            UNIT_ASSERT_VALUES_EQUAL(2, result.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", result.Devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", result.Devices[1].GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::VOLUME_IO_OK, result.IOMode);
            UNIT_ASSERT_UNEQUAL(TInstant::Zero(), result.IOModeTs);
            UNIT_ASSERT_VALUES_EQUAL(1, result.Migrations.size());
            const auto& m = result.Migrations[0];
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", m.GetSourceDeviceId());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.2", m.GetTargetDevice().GetDeviceUUID());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;

            const auto error = state.FinishDeviceMigration(
                db,
                "disk-1",
                "uuid-2.1",
                "uuid-1.2",
                Now(),
                &affectedDisk);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(affectedDisk.Defined());
            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_ONLINE, *affectedDisk);
            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisk->SeqNo);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TDiskRegistryState::TAllocateDiskResult result {};

            UNIT_ASSERT_SUCCESS(state.AllocateDisk(
                Now(),
                db,
                TDiskRegistryState::TAllocateDiskParams {
                    .DiskId = "disk-1",
                    .BlockSize = DefaultLogicalBlockSize,
                    .BlocksCount = 20_GB / DefaultLogicalBlockSize
                },
                &result));

            UNIT_ASSERT_VALUES_EQUAL(2, result.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.2", result.Devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", result.Devices[1].GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::VOLUME_IO_OK, result.IOMode);
            UNIT_ASSERT_UNEQUAL(TInstant::Zero(), result.IOModeTs);
            UNIT_ASSERT_VALUES_EQUAL(0, result.Migrations.size());
        });
    }

    Y_UNIT_TEST(ShouldNotMigrateMoreThanNDevicesAtTheSameTime)
    {
        const TVector agents {
            AgentConfig(1, {
                Device("dev-1", "uuid-1.1", "rack-1"),
                Device("dev-2", "uuid-1.2", "rack-1"),
            }),
            AgentConfig(2, {
                Device("dev-1", "uuid-2.1", "rack-2"),
            }),
            AgentConfig(3, {
                Device("dev-1", "uuid-3.1", "rack-3"),
                Device("dev-2", "uuid-3.2", "rack-3"),
                Device("dev-3", "uuid-3.3", "rack-3"),
            })
        };

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents(agents)
            .WithDisks({
                Disk("disk-1", {"uuid-1.1", "uuid-1.2"}),
                Disk("disk-2", {"uuid-2.1"}),
            })
            .WithStorageConfig([] {
                auto config = CreateDefaultStorageConfigProto();
                config.SetMaxNonReplicatedDeviceMigrationsInProgress(2);
                return config;
            }())
            .Build();

        UNIT_ASSERT_VALUES_EQUAL(0, state.BuildMigrationList().size());

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;

            const auto error = state.UpdateAgentState(
                db,
                "agent-1",
                NProto::AGENT_STATE_WARNING,
                Now(),
                "state message",
                affectedDisks
            );
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());
            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_MIGRATION, affectedDisks[0]);
        });

        {
            auto migrations = state.BuildMigrationList();
            UNIT_ASSERT_VALUES_EQUAL(2, migrations.size());
            UNIT_ASSERT_VALUES_EQUAL("disk-1", migrations[0].DiskId);
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", migrations[0].SourceDeviceId);
            UNIT_ASSERT_VALUES_EQUAL("disk-1", migrations[1].DiskId);
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.2", migrations[1].SourceDeviceId);
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;

            const auto error = state.UpdateAgentState(
                db,
                "agent-2",
                NProto::AGENT_STATE_WARNING,
                Now(),
                "state message",
                affectedDisks
            );
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());
            UNIT_ASSERT_DISK_STATE("disk-2", DISK_STATE_MIGRATION, affectedDisks[0]);
        });

        {
            auto migrations = state.BuildMigrationList();
            UNIT_ASSERT_VALUES_EQUAL(2, migrations.size());
            UNIT_ASSERT_VALUES_EQUAL("disk-1", migrations[0].DiskId);
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", migrations[0].SourceDeviceId);
            UNIT_ASSERT_VALUES_EQUAL("disk-1", migrations[1].DiskId);
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.2", migrations[1].SourceDeviceId);
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            for (const auto& [diskId, deviceId]: state.BuildMigrationList()) {
                UNIT_ASSERT_SUCCESS(
                    state.StartDeviceMigration(db, diskId, deviceId).GetError()
                );
            }
        });

        {
            auto migrations = state.BuildMigrationList();
            UNIT_ASSERT_VALUES_EQUAL(0, migrations.size());
        }

        // finish migration
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TDiskInfo diskInfo;
            UNIT_ASSERT_SUCCESS(state.GetDiskInfo("disk-1", diskInfo));
            for (const auto& m: diskInfo.Migrations) {
                TMaybe<TDiskStateUpdate> affectedDisk;
                UNIT_ASSERT_SUCCESS(state.FinishDeviceMigration(
                    db,
                    "disk-1",
                    m.GetSourceDeviceId(),
                    m.GetTargetDevice().GetDeviceUUID(),
                    Now(),
                    &affectedDisk));
            }
        });

        {
            auto migrations = state.BuildMigrationList();
            UNIT_ASSERT_VALUES_EQUAL(1, migrations.size());
            UNIT_ASSERT_VALUES_EQUAL("disk-2", migrations[0].DiskId);
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", migrations[0].SourceDeviceId);
        }
    }

    Y_UNIT_TEST(ShouldHandleBrokenDevices)
    {
        const auto agentA = AgentConfig(1111, {
            Device("dev-1", "uuid-1", "rack-1", DefaultBlockSize, 93_GB),
            Device("dev-2", "uuid-2", "rack-1", DefaultBlockSize, 93_GB),
            Device("dev-3", "uuid-3", "rack-1", DefaultBlockSize, 93_GB),
            Device("dev-4", "uuid-4", "rack-1", DefaultBlockSize, 93_GB),
        });

        auto agentB = AgentConfig(1111, {
            Device("dev-1", "uuid-1", "rack-2", DefaultBlockSize, 94_GB),
            Device("dev-2", "uuid-2", "rack-2", DefaultBlockSize, 94_GB),
            Device("dev-3", "uuid-3", "rack-2", DefaultBlockSize, 93_GB),
            Device("dev-4", "uuid-4", "rack-2", DefaultBlockSize, 94_GB),
        });
        agentB.SetNodeId(42);

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agentA })
            .WithStorageConfig([] {
                auto config = CreateDefaultStorageConfigProto();
                config.SetAllocationUnitNonReplicatedSSD(93);
                return config;
            }())
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agentB));
        });

        UNIT_ASSERT(state.FindAgent(1) == nullptr);
        UNIT_ASSERT(state.FindAgent(42) != nullptr);

        for (size_t i = 1; i != agentB.DevicesSize() + 1; ++i) {
            const auto device = state.GetDevice(Sprintf("uuid-%lu", i));
            UNIT_ASSERT_VALUES_EQUAL(Sprintf("dev-%lu", i), device.GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL(DefaultBlockSize, device.GetBlockSize());
            UNIT_ASSERT_VALUES_EQUAL(93_GB / DefaultBlockSize, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(42, device.GetNodeId());
            UNIT_ASSERT_VALUES_EQUAL("rack-2", device.GetRack());
            UNIT_ASSERT_VALUES_EQUAL(agentB.GetAgentId(), device.GetAgentId());

            if (i == 3) {
                UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, device.GetState());
            } else {
                UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ERROR, device.GetState());
            }
        }
    }

    Y_UNIT_TEST(ShouldTrackMaxMigrationTime)
    {
        const TVector agents {
            AgentConfig(1, {
                Device("dev-1", "uuid-1.1", "rack-1"),
                Device("dev-2", "uuid-1.2", "rack-1"),
                Device("dev-3", "uuid-1.3", "rack-1"),
                Device("dev-4", "uuid-1.4", "rack-1"),
                Device("dev-5", "uuid-1.5", "rack-1"),
                Device("dev-6", "uuid-1.6", "rack-1"),
            })
        };

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        auto monitoring = CreateMonitoringServiceStub();
        auto diskRegistryGroup = monitoring->GetCounters()
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "disk_registry");
        auto disksInMigrationState = diskRegistryGroup->GetCounter("DisksInMigrationState");
        auto devicesInMigrationState = diskRegistryGroup->GetCounter("DevicesInMigrationState");
        auto maxMigrationTime = diskRegistryGroup->GetCounter("MaxMigrationTime");

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .With(diskRegistryGroup)
            .WithKnownAgents(agents)
            .WithDisks({
                Disk("disk-1", {"uuid-1.1"}),
                Disk("disk-2", {"uuid-1.2"}),
                Disk("disk-3", {"uuid-1.3"}),
                Disk("disk-4", {"uuid-1.4"}),
                Disk("disk-5", {"uuid-1.5"}),
                Disk("disk-6", {"uuid-1.6"}),
            })
            .Build();

        auto kickDevice = [&] (int i, TInstant timestamp, auto newState) {
            TMaybe<TDiskStateUpdate> affectedDisk;

            executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
                const auto error = state.UpdateDeviceState(
                    db,
                    Sprintf("uuid-1.%d", i),
                    newState,
                    timestamp,
                    "test",
                    affectedDisk);
                UNIT_ASSERT_VALUES_EQUAL_C(S_OK, error.GetCode(), error);
                UNIT_ASSERT(affectedDisk.Defined());
            });

            return *affectedDisk;
        };

        auto warnDevice = [&] (int i, TInstant timestamp) {
            auto u = kickDevice(i, timestamp, NProto::DEVICE_STATE_WARNING);
            UNIT_ASSERT_DISK_STATE(Sprintf("disk-%d", i), DISK_STATE_MIGRATION, u);
        };

        auto errorDevice = [&] (int i, TInstant timestamp) {
            auto u = kickDevice(i, timestamp, NProto::DEVICE_STATE_ERROR);
            UNIT_ASSERT_DISK_STATE(Sprintf("disk-%d", i), DISK_STATE_ERROR, u);
        };

        auto onlineDevice = [&] (int i, TInstant timestamp) {
            auto u = kickDevice(i, timestamp, NProto::DEVICE_STATE_ONLINE);
            UNIT_ASSERT_DISK_STATE(Sprintf("disk-%d", i), DISK_STATE_ONLINE, u);
        };

        state.PublishCounters(TInstant::Zero());
        UNIT_ASSERT_VALUES_EQUAL(0, disksInMigrationState->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, devicesInMigrationState->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, maxMigrationTime->Val());

        auto disk1StartMigrationTs = TInstant::Seconds(100);
        auto disk2StartMigrationTs = TInstant::Seconds(200);
        auto disk3StartMigrationTs = TInstant::Seconds(300);

        warnDevice(1, disk1StartMigrationTs);
        warnDevice(2, disk2StartMigrationTs);
        warnDevice(3, disk3StartMigrationTs);

        auto now1 = TInstant::Seconds(1000);
        state.PublishCounters(now1);
        UNIT_ASSERT_VALUES_EQUAL(3, disksInMigrationState->Val());
        UNIT_ASSERT_VALUES_EQUAL(3, devicesInMigrationState->Val());
        UNIT_ASSERT_VALUES_EQUAL(
            (now1 - disk1StartMigrationTs).Seconds(),
            maxMigrationTime->Val());

        // cancel migration for disk-3
        errorDevice(3, TInstant::Seconds(1100));

        auto now2 = TInstant::Seconds(2000);
        state.PublishCounters(now2);
        UNIT_ASSERT_VALUES_EQUAL(2, disksInMigrationState->Val());
        UNIT_ASSERT_VALUES_EQUAL(2, devicesInMigrationState->Val());
        UNIT_ASSERT_VALUES_EQUAL(
            (now2 - disk1StartMigrationTs).Seconds(),
            maxMigrationTime->Val());

        // restart migration for disk-1
        disk1StartMigrationTs = TInstant::Seconds(2300);
        errorDevice(1, disk1StartMigrationTs);
        onlineDevice(1, disk1StartMigrationTs);
        warnDevice(1, disk1StartMigrationTs);

        auto now3 = TInstant::Seconds(3000);
        state.PublishCounters(now3);
        UNIT_ASSERT_VALUES_EQUAL(2, disksInMigrationState->Val());
        UNIT_ASSERT_VALUES_EQUAL(2, devicesInMigrationState->Val());
        UNIT_ASSERT_VALUES_EQUAL(
            (now3 - disk2StartMigrationTs).Seconds(),
            maxMigrationTime->Val());

        // cancel migration for disk-2
        errorDevice(2, TInstant::Seconds(3100));

        auto now4 = TInstant::Seconds(4000);
        state.PublishCounters(now4);
        UNIT_ASSERT_VALUES_EQUAL(1, disksInMigrationState->Val());
        UNIT_ASSERT_VALUES_EQUAL(1, devicesInMigrationState->Val());
        UNIT_ASSERT_VALUES_EQUAL(
            (now4 - disk1StartMigrationTs).Seconds(),
            maxMigrationTime->Val());

        // cancel migration for disk-1
        errorDevice(1, TInstant::Seconds(4100));
        state.PublishCounters(TInstant::Seconds(5000));
        UNIT_ASSERT_VALUES_EQUAL(0, disksInMigrationState->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, devicesInMigrationState->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, maxMigrationTime->Val());
    }

    Y_UNIT_TEST(ShouldGatherRacksInfo)
    {
        const TVector agents {
            AgentConfig(1000, {
                Device("dev-1", "uuid-1.1", "rack-1"),
                Device("dev-2", "uuid-1.2", "rack-1")
            }),
            AgentConfig(2000, {
                Device("dev-1", "uuid-2.1", "rack-2"),
                Device("dev-2", "uuid-2.2", "rack-2")
            }),
            AgentConfig(3000, {
                Device("dev-1", "uuid-3.1", "rack-2"),
                Device("dev-2", "uuid-3.2", "rack-2"),
                Device("dev-3", "uuid-3.3", "rack-2"),
            })
        };

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents(agents)
            .WithDisks({
                Disk("disk-1", {"uuid-1.1"}),
                Disk("disk-2", {"uuid-2.2"})
            })
            .WithDirtyDevices({"uuid-2.1", "uuid-3.3"})
            .Build();

        auto info = state.GatherRacksInfo();

        UNIT_ASSERT_VALUES_EQUAL(2, info.size());
        SortBy(info, [] (auto& x) { return x.Name; });

        {
            auto& rack = info[0];
            UNIT_ASSERT_VALUES_EQUAL("rack-1", rack.Name);
            UNIT_ASSERT_VALUES_EQUAL(10_GB, rack.FreeBytes);
            UNIT_ASSERT_VALUES_EQUAL(20_GB, rack.TotalBytes);
            UNIT_ASSERT_VALUES_EQUAL(1, rack.AgentInfos.size());

            auto& agent = rack.AgentInfos[0];
            UNIT_ASSERT_VALUES_EQUAL("agent-1000", agent.AgentId);
            UNIT_ASSERT_VALUES_EQUAL(1000, agent.NodeId);
            UNIT_ASSERT_VALUES_EQUAL(1, agent.FreeDevices);
            UNIT_ASSERT_VALUES_EQUAL(1, agent.AllocatedDevices);
            UNIT_ASSERT_VALUES_EQUAL(0, agent.BrokenDevices);
            UNIT_ASSERT_VALUES_EQUAL(2, agent.TotalDevices);
        }

        {
            auto& rack = info[1];
            UNIT_ASSERT_VALUES_EQUAL("rack-2", rack.Name);
            UNIT_ASSERT_VALUES_EQUAL(20_GB, rack.FreeBytes);
            UNIT_ASSERT_VALUES_EQUAL(50_GB, rack.TotalBytes);
            UNIT_ASSERT_VALUES_EQUAL(2, rack.AgentInfos.size());
            SortBy(rack.AgentInfos, [] (auto& x) { return x.NodeId; });

            auto& agent1 = rack.AgentInfos[0];
            UNIT_ASSERT_VALUES_EQUAL("agent-2000", agent1.AgentId);
            UNIT_ASSERT_VALUES_EQUAL(2000, agent1.NodeId);
            UNIT_ASSERT_VALUES_EQUAL(0, agent1.FreeDevices);
            UNIT_ASSERT_VALUES_EQUAL(1, agent1.AllocatedDevices);
            UNIT_ASSERT_VALUES_EQUAL(0, agent1.BrokenDevices);
            UNIT_ASSERT_VALUES_EQUAL(2, agent1.TotalDevices);

            auto& agent2 = rack.AgentInfos[1];
            UNIT_ASSERT_VALUES_EQUAL("agent-3000", agent2.AgentId);
            UNIT_ASSERT_VALUES_EQUAL(3000, agent2.NodeId);
            UNIT_ASSERT_VALUES_EQUAL(2, agent2.FreeDevices);
            UNIT_ASSERT_VALUES_EQUAL(0, agent2.AllocatedDevices);
            UNIT_ASSERT_VALUES_EQUAL(0, agent2.BrokenDevices);
            UNIT_ASSERT_VALUES_EQUAL(3, agent2.TotalDevices);
        }
    }

    Y_UNIT_TEST(ShouldCountBrokenPlacementGroups)
    {
        const TVector agents {
            AgentConfig(1000, {Device("dev-1", "uuid-1", "rack-1")}),
            AgentConfig(2000, {Device("dev-1", "uuid-2", "rack-2")}),
            AgentConfig(3000, {Device("dev-1", "uuid-3", "rack-3")}),
            AgentConfig(4000, {Device("dev-1", "uuid-4", "rack-4")}),
            AgentConfig(5000, {Device("dev-1", "uuid-5", "rack-5")}),
            AgentConfig(6000, {Device("dev-1", "uuid-6", "rack-6")}),
            AgentConfig(7000, {Device("dev-1", "uuid-7", "rack-7")}),
            AgentConfig(8000, {Device("dev-1", "uuid-8", "rack-8")}),
            AgentConfig(9000, {Device("dev-1", "uuid-9", "rack-9")}),
        };

        auto monitoring = CreateMonitoringServiceStub();
        auto diskRegistryGroup = monitoring->GetCounters()
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "disk_registry");

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents(agents)
            .With(diskRegistryGroup)
            .WithStorageConfig([] {
                auto config = CreateDefaultStorageConfigProto();
                config.SetPlacementGroupAlertPeriod(TDuration::Days(1).MilliSeconds());
                return config;
            }())
            .WithDisks({
                Disk("disk-1", {"uuid-1"}),
                Disk("disk-2", {"uuid-2"}),
                Disk("disk-3", {"uuid-3"}),
                Disk("disk-4", {"uuid-4"}),
                Disk("disk-5", {"uuid-5"}),
                Disk("disk-6", {"uuid-6"}),
                Disk("disk-7", {"uuid-7"}),
                Disk("disk-8", {"uuid-8"}),
                Disk("disk-9", {"uuid-9"}),
            })
            .AddPlacementGroup("group-1", {"disk-1", "disk-2", "disk-3"})
            .AddPlacementGroup("group-2", {"disk-4", "disk-5", "disk-6"})
            .AddPlacementGroup("group-3", {"disk-7", "disk-8", "disk-9"})
            .Build();

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        auto recentlySingle = diskRegistryGroup->GetCounter(
            "PlacementGroupsWithRecentlyBrokenSingleDisk");
        auto recentlyTwoOrMore = diskRegistryGroup->GetCounter(
            "PlacementGroupsWithRecentlyBrokenTwoOrMoreDisks");
        auto totalSingle = diskRegistryGroup->GetCounter(
            "PlacementGroupsWithBrokenSingleDisk");
        auto totalTwoOrMore = diskRegistryGroup->GetCounter(
            "PlacementGroupsWithBrokenTwoOrMoreDisks");

        state.PublishCounters(TInstant::Zero());

        UNIT_ASSERT_VALUES_EQUAL(0, recentlySingle->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, recentlyTwoOrMore->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, totalSingle->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, totalTwoOrMore->Val());

        auto breakDisk = [&] (int i, auto t) {
            executor.WriteTx([&] (TDiskRegistryDatabase db) {
                TMaybe<TDiskStateUpdate> affectedDisk;
                UNIT_ASSERT_SUCCESS(state.UpdateDeviceState(
                    db,
                    Sprintf("uuid-%d", i),
                    NProto::DEVICE_STATE_ERROR,
                    t,
                    "test",
                    affectedDisk));
                UNIT_ASSERT(affectedDisk);

                const auto& diskState = affectedDisk->State;
                UNIT_ASSERT_VALUES_EQUAL(Sprintf("disk-%d", i), diskState.GetDiskId());
                UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ERROR, diskState.GetState());
            });
        };

        breakDisk(1, TInstant::Hours(2));
        breakDisk(2, TInstant::Hours(4));
        breakDisk(4, TInstant::Hours(6));
        breakDisk(7, TInstant::Hours(8));
        breakDisk(8, TInstant::Hours(10));
        breakDisk(9, TInstant::Hours(12));

        state.PublishCounters(TInstant::Hours(13));

        // group-1: [disk-1(r) disk-2 (r) disk-2]     2/2
        // group-2: [disk-4(r) disk-5     disk-6]     1/1
        // group-3: [disk-7(r) disk-8(r)  disk-9 (r)] 3/3
        UNIT_ASSERT_VALUES_EQUAL(1, recentlySingle->Val());    // group-2
        UNIT_ASSERT_VALUES_EQUAL(2, recentlyTwoOrMore->Val()); // group-1 group-3
        UNIT_ASSERT_VALUES_EQUAL(1, totalSingle->Val());
        UNIT_ASSERT_VALUES_EQUAL(2, totalTwoOrMore->Val());

        state.PublishCounters(TInstant::Hours(27));

        // group-1: [disk-1(d) disk-2 (r) disk-2]     1/2
        // group-2: [disk-4(r) disk-5     disk-6]     1/1
        // group-3: [disk-7(r) disk-8(r)  disk-9 (r)] 3/3
        UNIT_ASSERT_VALUES_EQUAL(2, recentlySingle->Val());    // group-1 group-2
        UNIT_ASSERT_VALUES_EQUAL(1, recentlyTwoOrMore->Val()); // group-3
        UNIT_ASSERT_VALUES_EQUAL(1, totalSingle->Val());
        UNIT_ASSERT_VALUES_EQUAL(2, totalTwoOrMore->Val());

        state.PublishCounters(TInstant::Hours(31));

        // group-1: [disk-1(d) disk-2 (d) disk-2]     0/2
        // group-2: [disk-4(d) disk-5     disk-6]     0/1
        // group-3: [disk-7(r) disk-8(r)  disk-9 (r)] 3/3
        UNIT_ASSERT_VALUES_EQUAL(0, recentlySingle->Val());
        UNIT_ASSERT_VALUES_EQUAL(1, recentlyTwoOrMore->Val()); // group-3
        UNIT_ASSERT_VALUES_EQUAL(1, totalSingle->Val());
        UNIT_ASSERT_VALUES_EQUAL(2, totalTwoOrMore->Val());

        state.PublishCounters(TInstant::Hours(35));

        // group-1: [disk-1(d) disk-2 (d) disk-2]     0/2
        // group-2: [disk-4(d) disk-5     disk-6]     0/1
        // group-3: [disk-7(d) disk-8(d)  disk-9 (r)] 1/3
        UNIT_ASSERT_VALUES_EQUAL(1, recentlySingle->Val()); // group-3
        UNIT_ASSERT_VALUES_EQUAL(0, recentlyTwoOrMore->Val());
        UNIT_ASSERT_VALUES_EQUAL(1, totalSingle->Val());
        UNIT_ASSERT_VALUES_EQUAL(2, totalTwoOrMore->Val());

        state.PublishCounters(TInstant::Hours(37));

        // group-1: [disk-1(d) disk-2 (d) disk-2]     0/2
        // group-2: [disk-4(d) disk-5     disk-6]     0/1
        // group-3: [disk-7(d) disk-8(d)  disk-9 (d)] 0/3
        UNIT_ASSERT_VALUES_EQUAL(0, recentlySingle->Val());
        UNIT_ASSERT_VALUES_EQUAL(0, recentlyTwoOrMore->Val());
        UNIT_ASSERT_VALUES_EQUAL(1, totalSingle->Val());
        UNIT_ASSERT_VALUES_EQUAL(2, totalTwoOrMore->Val());
    }

    Y_UNIT_TEST(ShouldCountFreeBytes)
    {
        const TVector agents {
            AgentConfig(1000, {
                Device("dev-1", "uuid-1.1", "rack-1"),
                Device("dev-2", "uuid-1.2", "rack-1"),
            }),
            AgentConfig(2000, {
                Device("dev-1", "uuid-2.1", "rack-2"),
                Device("dev-2", "uuid-2.2", "rack-2"),
            }),
            AgentConfig(3000, {
                Device("dev-1", "uuid-3.1", "rack-3"),
                Device("dev-2", "uuid-3.2", "rack-3"),
            }),
        };

        auto monitoring = CreateMonitoringServiceStub();
        auto diskRegistryGroup = monitoring->GetCounters()
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "disk_registry");
        auto freeBytes = diskRegistryGroup->GetCounter("FreeBytes");

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents(agents)
            .With(diskRegistryGroup)
            .WithDisks({ Disk("disk-1", {"uuid-1.1"}) })
            .Build();

        state.PublishCounters(TInstant::Zero());

        UNIT_ASSERT_VALUES_EQUAL(10_GB * 5, freeBytes->Val());

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TMaybe<TDiskStateUpdate> affectedDisk;
            UNIT_ASSERT_SUCCESS(state.UpdateDeviceState(
                db,
                "uuid-2.2",
                NProto::DEVICE_STATE_ERROR,
                TInstant::Zero(),
                "test",
                affectedDisk));
            UNIT_ASSERT(!affectedDisk);
        });

        state.PublishCounters(TInstant::Zero());

        UNIT_ASSERT_VALUES_EQUAL(10_GB * 4, freeBytes->Val());

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                "agent-3000",
                NProto::AGENT_STATE_WARNING,
                TInstant::Zero(),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(affectedDisks.empty());
        });

        state.PublishCounters(TInstant::Zero());

        UNIT_ASSERT_VALUES_EQUAL(10_GB * 2, freeBytes->Val());
    }

    Y_UNIT_TEST(ShouldRejectDevicesWithBadRack)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto agent1a = AgentConfig(1000, {
            Device("dev-1", "uuid-1.1", ""),
            Device("dev-2", "uuid-1.2", ""),
        });

        const auto agent1b = AgentConfig(1000, {
            Device("dev-1", "uuid-1.2", "rack-1"),
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({agent1a})
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agent1b));

            {
                TVector<TDeviceConfig> devices;
                auto error = AllocateDisk(db, state, "disk", {}, 20_GB, devices);
                UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
            }

            {
                TVector<TDeviceConfig> devices;
                UNIT_ASSERT_SUCCESS(AllocateDisk(db, state, "disk", {}, 10_GB, devices));
                UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
                UNIT_ASSERT_VALUES_EQUAL("uuid-1.2", devices[0].GetDeviceUUID());
            }
        });
    }

    Y_UNIT_TEST(ShouldNotAllocateDiskOnAgentWithWarningState)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const TVector agents = {
            AgentConfig(1, {
                Device("dev-1", "uuid-1.1", "rack-1"),
                Device("dev-2", "uuid-1.2", "rack-1"),
                Device("dev-3", "uuid-1.3", "rack-1"),
                Device("dev-4", "uuid-1.4", "rack-1"),
            }),
            AgentConfig(2, {
                Device("dev-1", "uuid-2.1", "rack-2"),
                Device("dev-2", "uuid-2.2", "rack-2"),
                Device("dev-3", "uuid-2.3", "rack-2"),
                Device("dev-4", "uuid-2.4", "rack-2")
            })
        };

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithConfig(agents)
            .WithAgents({agents[0]})
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-1", {}, 40_GB, devices);
            UNIT_ASSERT_SUCCESS(error);
            UNIT_ASSERT_VALUES_EQUAL(4, devices.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_VALUES_EQUAL(0, state.GetDirtyDevices().size());
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agents[1]));
            auto dd = state.GetDirtyDevices();
            UNIT_ASSERT_VALUES_EQUAL(agents[1].DevicesSize(), dd.size());
            for (const auto& device: dd) {
                const auto& uuid = device.GetDeviceUUID();
                UNIT_ASSERT(state.MarkDeviceAsClean(db, uuid));
            }
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state.UpdateAgentState(
                db,
                agents[0].GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                Now(),
                "state message",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());
            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_MIGRATION, affectedDisks[0]);
        });

        // start migration
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            for (const auto& [diskId, deviceId]: state.BuildMigrationList()) {
                UNIT_ASSERT_SUCCESS(
                    state.StartDeviceMigration(db, diskId, deviceId).GetError()
                );
            }
        });

        // finish migration
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TDiskInfo diskInfo;
            UNIT_ASSERT_SUCCESS(state.GetDiskInfo("disk-1", diskInfo));
            for (const auto& m: diskInfo.Migrations) {
                TMaybe<TDiskStateUpdate> affectedDisk;
                UNIT_ASSERT_SUCCESS(state.FinishDeviceMigration(
                    db,
                    "disk-1",
                    m.GetSourceDeviceId(),
                    m.GetTargetDevice().GetDeviceUUID(),
                    Now(),
                    &affectedDisk));
            }
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto disksToNotify = state.GetDisksToNotify();
            UNIT_ASSERT_VALUES_EQUAL(1, disksToNotify.size());
            for (auto& [diskId, seqNo]: disksToNotify) {
                state.DeleteDiskToNotify(db, diskId, seqNo);
            }
        });

        // clean dirty devices
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto dd = state.GetDirtyDevices();
            Sort(dd, TByDeviceUUID());

            UNIT_ASSERT_VALUES_EQUAL(agents[0].DevicesSize(), dd.size());
            for (size_t i = 0; i != dd.size(); ++i) {
                const auto& uuid = agents[0].GetDevices(i).GetDeviceUUID();

                UNIT_ASSERT_VALUES_EQUAL(uuid, dd[i].GetDeviceUUID());

                UNIT_ASSERT(state.MarkDeviceAsClean(db, uuid));
            }
        });

        // should not allocate disk on agent in warning state
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            auto error = AllocateDisk(db, state, "disk-2", {}, 40_GB, devices);
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, error.GetCode());
        });
    }

    Y_UNIT_TEST(ShouldRestoreMigrationList)
    {
        const TVector agents = {
            AgentConfig(1, {
                Device("dev-1", "uuid-1.1", NProto::DEVICE_STATE_WARNING),
                Device("dev-2", "uuid-1.2"),
            }),
            AgentConfig(2, NProto::AGENT_STATE_WARNING, {
                Device("dev-1", "uuid-2.1"),
                Device("dev-2", "uuid-2.2"),
            }),
            AgentConfig(3, {
                Device("dev-1", "uuid-3.1"),
                Device("dev-2", "uuid-3.2", NProto::DEVICE_STATE_WARNING),
            })
        };

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents(agents)
            .WithDisks({
                Disk("disk-1", {"uuid-1.1", "uuid-3.1"}, NProto::DISK_STATE_MIGRATION),
                Disk("disk-2", {"uuid-1.2", "uuid-2.2"}, NProto::DISK_STATE_MIGRATION),
                Disk("disk-3", {"uuid-2.1", "uuid-3.2"}, NProto::DISK_STATE_MIGRATION),
            })
            .Build();

        auto migrations = state.BuildMigrationList();
        UNIT_ASSERT_VALUES_EQUAL(4, migrations.size());

        UNIT_ASSERT_VALUES_EQUAL("disk-1", migrations[0].DiskId);
        UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", migrations[0].SourceDeviceId);

        UNIT_ASSERT_VALUES_EQUAL("disk-2", migrations[1].DiskId);
        UNIT_ASSERT_VALUES_EQUAL("uuid-2.2", migrations[1].SourceDeviceId);

        UNIT_ASSERT_VALUES_EQUAL("disk-3", migrations[2].DiskId);
        UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", migrations[2].SourceDeviceId);

        UNIT_ASSERT_VALUES_EQUAL("disk-3", migrations[3].DiskId);
        UNIT_ASSERT_VALUES_EQUAL("uuid-3.2", migrations[3].SourceDeviceId);
    }

    Y_UNIT_TEST(ShouldRestartMigrationInCaseOfBrokenTargets)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const TVector agents {
            AgentConfig(1000, {
                Device("dev-1", "uuid-1.1", "rack-1"),
                Device("dev-2", "uuid-1.2", "rack-1"),
            }),
            AgentConfig(2000, {
                Device("dev-1", "uuid-2.1", "rack-2"),
                Device("dev-2", "uuid-2.2", "rack-2"),
            }),
            AgentConfig(3000, {
                Device("dev-1", "uuid-3.1", "rack-3"),
                Device("dev-2", "uuid-3.2", "rack-3"),
            })
        };

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithConfig(agents)
            .WithAgents({ agents[0], agents[1] })
            .WithDisks({ Disk("disk-1", {"uuid-1.1", "uuid-1.2"}) })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            UNIT_ASSERT_SUCCESS(state.UpdateAgentState(
                db,
                agents[0].GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                Now(),
                "state message",
                affectedDisks
            ));

            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());
            UNIT_ASSERT_VALUES_EQUAL("disk-1", affectedDisks[0].State.GetDiskId());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto migrations = state.BuildMigrationList();
            UNIT_ASSERT_VALUES_EQUAL(2, migrations.size());

            for (const auto& [diskId, deviceId]: migrations) {
                UNIT_ASSERT_SUCCESS(
                    state.StartDeviceMigration(db, diskId, deviceId).GetError()
                );
            }
        });

        UNIT_ASSERT(state.IsMigrationListEmpty());

        auto getTargets = [&] {
            TVector<TString> targets;
            executor.WriteTx([&] (TDiskRegistryDatabase db) {
                TDiskRegistryState::TAllocateDiskResult result {};

                auto error = state.AllocateDisk(
                    Now(),
                    db,
                    TDiskRegistryState::TAllocateDiskParams {
                        .DiskId = "disk-1",
                        .BlockSize = DefaultLogicalBlockSize,
                        .BlocksCount = 20_GB / DefaultLogicalBlockSize
                    },
                    &result);

                UNIT_ASSERT_VALUES_EQUAL(S_ALREADY, error.GetCode());
                UNIT_ASSERT_VALUES_EQUAL(2, result.Devices.size());
                targets.reserve(result.Migrations.size());
                for (const auto& m: result.Migrations) {
                    targets.push_back(m.GetTargetDevice().GetDeviceUUID());
                }
                Sort(targets);
            });

            return targets;
        };

        const auto targets = getTargets();

        UNIT_ASSERT_VALUES_EQUAL(2, targets.size());
        UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", targets[0]);
        UNIT_ASSERT_VALUES_EQUAL("uuid-2.2", targets[1]);

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDiskStateUpdate> affectedDisks;
            THashMap<TString, ui64> disksToNotify;

            UNIT_ASSERT_SUCCESS(state.RegisterAgent(
                db,
                agents[2],
                Now(),
                &affectedDisks,
                &disksToNotify
            ));

            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
            UNIT_ASSERT_VALUES_EQUAL(2, state.GetDirtyDevices().size());
            CleanDevices(state, db);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDiskStateUpdate> affectedDisks;
            THashMap<TString, ui64> disksToNotify;

            UNIT_ASSERT_SUCCESS(state.RegisterAgent(
                db,
                AgentConfig(agents[1].GetNodeId(), {}),
                Now(),
                &affectedDisks,
                &disksToNotify
            ));

            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto migrations = state.BuildMigrationList();
            UNIT_ASSERT_VALUES_EQUAL(2, migrations.size());

            for (const auto& [diskId, deviceId]: migrations) {
                UNIT_ASSERT_SUCCESS(
                    state.StartDeviceMigration(db, diskId, deviceId).GetError()
                );
            }
        });

        const auto newTargets = getTargets();

        UNIT_ASSERT_VALUES_EQUAL(2, newTargets.size());
        UNIT_ASSERT_VALUES_EQUAL("uuid-3.1", newTargets[0]);
        UNIT_ASSERT_VALUES_EQUAL("uuid-3.2", newTargets[1]);

    }

    Y_UNIT_TEST(ShouldAdjustDevices)
    {
        const ui64 referenceDeviceSize = 10_GB;

        const auto agent1a = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1", 512,  referenceDeviceSize + 1_GB),
            Device("dev-2", "uuid-2", "rack-1", 8_KB, referenceDeviceSize),
            Device("dev-3", "uuid-3", "rack-1", 4_KB, referenceDeviceSize - 1_GB),
            Device("dev-4", "uuid-4", "rack-1", 4_KB, referenceDeviceSize - 2_GB),
        });

        const auto agent1b = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1", 512,  referenceDeviceSize + 1_GB),
            Device("dev-2", "uuid-2", "rack-1", 8_KB, referenceDeviceSize),
            Device("dev-3", "uuid-3", "rack-1", 4_KB, referenceDeviceSize - 1_GB),
            Device("dev-4", "uuid-4", "rack-1", 4_KB, referenceDeviceSize - 2_GB),

            Device("dev-5", "uuid-5", "rack-1", 64_KB, referenceDeviceSize + 2_GB),
            Device("dev-6", "uuid-6", "rack-1", 4_KB, referenceDeviceSize - 3_GB),
        });

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        auto config = CreateDefaultStorageConfigProto();
        config.SetAllocationUnitNonReplicatedSSD(10);

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithStorageConfig(config)
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UpdateConfig(state, db, {agent1b});
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agent1a));
        });

        {
            auto device = state.GetDevice("uuid-1");
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ONLINE, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(
                referenceDeviceSize / device.GetBlockSize(),
                device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(
                (referenceDeviceSize + 1_GB) / device.GetBlockSize(),
                device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(512, device.GetBlockSize());
        }

        {
            auto device = state.GetDevice("uuid-2");
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ONLINE, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(
                referenceDeviceSize / device.GetBlockSize(),
                device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(
                device.GetBlocksCount(),
                device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(8_KB, device.GetBlockSize());
        }

        {
            auto device = state.GetDevice("uuid-3");
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ERROR, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(
                device.GetBlocksCount(),
                device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(
                (referenceDeviceSize - 1_GB) / device.GetBlockSize(),
                device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(4_KB, device.GetBlockSize());
        }

        {
            auto device = state.GetDevice("uuid-4");
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ERROR, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(
                device.GetBlocksCount(),
                device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(
                (referenceDeviceSize - 2_GB) / device.GetBlockSize(),
                device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(4_KB, device.GetBlockSize());
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agent1b));
        });

        {
            auto device = state.GetDevice("uuid-1");
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ONLINE, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(
                referenceDeviceSize / device.GetBlockSize(),
                device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(
                (referenceDeviceSize + 1_GB) / device.GetBlockSize(),
                device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(512, device.GetBlockSize());
        }

        {
            auto device = state.GetDevice("uuid-2");
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ONLINE, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(
                referenceDeviceSize / device.GetBlockSize(),
                device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(
                device.GetBlocksCount(),
                device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(8_KB, device.GetBlockSize());
        }

        {
            auto device = state.GetDevice("uuid-3");
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ERROR, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(
                device.GetBlocksCount(),
                device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(
                (referenceDeviceSize - 1_GB) / device.GetBlockSize(),
                device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(4_KB, device.GetBlockSize());
        }

        {
            auto device = state.GetDevice("uuid-4");
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ERROR, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(
                device.GetBlocksCount(),
                device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(
                (referenceDeviceSize - 2_GB) / device.GetBlockSize(),
                device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(4_KB, device.GetBlockSize());
        }

        {
            auto device = state.GetDevice("uuid-5");
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ONLINE, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(
                referenceDeviceSize / device.GetBlockSize(),
                device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(
                (referenceDeviceSize + 2_GB) / device.GetBlockSize(),
                device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(64_KB, device.GetBlockSize());
        }

        {
            auto device = state.GetDevice("uuid-6");
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ERROR, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(
                device.GetBlocksCount(),
                device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(
                (referenceDeviceSize - 3_GB) / device.GetBlockSize(),
                device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(4_KB, device.GetBlockSize());
        }
    }

    Y_UNIT_TEST(ShouldAdjustDeviceAtMigration)
    {
        const ui64 referenceDeviceSize = 93_GB;
        const ui64 blockCount = 24413696; // ~ 93.13 GB
        const ui64 blockSize = 4_KB;

        const TVector agents {
            AgentConfig(1000, {
                Device("dev-1", "uuid-1.1", "rack-1", blockSize, blockCount * blockSize),
                Device("dev-2", "uuid-1.2", "rack-1", blockSize, blockCount * blockSize),
            }),
            AgentConfig(2000, {
                Device("dev-1", "uuid-2.1", "rack-2", blockSize, 94_GB),
                Device("dev-2", "uuid-2.2", "rack-2", blockSize, 94_GB),
            }),
        };

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        auto config = CreateDefaultStorageConfigProto();
        config.SetAllocationUnitNonReplicatedSSD(referenceDeviceSize / 1_GB);

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithStorageConfig(config)
            .WithDisks({Disk("disk-1", {"uuid-1.1", "uuid-1.2"})})
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UpdateConfig(state, db, agents);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agents[0]));
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agents[1]));
            UNIT_ASSERT_VALUES_EQUAL(2, state.GetDirtyDevices().size());
            CleanDevices(state, db);
        });

        {
            auto device = state.GetDevice("uuid-1.1");
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ONLINE, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(blockCount, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(blockCount, device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(blockSize, device.GetBlockSize());
        }

        {
            auto device = state.GetDevice("uuid-1.2");
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ONLINE, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(blockCount, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(blockCount, device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(blockSize, device.GetBlockSize());
        }

        {
            auto device = state.GetDevice("uuid-2.1");
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ONLINE, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(
                referenceDeviceSize / device.GetBlockSize(),
                device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(94_GB/blockSize, device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(blockSize, device.GetBlockSize());
        }

        {
            auto device = state.GetDevice("uuid-2.2");
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ONLINE, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(
                referenceDeviceSize / device.GetBlockSize(),
                device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(94_GB/blockSize, device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(blockSize, device.GetBlockSize());
        }

        UNIT_ASSERT_VALUES_EQUAL(0, state.BuildMigrationList().size());

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;
            const auto error = state.UpdateDeviceState(
                db,
                "uuid-1.1",
                NProto::DEVICE_STATE_WARNING,
                Now(),
                "test",
                affectedDisk);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT(affectedDisk.Defined());
            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_MIGRATION, *affectedDisk);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            const auto ml = state.BuildMigrationList();
            UNIT_ASSERT_VALUES_EQUAL(1, ml.size());
            const auto& m = ml[0];
            UNIT_ASSERT_VALUES_EQUAL("disk-1", m.DiskId);
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", m.SourceDeviceId);
            UNIT_ASSERT_SUCCESS(
                state.StartDeviceMigration(db, m.DiskId, m.SourceDeviceId).GetError()
            );
            UNIT_ASSERT_VALUES_EQUAL(0, state.BuildMigrationList().size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TDiskRegistryState::TAllocateDiskResult result {};

            UNIT_ASSERT_SUCCESS(state.AllocateDisk(
                Now(),
                db,
                TDiskRegistryState::TAllocateDiskParams {
                    .DiskId = "disk-1",
                    .BlockSize = DefaultLogicalBlockSize,
                    .BlocksCount = blockCount*2
                },
                &result));

            UNIT_ASSERT_VALUES_EQUAL(2, result.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", result.Devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.2", result.Devices[1].GetDeviceUUID());

            UNIT_ASSERT_EQUAL(NProto::VOLUME_IO_OK, result.IOMode);
            UNIT_ASSERT_UNEQUAL(TInstant::Zero(), result.IOModeTs);

            UNIT_ASSERT_VALUES_EQUAL(1, result.Migrations.size());
            const auto& m = result.Migrations[0];
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", m.GetSourceDeviceId());

            THashSet<TString> ids { "uuid-2.1", "uuid-2.2" };

            const auto targetId = m.GetTargetDevice().GetDeviceUUID();

            // adjusted to source device
            {
                auto device = state.GetDevice(targetId);
                UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ONLINE, device.GetState(), device);
                UNIT_ASSERT_VALUES_EQUAL(blockCount, device.GetBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(94_GB/blockSize, device.GetUnadjustedBlockCount());
                UNIT_ASSERT_VALUES_EQUAL(blockSize, device.GetBlockSize());
            }

            ids.erase(targetId);

            // adjusted to reference size
            {
                auto device = state.GetDevice(*ids.begin());
                UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ONLINE, device.GetState(), device);
                UNIT_ASSERT_VALUES_EQUAL(
                    referenceDeviceSize / device.GetBlockSize(),
                    device.GetBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(94_GB/blockSize, device.GetUnadjustedBlockCount());
                UNIT_ASSERT_VALUES_EQUAL(blockSize, device.GetBlockSize());
            }
        });
    }

    Y_UNIT_TEST(ShouldCheckDeviceOverridesUponMigrationStart)
    {
        const ui64 referenceDeviceSize = 93_GB;
        const ui64 blockCount = 24413696; // ~ 93.13 GB
        const ui64 blockSize = 4_KB;

        const TVector agents {
            AgentConfig(1000, {
                Device("dev-1", "uuid-1.1", "rack-1", blockSize, blockCount * blockSize),
                Device("dev-2", "uuid-1.2", "rack-1", blockSize, blockCount * blockSize),
            }),
            AgentConfig(2000, {
                Device("dev-1", "uuid-2.1", "rack-2", blockSize, 94_GB),
                Device("dev-2", "uuid-2.2", "rack-2", blockSize, 94_GB),
            }),
        };

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        auto config = CreateDefaultStorageConfigProto();
        config.SetAllocationUnitNonReplicatedSSD(referenceDeviceSize / 1_GB);

        TVector<NProto::TDeviceOverride> deviceOverrides;
        deviceOverrides.emplace_back();
        deviceOverrides.back().SetDiskId("disk-1");
        deviceOverrides.back().SetDevice("uuid-1.1");
        deviceOverrides.back().SetBlocksCount(referenceDeviceSize / blockSize);

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithStorageConfig(config)
            .WithDisks({Disk("disk-1", {"uuid-1.1", "uuid-1.2"})})
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UpdateConfig(state, db, agents, deviceOverrides);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agents[0]));
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agents[1]));
            UNIT_ASSERT_VALUES_EQUAL(2, state.GetDirtyDevices().size());
            CleanDevices(state, db);
        });

        {
            auto device = state.GetDevice("uuid-1.1");
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ONLINE, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(blockCount, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(blockCount, device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(blockSize, device.GetBlockSize());
        }

        {
            auto device = state.GetDevice("uuid-1.2");
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ONLINE, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(blockCount, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(blockCount, device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(blockSize, device.GetBlockSize());
        }

        {
            auto device = state.GetDevice("uuid-2.1");
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ONLINE, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(
                referenceDeviceSize / device.GetBlockSize(),
                device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(94_GB / blockSize, device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(blockSize, device.GetBlockSize());
        }

        {
            auto device = state.GetDevice("uuid-2.2");
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ONLINE, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(
                referenceDeviceSize / device.GetBlockSize(),
                device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(94_GB / blockSize, device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(blockSize, device.GetBlockSize());
        }

        UNIT_ASSERT_VALUES_EQUAL(0, state.BuildMigrationList().size());

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            const auto error = state.UpdateAgentState(
                db,
                "agent-1000",
                NProto::AGENT_STATE_WARNING,
                Now(),
                "state message",
                affectedDisks);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());
            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_MIGRATION, affectedDisks[0]);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            const auto ml = state.BuildMigrationList();
            UNIT_ASSERT_VALUES_EQUAL(2, ml.size());
            const auto& m0 = ml[0];
            UNIT_ASSERT_VALUES_EQUAL("disk-1", m0.DiskId);
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", m0.SourceDeviceId);
            UNIT_ASSERT_SUCCESS(
                state.StartDeviceMigration(db, m0.DiskId, m0.SourceDeviceId).GetError()
            );
            const auto& m1 = ml[1];
            UNIT_ASSERT_VALUES_EQUAL("disk-1", m1.DiskId);
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.2", m1.SourceDeviceId);
            UNIT_ASSERT_SUCCESS(
                state.StartDeviceMigration(db, m1.DiskId, m1.SourceDeviceId).GetError()
            );
            UNIT_ASSERT_VALUES_EQUAL(0, state.BuildMigrationList().size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TDiskRegistryState::TAllocateDiskResult result {};

            UNIT_ASSERT_SUCCESS(state.AllocateDisk(
                Now(),
                db,
                TDiskRegistryState::TAllocateDiskParams {
                    .DiskId = "disk-1",
                    .BlockSize = DefaultLogicalBlockSize,
                    .BlocksCount = referenceDeviceSize * 2 / DefaultLogicalBlockSize
                },
                &result));

            UNIT_ASSERT_VALUES_EQUAL(2, result.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", result.Devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.2", result.Devices[1].GetDeviceUUID());

            UNIT_ASSERT_EQUAL(NProto::VOLUME_IO_OK, result.IOMode);
            UNIT_ASSERT_UNEQUAL(TInstant::Zero(), result.IOModeTs);

            SortBy(result.Migrations, [] (const auto& x) {
                return x.GetSourceDeviceId();
            });

            UNIT_ASSERT_VALUES_EQUAL(2, result.Migrations.size());
            const auto& m0 = result.Migrations[0];
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", m0.GetSourceDeviceId());
            const auto& m1 = result.Migrations[1];
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.2", m1.GetSourceDeviceId());

            // adjusted to source device
            {
                auto device = state.GetDevice(m0.GetTargetDevice().GetDeviceUUID());
                UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ONLINE, device.GetState(), device);
                UNIT_ASSERT_VALUES_EQUAL(
                    referenceDeviceSize / blockSize,
                    device.GetBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(
                    referenceDeviceSize / DefaultLogicalBlockSize,
                    m0.GetTargetDevice().GetBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(94_GB / blockSize, device.GetUnadjustedBlockCount());
                UNIT_ASSERT_VALUES_EQUAL(blockSize, device.GetBlockSize());
            }

            // adjusted to source device
            {
                auto device = state.GetDevice(m1.GetTargetDevice().GetDeviceUUID());
                UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ONLINE, device.GetState(), device);
                UNIT_ASSERT_VALUES_EQUAL(blockCount, device.GetBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(blockCount, m1.GetTargetDevice().GetBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(94_GB / blockSize, device.GetUnadjustedBlockCount());
                UNIT_ASSERT_VALUES_EQUAL(blockSize, device.GetBlockSize());
            }
        });
    }

    Y_UNIT_TEST(ShouldProcessPlacementGroups)
    {
        const TVector<TString> expected {"group1", "group2", "group3"};

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithPlacementGroups(expected)
            .Build();

        TVector<TPlacementGroupInfo> groups;

        for (const auto& [diskId, config]: state.GetPlacementGroups()) {
            groups.push_back(config);
        }

        SortBy(groups, [] (const auto& x) {
            return x.Config.GetGroupId();
        });

        UNIT_ASSERT_VALUES_EQUAL(expected.size(), groups.size());
        for (size_t i = 0; i != expected.size(); ++i) {
            UNIT_ASSERT_VALUES_EQUAL(expected[i], groups[i].Config.GetGroupId());
        }
    }

    Y_UNIT_TEST(ShouldRemoveDestroyedDiskFromNotifyList)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const TVector agents {
            AgentConfig(1, {
                Device("dev-1", "uuid-1.1", "rack-1"),
                Device("dev-2", "uuid-1.2", "rack-1")
            }),
            AgentConfig(2, {
                Device("dev-1", "uuid-2.1", "rack-2"),
                Device("dev-2", "uuid-2.2", "rack-2")
            }),
        };

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents(agents)
            .WithDisksToNotify({ "disk-1" })
            .WithDisks({
                Disk("disk-1", {"uuid-1.1", "uuid-2.1"}),
                Disk("disk-2", {"uuid-1.2", "uuid-2.2"})
            })
            .Build();

        UNIT_ASSERT_VALUES_EQUAL(1, state.GetDisksToNotify().size());
        UNIT_ASSERT_GT(state.GetDisksToNotify().at("disk-1"), 0);
        UNIT_ASSERT_VALUES_EQUAL(0, state.GetDiskStateUpdates().size());

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDiskStateUpdate> affectedDisk;
            UNIT_ASSERT_SUCCESS(state.UpdateAgentState(
                db,
                agents[0].GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                Now(),
                "state message",
                affectedDisk));

            UNIT_ASSERT_VALUES_EQUAL(2, affectedDisk.size());
            UNIT_ASSERT_VALUES_EQUAL(2, state.GetDiskStateUpdates().size());
        });

        UNIT_ASSERT_VALUES_EQUAL(2, state.GetDisksToNotify().size());
        UNIT_ASSERT(state.GetDisksToNotify().contains("disk-1"));
        UNIT_ASSERT(state.GetDisksToNotify().contains("disk-2"));

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(state.DeallocateDisk(db, "disk-1"));
        });

        UNIT_ASSERT_VALUES_EQUAL(1, state.GetDisksToNotify().size());
        UNIT_ASSERT(state.GetDisksToNotify().contains("disk-2"));

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(state.DeallocateDisk(db, "disk-2"));
        });

        UNIT_ASSERT_VALUES_EQUAL(0, state.GetDisksToNotify().size());
    }

    Y_UNIT_TEST(ShouldMigrateToDeviceWithBiggerSize)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const TVector agents {
            AgentConfig(1, {
                Device("dev-1", "uuid-1.1", "rack-1", DefaultBlockSize, 11_GB),
            }),
            AgentConfig(2, {
                Device("dev-1", "uuid-2.1", "rack-2", DefaultBlockSize, 12_GB),
            }),
        };

        auto config = CreateDefaultStorageConfigProto();
        config.SetAllocationUnitNonReplicatedSSD(10);

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithStorageConfig(config)
            .WithKnownAgents(agents)
            .WithDisksToNotify({ "disk-1" })
            .WithDisks({ Disk("disk-1", { "uuid-1.1" }) })
            .Build();

        {
            TVector<NProto::TDeviceConfig> devices;
            UNIT_ASSERT_SUCCESS(state.GetDiskDevices("disk-1", devices));
            UNIT_ASSERT_VALUES_EQUAL(1, devices.size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", devices[0].GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(11_GB/DefaultBlockSize, devices[0].GetBlocksCount());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, devices[0].GetState());
        }

        {
            auto device = state.GetDevice("uuid-1.1");
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", device.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(11_GB/DefaultBlockSize, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(11_GB/DefaultBlockSize, device.GetUnadjustedBlockCount());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, device.GetState());
        }

        {
            auto device = state.GetDevice("uuid-2.1");
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", device.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(12_GB/DefaultBlockSize, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(12_GB/DefaultBlockSize, device.GetUnadjustedBlockCount());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, device.GetState());
        }

        // migrate from uuid-1.1 to uuid-2.1

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDiskStateUpdate> affectedDisks;
            UNIT_ASSERT_SUCCESS(state.UpdateAgentState(
                db,
                agents[0].GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                Now(),
                "state message",
                affectedDisks));

            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());
            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_MIGRATION, affectedDisks[0]);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            const auto m = state.BuildMigrationList();
            UNIT_ASSERT_VALUES_EQUAL(1, m.size());
            auto& [diskId, sourceId] = m[0];

            UNIT_ASSERT_VALUES_EQUAL("disk-1", diskId);
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", sourceId);

            auto target = state.StartDeviceMigration(db, diskId, sourceId);
            UNIT_ASSERT_SUCCESS(target.GetError());
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", target.GetResult().GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(0, state.BuildMigrationList().size());
        });

        {
            auto device = state.GetDevice("uuid-1.1");
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", device.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(11_GB/DefaultBlockSize, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(11_GB/DefaultBlockSize, device.GetUnadjustedBlockCount());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, device.GetState());
        }

        {
            auto device = state.GetDevice("uuid-2.1");
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", device.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(11_GB/DefaultBlockSize, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(12_GB/DefaultBlockSize, device.GetUnadjustedBlockCount());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, device.GetState());
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agents[1]));
        });

        {
            auto device = state.GetDevice("uuid-2.1");
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", device.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(11_GB/DefaultBlockSize, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(12_GB/DefaultBlockSize, device.GetUnadjustedBlockCount());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, device.GetState());
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;

            UNIT_ASSERT_SUCCESS(state.FinishDeviceMigration(
                db,
                "disk-1",
                "uuid-1.1",
                "uuid-2.1",
                Now(),
                &affectedDisk));

            UNIT_ASSERT(affectedDisk.Defined());
            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_ONLINE, *affectedDisk);
            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisk->SeqNo);
            UNIT_ASSERT_VALUES_EQUAL(0, state.GetDirtyDevices().size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto disksToNotify = state.GetDisksToNotify();
            UNIT_ASSERT_VALUES_EQUAL(1, disksToNotify.size());
            for (auto& [diskId, seqNo]: disksToNotify) {
                state.DeleteDiskToNotify(db, diskId, seqNo);
            }

            UNIT_ASSERT_VALUES_EQUAL(1, state.GetDirtyDevices().size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", state.GetDirtyDevices()[0].GetDeviceUUID());

            CleanDevices(state, db);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agents[1]));
        });

        {
            auto device = state.GetDevice("uuid-2.1");
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", device.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(11_GB/DefaultBlockSize, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(12_GB/DefaultBlockSize, device.GetUnadjustedBlockCount());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, device.GetState());
        }

        // migrate from uuid-2.1 to uuid-1.1

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDiskStateUpdate> affectedDisks;
            UNIT_ASSERT_SUCCESS(state.UpdateAgentState(
                db,
                agents[0].GetAgentId(),
                NProto::AGENT_STATE_ONLINE,
                Now(),
                "state message",
                affectedDisks));

            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDiskStateUpdate> affectedDisks;
            UNIT_ASSERT_SUCCESS(state.UpdateAgentState(
                db,
                agents[1].GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                Now(),
                "state message",
                affectedDisks));

            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());
            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_MIGRATION, affectedDisks[0]);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            const auto m = state.BuildMigrationList();
            UNIT_ASSERT_VALUES_EQUAL(1, m.size());
            auto& [diskId, sourceId] = m[0];

            UNIT_ASSERT_VALUES_EQUAL("disk-1", diskId);
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", sourceId);

            auto target = state.StartDeviceMigration(db, diskId, sourceId);
            UNIT_ASSERT_SUCCESS(target.GetError());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", target.GetResult().GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(0, state.BuildMigrationList().size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TMaybe<TDiskStateUpdate> affectedDisk;

            UNIT_ASSERT_SUCCESS(state.FinishDeviceMigration(
                db,
                "disk-1",
                "uuid-2.1",
                "uuid-1.1",
                Now(),
                &affectedDisk));

            UNIT_ASSERT(affectedDisk.Defined());
            UNIT_ASSERT_DISK_STATE("disk-1", DISK_STATE_ONLINE, *affectedDisk);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto disksToNotify = state.GetDisksToNotify();
            UNIT_ASSERT_VALUES_EQUAL(1, disksToNotify.size());
            for (auto& [diskId, seqNo]: disksToNotify) {
                state.DeleteDiskToNotify(db, diskId, seqNo);
            }

            UNIT_ASSERT_VALUES_EQUAL(1, state.GetDirtyDevices().size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", state.GetDirtyDevices()[0].GetDeviceUUID());

            CleanDevices(state, db);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agents[1]));
        });

        {
            auto device = state.GetDevice("uuid-2.1");
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", device.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(10_GB/DefaultBlockSize, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(12_GB/DefaultBlockSize, device.GetUnadjustedBlockCount());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, device.GetState());
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(RegisterAgent(state, db, agents[1]));
        });

        {
            auto device = state.GetDevice("uuid-2.1");
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", device.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(10_GB/DefaultBlockSize, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(12_GB/DefaultBlockSize, device.GetUnadjustedBlockCount());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, device.GetState());
        }
    }

    Y_UNIT_TEST(ShouldNotRegisterAgentIfSeqNumberIsSmallerThanCurrent)
    {
        auto agentId = "host-1.cloud.yandex.net";

        const auto agent1a = AgentConfig(1, agentId, 0, {
            Device("dev-1", "uuid-1.1", "rack-1", DefaultBlockSize, 10_GB),
            Device("dev-2", "uuid-1.2", "rack-1", DefaultBlockSize, 10_GB, "#1"),
            Device("dev-3", "uuid-1.3", "rack-1", DefaultBlockSize, 10_GB)
        });

        const auto agent1b = AgentConfig(2, agentId, 1, {
            Device("dev-1", "uuid-1.1", "rack-1", DefaultBlockSize, 10_GB),
            Device("dev-2", "uuid-1.2", "rack-1", DefaultBlockSize, 10_GB, "#1"),
            Device("dev-3", "uuid-1.3", "rack-1", DefaultBlockSize, 10_GB)
        });

        const auto agent1c = AgentConfig(3, agentId, 0, {
            Device("dev-2", "uuid-1.2", "rack-1", DefaultBlockSize, 10_GB, "#2"),
            Device("dev-3", "uuid-1.3", "rack-1", DefaultBlockSize, 20_GB)
        });

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agent1a })
            .Build();

        auto CheckDevices = [&] () {
            const auto dev1 = state.GetDevice("uuid-1.1");
            UNIT_ASSERT_VALUES_EQUAL(10_GB / DefaultBlockSize, dev1.GetBlocksCount());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, dev1.GetState());

            const auto dev2 = state.GetDevice("uuid-1.2");
            UNIT_ASSERT_VALUES_EQUAL(10_GB / DefaultBlockSize, dev2.GetBlocksCount());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, dev2.GetState());
            UNIT_ASSERT_EQUAL("#1", dev2.GetTransportId());

            const auto dev3 = state.GetDevice("uuid-1.3");
            UNIT_ASSERT_VALUES_EQUAL(10_GB / DefaultBlockSize, dev3.GetBlocksCount());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, dev3.GetState());
        };

        CheckDevices();

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            THashMap<TString, ui64> disksToNotify;
            auto error = state.RegisterAgent(
                db,
                agent1b,
                Now(),
                &affectedDisks,
                &disksToNotify
            );

            UNIT_ASSERT_C(!HasError(error), error);

            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
            UNIT_ASSERT_VALUES_EQUAL(0, disksToNotify.size());
        });

        CheckDevices();

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            THashMap<TString, ui64> disksToNotify;
            auto error = state.RegisterAgent(
                db,
                agent1c,
                Now(),
                &affectedDisks,
                &disksToNotify
            );

            UNIT_ASSERT_VALUES_EQUAL_C(E_INVALID_STATE, error.GetCode(), error);

            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
            UNIT_ASSERT_VALUES_EQUAL(0, disksToNotify.size());
        });

        CheckDevices();
    }

    Y_UNIT_TEST(ShouldSetUserId)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const TVector agents {
            AgentConfig(1, { Device("dev-1", "uuid-1.1") })
        };

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents(agents)
            .WithDisksToNotify({ "disk-1" })
            .WithDisks({ Disk("disk-1", { "uuid-1.1" }) })
            .Build();

        {
            TDiskInfo info;

            UNIT_ASSERT_SUCCESS(state.GetDiskInfo("disk-1", info));
            UNIT_ASSERT_VALUES_EQUAL("", info.UserId);
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            UNIT_ASSERT_SUCCESS(state.SetUserId(db, "disk-1", "foo"));
            UNIT_ASSERT_VALUES_EQUAL(
                E_NOT_FOUND,
                state.SetUserId(db, "disk-x", "bar").GetCode());
        });

        {
            TDiskInfo info;

            UNIT_ASSERT_SUCCESS(state.GetDiskInfo("disk-1", info));
            UNIT_ASSERT_VALUES_EQUAL("foo", info.UserId);
        }
    }

    Y_UNIT_TEST(ShouldUpdateVolumeConfig)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        auto agentConfig1 = AgentConfig(2, {
            Device("dev-1", "uuid-1", "rack-1"),
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({ agentConfig1 })
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(state.CreatePlacementGroup(db, "group-1"));
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(state.CreatePlacementGroup(db, "group-2"));
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDeviceConfig> devices;
            UNIT_ASSERT_SUCCESS(AllocateDisk(db, state, "disk-1", "group-1", 10_GB, devices));

            auto [config, seqNo] = state.GetVolumeConfigUpdate("disk-1");
            UNIT_ASSERT_VALUES_EQUAL("", config.GetPlacementGroupId());
            UNIT_ASSERT_VALUES_EQUAL(0, seqNo);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> disksToAdd = {};
            UNIT_ASSERT_SUCCESS(
                state.AlterPlacementGroupMembership(
                    db,
                    "group-1",
                    2,
                    disksToAdd,
                    {"disk-1"}));

            auto [config, seqNo] = state.GetVolumeConfigUpdate("disk-1");
            UNIT_ASSERT_VALUES_EQUAL("", config.GetPlacementGroupId());
            UNIT_ASSERT_VALUES_EQUAL(0, seqNo);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> disksToAdd = {"disk-1"};
            UNIT_ASSERT_SUCCESS(
                state.AlterPlacementGroupMembership(
                    db,
                    "group-2",
                    1,
                    disksToAdd,
                    {}));

            auto [config, seqNo] = state.GetVolumeConfigUpdate("disk-1");
            UNIT_ASSERT_VALUES_EQUAL("group-2", config.GetPlacementGroupId());
            UNIT_ASSERT_VALUES_EQUAL(1, seqNo);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> affectedDisks;
            UNIT_ASSERT_SUCCESS(state.DestroyPlacementGroup(db, "group-2", affectedDisks));

            auto [config, seqNo] = state.GetVolumeConfigUpdate("disk-1");
            UNIT_ASSERT(config.HasPlacementGroupId());
            UNIT_ASSERT_VALUES_EQUAL("", config.GetPlacementGroupId());
            UNIT_ASSERT_VALUES_EQUAL(seqNo, 2);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TString> disksToAdd = {"disk-1"};
            UNIT_ASSERT_SUCCESS(
                state.AlterPlacementGroupMembership(
                    db,
                    "group-1",
                    3,
                    disksToAdd,
                    {}));

            auto [config, seqNo] = state.GetVolumeConfigUpdate("disk-1");
            UNIT_ASSERT_VALUES_EQUAL("group-1", config.GetPlacementGroupId());
            UNIT_ASSERT_VALUES_EQUAL(seqNo, 3);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            UNIT_ASSERT_SUCCESS(state.DeallocateDisk(db, "disk-1"));

            auto [config, seqNo] = state.GetVolumeConfigUpdate("disk-1");
            UNIT_ASSERT(!config.HasPlacementGroupId());
            UNIT_ASSERT_VALUES_EQUAL(0, seqNo);
        });
    }

    Y_UNIT_TEST(ShouldMuteIOErrors)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
            Device("dev-2", "uuid-2", "rack-1")
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({agent1})
            .WithDisks({
                Disk("disk-1", {"uuid-1", "uuid-2"}),
            })
            .Build();

        auto allocateDisk = [&] (auto& db, auto& diskId, auto totalSize) {
            TDiskRegistryState::TAllocateDiskResult result {};
            state.AllocateDisk(
                Now(),
                db,
                TDiskRegistryState::TAllocateDiskParams {
                    .DiskId = diskId,
                    .PlacementGroupId = {},
                    .BlockSize = DefaultLogicalBlockSize,
                    .BlocksCount = totalSize / DefaultLogicalBlockSize,
                },
                &result);
            return result;
        };

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            state.UpdateAgentState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_WARNING,
                Now(),
                "state message",
                affectedDisks);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto result = allocateDisk(db, "disk-1", 20_GB);
            UNIT_ASSERT_EQUAL(result.IOMode, NProto::VOLUME_IO_OK);
            UNIT_ASSERT_EQUAL(result.MuteIOErrors, false);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            state.UpdateAgentState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_UNAVAILABLE,
                Now(),
                "state message",
                affectedDisks);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto result = allocateDisk(db, "disk-1", 20_GB);
            UNIT_ASSERT_EQUAL(result.IOMode, NProto::VOLUME_IO_OK);
            UNIT_ASSERT_EQUAL(result.MuteIOErrors, true);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            TVector<TDiskStateUpdate> affectedDisks;
            state.UpdateAgentState(
                db,
                agent1.GetAgentId(),
                NProto::AGENT_STATE_ONLINE,
                Now(),
                "state message",
                affectedDisks);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto result = allocateDisk(db, "disk-1", 20_GB);
            UNIT_ASSERT_EQUAL(result.IOMode, NProto::VOLUME_IO_OK);
            UNIT_ASSERT_EQUAL(result.MuteIOErrors, false);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TMaybe<TDiskStateUpdate> affectedDisk;
            state.UpdateDeviceState(
                db,
                "uuid-1",
                NProto::DEVICE_STATE_ERROR,
                Now(),
                "reason",
                affectedDisk);
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto result = allocateDisk(db, "disk-1", 20_GB);
            UNIT_ASSERT_EQUAL(result.IOMode, NProto::VOLUME_IO_ERROR_READ_ONLY);
            UNIT_ASSERT_EQUAL(result.MuteIOErrors, true);
        });
    }

    Y_UNIT_TEST(ShouldAllowMigrationForDiskWithUnavailableState)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const auto agent1 = AgentConfig(1, {
            Device("dev-1", "uuid-1", "rack-1"),
        });

        const auto agent2 = AgentConfig(2, {
            Device("dev-1", "uuid-2", "rack-2"),
        });

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents({agent1, agent2})
            .WithDisks({
                Disk("disk-1", {"uuid-1", "uuid-2"}),
            })
            .Build();

        auto updateAgentState = [&] (auto db, const auto& agent, auto desiredState) {
            TVector<TDiskStateUpdate> affectedDisks;
            UNIT_ASSERT_SUCCESS(state.UpdateAgentState(
                db,
                agent.GetAgentId(),
                desiredState,
                Now(),
                "state message",
                affectedDisks));
            return affectedDisks;
        };

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            auto affectedDisks = updateAgentState(db, agent1, NProto::AGENT_STATE_UNAVAILABLE);
            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());
            UNIT_ASSERT(state.IsMigrationListEmpty());
        });

        {
            TDiskInfo diskInfo;
            UNIT_ASSERT_SUCCESS(state.GetDiskInfo("disk-1", diskInfo));
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_TEMPORARILY_UNAVAILABLE, diskInfo.State);
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            auto affectedDisks = updateAgentState(db, agent2, NProto::AGENT_STATE_WARNING);
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
            auto migrations = state.BuildMigrationList();
            UNIT_ASSERT_VALUES_EQUAL(1, migrations.size());
            UNIT_ASSERT_VALUES_EQUAL("disk-1", migrations[0].DiskId);
            UNIT_ASSERT_VALUES_EQUAL("uuid-2", migrations[0].SourceDeviceId);
        });

        {
            TDiskInfo diskInfo;
            UNIT_ASSERT_SUCCESS(state.GetDiskInfo("disk-1", diskInfo));
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_TEMPORARILY_UNAVAILABLE, diskInfo.State);
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            auto affectedDisks = updateAgentState(db, agent1, NProto::AGENT_STATE_WARNING);
            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());
            UNIT_ASSERT_VALUES_EQUAL(2, state.BuildMigrationList().size());
        });

        {
            TDiskInfo diskInfo;
            UNIT_ASSERT_SUCCESS(state.GetDiskInfo("disk-1", diskInfo));
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_MIGRATION, diskInfo.State);
        }
    }

    Y_UNIT_TEST(ShouldDeferReleaseMigrationDevices)
    {
        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            db.InitSchema();
        });

        const TVector agents {
            AgentConfig(1, {
                Device("dev-1", "uuid-1.1", "rack-1"),
                Device("dev-2", "uuid-1.2", "rack-1"),
            }),
            AgentConfig(2, {
                Device("dev-1", "uuid-2.1", "rack-2"),
                Device("dev-2", "uuid-2.2", "rack-2"),
            })
        };

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents(agents)
            .WithDisks({
                Disk("disk-1", {"uuid-1.1", "uuid-1.2"}),
            })
            .Build();

        UNIT_ASSERT_VALUES_EQUAL(0, state.BuildMigrationList().size());
        UNIT_ASSERT(state.IsMigrationListEmpty());

        auto allocateDisk = [&] (auto& db)
            -> TResultOrError<TDiskRegistryState::TAllocateDiskResult>
        {
            TDiskRegistryState::TAllocateDiskResult result {};

            auto error = state.AllocateDisk(
                Now(),
                db,
                TDiskRegistryState::TAllocateDiskParams {
                    .DiskId = "disk-1",
                    .BlockSize = DefaultLogicalBlockSize,
                    .BlocksCount = 20_GB / DefaultLogicalBlockSize
                },
                &result);
            if (HasError(error)) {
                return error;
            }

            return result;
        };

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto [result, error] = allocateDisk(db);
            UNIT_ASSERT_SUCCESS(error);

            UNIT_ASSERT_VALUES_EQUAL(2, result.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL(0, result.Migrations.size());
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            auto affectedDisks = UpdateAgentState(
                state,
                db,
                agents[0],
                NProto::AGENT_STATE_WARNING);
            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());
            UNIT_ASSERT(!state.IsMigrationListEmpty());
        });

        const auto migrations = state.BuildMigrationList();
        UNIT_ASSERT_VALUES_EQUAL(2, migrations.size());

        TVector<TString> targets;
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            for (const auto& [diskId, uuid]: migrations) {
                auto [config, error] = state.StartDeviceMigration(db, diskId, uuid);
                UNIT_ASSERT_SUCCESS(error);
                targets.push_back(config.GetDeviceUUID());
            }
        });
        Sort(targets);

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            auto [result, error] = allocateDisk(db);
            UNIT_ASSERT_SUCCESS(error);

            UNIT_ASSERT_VALUES_EQUAL(2, result.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL(2, result.Migrations.size());
        });

        {
            TDiskInfo diskInfo;
            UNIT_ASSERT_SUCCESS(state.GetDiskInfo("disk-1", diskInfo));
            UNIT_ASSERT_VALUES_EQUAL(2, diskInfo.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL(2, diskInfo.Migrations.size());
            UNIT_ASSERT_VALUES_EQUAL(0, diskInfo.FinishedMigrations.size());
        }

        UNIT_ASSERT_VALUES_EQUAL(0, state.GetDirtyDevices().size());

        // cancel migrations
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            auto affectedDisks = UpdateAgentState(
                state,
                db,
                agents[0],
                NProto::AGENT_STATE_ONLINE);
            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());
            UNIT_ASSERT(state.IsMigrationListEmpty());
        });

        UNIT_ASSERT_VALUES_EQUAL(0, state.GetDirtyDevices().size());

        {
            TDiskInfo diskInfo;
            UNIT_ASSERT_SUCCESS(state.GetDiskInfo("disk-1", diskInfo));
            UNIT_ASSERT_VALUES_EQUAL(2, diskInfo.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL(0, diskInfo.Migrations.size());
            UNIT_ASSERT_VALUES_EQUAL(2, diskInfo.FinishedMigrations.size());
            SortBy(diskInfo.FinishedMigrations, [] (auto& m) {
                return m.DeviceId;
            });

            for (size_t i = 0; i != targets.size(); ++i) {
                UNIT_ASSERT_VALUES_EQUAL(
                    targets[i],
                    diskInfo.FinishedMigrations[i].DeviceId);
            }
        }

        UNIT_ASSERT(!state.GetDisksToNotify().empty());
        UNIT_ASSERT_VALUES_EQUAL(0, state.GetDirtyDevices().size());

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            auto notifications = state.GetDisksToNotify();
            for (const auto& [diskId, seqNo]: notifications) {
                state.DeleteDiskToNotify(db, diskId, seqNo);
            }
        });

        UNIT_ASSERT_VALUES_EQUAL(2, state.GetDirtyDevices().size());

        {
            TDiskInfo diskInfo;
            UNIT_ASSERT_SUCCESS(state.GetDiskInfo("disk-1", diskInfo));
            UNIT_ASSERT_VALUES_EQUAL(2, diskInfo.Devices.size());
            UNIT_ASSERT_VALUES_EQUAL(0, diskInfo.Migrations.size());
            UNIT_ASSERT_VALUES_EQUAL(0, diskInfo.FinishedMigrations.size());
        }
    }

    Y_UNIT_TEST(ShouldAdjustDeviceBlockCountAfterSecureErase)
    {
        const ui64 referenceDeviceSize = 93_GB;
        const ui64 physicalDeviceBlockCount = 24641024;

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            db.InitSchema();
        });

        const TVector agents {
            AgentConfig(1, {
                Device("dev-1", "uuid-1.1", "rack-1", 4_KB, physicalDeviceBlockCount * 4_KB),
                Device("dev-2", "uuid-1.2", "rack-1", 4_KB, physicalDeviceBlockCount * 4_KB),
            })
        };

        TDiskRegistryState state = TDiskRegistryStateBuilder()
            .WithKnownAgents(agents)
            .WithStorageConfig([] {
                auto config = CreateDefaultStorageConfigProto();
                config.SetAllocationUnitNonReplicatedSSD(referenceDeviceSize / 1_GB);
                return config;
            }())
            .WithDisks({
                Disk("disk-1", {"uuid-1.1", "uuid-1.2"}),
            })
            .Build();

        for (TString uuid: { "uuid-1.1", "uuid-1.2" }) {
            auto device = state.GetDevice(uuid);
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ONLINE, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(physicalDeviceBlockCount, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(physicalDeviceBlockCount, device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(4_KB, device.GetBlockSize());
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            UNIT_ASSERT_SUCCESS(state.DeallocateDisk(db, "disk-1"));
            CleanDevices(state, db);
        });

        for (TString uuid: { "uuid-1.1", "uuid-1.2" }) {
            auto device = state.GetDevice(uuid);
            UNIT_ASSERT_EQUAL_C(NProto::DEVICE_STATE_ONLINE, device.GetState(), device);
            UNIT_ASSERT_VALUES_EQUAL(referenceDeviceSize / 4_KB, device.GetBlocksCount());
            UNIT_ASSERT_VALUES_EQUAL(physicalDeviceBlockCount, device.GetUnadjustedBlockCount());
            UNIT_ASSERT_VALUES_EQUAL(4_KB, device.GetBlockSize());
        }
    }

    Y_UNIT_TEST(ShouldHandleLostDevices)
    {
        const TVector agents {
            AgentConfig(1, {
                Device("dev-1", "uuid-1.1") | WithTotalSize(94_GB, 4_KB),
                Device("dev-2", "uuid-1.2") | WithTotalSize(94_GB, 4_KB),
            }),
            AgentConfig(2, {
                Device("dev-1", "uuid-2.1") | WithTotalSize(94_GB, 4_KB),
                Device("dev-2", "uuid-2.2") | WithTotalSize(94_GB, 4_KB),
            }),
            AgentConfig(3, {
                Device("dev-1", "uuid-3.1") | WithTotalSize(94_GB, 4_KB),
                Device("dev-2", "uuid-3.2") | WithTotalSize(94_GB, 4_KB),
            })
        };

        TTestExecutor executor;
        executor.WriteTx([&] (TDiskRegistryDatabase db) mutable {
            db.InitSchema();
            db.WriteDiskAllocationAllowed(true);
            db.WriteDiskRegistryConfig(MakeConfig(agents));
        });

        std::optional<TDiskRegistryState> state = TDiskRegistryStateBuilder()
            .WithConfig(agents)
            .WithStorageConfig([] {
                auto config = CreateDefaultStorageConfigProto();
                config.SetAllocationUnitNonReplicatedSSD(93);
                return config;
            }())
            .Build();

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            for (size_t i = 0; i != agents.size() - 1; ++i) {
                TVector<TDiskStateUpdate> affectedDisks;
                THashMap<TString, ui64> notifiedDisks;

                auto error = state->RegisterAgent(
                    db,
                    agents[i],
                    TInstant::Now(),
                    &affectedDisks,
                    &notifiedDisks);
                UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
                UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());
                UNIT_ASSERT_VALUES_EQUAL(0, notifiedDisks.size());

                UNIT_ASSERT_VALUES_EQUAL(
                    agents[i].DevicesSize(),
                    state->GetDirtyDevices().size());

                for (auto& d: state->GetDirtyDevices()) {
                    state->MarkDeviceAsClean(db, d.GetDeviceUUID());
                }
            }
        });

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TDiskRegistryState::TAllocateDiskResult result {};
            auto error = state->AllocateDisk(
                TInstant::Now(),
                db,
                TDiskRegistryState::TAllocateDiskParams {
                    .DiskId = "foo",
                    .PlacementGroupId = {},
                    .BlockSize = 4_KB,
                    .BlocksCount = 93_GB / 4_KB * 4,
                },
                &result);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(4, result.Devices.size());
            for (auto& d: result.Devices) {
                UNIT_ASSERT_VALUES_UNEQUAL(0, d.GetNodeId());
                UNIT_ASSERT_VALUES_EQUAL(4_KB, d.GetBlockSize());
                UNIT_ASSERT_VALUES_EQUAL(93_GB / 4_KB, d.GetBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(94_GB / 4_KB, d.GetUnadjustedBlockCount());
            }
        });

        // reject agent

        NProto::TAgentConfig agentToAbuse;

        {
            TVector<NProto::TDeviceConfig> devices;
            auto error = state->GetDiskDevices("foo", devices);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(4, devices.size());
            ui32 nodeIdToAbuse = 0;
            for (auto& d: devices) {
                if (d.GetNodeId() != devices[0].GetNodeId()) {
                    nodeIdToAbuse = d.GetNodeId();
                    break;
                }
            }
            UNIT_ASSERT_VALUES_UNEQUAL(0, nodeIdToAbuse);
            auto* config = FindIfPtr(agents, [=] (auto& agent) {
                return agent.GetNodeId() == nodeIdToAbuse;
            });

            UNIT_ASSERT(config != nullptr);
            agentToAbuse = *config;
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDiskStateUpdate> affectedDisks;
            auto error = state->UpdateAgentState(
                db,
                agentToAbuse.GetAgentId(),
                NProto::AGENT_STATE_UNAVAILABLE,
                TInstant::Now(),
                "test",
                affectedDisks);

            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(1, affectedDisks.size());
            UNIT_ASSERT_VALUES_EQUAL("foo", affectedDisks[0].State.GetDiskId());
            UNIT_ASSERT_EQUAL(
                NProto::DISK_STATE_TEMPORARILY_UNAVAILABLE,
                affectedDisks[0].State.GetState());
        });

        {
            TVector<NProto::TDeviceConfig> devices;
            auto error = state->GetDiskDevices("foo", devices);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(4, devices.size());

            for (auto& d: devices) {
                UNIT_ASSERT_VALUES_UNEQUAL(0, d.GetNodeId());
                UNIT_ASSERT_VALUES_EQUAL(4_KB, d.GetBlockSize());
                UNIT_ASSERT_VALUES_EQUAL(93_GB / 4_KB, d.GetBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(94_GB / 4_KB, d.GetUnadjustedBlockCount());
            }
        }

        // replace agent with new one

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TVector<TDiskStateUpdate> affectedDisks;
            THashMap<TString, ui64> notifiedDisks;

            auto agent = agents.back();
            agent.SetNodeId(agentToAbuse.GetNodeId());

            auto error = state->RegisterAgent(
                db,
                agent,
                TInstant::Now(),
                &affectedDisks,
                &notifiedDisks);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(0, affectedDisks.size());

            UNIT_ASSERT_VALUES_EQUAL(1, notifiedDisks.size());
            UNIT_ASSERT(notifiedDisks.contains("foo"));

            UNIT_ASSERT_VALUES_EQUAL(
                agent.DevicesSize(),
                state->GetDirtyDevices().size());

            for (auto& d: state->GetDirtyDevices()) {
                state->MarkDeviceAsClean(db, d.GetDeviceUUID());
            }
        });

        {
            TVector<NProto::TDeviceConfig> devices;
            auto error = state->GetDiskDevices("foo", devices);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(4, devices.size());

            for (auto& d: devices) {
                if (d.GetNodeId() == 0) {
                    UNIT_ASSERT_VALUES_EQUAL(
                        agentToAbuse.GetAgentId(),
                        d.GetAgentId());
                }
                UNIT_ASSERT_VALUES_EQUAL(4_KB, d.GetBlockSize());
                UNIT_ASSERT_VALUES_EQUAL(93_GB / 4_KB, d.GetBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(94_GB / 4_KB, d.GetUnadjustedBlockCount());
            }
            UNIT_ASSERT_VALUES_EQUAL(2, CountIf(devices, [] (auto& d) {
                return d.GetNodeId() == 0;
            }));
        }

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            TDiskRegistryState::TAllocateDiskResult result {};
            auto error = state->AllocateDisk(
                TInstant::Now(),
                db,
                TDiskRegistryState::TAllocateDiskParams {
                    .DiskId = "foo",
                    .PlacementGroupId = {},
                    .BlockSize = 4_KB,
                    .BlocksCount = 93_GB / 4_KB * 4,
                },
                &result);

            UNIT_ASSERT_VALUES_EQUAL(S_ALREADY, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(4, result.Devices.size());
            for (auto& d: result.Devices) {
                if (d.GetNodeId() == 0) {
                    UNIT_ASSERT_VALUES_EQUAL(
                        agentToAbuse.GetAgentId(),
                        d.GetAgentId());
                }
                UNIT_ASSERT_VALUES_EQUAL(4_KB, d.GetBlockSize());
                UNIT_ASSERT_VALUES_EQUAL(93_GB / 4_KB, d.GetBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(94_GB / 4_KB, d.GetUnadjustedBlockCount());
            }
            UNIT_ASSERT_VALUES_EQUAL(2, CountIf(result.Devices, [] (auto& d) {
                return d.GetNodeId() == 0;
            }));
        });

        // restore from DB
        state.reset();

        auto monitoring = CreateMonitoringServiceStub();
        auto rootGroup = monitoring->GetCounters()
            ->GetSubgroup("counters", "blockstore");

        auto serverGroup = rootGroup->GetSubgroup("component", "server");
        InitCriticalEventsCounter(serverGroup);

        auto criticalEvents = serverGroup->FindCounter(
            "AppCriticalEvents/DiskRegistryDeviceNotFound");

        UNIT_ASSERT_VALUES_EQUAL(0, criticalEvents->Val());

        executor.ReadTx([&] (TDiskRegistryDatabase db) {
            NProto::TDiskRegistryConfig config;
            UNIT_ASSERT(db.ReadDiskRegistryConfig(config));

            TVector<NProto::TAgentConfig> oldAgents;
            UNIT_ASSERT(db.ReadOldAgents(oldAgents));
            UNIT_ASSERT_VALUES_EQUAL(2, oldAgents.size());
            Sort(oldAgents, TByAgentId());
            if (agentToAbuse.GetAgentId() == "agent-1") {
                UNIT_ASSERT_VALUES_EQUAL("agent-2", oldAgents[0].GetAgentId());
                UNIT_ASSERT_VALUES_EQUAL(2, oldAgents[0].GetNodeId());
            } else {
                UNIT_ASSERT_VALUES_EQUAL("agent-1", oldAgents[0].GetAgentId());
                UNIT_ASSERT_VALUES_EQUAL(1, oldAgents[0].GetNodeId());
            }

            UNIT_ASSERT_VALUES_EQUAL("agent-3", oldAgents[1].GetAgentId());
            UNIT_ASSERT_VALUES_EQUAL(agentToAbuse.GetNodeId(), oldAgents[1].GetNodeId());

            TVector<NProto::TAgentConfig> agents;
            UNIT_ASSERT(db.ReadAgents(agents));
            UNIT_ASSERT_VALUES_EQUAL(3, agents.size());
            Sort(agents, TByAgentId());

            if (agentToAbuse.GetAgentId() == "agent-1") {
                UNIT_ASSERT_VALUES_EQUAL("agent-1", agents[0].GetAgentId());
                UNIT_ASSERT_VALUES_EQUAL(0, agents[0].GetNodeId());

                UNIT_ASSERT_VALUES_EQUAL("agent-2", agents[1].GetAgentId());
                UNIT_ASSERT_VALUES_EQUAL(2, agents[1].GetNodeId());
            } else {
                UNIT_ASSERT_VALUES_EQUAL("agent-1", agents[0].GetAgentId());
                UNIT_ASSERT_VALUES_EQUAL(1, agents[0].GetNodeId());

                UNIT_ASSERT_VALUES_EQUAL("agent-2", agents[1].GetAgentId());
                UNIT_ASSERT_VALUES_EQUAL(0, agents[1].GetNodeId());
            }

            UNIT_ASSERT_VALUES_EQUAL("agent-3", agents[2].GetAgentId());
            UNIT_ASSERT_VALUES_EQUAL(agentToAbuse.GetNodeId(), agents[2].GetNodeId());

            TVector<NProto::TDiskConfig> disks;
            UNIT_ASSERT(db.ReadDisks(disks));
            UNIT_ASSERT_VALUES_EQUAL(1, disks.size());

            TVector<NProto::TPlacementGroupConfig> placementGroups;
            UNIT_ASSERT(db.ReadPlacementGroups(placementGroups));
            UNIT_ASSERT_VALUES_EQUAL(0, placementGroups.size());

            TVector<TBrokenDiskInfo> brokenDisks;
            UNIT_ASSERT(db.ReadBrokenDisks(brokenDisks));
            UNIT_ASSERT_VALUES_EQUAL(0, brokenDisks.size());

            TVector<TString> disksToNotify;
            UNIT_ASSERT(db.ReadDisksToNotify(disksToNotify));
            UNIT_ASSERT_VALUES_EQUAL(1, disksToNotify.size());

            TVector<TDiskStateUpdate> diskStateUpdates;
            UNIT_ASSERT(db.ReadDiskStateChanges(diskStateUpdates));
            UNIT_ASSERT_VALUES_EQUAL(1, diskStateUpdates.size());

            ui64 diskStateSeqNo = 0;
            UNIT_ASSERT(db.ReadLastDiskStateSeqNo(diskStateSeqNo));
            UNIT_ASSERT_LT(0, diskStateSeqNo);

            TVector<TString> dirtyDevices;
            UNIT_ASSERT(db.ReadDirtyDevices(dirtyDevices));
            UNIT_ASSERT_VALUES_EQUAL(0, dirtyDevices.size());

            bool diskAllocationAllowed = false;
            UNIT_ASSERT(db.ReadDiskAllocationAllowed(diskAllocationAllowed));
            UNIT_ASSERT(diskAllocationAllowed);

            TVector<TString> disksToCleanup;
            UNIT_ASSERT(db.ReadDisksToCleanup(disksToCleanup));
            UNIT_ASSERT_VALUES_EQUAL(0, disksToCleanup.size());

            TVector<TString> errorNotifications;
            UNIT_ASSERT(db.ReadErrorNotifications(errorNotifications));
            UNIT_ASSERT_VALUES_EQUAL(1, errorNotifications.size());
            UNIT_ASSERT_VALUES_EQUAL("foo", errorNotifications[0]);

            TVector<TString> outdatedVolumeConfigs;
            UNIT_ASSERT(db.ReadOutdatedVolumeConfigs(outdatedVolumeConfigs));
            UNIT_ASSERT_VALUES_EQUAL(0, outdatedVolumeConfigs.size());

            TVector<TString> suspendedDevices;
            UNIT_ASSERT(db.ReadSuspendedDevices(suspendedDevices));
            UNIT_ASSERT_VALUES_EQUAL(0, suspendedDevices.size());

            state.emplace(TDiskRegistryState {
                CreateStorageConfig([] {
                    auto proto = CreateDefaultStorageConfigProto();
                    proto.SetAllocationUnitNonReplicatedSSD(93);
                    return proto;
                }()),
                rootGroup,
                std::move(config),
                std::move(agents),
                std::move(disks),
                std::move(placementGroups),
                std::move(brokenDisks),
                std::move(disksToNotify),
                std::move(diskStateUpdates),
                diskStateSeqNo,
                std::move(dirtyDevices),
                diskAllocationAllowed,
                std::move(disksToCleanup),
                std::move(errorNotifications),
                std::move(outdatedVolumeConfigs),
                std::move(suspendedDevices)
            });
        });

        UNIT_ASSERT_VALUES_EQUAL(0, criticalEvents->Val());

        // return agent

        executor.WriteTx([&] (TDiskRegistryDatabase db) {
            agentToAbuse.SetNodeId(100);

            TVector<TDiskStateUpdate> affectedDisks;
            THashMap<TString, ui64> notifiedDisks;

            auto error = state->RegisterAgent(
                db,
                agentToAbuse,
                TInstant::Now(),
                &affectedDisks,
                &notifiedDisks);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());

            UNIT_ASSERT_VALUES_EQUAL("foo", affectedDisks[0].State.GetDiskId());
            UNIT_ASSERT_EQUAL(
                NProto::DISK_STATE_MIGRATION,
                affectedDisks[0].State.GetState());

            UNIT_ASSERT_VALUES_EQUAL(1, notifiedDisks.size());

            UNIT_ASSERT_VALUES_EQUAL(0, state->GetDirtyDevices().size());
        });

        {
            TVector<NProto::TDeviceConfig> devices;
            auto error = state->GetDiskDevices("foo", devices);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, error.GetCode());
            UNIT_ASSERT_VALUES_EQUAL(4, devices.size());

            for (auto& d: devices) {
                if (d.GetAgentId() == agentToAbuse.GetAgentId()) {
                    UNIT_ASSERT_VALUES_EQUAL(
                        agentToAbuse.GetNodeId(),
                        d.GetNodeId());
                }
                UNIT_ASSERT_VALUES_UNEQUAL(0, d.GetNodeId());
                UNIT_ASSERT_VALUES_EQUAL(4_KB, d.GetBlockSize());
                UNIT_ASSERT_VALUES_EQUAL(93_GB / 4_KB, d.GetBlocksCount());
                UNIT_ASSERT_VALUES_EQUAL(94_GB / 4_KB, d.GetUnadjustedBlockCount());
            }
            UNIT_ASSERT_VALUES_EQUAL(0, CountIf(devices, [] (auto& d) {
                return d.GetNodeId() == 0;
            }));
        }
    }
}

}   // namespace NCloud::NBlockStore::NStorage
