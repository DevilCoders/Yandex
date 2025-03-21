#include "disk_registry.h"
#include "disk_registry_actor.h"

#include <cloud/blockstore/config/disk.pb.h>

#include <cloud/blockstore/libs/storage/api/disk_agent.h>
#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/api/volume_proxy.h>
#include <cloud/blockstore/libs/storage/disk_registry/testlib/test_env.h>
#include <cloud/blockstore/libs/storage/testlib/ss_proxy_client.h>

#include <ydb/core/testlib/basics/runtime.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/datetime/base.h>
#include <util/generic/size_literals.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;
using namespace NKikimr;
using namespace NDiskRegistryTest;

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TDiskRegistryTest)
{
    void ShouldMigrateDeviceImpl(bool rebootTablet)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1.1", "rack-1", 10_GB),
                Device("dev-2", "uuid-1.2", "rack-1", 10_GB)
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-1", "uuid-2.1", "rack-2", 10_GB),
                Device("dev-2", "uuid-2.2", "rack-2", 10_GB)
            })
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents(agents)
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(agents));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 2);
        WaitForAgents(*runtime, 2);
        WaitForSecureErase(*runtime, agents);

        const auto devices = [&] {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());

            return TVector {
                msg.GetDevices(0).GetDeviceUUID(),
                msg.GetDevices(1).GetDeviceUUID()
            };
        }();

        auto getTarget = [&] {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL(devices[0], msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(devices[1], msg.GetDevices(1).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(1, msg.MigrationsSize());
            const auto& m = msg.GetMigrations(0);
            UNIT_ASSERT_VALUES_EQUAL(devices[0], m.GetSourceDeviceId());
            UNIT_ASSERT(!m.GetTargetDevice().GetDeviceUUID().empty());

            return m.GetTargetDevice().GetDeviceUUID();
        };

        diskRegistry.ChangeDeviceState(devices[0], NProto::DEVICE_STATE_WARNING);
        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        const auto target = getTarget();

        if (rebootTablet) {
            diskRegistry.RebootTablet();
            diskRegistry.WaitReady();

            RegisterAgents(*runtime, 2);
            WaitForAgents(*runtime, 2);
        }

        UNIT_ASSERT_VALUES_EQUAL(target, getTarget());

        diskRegistry.ChangeDeviceState(target, NProto::DEVICE_STATE_WARNING);
        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        const auto newTarget = getTarget();
        UNIT_ASSERT_VALUES_UNEQUAL(target, newTarget);

        {
            diskRegistry.SendFinishMigrationRequest("disk-1", devices[0], target);
            auto response = diskRegistry.RecvFinishMigrationResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_ARGUMENT, response->GetStatus());
        }

        diskRegistry.FinishMigration("disk-1", devices[0], newTarget);

        {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL(newTarget, msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(devices[1], msg.GetDevices(1).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());
        }
    }

    Y_UNIT_TEST(ShouldMigrateDevice)
    {
        ShouldMigrateDeviceImpl(false);
    }

    Y_UNIT_TEST(ShouldMigrateDeviceAfterReboot)
    {
        ShouldMigrateDeviceImpl(true);
    }

    Y_UNIT_TEST(ShouldCancelMigrationAfterDiskDeallocation)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1.1", "rack-1", 10_GB),
                Device("dev-2", "uuid-1.2", "rack-1", 10_GB)
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-1", "uuid-2.1", "rack-2", 10_GB),
                Device("dev-2", "uuid-2.2", "rack-2", 10_GB)
            })
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents(agents)
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(agents));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 2);
        WaitForAgents(*runtime, 2);
        WaitForSecureErase(*runtime, agents);

        const auto devices = [&] {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());

            return TVector {
                msg.GetDevices(0).GetDeviceUUID(),
                msg.GetDevices(1).GetDeviceUUID()
            };
        }();

        diskRegistry.ChangeDeviceState(devices[0], NProto::DEVICE_STATE_WARNING);
        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL(devices[0], msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(devices[1], msg.GetDevices(1).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(1, msg.MigrationsSize());
            const auto& m = msg.GetMigrations(0);
            UNIT_ASSERT_VALUES_EQUAL(devices[0], m.GetSourceDeviceId());
            UNIT_ASSERT(!m.GetTargetDevice().GetDeviceUUID().empty());
        }

        {
            diskRegistry.SendAllocateDiskRequest("disk-2", 20_GB);
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, response->GetStatus());
        }

        diskRegistry.DeallocateDisk("disk-1", true);
        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        {
            auto response = diskRegistry.AllocateDisk("disk-2", 30_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(3, msg.DevicesSize());
            for (auto& d: msg.GetDevices()) {
                UNIT_ASSERT_VALUES_UNEQUAL(devices[0], d.GetDeviceUUID());
            }
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());
        }
    }

    Y_UNIT_TEST(ShouldMigrateWithRespectToPlacementGroup)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1.1", "rack-1", 10_GB),
                Device("dev-2", "uuid-1.2", "rack-1", 10_GB)
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-1", "uuid-2.1", "rack-2", 10_GB),
                Device("dev-2", "uuid-2.2", "rack-2", 10_GB)
            }),
            CreateAgentConfig("agent-3", {
                Device("dev-1", "uuid-3.1", "rack-3", 10_GB),
                Device("dev-2", "uuid-3.2", "rack-3", 10_GB)
            })
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents(agents)
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig({agents[0]}));
        diskRegistry.AllowDiskAllocation();

        RegisterAgent(*runtime, 0);
        WaitForAgent(*runtime, 0);
        WaitForSecureErase(*runtime, {agents[0]});

        diskRegistry.CreatePlacementGroup("pg");

        auto device1 = [&] {
            auto response = diskRegistry.AllocateDisk(
                "disk-1", 10_GB, DefaultLogicalBlockSize, "pg");
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(1, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL("rack-1", msg.GetDevices(0).GetRack());
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());

            return msg.GetDevices(0).GetDeviceUUID();
        }();

        {
            auto response = diskRegistry.AllocateDisk("dummy", 10_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(1, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL("rack-1", msg.GetDevices(0).GetRack());
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());
        }

        diskRegistry.UpdateConfig(CreateRegistryConfig(1, {agents[0], agents[1]}));
        diskRegistry.AllowDiskAllocation();

        RegisterAgent(*runtime, 1);
        WaitForAgent(*runtime, 1);
        WaitForSecureErase(*runtime, {agents[1]});

        {
            auto response = diskRegistry.AllocateDisk(
                "disk-2", 10_GB, DefaultLogicalBlockSize, "pg");
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(1, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL("rack-2", msg.GetDevices(0).GetRack());
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());
        }

        diskRegistry.UpdateConfig(CreateRegistryConfig(2, agents));
        RegisterAgent(*runtime, 2);
        WaitForAgent(*runtime, 2);
        WaitForSecureErase(*runtime, {agents[2]});

        diskRegistry.ChangeDeviceState(device1, NProto::DEVICE_STATE_WARNING);
        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        auto target1 = [&] {
            auto response = diskRegistry.AllocateDisk(
                "disk-1", 10_GB, DefaultLogicalBlockSize, "pg");
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(1, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL(device1, msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(1, msg.MigrationsSize());
            const auto& m = msg.GetMigrations(0);

            UNIT_ASSERT_VALUES_EQUAL(device1, m.GetSourceDeviceId());
            UNIT_ASSERT_VALUES_EQUAL("rack-3", m.GetTargetDevice().GetRack());

            return m.GetTargetDevice().GetDeviceUUID();
        }();

        {
            diskRegistry.SendAllocateDiskRequest(
                "disk-3", 10_GB, DefaultLogicalBlockSize, "pg");
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, response->GetStatus());
        }

        diskRegistry.FinishMigration("disk-1", device1, target1);

        {
            auto response = diskRegistry.AllocateDisk(
                "disk-1", 10_GB, DefaultLogicalBlockSize, "pg");
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(1, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL(target1, msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());
        }

        {
            diskRegistry.SendAllocateDiskRequest(
                "disk-3", 10_GB, DefaultLogicalBlockSize, "pg");
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, response->GetStatus());
        }

        diskRegistry.DeallocateDisk("dummy", true);
        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        {
            auto response = diskRegistry.AllocateDisk(
                "disk-3", 10_GB, DefaultLogicalBlockSize, "pg");
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(1, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL("rack-1", msg.GetDevices(0).GetRack());
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());
        }
    }

    Y_UNIT_TEST(ShouldCancelMigrationFromBrokenAgent)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1.1", "rack-1", 10_GB),
                Device("dev-2", "uuid-1.2", "rack-1", 10_GB)
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-1", "uuid-2.1", "rack-2", 10_GB),
                Device("dev-2", "uuid-2.2", "rack-2", 10_GB)
            })
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents(agents)
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(agents));
        diskRegistry.AllowDiskAllocation();

        // register agent-1
        RegisterAgent(*runtime, 0);
        WaitForAgent(*runtime, 0);
        WaitForSecureErase(*runtime, {agents[0]});

        const auto devices = [&] {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());

            return TVector {
                msg.GetDevices(0).GetDeviceUUID(),
                msg.GetDevices(1).GetDeviceUUID()
            };
        }();

        UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", devices[0]);
        UNIT_ASSERT_VALUES_EQUAL("uuid-1.2", devices[1]);

        // register agent-2
        RegisterAgent(*runtime, 1);
        WaitForAgent(*runtime, 1);
        WaitForSecureErase(*runtime, {agents[1]});

        diskRegistry.ChangeAgentState("agent-1", NProto::AGENT_STATE_WARNING);
        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(devices[0], msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(devices[1], msg.GetDevices(1).GetDeviceUUID());

            UNIT_ASSERT_VALUES_EQUAL(2, msg.MigrationsSize());
            const auto& m1 = msg.GetMigrations(0);
            const auto& m2 = msg.GetMigrations(1);

            TVector sources {
                m1.GetSourceDeviceId(),
                m2.GetSourceDeviceId()
            };

            Sort(sources);
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", sources[0]);
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.2", sources[1]);

            TVector targets {
                m1.GetTargetDevice().GetDeviceUUID(),
                m2.GetTargetDevice().GetDeviceUUID()
            };

            Sort(targets);
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", targets[0]);
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.2", targets[1]);
        }

        diskRegistry.ChangeAgentState("agent-1", NProto::AGENT_STATE_UNAVAILABLE);

        {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(devices[0], msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(devices[1], msg.GetDevices(1).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());
            UNIT_ASSERT_EQUAL(NProto::VOLUME_IO_OK, msg.GetIOMode());
        }

        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        {
            auto response = diskRegistry.AllocateDisk("disk-2", 20_GB);
            auto& msg = response->Record;
            SortBy(*msg.MutableDevices(), TByUUID());
            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-2.2", msg.GetDevices(1).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());
        }
    }

    Y_UNIT_TEST(ShouldCancelMigrationFromBrokenDevice)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1.1", "rack-1", 10_GB),
                Device("dev-2", "uuid-1.2", "rack-1", 10_GB)
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-1", "uuid-2.1", "rack-2", 10_GB),
                Device("dev-2", "uuid-2.2", "rack-2", 10_GB)
            })
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents(agents)
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(agents));
        diskRegistry.AllowDiskAllocation();

        RegisterAndWaitForAgent(*runtime, 0, 2);

        auto allocateDisk = [&] {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());

            return msg;
        };

        const auto devices = [&] {
            auto msg = allocateDisk();
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());

            return TVector {
                msg.GetDevices(0).GetDeviceUUID(),
                msg.GetDevices(1).GetDeviceUUID()
            };
        }();

        UNIT_ASSERT(devices[0].StartsWith("uuid-1."));
        UNIT_ASSERT(devices[1].StartsWith("uuid-1."));

        RegisterAndWaitForAgent(*runtime, 1, 2);

        // start migration from agent-1 to agent-2
        diskRegistry.ChangeAgentState("agent-1", NProto::AGENT_STATE_WARNING);
        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        const auto targets = [&] {
            auto msg = allocateDisk();

            UNIT_ASSERT_VALUES_EQUAL(devices[0], msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(devices[1], msg.GetDevices(1).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(2, msg.MigrationsSize());
            SortBy(*msg.MutableMigrations(), [] (auto& x) {
                return x.GetSourceDeviceId();
            });

            const auto& m1 = msg.GetMigrations(0);
            UNIT_ASSERT_VALUES_EQUAL(devices[0], m1.GetSourceDeviceId());
            UNIT_ASSERT(!m1.GetTargetDevice().GetDeviceUUID().empty());
            const auto& m2 = msg.GetMigrations(1);
            UNIT_ASSERT_VALUES_EQUAL(devices[1], m2.GetSourceDeviceId());
            UNIT_ASSERT(!m2.GetTargetDevice().GetDeviceUUID().empty());

            return TVector {
                m1.GetTargetDevice().GetDeviceUUID(),
                m2.GetTargetDevice().GetDeviceUUID()
            };
        }();

        UNIT_ASSERT(targets[0].StartsWith("uuid-2."));
        UNIT_ASSERT(targets[1].StartsWith("uuid-2."));

        // cancel migration for uuid-1.1
        diskRegistry.ChangeDeviceState(devices[0], NProto::DEVICE_STATE_ERROR);
        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        {
            auto msg = allocateDisk();

            UNIT_ASSERT_VALUES_EQUAL(devices[0], msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(devices[1], msg.GetDevices(1).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(1, msg.MigrationsSize());
            UNIT_ASSERT_VALUES_EQUAL(devices[1], msg.GetMigrations(0).GetSourceDeviceId());
            UNIT_ASSERT_VALUES_EQUAL(
                targets[1],
                msg.GetMigrations(0).GetTargetDevice().GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::VOLUME_IO_ERROR_READ_ONLY, msg.GetIOMode());
        }

        {
            diskRegistry.SendFinishMigrationRequest("disk-1", devices[0], targets[0]);
            auto response = diskRegistry.RecvFinishMigrationResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_ARGUMENT, response->GetStatus());
        }

        {
            diskRegistry.SendFinishMigrationRequest("disk-1", devices[1], targets[1]);
            auto response = diskRegistry.RecvFinishMigrationResponse();
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());
        }

        diskRegistry.RebootTablet();
        diskRegistry.WaitReady();

        WaitForAgents(*runtime, agents.size());

        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        {
            auto msg = allocateDisk();

            UNIT_ASSERT_VALUES_EQUAL(devices[0], msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(targets[1], msg.GetDevices(1).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());
            UNIT_ASSERT_EQUAL(NProto::VOLUME_IO_ERROR_READ_ONLY, msg.GetIOMode());
        }
    }

    Y_UNIT_TEST(ShouldHandleMultipleCmsRequests)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1.1", "rack-1", 10_GB),
                Device("dev-2", "uuid-1.2", "rack-1", 10_GB)
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-1", "uuid-2.1", "rack-2", 10_GB),
                Device("dev-2", "uuid-2.2", "rack-2", 10_GB)
            }),
            CreateAgentConfig("agent-3", {
                Device("dev-1", "uuid-3.1", "rack-3", 10_GB),
                Device("dev-2", "uuid-3.2", "rack-3", 10_GB)
            })
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents(agents)
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(agents));
        diskRegistry.AllowDiskAllocation();

        auto addAgent = [&] (int i) {
            RegisterAgent(*runtime, i);
            WaitForAgent(*runtime, i);
            WaitForSecureErase(*runtime, {agents[i]});
        };

        addAgent(0);

        const auto devices = [&] {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());

            return TVector {
                msg.GetDevices(0).GetDeviceUUID(),
                msg.GetDevices(1).GetDeviceUUID()
            };
        }();

        addAgent(1);
        addAgent(2);

        auto getTargets = [&] {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL(devices[0], msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(devices[1], msg.GetDevices(1).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(2, msg.MigrationsSize());
            SortBy(*msg.MutableMigrations(), [] (auto& x) {
                return x.GetSourceDeviceId();
            });

            const auto& m1 = msg.GetMigrations(0);
            UNIT_ASSERT_VALUES_EQUAL(devices[0], m1.GetSourceDeviceId());
            UNIT_ASSERT(!m1.GetTargetDevice().GetDeviceUUID().empty());
            const auto& m2 = msg.GetMigrations(1);
            UNIT_ASSERT_VALUES_EQUAL(devices[1], m2.GetSourceDeviceId());
            UNIT_ASSERT(!m2.GetTargetDevice().GetDeviceUUID().empty());

            return TVector {
                m1.GetTargetDevice().GetDeviceUUID(),
                m2.GetTargetDevice().GetDeviceUUID()
            };
        };

        auto changeAgentState = [&] {
            diskRegistry.ChangeAgentState("agent-1", NProto::AGENT_STATE_WARNING);
            runtime->AdvanceCurrentTime(TDuration::Seconds(20));
            runtime->DispatchEvents({}, TDuration::MilliSeconds(10));
        };

        changeAgentState();

        auto targets = getTargets();
        UNIT_ASSERT_VALUES_EQUAL(2, targets.size());

        changeAgentState();

        const auto newTargets = getTargets();
        UNIT_ASSERT_VALUES_EQUAL(2, newTargets.size());
        UNIT_ASSERT_VALUES_EQUAL(targets[0], newTargets[0]);
        UNIT_ASSERT_VALUES_EQUAL(targets[1], newTargets[1]);
    }

    Y_UNIT_TEST(ShouldNotifyVolumeAboutNewMigration)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1.1", "rack-1", 10_GB),
                Device("dev-2", "uuid-1.2", "rack-1", 10_GB),
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-1", "uuid-2.1", "rack-2", 10_GB),
                Device("dev-2", "uuid-2.2", "rack-2", 10_GB),
            }),
            CreateAgentConfig("agent-3", {
                Device("dev-1", "uuid-3.1", "rack-3", 10_GB),
                Device("dev-3", "uuid-3.2", "rack-3", 10_GB),
            }),
            CreateAgentConfig("agent-4", {
                Device("dev-1", "uuid-4.1", "rack-4", 10_GB),
                Device("dev-4", "uuid-4.2", "rack-4", 10_GB),
            })
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents(agents)
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(agents));
        diskRegistry.AllowDiskAllocation();

        size_t cleanDevices = 0;
        size_t reallocateCount = 0;

        runtime->SetObserverFunc(
        [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
            switch (event->GetTypeRewrite()) {
                case TEvDiskAgent::EvSecureEraseDeviceResponse: {
                    auto* msg = event->Get<TEvDiskAgent::TEvSecureEraseDeviceResponse>();
                    cleanDevices += !HasError(msg->GetError());
                    break;
                }
                case TEvVolume::EvReallocateDiskResponse: {
                    auto* msg = event->Get<TEvVolume::TEvReallocateDiskResponse>();
                    reallocateCount += !HasError(msg->GetError());
                    break;
                }
            }

            return TTestActorRuntime::DefaultObserverFunc(runtime, event);
        });

        auto addAgent = [&] (int i) {
            const size_t prevCleanDevices = cleanDevices;
            const size_t deviceCount = agents[i].DevicesSize();

            RegisterAgent(*runtime, i);
            WaitForAgent(*runtime, i);

            runtime->AdvanceCurrentTime(TDuration::Seconds(5));

            TDispatchOptions options;
            options.CustomFinalCondition = [&] {
                return cleanDevices - prevCleanDevices >= deviceCount;
            };
            runtime->DispatchEvents(options);
        };

        auto getDevices = [&] {
            auto response = diskRegistry.AllocateDisk("disk-1", 40_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(4, msg.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());

            return TVector {
                msg.GetDevices(0).GetDeviceUUID(),
                msg.GetDevices(1).GetDeviceUUID(),
                msg.GetDevices(2).GetDeviceUUID(),
                msg.GetDevices(3).GetDeviceUUID()
            };
        };

        auto changeAgentState = [&] (const auto* name) {
            const size_t prevReallocateCount = reallocateCount;

            diskRegistry.ChangeAgentState(name, NProto::AGENT_STATE_WARNING);
            runtime->AdvanceCurrentTime(TDuration::Seconds(20));

            // wait for notification
            TDispatchOptions options;
            options.CustomFinalCondition = [&] {
                return reallocateCount > prevReallocateCount;
            };
            runtime->DispatchEvents(options);
        };

        addAgent(0);
        addAgent(1);

        const auto devices = getDevices();

        auto getTargets = [&] {
            auto response = diskRegistry.AllocateDisk("disk-1", 40_GB);
            auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(4, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL(devices[0], msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(devices[1], msg.GetDevices(1).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(devices[2], msg.GetDevices(2).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(devices[3], msg.GetDevices(3).GetDeviceUUID());

            UNIT_ASSERT_GE(devices.size(), msg.MigrationsSize());

            SortBy(*msg.MutableMigrations(), [] (auto& x) {
                return x.GetSourceDeviceId();
            });

            TVector<TString> r;

            for (size_t i = 0; i != msg.MigrationsSize(); ++i) {
                auto& m = msg.GetMigrations(i);
                UNIT_ASSERT(!m.GetTargetDevice().GetDeviceUUID().empty());
                r.push_back(m.GetTargetDevice().GetDeviceUUID());
            }

            Sort(r);

            return r;
        };

        addAgent(2);
        changeAgentState("agent-1");

        const auto targets = getTargets();
        UNIT_ASSERT_VALUES_EQUAL(2, targets.size());
        UNIT_ASSERT_VALUES_EQUAL("uuid-3.1", targets[0]);
        UNIT_ASSERT_VALUES_EQUAL("uuid-3.2", targets[1]);

        addAgent(3);
        changeAgentState("agent-2");

        const auto newTargets = getTargets();
        UNIT_ASSERT_VALUES_EQUAL(4, newTargets.size());

        UNIT_ASSERT_VALUES_EQUAL(targets[0], newTargets[0]);
        UNIT_ASSERT_VALUES_EQUAL(targets[1], newTargets[1]);
        UNIT_ASSERT_VALUES_EQUAL("uuid-4.1", newTargets[2]);
        UNIT_ASSERT_VALUES_EQUAL("uuid-4.2", newTargets[3]);
    }

    Y_UNIT_TEST(ShouldReleaseMigrationTargets)
    {
        const auto agentConfig = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1.1", "rack-1", 10_GB),
            Device("dev-2", "uuid-1.2", "rack-1", 10_GB),
            Device("dev-3", "uuid-1.3", "rack-1", 10_GB),
            Device("dev-4", "uuid-1.4", "rack-1", 10_GB),
        });

        THashSet<TString> acquiredDevices;

        auto agent = CreateTestDiskAgent(agentConfig);

        agent->HandleAcquireDevicesImpl = [&] (
            const TEvDiskAgent::TEvAcquireDevicesRequest::TPtr& ev,
            const TActorContext& ctx)
        {
            const auto& record = ev->Get()->Record;

            auto code = S_OK;
            for (const auto& uuid: record.GetDeviceUUIDs()) {
                if (acquiredDevices.contains(uuid)) {
                    code = E_BS_INVALID_SESSION;
                    break;
                }
            }

            if (code == S_OK) {
                for (const auto& uuid: record.GetDeviceUUIDs()) {
                    acquiredDevices.insert(uuid);
                }
            }

            NCloud::Reply(
                ctx,
                *ev,
                std::make_unique<TEvDiskAgent::TEvAcquireDevicesResponse>(
                    MakeError(code)
                )
            );
            return true;
        };

        agent->HandleReleaseDevicesImpl = [&] (
            const TEvDiskAgent::TEvReleaseDevicesRequest::TPtr& ev,
            const TActorContext& ctx)
        {
            Y_UNUSED(ctx);

            const auto& record = ev->Get()->Record;
            for (const auto& uuid: record.GetDeviceUUIDs()) {
                acquiredDevices.erase(uuid);
            }
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig({ agentConfig }));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 1);
        WaitForAgents(*runtime, 1);
        WaitForSecureErase(*runtime, { agentConfig });

        {
            auto response = diskRegistry.AllocateDisk("disk-1", 30_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(3, msg.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());

            diskRegistry.ChangeDeviceState(
                msg.GetDevices(0).GetDeviceUUID(),
                NProto::DEVICE_STATE_WARNING);
        }

        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        {
            auto response = diskRegistry.AcquireDisk("disk-1", "session-1");
            const auto& record = response->Record;
            UNIT_ASSERT_C(!HasError(record), record.GetError());
            UNIT_ASSERT_VALUES_EQUAL(4, record.DevicesSize());
        }

        UNIT_ASSERT_VALUES_EQUAL(4, acquiredDevices.size());

        diskRegistry.ReleaseDisk("disk-1", "session-1");

        UNIT_ASSERT_VALUES_EQUAL(0, acquiredDevices.size());

        {
            auto response = diskRegistry.AcquireDisk("disk-1", "session-2");
            const auto& record = response->Record;
            UNIT_ASSERT_C(!HasError(record), record.GetError());
            UNIT_ASSERT_VALUES_EQUAL(4, record.DevicesSize());
        }

        UNIT_ASSERT_VALUES_EQUAL(4, acquiredDevices.size());
    }

    Y_UNIT_TEST(ShouldCancelMigrationFromLostDevices)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1.1", "rack-1", 10_GB, 4_KB),
                Device("dev-2", "uuid-1.2", "rack-1", 10_GB, 4_KB),
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-1", "uuid-2.1", "rack-2", 10_GB, 4_KB),
                Device("dev-2", "uuid-2.2", "rack-2", 10_GB, 4_KB),
            })
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents(agents)
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(agents));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 2);
        WaitForAgents(*runtime, 2);
        WaitForSecureErase(*runtime, agents);

        {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());

            for (auto& d: msg.GetDevices()) {
                diskRegistry.ChangeDeviceState(d.GetDeviceUUID(), NProto::DEVICE_STATE_WARNING);
            }
            runtime->AdvanceCurrentTime(TDuration::Seconds(20));
            runtime->DispatchEvents({}, TDuration::MilliSeconds(10));
        }

        {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(2, msg.MigrationsSize());
        }

        diskRegistry.RegisterAgent([] {
            auto config = CreateAgentConfig("agent-1", {});
            config.SetNodeId(42);
            return config;
        }());

        runtime->AdvanceCurrentTime(TDuration::MilliSeconds(500));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());
        }
    }

    Y_UNIT_TEST(ShouldRestartMigrationInCaseOfBrokenTragets)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1.1", "uuid-1.1", "rack-1", 10_GB, 4_KB),
                Device("dev-1.2", "uuid-1.2", "rack-1", 10_GB, 4_KB),
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-2.1", "uuid-2.1", "rack-2", 10_GB, 4_KB),
                Device("dev-2.2", "uuid-2.2", "rack-2", 10_GB, 4_KB),
            }),
            CreateAgentConfig("agent-3", {
                Device("dev-3.1", "uuid-3.1", "rack-3", 10_GB, 4_KB),
                Device("dev-3.2", "uuid-3.2", "rack-3", 10_GB, 4_KB),
            })
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents(agents)
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(agents));
        diskRegistry.AllowDiskAllocation();

        auto registerAndWaitForAgent = [&] (int index) {
            RegisterAndWaitForAgent(*runtime, index, agents[index].DevicesSize());
        };

        registerAndWaitForAgent(0);

        const auto sources = [&] {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());

            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());

            TVector<TString> devices;

            for (auto& d: msg.GetDevices()) {
                devices.push_back(d.GetDeviceUUID());
            }

            Sort(devices);

            return devices;
        }();

        UNIT_ASSERT_VALUES_EQUAL(2, sources.size());
        UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", sources[0]);
        UNIT_ASSERT_VALUES_EQUAL("uuid-1.2", sources[1]);

        registerAndWaitForAgent(1);

        diskRegistry.ChangeAgentState("agent-1", NProto::AGENT_STATE_WARNING);
        for (auto& uuid: sources) {
            diskRegistry.SendChangeDeviceStateRequest(uuid, NProto::DEVICE_STATE_WARNING);
        }

        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        auto getTargets = [&] {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());

            TVector<TString> targets;
            targets.reserve(msg.MigrationsSize());

            for (const auto& m: msg.GetMigrations()) {
                targets.push_back(m.GetTargetDevice().GetDeviceUUID());
            }

            Sort(targets);

            return targets;
        };

        const auto targets = getTargets();
        UNIT_ASSERT_VALUES_EQUAL(2, targets.size());
        UNIT_ASSERT_VALUES_EQUAL("uuid-2.1", targets[0]);
        UNIT_ASSERT_VALUES_EQUAL("uuid-2.2", targets[1]);

        registerAndWaitForAgent(2);

        diskRegistry.RegisterAgent([]{
            auto config = CreateAgentConfig("agent-2", {});
            config.SetNodeId(42);
            return config;
        }());
        runtime->AdvanceCurrentTime(TDuration::MilliSeconds(500));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        const auto newTargets = getTargets();
        UNIT_ASSERT_VALUES_EQUAL(2, targets.size());
        UNIT_ASSERT_VALUES_EQUAL("uuid-3.1", newTargets[0]);
        UNIT_ASSERT_VALUES_EQUAL("uuid-3.2", newTargets[1]);
    }

    Y_UNIT_TEST(ShouldMigrateDiskWith64KBlockSize)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1.1", "uuid-1.1", "rack-1", 93_GB, 4_KB),
                Device("dev-1.2", "uuid-1.2", "rack-1", 93_GB, 4_KB),
            })
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents(agents)
            .With([] {
                auto config = CreateDefaultStorageConfig();
                config.SetAllocationUnitNonReplicatedSSD(93);
                return config;
            }())
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(agents));
        diskRegistry.AllowDiskAllocation();

        RegisterAndWaitForAgents(*runtime, agents);

        const auto source = [&] {
            auto response = diskRegistry.AllocateDisk("disk-1", 93_GB, 64_KB);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());

            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());
            UNIT_ASSERT_VALUES_EQUAL(1, msg.DevicesSize());

            return msg.GetDevices(0).GetDeviceUUID();
        }();

        diskRegistry.ChangeDeviceState(source, NProto::DEVICE_STATE_WARNING);

        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        auto response = diskRegistry.AllocateDisk("disk-1", 93_GB, 64_KB);

        const auto& msg = response->Record;
        UNIT_ASSERT_VALUES_EQUAL(1, msg.DevicesSize());
        UNIT_ASSERT_VALUES_EQUAL(1, msg.MigrationsSize());

        const auto& m = msg.GetMigrations(0);
        const auto& target = m.GetTargetDevice();

        UNIT_ASSERT_VALUES_EQUAL(source, m.GetSourceDeviceId());

        UNIT_ASSERT_VALUES_EQUAL(64_KB, target.GetBlockSize());
        UNIT_ASSERT_VALUES_EQUAL(93_GB / 64_KB, target.GetBlocksCount());
    }

    Y_UNIT_TEST(ShouldAllowMigrationForDiskWithUnavailableState)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1.1", "rack-1", 10_GB, 4_KB),
                Device("dev-2", "uuid-1.2", "rack-1", 10_GB, 4_KB),
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-1", "uuid-2.1", "rack-2", 10_GB, 4_KB),
                Device("dev-2", "uuid-2.2", "rack-2", 10_GB, 4_KB),
            }),
            CreateAgentConfig("agent-3", {
                Device("dev-1", "uuid-3.1", "rack-3", 10_GB, 4_KB),
                Device("dev-2", "uuid-3.2", "rack-3", 10_GB, 4_KB),
            }),
            CreateAgentConfig("agent-4", {
                Device("dev-1", "uuid-4.1", "rack-4", 10_GB, 4_KB),
                Device("dev-2", "uuid-4.2", "rack-4", 10_GB, 4_KB),
            })
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents(agents)
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(agents));
        diskRegistry.AllowDiskAllocation();

        // agent-1 & agent-2
        RegisterAndWaitForAgent(*runtime, 0, 2);
        RegisterAndWaitForAgent(*runtime, 1, 2);

        const auto sources = [&] {
            auto response = diskRegistry.AllocateDisk("disk-1", 40_GB);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());

            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());

            return TVector<NProto::TDeviceConfig>{
                msg.GetDevices().begin(),
                msg.GetDevices().end()
            };
        }();

        // agent-3 & agent-4
        RegisterAndWaitForAgent(*runtime, 2, 2);
        RegisterAndWaitForAgent(*runtime, 3, 2);

        auto getMigrationsSize = [&] {
            auto response = diskRegistry.AllocateDisk("disk-1", 40_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(4, msg.DevicesSize());

            return msg.MigrationsSize();
        };

        UNIT_ASSERT_VALUES_EQUAL(4, sources.size());

        diskRegistry.ChangeDeviceState(
            sources[0].GetDeviceUUID(),
            NProto::DEVICE_STATE_WARNING);

        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        UNIT_ASSERT_VALUES_EQUAL(1, getMigrationsSize());

        const auto unavailableAgentId = [&] {
            auto* device = FindIfPtr(sources, [&] (const auto& d) {
                return d.GetNodeId() != sources[0].GetNodeId();
            });

            UNIT_ASSERT(device);
            return device->GetAgentId();
        }();

        diskRegistry.ChangeAgentState(
            unavailableAgentId,
            NProto::AGENT_STATE_UNAVAILABLE);

        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        UNIT_ASSERT_VALUES_EQUAL(1, getMigrationsSize());

        for (auto& s: sources) {
            diskRegistry.ChangeDeviceState(
                s.GetDeviceUUID(),
                NProto::DEVICE_STATE_WARNING);
        }

        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        UNIT_ASSERT_VALUES_EQUAL(2, getMigrationsSize());

        diskRegistry.RebootTablet();
        diskRegistry.WaitReady();

        // register only alive agents
        for (size_t i = 0; i != agents.size(); ++i) {
            if (agents[i].GetAgentId() != unavailableAgentId) {
                RegisterAgent(*runtime, i);
            }
        }

        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        UNIT_ASSERT_VALUES_EQUAL(2, getMigrationsSize());
    }

    Y_UNIT_TEST(ShouldDeferReleaseMigrationDevices)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1.1", "rack-1", 10_GB, 4_KB),
                Device("dev-2", "uuid-1.2", "rack-1", 10_GB, 4_KB),
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-1", "uuid-2.1", "rack-2", 10_GB, 4_KB),
                Device("dev-2", "uuid-2.2", "rack-2", 10_GB, 4_KB),
            })
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents(agents)
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(agents));
        diskRegistry.AllowDiskAllocation();

        // agent-1
        RegisterAndWaitForAgent(*runtime, 0, 2);

        const auto sources = [&] {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());

            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());

            return TVector<NProto::TDeviceConfig>{
                msg.GetDevices().begin(),
                msg.GetDevices().end()
            };
        }();

        // agent-2
        RegisterAndWaitForAgent(*runtime, 1, 2);

        diskRegistry.ChangeAgentState("agent-1", NProto::AGENT_STATE_WARNING);
        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        // migration in progress
        {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());

            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, msg.MigrationsSize());
        }

        ui32 cleanDevices = 0;
        TAutoPtr<IEventHandle> reallocRequest;

        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvDiskRegistryPrivate::EvSecureEraseResponse: {
                        auto* msg = event->Get<TEvDiskRegistryPrivate::TEvSecureEraseResponse>();
                        cleanDevices += msg->CleanDevices;
                        break;
                    }

                    case TEvVolume::EvReallocateDiskResponse: {
                        reallocRequest = event.Release();
                        return TTestActorRuntime::EEventAction::DROP;
                    }
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        // cancel migration
        diskRegistry.ChangeAgentState("agent-1", NProto::AGENT_STATE_ONLINE);
        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());

            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(0, msg.MigrationsSize());
        }

        UNIT_ASSERT_VALUES_EQUAL(0, cleanDevices);

        // wait for realloc
        {
            TDispatchOptions options;
            options.CustomFinalCondition = [&] {
                return reallocRequest != nullptr;
            };
            runtime->DispatchEvents(options);
        }

        // can't allocate new disk
        {
            diskRegistry.SendAllocateDiskRequest("disk-2", 20_GB);
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, response->GetStatus());
        }

        runtime->Send(reallocRequest.Release());
        runtime->AdvanceCurrentTime(TDuration::Seconds(20));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        // wait for secure erase
        {
            TDispatchOptions options;
            options.CustomFinalCondition = [&] {
                return cleanDevices == 2;
            };
            runtime->DispatchEvents(options);
        }

        UNIT_ASSERT_VALUES_EQUAL(2, cleanDevices);

        diskRegistry.AllocateDisk("disk-2", 20_GB);
    }

    Y_UNIT_TEST(ShouldFinishMigrationForMirroredDisk)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB),
        });

        const auto agent2 = CreateAgentConfig("agent-2", {
            Device("dev-1", "uuid-3", "rack-2", 10_GB),
            Device("dev-2", "uuid-4", "rack-2", 10_GB),
        });

        const auto agent3 = CreateAgentConfig("agent-3", {
            Device("dev-1", "uuid-5", "rack-3", 10_GB),
            Device("dev-2", "uuid-6", "rack-3", 10_GB),
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent1, agent2, agent3 })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(
            CreateRegistryConfig(0, {agent1, agent2, agent3 }));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 3);
        WaitForAgents(*runtime, 3);
        WaitForSecureErase(*runtime, {agent1, agent2, agent3 });

        {
            auto response = diskRegistry.AllocateDisk(
                "disk-1",
                20_GB,
                DefaultLogicalBlockSize,
                "", // placementGroupId
                "", // cloudId
                "", // folderId
                1   // replicaCount
            );

            auto& msg = response->Record;
            SortBy(*msg.MutableDevices(), TByUUID());

            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-1",
                msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-2",
                msg.GetDevices(1).GetDeviceUUID());

            UNIT_ASSERT_VALUES_EQUAL(1, msg.ReplicasSize());

            SortBy(*msg.MutableReplicas(0)->MutableDevices(), TByUUID());

            UNIT_ASSERT_VALUES_EQUAL(2, msg.GetReplicas(0).DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-3",
                msg.GetReplicas(0).GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-4",
                msg.GetReplicas(0).GetDevices(1).GetDeviceUUID());
        }

        diskRegistry.ChangeAgentState(
            "agent-1",
            NProto::EAgentState::AGENT_STATE_UNAVAILABLE);

        {
            auto response = diskRegistry.AllocateDisk(
                "disk-1",
                20_GB,
                DefaultLogicalBlockSize,
                "", // placementGroupId
                "", // cloudId
                "", // folderId
                1   // replicaCount
            );

            auto& msg = response->Record;
            SortBy(*msg.MutableDevices(), TByUUID());

            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-5",
                msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-6",
                msg.GetDevices(1).GetDeviceUUID());

            UNIT_ASSERT_VALUES_EQUAL(2, msg.DeviceReplacementUUIDsSize());

            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-5",
                msg.GetDeviceReplacementUUIDs(0));
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-6",
                msg.GetDeviceReplacementUUIDs(1));

            UNIT_ASSERT_VALUES_EQUAL(1, msg.ReplicasSize());

            SortBy(*msg.MutableReplicas(0)->MutableDevices(), TByUUID());

            UNIT_ASSERT_VALUES_EQUAL(2, msg.GetReplicas(0).DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-3",
                msg.GetReplicas(0).GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-4",
                msg.GetReplicas(0).GetDevices(1).GetDeviceUUID());
        }

        diskRegistry.FinishMigration("disk-1", "doesnt-matter", "uuid-5");

        {
            auto response = diskRegistry.AllocateDisk(
                "disk-1",
                20_GB,
                DefaultLogicalBlockSize,
                "", // placementGroupId
                "", // cloudId
                "", // folderId
                1   // replicaCount
            );

            auto& msg = response->Record;
            SortBy(*msg.MutableDevices(), TByUUID());

            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-5",
                msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-6",
                msg.GetDevices(1).GetDeviceUUID());

            UNIT_ASSERT_VALUES_EQUAL(1, msg.DeviceReplacementUUIDsSize());

            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-6",
                msg.GetDeviceReplacementUUIDs(0));

            UNIT_ASSERT_VALUES_EQUAL(1, msg.ReplicasSize());

            SortBy(*msg.MutableReplicas(0)->MutableDevices(), TByUUID());

            UNIT_ASSERT_VALUES_EQUAL(2, msg.GetReplicas(0).DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-3",
                msg.GetReplicas(0).GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-4",
                msg.GetReplicas(0).GetDevices(1).GetDeviceUUID());
        }
    }
}

}   // namespace NCloud::NBlockStore::NStorage
