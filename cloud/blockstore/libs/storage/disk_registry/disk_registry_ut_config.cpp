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
    Y_UNIT_TEST(ShouldWaitReady)
    {
        auto runtime = TTestRuntimeBuilder().Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();
    }

    Y_UNIT_TEST(ShouldDescribeConfig)
    {
        const auto agent = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB)
        });

        auto runtime = TTestRuntimeBuilder().Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig([&] {
            auto config = CreateRegistryConfig(0, {agent});

            auto* d = config.AddDeviceOverrides();
            d->SetDevice("foo");
            d->SetDiskId("bar");
            d->SetBlocksCount(42);

            auto* ssd = config.AddDevicePoolConfigs();
            ssd->SetName("ssd");
            ssd->SetKind(NProto::DEVICE_POOL_KIND_LOCAL);
            ssd->SetAllocationUnit(368_GB);

            return config;
        }());

        const auto response = diskRegistry.DescribeConfig();
        const auto& current = response->Record.GetConfig();

        UNIT_ASSERT_VALUES_EQUAL(1, current.GetVersion());
        UNIT_ASSERT_VALUES_EQUAL(1, current.KnownAgentsSize());
        UNIT_ASSERT_VALUES_EQUAL(1, current.DeviceOverridesSize());
        UNIT_ASSERT_VALUES_EQUAL(1, current.DevicePoolConfigsSize());

        const auto& knownAgent = current.GetKnownAgents(0);
        UNIT_ASSERT_VALUES_EQUAL(agent.GetAgentId(), knownAgent.GetAgentId());
        UNIT_ASSERT_VALUES_EQUAL(agent.DevicesSize(), knownAgent.DevicesSize());

        const auto& d = current.GetDeviceOverrides(0);
        UNIT_ASSERT_VALUES_EQUAL("foo", d.GetDevice());
        UNIT_ASSERT_VALUES_EQUAL("bar", d.GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL(42, d.GetBlocksCount());

        const auto& ssd = current.GetDevicePoolConfigs(0);
        UNIT_ASSERT_VALUES_EQUAL("ssd", ssd.GetName());
        UNIT_ASSERT_EQUAL(NProto::DEVICE_POOL_KIND_LOCAL, ssd.GetKind());
        UNIT_ASSERT_VALUES_EQUAL(368_GB, ssd.GetAllocationUnit());
    }

    Y_UNIT_TEST(ShouldRejectConfigWithBadVersion)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB)
        });

        const auto agent2 = CreateAgentConfig("agent-2", {
            Device("dev-1", "uuid-3", "rack-1", 10_GB),
            Device("dev-2", "uuid-4", "rack-1", 10_GB)
        });

        auto runtime = TTestRuntimeBuilder().Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, {agent1}));

        {
            diskRegistry.SendUpdateConfigRequest(
                CreateRegistryConfig(0, {agent1, agent2}));
            auto response = diskRegistry.RecvUpdateConfigResponse();
            UNIT_ASSERT_VALUES_EQUAL(
                response->GetStatus(),
                E_CONFIG_VERSION_MISMATCH
            );
        }

        {
            diskRegistry.SendUpdateConfigRequest(
                CreateRegistryConfig(10, {agent1, agent2}));
            auto response = diskRegistry.RecvUpdateConfigResponse();
            UNIT_ASSERT_VALUES_EQUAL(
                response->GetStatus(),
                E_CONFIG_VERSION_MISMATCH
            );
        }

        const auto response = diskRegistry.DescribeConfig();
        const auto& current = response->Record.GetConfig();

        UNIT_ASSERT_VALUES_EQUAL(current.GetVersion(), 1);
        UNIT_ASSERT_VALUES_EQUAL(current.KnownAgentsSize(), 1);
    }

    Y_UNIT_TEST(ShouldIgnoreConfigVersion)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB)
        });

        const auto agent2 = CreateAgentConfig("agent-2", {
            Device("dev-1", "uuid-3", "rack-1", 10_GB),
            Device("dev-2", "uuid-4", "rack-1", 10_GB)
        });

        auto runtime = TTestRuntimeBuilder().Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        const auto config1 = CreateRegistryConfig(0, {agent1});
        const auto config2 = CreateRegistryConfig(10, {agent1, agent2});

        diskRegistry.UpdateConfig(config1);
        diskRegistry.UpdateConfig(config2, true);
    }

    Y_UNIT_TEST(ShouldRegisterAgent)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("test", "uuid-1", "rack-1", 4_MB)
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent1 })
            .Build();

        bool registerAgentSeen = false;

        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvDiskRegistry::EvRegisterAgentRequest:
                        {
                            registerAgentSeen = true;

                            auto& msg = *event->Get<TEvDiskRegistry::TEvRegisterAgentRequest>();
                            UNIT_ASSERT_VALUES_EQUAL(msg.Record.GetAgentConfig().DevicesSize(), 1);

                            auto& disk = msg.Record.GetAgentConfig().GetDevices()[0];

                            UNIT_ASSERT_VALUES_EQUAL("test", disk.GetDeviceName());
                            UNIT_ASSERT_VALUES_EQUAL(
                                4_MB / DefaultBlockSize,
                                disk.GetBlocksCount());

                            UNIT_ASSERT_VALUES_EQUAL(
                                DefaultBlockSize,
                                disk.GetBlockSize());

                            UNIT_ASSERT(!disk.GetDeviceUUID().empty());
                            UNIT_ASSERT(!disk.GetTransportId().empty());
                            UNIT_ASSERT(!disk.GetRdmaEndpoint().GetHost().empty());
                        }
                    break;
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(
            CreateRegistryConfig(0, {agent1}));

        RegisterAgents(*runtime, 1);
        WaitForAgents(*runtime, 1);

        UNIT_ASSERT(registerAgentSeen);
    }

    Y_UNIT_TEST(ShouldUpdateAgentStats)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB)
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent1 })
            .Build();

        runtime->SetRegistrationObserverFunc(
            [] (auto& runtime, const auto& parentId, const auto& actorId)
        {
            Y_UNUSED(parentId);
            runtime.EnableScheduleForActor(actorId);
        });

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, { agent1 }));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 1);
        WaitForAgents(*runtime, 1);

        NProto::TAgentStats agentStats;

        agentStats.SetNodeId(runtime->GetNodeId(0));
        agentStats.SetInitErrorsCount(42);

        auto& device1 = *agentStats.AddDeviceStats();
        device1.SetDeviceUUID("uuid-1");
        device1.SetBytesRead(10000);
        device1.SetNumReadOps(100);
        device1.SetBytesWritten(3000000);
        device1.SetNumWriteOps(200);

        auto& device2 = *agentStats.AddDeviceStats();
        device2.SetDeviceUUID("uuid-2");
        device2.SetBytesRead(20000);
        device2.SetNumReadOps(200);
        device2.SetBytesWritten(6000000);
        device2.SetNumWriteOps(300);

        diskRegistry.UpdateAgentStats(std::move(agentStats));

        // wait for publish stats
        runtime->AdvanceCurrentTime(TDuration::Seconds(15));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        auto counters = runtime->GetAppData(0).Counters
            ->GetSubgroup("counters", "blockstore")
            ->GetSubgroup("component", "disk_registry");

        auto agentCounters = counters
            ->GetSubgroup("agent", "agent-1");

        auto totalInitErrors = agentCounters->GetCounter("Errors/Init");

        UNIT_ASSERT_VALUES_EQUAL(42, totalInitErrors->Val());

        auto totalReadCount = agentCounters->GetCounter("ReadCount");
        auto totalReadBytes = agentCounters->GetCounter("ReadBytes");
        auto totalWriteCount = agentCounters->GetCounter("WriteCount");
        auto totalWriteBytes = agentCounters->GetCounter("WriteBytes");

        UNIT_ASSERT_VALUES_EQUAL(300, totalReadCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(30000, totalReadBytes->Val());
        UNIT_ASSERT_VALUES_EQUAL(500, totalWriteCount->Val());
        UNIT_ASSERT_VALUES_EQUAL(9000000, totalWriteBytes->Val());

        {
            auto device = agentCounters->GetSubgroup("device", "uuid-1");

            UNIT_ASSERT_VALUES_EQUAL(100, device->GetCounter("ReadCount")->Val());
            UNIT_ASSERT_VALUES_EQUAL(10000, device->GetCounter("ReadBytes")->Val());
            UNIT_ASSERT_VALUES_EQUAL(200, device->GetCounter("WriteCount")->Val());
            UNIT_ASSERT_VALUES_EQUAL(3000000, device->GetCounter("WriteBytes")->Val());
        }

        {
            auto device = agentCounters->GetSubgroup("device", "uuid-2");

            UNIT_ASSERT_VALUES_EQUAL(200, device->GetCounter("ReadCount")->Val());
            UNIT_ASSERT_VALUES_EQUAL(20000, device->GetCounter("ReadBytes")->Val());
            UNIT_ASSERT_VALUES_EQUAL(300, device->GetCounter("WriteCount")->Val());
            UNIT_ASSERT_VALUES_EQUAL(6000000, device->GetCounter("WriteBytes")->Val());
        }
    }

    Y_UNIT_TEST(ShouldRejectDestructiveUpdateConfig)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB)
        });

        const auto agent2 = CreateAgentConfig("agent-2", {
            Device("dev-1", "uuid-3", "rack-1", 10_GB),
            Device("dev-2", "uuid-4", "rack-1", 10_GB)
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent1, agent2 })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        const auto config1 = CreateRegistryConfig(0, {agent1, agent2});
        const auto config2 = CreateRegistryConfig(1, {agent1});

        diskRegistry.UpdateConfig(config1);
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 2);
        WaitForSecureErase(*runtime, {agent1, agent2});

        diskRegistry.AllocateDisk("disk-1", 40_GB, DefaultLogicalBlockSize);

        {
            diskRegistry.SendUpdateConfigRequest(config2);
            auto response = diskRegistry.RecvUpdateConfigResponse();

            UNIT_ASSERT_VALUES_EQUAL(E_INVALID_STATE, response->GetStatus());

            UNIT_ASSERT_VALUES_EQUAL(1, response->Record.AffectedDisksSize());
            UNIT_ASSERT_VALUES_EQUAL("disk-1", response->Record.GetAffectedDisks(0));
        }
    }

    Y_UNIT_TEST(ShouldBackup)
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

        RegisterAndWaitForAgents(*runtime, agents);

        diskRegistry.CreatePlacementGroup("pg1");
        diskRegistry.CreatePlacementGroup("pg2");
        diskRegistry.CreatePlacementGroup("pg3");

        auto allocateDisk = [&] (TString id, TString groupName) {
            auto response = diskRegistry.AllocateDisk(id, 10_GB, 4_KB, groupName);

            return TVector<NProto::TDeviceConfig>(
                response->Record.GetDevices().begin(),
                response->Record.GetDevices().end()
            );
        };

        auto disk1 = allocateDisk("disk-1", "pg1");
        auto disk2 = allocateDisk("disk-2", "pg1");
        auto disk3 = allocateDisk("disk-3", "pg1");
        auto disk4 = allocateDisk("disk-4", "pg2");

        auto makeBackup = [&] (bool localDB) {
            NProto::TDiskRegistryStateBackup backup;
            auto response = diskRegistry.BackupDiskRegistryState(localDB);
            backup.Swap(response->Record.MutableBackup());

            return backup;
        };

        auto validate = [](auto& backup, const TString& name) {
            auto& config = *backup.MutableConfig();

            SortBy(*config.MutableKnownAgents(), [] (auto& x) { return x.GetAgentId(); });
            SortBy(*backup.MutableAgents(), [] (auto& x) { return x.GetAgentId(); });
            SortBy(*backup.MutablePlacementGroups(), [] (auto& x) { return x.GetGroupId(); });
            SortBy(*backup.MutableDisks(), [] (auto& x) { return x.GetDiskId(); });

            const auto& agents = backup.GetAgents();
            UNIT_ASSERT_VALUES_EQUAL_C(3, agents.size(), name);
            UNIT_ASSERT_VALUES_EQUAL_C("agent-1", agents[0].GetAgentId(), name);
            UNIT_ASSERT_VALUES_EQUAL_C("agent-2", agents[1].GetAgentId(), name);
            UNIT_ASSERT_VALUES_EQUAL_C("agent-3", agents[2].GetAgentId(), name);

            const auto& disks = backup.GetDisks();
            UNIT_ASSERT_VALUES_EQUAL_C(4, disks.size(), name);
            UNIT_ASSERT_VALUES_EQUAL_C("disk-1", disks[0].GetDiskId(), name);
            UNIT_ASSERT_VALUES_EQUAL_C("disk-2", disks[1].GetDiskId(), name);
            UNIT_ASSERT_VALUES_EQUAL_C("disk-3", disks[2].GetDiskId(), name);
            UNIT_ASSERT_VALUES_EQUAL_C("disk-4", disks[3].GetDiskId(), name);

            const auto& pg = backup.GetPlacementGroups();
            UNIT_ASSERT_VALUES_EQUAL_C(3, pg.size(), name);
            UNIT_ASSERT_VALUES_EQUAL_C("pg1", pg[0].GetGroupId(), name);
            UNIT_ASSERT_VALUES_EQUAL_C("pg2", pg[1].GetGroupId(), name);
            UNIT_ASSERT_VALUES_EQUAL_C("pg3", pg[2].GetGroupId(), name);

            UNIT_ASSERT_VALUES_EQUAL_C(3, pg[0].DisksSize(), name);
            UNIT_ASSERT_VALUES_EQUAL_C(1, pg[1].DisksSize(), name);
            UNIT_ASSERT_VALUES_EQUAL_C(0, pg[2].DisksSize(), name);

            UNIT_ASSERT_VALUES_EQUAL_C("disk-1", pg[0].GetDisks(0).GetDiskId(), name);
            UNIT_ASSERT_VALUES_EQUAL_C("disk-2", pg[0].GetDisks(1).GetDiskId(), name);
            UNIT_ASSERT_VALUES_EQUAL_C("disk-3", pg[0].GetDisks(2).GetDiskId(), name);

            UNIT_ASSERT_VALUES_EQUAL_C("disk-4", pg[1].GetDisks(0).GetDiskId(), name);

            UNIT_ASSERT_VALUES_EQUAL_C(true, config.GetDiskAllocationAllowed(), name);

            const auto& knownAgents = config.GetKnownAgents();
            UNIT_ASSERT_VALUES_EQUAL_C(3, knownAgents.size(), name);
            UNIT_ASSERT_VALUES_EQUAL_C("agent-1", knownAgents[0].GetAgentId(), name);
            UNIT_ASSERT_VALUES_EQUAL_C("agent-2", knownAgents[1].GetAgentId(), name);
            UNIT_ASSERT_VALUES_EQUAL_C("agent-3", knownAgents[2].GetAgentId(), name);
        };

        auto backupState = makeBackup(false);
        auto backupDB = makeBackup(true);

        validate(backupState, "state backup");
        validate(backupDB, "local DB backup");
    }

    Y_UNIT_TEST(ShouldRestore)
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

        RegisterAndWaitForAgents(*runtime, agents);

        diskRegistry.CreatePlacementGroup("pg1");
        diskRegistry.CreatePlacementGroup("pg2");
        diskRegistry.CreatePlacementGroup("pg3");

        auto allocateDisk = [&] (TString id, TString groupName) {
            auto response = diskRegistry.AllocateDisk(id, 10_GB, 4_KB, groupName);

            return TVector<NProto::TDeviceConfig>(
                response->Record.GetDevices().begin(),
                response->Record.GetDevices().end()
            );
        };

        auto disk1 = allocateDisk("disk-1", "pg1");
        auto disk2 = allocateDisk("disk-2", "pg1");
        auto disk3 = allocateDisk("disk-3", "pg1");
        auto disk4 = allocateDisk("disk-4", "pg2");

        TString backupJson = R"--(
            {
              "Backup": {
                "Agents": [
                  {
                    "NodeId": 1,
                    "Devices": [
                      {
                        "DeviceName": "disk-1.bin",
                        "DeviceUUID": "1",
                        "BlockSize": 4096,
                        "BlocksCount": "262144",
                        "NodeId": 1,
                        "Rack": "rack-2",
                        "StateTs": "1657793753613714",
                        "AgentId": "agent-1",
                        "UnadjustedBlockCount": "262144",
                        "PoolName": "local:ssd"
                      },
                      {
                        "DeviceName": "disk-2.bin",
                        "DeviceUUID": "2",
                        "BlockSize": 4096,
                        "BlocksCount": "262144",
                        "NodeId": 1,
                        "Rack": "rack-2",
                        "StateTs": "1657793753613714",
                        "AgentId": "agent-1",
                        "UnadjustedBlockCount": "262144"
                      }
                    ],
                    "AgentId": "agent-1",
                    "StateTs": "1657793753613714",
                    "DedicatedDiskAgent": true
                  },
                  {
                    "NodeId": 2,
                    "Devices": [
                      {
                        "DeviceName": "disk-3.bin",
                        "DeviceUUID": "3",
                        "BlockSize": 4096,
                        "BlocksCount": "262144",
                        "NodeId": 2,
                        "Rack": "rack-1",
                        "StateTs": "1657793750045586",
                        "AgentId": "agent-2",
                        "UnadjustedBlockCount": "262144",
                        "PoolName": "local:ssd"
                      },
                      {
                        "DeviceName": "disk-4.bin",
                        "DeviceUUID": "4",
                        "BlockSize": 4096,
                        "BlocksCount": "262144",
                        "NodeId": 2,
                        "Rack": "rack-1",
                        "StateTs": "1657793750045586",
                        "AgentId": "agent-2",
                        "UnadjustedBlockCount": "262144"
                      }
                    ],
                    "AgentId": "agent-2",
                    "StateTs": "1657793750045586",
                    "DedicatedDiskAgent": true
                  }
                ],
                "Config": {
                  "Version": 1,
                  "KnownAgents": [
                    {
                      "Devices": [
                        {
                          "DeviceUUID": "3"
                        },
                        {
                          "DeviceUUID": "4"
                        }
                      ],
                      "AgentId": "agent-2"
                    },
                    {
                      "Devices": [
                        {
                          "DeviceUUID": "1"
                        },
                        {
                          "DeviceUUID": "2"
                        }
                      ],
                      "AgentId": "agent-1"
                    }
                  ],
                  "DiskAllocationAllowed": true
                }
              }
            }
            )--";

        diskRegistry.RestoreDiskRegistryState(backupJson);

        NProto::TDiskRegistryStateBackup backup;
        auto response = diskRegistry.BackupDiskRegistryState(true);
        backup.Swap(response->Record.MutableBackup());

        auto& config = *backup.MutableConfig();

        SortBy(*config.MutableKnownAgents(), [] (auto& x) { return x.GetAgentId(); });
        SortBy(*backup.MutableAgents(), [] (auto& x) { return x.GetAgentId(); });
        SortBy(*backup.MutablePlacementGroups(), [] (auto& x) { return x.GetGroupId(); });
        SortBy(*backup.MutableDisks(), [] (auto& x) { return x.GetDiskId(); });

        const auto& backupAgents = backup.GetAgents();
        UNIT_ASSERT_VALUES_EQUAL(2, backupAgents.size());
        UNIT_ASSERT_VALUES_EQUAL("agent-1", backupAgents[0].GetAgentId());
        UNIT_ASSERT_VALUES_EQUAL("agent-2", backupAgents[1].GetAgentId());

        const auto& backupPlacementGroups = backup.GetPlacementGroups();
        UNIT_ASSERT_VALUES_EQUAL(0, backupPlacementGroups.size());

        const auto& backupDisks = backup.GetDisks();
        UNIT_ASSERT_VALUES_EQUAL(0, backupDisks.size());

        const auto& backupKnownAgents = config.GetKnownAgents();
        UNIT_ASSERT_VALUES_EQUAL(2, backupKnownAgents.size());
        UNIT_ASSERT_VALUES_EQUAL("agent-1", backupKnownAgents[0].GetAgentId());
        UNIT_ASSERT_VALUES_EQUAL("agent-2", backupKnownAgents[1].GetAgentId());
    }
}

}   // namespace NCloud::NBlockStore::NStorage
