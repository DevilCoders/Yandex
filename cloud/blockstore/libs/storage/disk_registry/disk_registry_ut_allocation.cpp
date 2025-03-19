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
    Y_UNIT_TEST(ShouldAllocateDisk)
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

        diskRegistry.UpdateConfig(
            CreateRegistryConfig(0, {agent1, agent2}));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 2);
        WaitForAgents(*runtime, 2);
        WaitForSecureErase(*runtime, {agent1, agent2});

        {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);

            auto& msg = response->Record;
            SortBy(*msg.MutableDevices(), TByUUID());

            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL("dev-1", msg.GetDevices(0).GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-2", msg.GetDevices(1).GetDeviceName());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetDevices(0).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetDevices(1).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetDevices(0).GetBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetDevices(1).GetBlocksCount());

            UNIT_ASSERT(msg.GetDevices(0).GetNodeId() != 0);

            // from same node
            UNIT_ASSERT_VALUES_EQUAL(
                msg.GetDevices(0).GetNodeId(),
                msg.GetDevices(1).GetNodeId());
        }

        {
            auto response = diskRegistry.AllocateDisk("disk-1", 30_GB);

            auto& msg = response->Record;
            SortBy(*msg.MutableDevices(), TByUUID());

            UNIT_ASSERT_VALUES_EQUAL(3, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL("dev-1", msg.GetDevices(0).GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-2", msg.GetDevices(1).GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-1", msg.GetDevices(2).GetDeviceName());

            UNIT_ASSERT(msg.GetDevices(0).GetNodeId() != 0);

            // from same node
            UNIT_ASSERT_VALUES_EQUAL(
                msg.GetDevices(0).GetNodeId(),
                msg.GetDevices(1).GetNodeId());

            // from different nodes
            UNIT_ASSERT_VALUES_UNEQUAL(
                msg.GetDevices(1).GetNodeId(),
                msg.GetDevices(2).GetNodeId());
        }
    }

    Y_UNIT_TEST(ShouldTakeDeviceOverridesIntoAccount)
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

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, {agent1, agent2}));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 2);
        WaitForAgents(*runtime, 2);
        WaitForSecureErase(*runtime, {agent1, agent2});

        TString uuid0;
        TString uuid1;

        {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);

            auto& msg = response->Record;
            SortBy(*msg.MutableDevices(), TByUUID());

            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL("dev-1", msg.GetDevices(0).GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-2", msg.GetDevices(1).GetDeviceName());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetDevices(0).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetDevices(1).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetDevices(0).GetBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetDevices(1).GetBlocksCount());

            UNIT_ASSERT(msg.GetDevices(0).GetNodeId() != 0);

            // from same node
            UNIT_ASSERT_VALUES_EQUAL(
                msg.GetDevices(0).GetNodeId(),
                msg.GetDevices(1).GetNodeId());

            uuid0 = msg.GetDevices(0).GetDeviceUUID();
            uuid1 = msg.GetDevices(1).GetDeviceUUID();
        }

        TVector<NProto::TDeviceOverride> deviceOverrides;
        deviceOverrides.emplace_back();
        deviceOverrides.back().SetDiskId("disk-1");
        deviceOverrides.back().SetDevice(uuid0);
        deviceOverrides.back().SetBlocksCount(9_GB / DefaultBlockSize);
        deviceOverrides.emplace_back();
        deviceOverrides.back().SetDiskId("disk-1");
        deviceOverrides.back().SetDevice(uuid1);
        deviceOverrides.back().SetBlocksCount(9_GB / DefaultBlockSize);

        diskRegistry.UpdateConfig(
            CreateRegistryConfig(
                1,
                {agent1, agent2},
                deviceOverrides
            )
        );

        auto testRealloc = [&] {
            auto response = diskRegistry.AllocateDisk("disk-1", 28_GB);

            auto& msg = response->Record;
            SortBy(*msg.MutableDevices(), TByUUID());

            UNIT_ASSERT_VALUES_EQUAL(3, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL("dev-1", msg.GetDevices(0).GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-2", msg.GetDevices(1).GetDeviceName());
            UNIT_ASSERT_VALUES_EQUAL("dev-1", msg.GetDevices(2).GetDeviceName());

            UNIT_ASSERT(msg.GetDevices(0).GetNodeId() != 0);

            // from same node
            UNIT_ASSERT_VALUES_EQUAL(
                msg.GetDevices(0).GetNodeId(),
                msg.GetDevices(1).GetNodeId());

            // from different nodes
            UNIT_ASSERT_VALUES_UNEQUAL(
                msg.GetDevices(1).GetNodeId(),
                msg.GetDevices(2).GetNodeId());

            // overridden device sizes should take effect
            UNIT_ASSERT_VALUES_EQUAL(
                9_GB / DefaultLogicalBlockSize,
                msg.GetDevices(0).GetBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(
                9_GB / DefaultLogicalBlockSize,
                msg.GetDevices(1).GetBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetDevices(2).GetBlocksCount());
        };

        testRealloc();

        diskRegistry.RebootTablet();
        diskRegistry.WaitReady();

        testRealloc();
    }

    Y_UNIT_TEST(ShouldAllocateWithCorrectBlockSize)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1", "rack-1", 10_GB, 4_KB),
                Device("dev-2", "uuid-2", "rack-1", 10_GB, 1_KB),
                Device("dev-3", "uuid-3", "rack-1", 10_GB, 4_KB)
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-4", "uuid-4", "rack-1", 10_GB, 8_KB),
                Device("dev-5", "uuid-5", "rack-1", 10_GB, 1_KB),
                Device("dev-6", "uuid-6", "rack-1", 10_GB, 8_KB)
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
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB, 1_KB);

            auto& msg = response->Record;
            SortBy(*msg.MutableDevices(), TByUUID());

            UNIT_ASSERT_VALUES_EQUAL(msg.DevicesSize(), 2);
            UNIT_ASSERT_VALUES_EQUAL("uuid-2", msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-5", msg.GetDevices(1).GetDeviceUUID());
        }

        {
            diskRegistry.SendAllocateDiskRequest("disk-2", 10_GB, 1_KB);
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, response->GetStatus());
        }

        {
            auto response = diskRegistry.AllocateDisk("disk-3", 40_GB, 8_KB);

            auto& msg = response->Record;
            SortBy(*msg.MutableDevices(), TByUUID());

            UNIT_ASSERT_VALUES_EQUAL(msg.DevicesSize(), 4);
            UNIT_ASSERT_VALUES_EQUAL("uuid-1", msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-3", msg.GetDevices(1).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-4", msg.GetDevices(2).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-6", msg.GetDevices(3).GetDeviceUUID());
        }
    }

    Y_UNIT_TEST(ShouldDescribeDisk)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB),
            Device("dev-3", "uuid-3", "rack-1", 10_GB)
        });

        const auto agent2 = CreateAgentConfig("agent-2", {
            Device("dev-4", "uuid-4", "rack-1", 10_GB),
            Device("dev-5", "uuid-5", "rack-1", 10_GB)
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent1, agent2 })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(
            CreateRegistryConfig(0, {agent1, agent2}));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 2);
        WaitForAgents(*runtime, 2);
        WaitForSecureErase(*runtime, {agent1, agent2});

        diskRegistry.AllocateDisk("disk-1", 20_GB);
        diskRegistry.AllocateDisk("disk-2", 30_GB);

        {
            auto response = diskRegistry.DescribeDisk("disk-1");
            auto& msg = response->Record;
            SortBy(*msg.MutableDevices(), TByUUID());

            UNIT_ASSERT_VALUES_EQUAL(msg.DevicesSize(), 2);

            UNIT_ASSERT_VALUES_EQUAL(msg.GetDevices(0).GetDeviceName(), "dev-1");
            UNIT_ASSERT_VALUES_EQUAL(msg.GetDevices(1).GetDeviceName(), "dev-2");
        }

        {
            auto response = diskRegistry.DescribeDisk("disk-2");
            auto& msg = response->Record;
            SortBy(*msg.MutableDevices(), TByUUID());

            UNIT_ASSERT_VALUES_EQUAL(msg.DevicesSize(), 3);

            UNIT_ASSERT_VALUES_EQUAL(msg.GetDevices(0).GetDeviceName(), "dev-3");
            UNIT_ASSERT_VALUES_EQUAL(msg.GetDevices(1).GetDeviceName(), "dev-4");
            UNIT_ASSERT_VALUES_EQUAL(msg.GetDevices(2).GetDeviceName(), "dev-5");
        }
    }

    Y_UNIT_TEST(ShouldSupportPlacementGroups)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB)
        });

        const auto agent2 = CreateAgentConfig("agent-2", {
            Device("dev-1", "uuid-3", "rack-2", 10_GB),
            Device("dev-2", "uuid-4", "rack-2", 10_GB)
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent1, agent2 })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(
            CreateRegistryConfig(0, {agent1, agent2}));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 2);
        WaitForAgents(*runtime, 2);
        WaitForSecureErase(*runtime, {agent1, agent2});

        diskRegistry.AllocateDisk("disk-1", 10_GB);
        diskRegistry.AllocateDisk("disk-2", 10_GB);
        diskRegistry.AllocateDisk("disk-3", 10_GB);
        diskRegistry.AllocateDisk("disk-4", 10_GB);

        diskRegistry.CreatePlacementGroup("group-1");
        diskRegistry.AlterPlacementGroupMembership(
            "group-1",
            1,
            TVector<TString>{"disk-1"},
            TVector<TString>()
        );

        diskRegistry.SendAlterPlacementGroupMembershipRequest(
            "group-1",
            2,
            TVector<TString>{"disk-3"},
            TVector<TString>()
        );

        {
            auto response = diskRegistry.RecvAlterPlacementGroupMembershipResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_PRECONDITION_FAILED, response->GetStatus());
            UNIT_ASSERT_VALUES_EQUAL(1, response->Record.DisksImpossibleToAddSize());
            UNIT_ASSERT_VALUES_EQUAL("disk-3", response->Record.GetDisksImpossibleToAdd(0));
        }

        diskRegistry.SendAlterPlacementGroupMembershipRequest(
            "group-1",
            2,
            TVector<TString>{"disk-2"},
            TVector<TString>()
        );

        {
            auto response = diskRegistry.RecvAlterPlacementGroupMembershipResponse();
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());
        }

        {
            auto response = diskRegistry.ListPlacementGroups();
            UNIT_ASSERT_VALUES_EQUAL(1, response->Record.GroupIdsSize());
            UNIT_ASSERT_VALUES_EQUAL("group-1", response->Record.GetGroupIds(0));
        }

        {
            auto response = diskRegistry.DescribePlacementGroup("group-1");
            const auto& group = response->Record.GetGroup();
            UNIT_ASSERT_VALUES_EQUAL("group-1", group.GetGroupId());
            UNIT_ASSERT_VALUES_EQUAL(3, group.GetConfigVersion());
            UNIT_ASSERT_VALUES_EQUAL(2, group.DiskIdsSize());
            UNIT_ASSERT_VALUES_EQUAL("disk-1", group.GetDiskIds(0));
            UNIT_ASSERT_VALUES_EQUAL("disk-2", group.GetDiskIds(1));
        }

        diskRegistry.DestroyPlacementGroup("group-1");

        {
            auto response = diskRegistry.ListPlacementGroups();
            UNIT_ASSERT_VALUES_EQUAL(0, response->Record.GroupIdsSize());
        }
    }

    Y_UNIT_TEST(ShouldAllocateInPlacementGroup)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB)
        });

        const auto agent2 = CreateAgentConfig("agent-2", {
            Device("dev-1", "uuid-3", "rack-2", 10_GB),
            Device("dev-2", "uuid-4", "rack-2", 10_GB)
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent1, agent2 })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(
            CreateRegistryConfig(0, {agent1, agent2}));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 2);
        WaitForAgents(*runtime, 2);
        WaitForSecureErase(*runtime, {agent1, agent2});

        diskRegistry.CreatePlacementGroup("group-1");

        diskRegistry.AllocateDisk(
            "disk-1",
            10_GB,
            DefaultLogicalBlockSize,
            "group-1"
        );

        diskRegistry.AllocateDisk(
            "disk-2",
            10_GB,
            DefaultLogicalBlockSize,
            "group-1"
        );

        diskRegistry.SendAllocateDiskRequest(
            "disk-3",
            10_GB,
            DefaultLogicalBlockSize,
            "group-1"
        );

        {
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, response->GetStatus());
        }

        diskRegistry.AllocateDisk("disk-3", 10_GB);
        diskRegistry.AllocateDisk("disk-4", 10_GB);

        diskRegistry.SendAllocateDiskRequest("disk-5", 10_GB);

        {
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, response->GetStatus());
        }

        {
            auto response = diskRegistry.DescribePlacementGroup("group-1");
            const auto& group = response->Record.GetGroup();
            UNIT_ASSERT_VALUES_EQUAL("group-1", group.GetGroupId());
            UNIT_ASSERT_VALUES_EQUAL(3, group.GetConfigVersion());
            UNIT_ASSERT_VALUES_EQUAL(2, group.DiskIdsSize());
            UNIT_ASSERT_VALUES_EQUAL("disk-1", group.GetDiskIds(0));
            UNIT_ASSERT_VALUES_EQUAL("disk-2", group.GetDiskIds(1));
        }

        // TODO: test placement group persistence (call RebootTablet during test)
    }

    Y_UNIT_TEST(ShouldNotAllocateMoreThanMaxDisksInPlacementGroup)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1", "rack-1", 10_GB),
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-1", "uuid-2", "rack-2", 10_GB),
            }),
            CreateAgentConfig("agent-3", {
                Device("dev-1", "uuid-3", "rack-3", 10_GB),
            }),
            CreateAgentConfig("agent-4", {
                Device("dev-1", "uuid-4", "rack-4", 10_GB),
            })
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents(agents)
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, agents));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 4);
        WaitForAgents(*runtime, 4);
        WaitForSecureErase(*runtime, agents);

        diskRegistry.CreatePlacementGroup("group-1");

        diskRegistry.AllocateDisk(
            "disk-1",
            10_GB,
            DefaultLogicalBlockSize,
            "group-1"
        );

        diskRegistry.AllocateDisk(
            "disk-2",
            10_GB,
            DefaultLogicalBlockSize,
            "group-1"
        );

        diskRegistry.AllocateDisk(
            "disk-3",
            10_GB,
            DefaultLogicalBlockSize,
            "group-1"
        );

        TVector<TString> destroyedDiskIds;
        TAutoPtr<IEventHandle> destroyVolumeRequest;
        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                if (event->GetTypeRewrite() == TEvService::EvDestroyVolumeRequest
                        && event->Recipient == MakeStorageServiceId())
                {
                    auto* msg = event->Get<TEvService::TEvDestroyVolumeRequest>();
                    UNIT_ASSERT(msg->Record.GetDestroyIfBroken());
                    destroyedDiskIds.push_back(msg->Record.GetDiskId());
                    destroyVolumeRequest = event.Release();
                    return TTestActorRuntime::EEventAction::DROP;
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        diskRegistry.SendAllocateDiskRequest(
            "disk-4",
            10_GB,
            DefaultLogicalBlockSize,
            "group-1"
        );

        {
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_BS_RESOURCE_EXHAUSTED, response->GetStatus());
        }

        {
            auto response = diskRegistry.ListBrokenDisks();
            UNIT_ASSERT_VALUES_EQUAL(1, response->DiskIds.size());
            UNIT_ASSERT_VALUES_EQUAL("disk-4", response->DiskIds[0]);
        }

        auto wait = [&] (auto dt) {
            runtime->AdvanceCurrentTime(dt);
            runtime->DispatchEvents({}, TDuration::MilliSeconds(10));
        };

        wait(TDuration::Seconds(4));
        UNIT_ASSERT(!destroyVolumeRequest);

        wait(TDuration::Seconds(1));
        UNIT_ASSERT(destroyVolumeRequest);
        runtime->Send(destroyVolumeRequest.Release());

        wait(TDuration::Seconds(1));
        UNIT_ASSERT_VALUES_EQUAL(1, destroyedDiskIds.size());
        UNIT_ASSERT_VALUES_EQUAL("disk-4", destroyedDiskIds[0]);

        {
            auto response = diskRegistry.ListBrokenDisks();
            UNIT_ASSERT_VALUES_EQUAL(0, response->DiskIds.size());
        }

        {
            auto response = diskRegistry.DescribePlacementGroup("group-1");
            const auto& group = response->Record.GetGroup();
            UNIT_ASSERT_VALUES_EQUAL("group-1", group.GetGroupId());
            UNIT_ASSERT_VALUES_EQUAL(4, group.GetConfigVersion());
            UNIT_ASSERT_VALUES_EQUAL(3, group.DiskIdsSize());
            UNIT_ASSERT_VALUES_EQUAL("disk-1", group.GetDiskIds(0));
            UNIT_ASSERT_VALUES_EQUAL("disk-2", group.GetDiskIds(1));
            UNIT_ASSERT_VALUES_EQUAL("disk-3", group.GetDiskIds(2));
        }

        {
            NProto::TPlacementGroupSettings settings;
            settings.SetMaxDisksInGroup(4);
            diskRegistry.UpdatePlacementGroupSettings(
                "group-1",
                4,
                std::move(settings));
        }

        diskRegistry.SendAllocateDiskRequest(
            "disk-4",
            10_GB,
            DefaultLogicalBlockSize,
            "group-1"
        );

        {
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());
        }

        {
            auto response = diskRegistry.DescribePlacementGroup("group-1");
            const auto& group = response->Record.GetGroup();
            UNIT_ASSERT_VALUES_EQUAL("group-1", group.GetGroupId());
            UNIT_ASSERT_VALUES_EQUAL(6, group.GetConfigVersion());
            UNIT_ASSERT_VALUES_EQUAL(4, group.DiskIdsSize());
            UNIT_ASSERT_VALUES_EQUAL("disk-1", group.GetDiskIds(0));
            UNIT_ASSERT_VALUES_EQUAL("disk-2", group.GetDiskIds(1));
            UNIT_ASSERT_VALUES_EQUAL("disk-3", group.GetDiskIds(2));
            UNIT_ASSERT_VALUES_EQUAL("disk-4", group.GetDiskIds(3));
        }
    }

    Y_UNIT_TEST(ShouldPreventAllocation)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1", "rack-1", 10_GB),
                Device("dev-2", "uuid-2", "rack-1", 10_GB),
            })
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents(agents)
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);

        auto tryAllocate = [&] (auto name) {
            diskRegistry.SendAllocateDiskRequest(name, 10_GB);
            auto response = diskRegistry.RecvAllocateDiskResponse();
            return response->GetStatus();
        };

        diskRegistry.WaitReady();
        diskRegistry.UpdateConfig(CreateRegistryConfig(agents));

        {
            diskRegistry.SendRegisterAgentRequest(agents[0]);
            auto response = diskRegistry.RecvRegisterAgentResponse();
            UNIT_ASSERT_VALUES_EQUAL(response->GetStatus(), E_INVALID_STATE);
        }

        diskRegistry.AllowDiskAllocation();
        RegisterAndWaitForAgents(*runtime, agents);
        diskRegistry.AllowDiskAllocation(false);

        UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, tryAllocate("disk-1"));

        diskRegistry.AllowDiskAllocation();

        UNIT_ASSERT_VALUES_EQUAL(S_OK, tryAllocate("disk-1"));

        diskRegistry.AllowDiskAllocation(false);

        UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, tryAllocate("disk-2"));

        diskRegistry.RebootTablet();
        diskRegistry.WaitReady();

        UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, tryAllocate("disk-2"));
    }

    Y_UNIT_TEST(ShouldNotDeallocateUnmarkedDisk)
    {
        const auto agent = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB),
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);

        diskRegistry.WaitReady();
        diskRegistry.UpdateConfig(CreateRegistryConfig(0, {agent}));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 1);
        WaitForAgents(*runtime, 1);
        WaitForSecureErase(*runtime, {agent});

        diskRegistry.AllocateDisk("disk-0", 10_GB);
        diskRegistry.AllocateDisk("disk-1", 10_GB);

        diskRegistry.MarkDiskForCleanup("disk-1");

        auto deallocate = [&] (auto name, auto force) {
            diskRegistry.SendDeallocateDiskRequest(name, force);
            return diskRegistry.RecvDeallocateDiskResponse()->GetStatus();
        };

        UNIT_ASSERT_VALUES_EQUAL(E_INVALID_STATE, deallocate("disk-0", false));
        UNIT_ASSERT_VALUES_EQUAL(S_OK, deallocate("disk-0", true));
        UNIT_ASSERT_VALUES_EQUAL(S_OK, deallocate("disk-1", false));
    }

    Y_UNIT_TEST(ShouldUpdateVolumeConfig)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB)
        });

        const auto agent2 = CreateAgentConfig("agent-2", {
            Device("dev-1", "uuid-3", "rack-2", 10_GB),
            Device("dev-2", "uuid-4", "rack-2", 10_GB)
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent1, agent2 })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);

        diskRegistry.WaitReady();
        diskRegistry.UpdateConfig(CreateRegistryConfig(0, {agent1, agent2}));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 2);
        WaitForAgents(*runtime, 2);
        WaitForSecureErase(*runtime, {agent1, agent2});

        TSSProxyClient ssProxy(*runtime);

        ssProxy.CreateVolume("disk-1");
        ssProxy.CreateVolume("disk-2");
        ssProxy.CreateVolume("disk-3");

        diskRegistry.AllocateDisk("disk-1", 10_GB);
        diskRegistry.AllocateDisk("disk-2", 10_GB);
        diskRegistry.AllocateDisk("disk-3", 10_GB);

        ui32 updateCount = 0;

        runtime->SetObserverFunc([&] (auto& runtime, auto& event) {
            switch (event->GetTypeRewrite()) {
            case TEvDiskRegistryPrivate::EvUpdateVolumeConfigRequest:
                updateCount++;
                break;
            }
            return TTestActorRuntime::DefaultObserverFunc(runtime, event);
        });

        diskRegistry.CreatePlacementGroup("group-1");
        diskRegistry.AlterPlacementGroupMembership(
            "group-1",
            1, // version
            TVector<TString>{"disk-1"},
            TVector<TString>());

        UNIT_ASSERT_VALUES_EQUAL(updateCount, 1);

        diskRegistry.SendAlterPlacementGroupMembershipRequest(
            "group-1",
            2, // version
            TVector<TString>{"disk-3"},
            TVector<TString>());
        auto response = diskRegistry.RecvAlterPlacementGroupMembershipResponse();

        UNIT_ASSERT_VALUES_EQUAL(E_PRECONDITION_FAILED, response->GetStatus());
        UNIT_ASSERT_VALUES_EQUAL(updateCount, 1);

        diskRegistry.AlterPlacementGroupMembership(
            "group-1",
            2, // version
            TVector<TString>{"disk-2"},
            TVector<TString>());

        UNIT_ASSERT_VALUES_EQUAL(updateCount, 2);

        diskRegistry.DestroyPlacementGroup("group-1");

        UNIT_ASSERT_VALUES_EQUAL(updateCount, 4);
    }

    Y_UNIT_TEST(ShouldRetryVolumeConfigUpdate)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({agent1})
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);

        diskRegistry.WaitReady();
        diskRegistry.UpdateConfig(CreateRegistryConfig(0, {agent1}));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 1);
        WaitForAgents(*runtime, 1);
        WaitForSecureErase(*runtime, {agent1});

        diskRegistry.AllocateDisk("disk-1", 10_GB);
        diskRegistry.CreatePlacementGroup("group-1");

        ui32 describeCount = 0;
        ui32 modifyCount = 0;
        ui32 finishCount = 0;

        runtime->SetEventFilter([&] (auto& runtime, auto& event) {
            switch (event->GetTypeRewrite()) {
            case TEvSSProxy::EvDescribeVolumeRequest:
                describeCount++;
                break;

            case TEvSSProxy::EvModifySchemeRequest:
                modifyCount++;

                runtime.Schedule(
                    new IEventHandle(
                        event->Sender,
                        event->Recipient,
                        new TEvSSProxy::TEvModifySchemeResponse()),
                    TDuration::Seconds(0));

                return true;

            case TEvDiskRegistryPrivate::EvFinishVolumeConfigUpdateRequest:
                finishCount++;
                break;
            }

            return false;
        });

        diskRegistry.AlterPlacementGroupMembership(
            "group-1",
            1, // version
            TVector<TString>{"disk-1"},
            TVector<TString>());

        UNIT_ASSERT_VALUES_EQUAL(describeCount, 1);
        UNIT_ASSERT_VALUES_EQUAL(modifyCount, 0);
        UNIT_ASSERT_VALUES_EQUAL(finishCount, 0);

        TSSProxyClient ssProxy(*runtime);
        ssProxy.CreateVolume("disk-1");

        diskRegistry.RebootTablet();
        diskRegistry.WaitReady();

        runtime->AdvanceCurrentTime(TDuration::Seconds(5));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        UNIT_ASSERT_VALUES_EQUAL(describeCount, 2);
        UNIT_ASSERT_VALUES_EQUAL(modifyCount, 1);
        UNIT_ASSERT_VALUES_EQUAL(finishCount, 1);
    }

    Y_UNIT_TEST(ShouldRespectVolumeConfigVersion)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({agent1})
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);

        diskRegistry.WaitReady();
        diskRegistry.UpdateConfig(CreateRegistryConfig(0, {agent1}));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 1);
        WaitForAgents(*runtime, 1);
        WaitForSecureErase(*runtime, {agent1});

        diskRegistry.AllocateDisk("disk-1", 10_GB);

        TSSProxyClient ssProxy(*runtime);
        ssProxy.CreateVolume("disk-1");

        diskRegistry.CreatePlacementGroup("group-1");

        TDuration responseDelay;
        ui64 finishCount = 0;

        runtime->SetEventFilter([&] (auto& runtime, auto& event) {
            switch (event->GetTypeRewrite()) {
            case TEvSSProxy::EvModifySchemeRequest:
                runtime.Schedule(
                    new IEventHandle(
                        event->Sender,
                        event->Recipient,
                        new TEvSSProxy::TEvModifySchemeResponse()),
                    responseDelay);
                break;

            case TEvDiskRegistryPrivate::EvFinishVolumeConfigUpdateRequest:
                finishCount++;
                break;
            }

            return false;
        });

        // set up ABA configuration

        responseDelay = TDuration::Seconds(1);
        diskRegistry.AlterPlacementGroupMembership(
            "group-1",
            1, // version
            TVector<TString>{"disk-1"},
            TVector<TString>());

        responseDelay = TDuration::Seconds(2);
        diskRegistry.AlterPlacementGroupMembership(
            "group-1",
            2, // version
            TVector<TString>{},
            TVector<TString>{"disk-1"});

        responseDelay = TDuration::Seconds(3);
        diskRegistry.AlterPlacementGroupMembership(
            "group-1",
            3, // version
            TVector<TString>{"disk-1"},
            TVector<TString>());

        // wait for the 1st and 2nd update

        runtime->AdvanceCurrentTime(TDuration::Seconds(2));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));
        UNIT_ASSERT_VALUES_EQUAL(finishCount, 0);

        // wait for the 3rd update

        responseDelay = TDuration::Seconds(0);
        diskRegistry.RebootTablet();
        diskRegistry.WaitReady();
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));
        UNIT_ASSERT_VALUES_EQUAL(finishCount, 1);
    }

    Y_UNIT_TEST(ShouldAllocateMirroredDisk)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB),
            Device("dev-3", "uuid-3", "rack-1", 10_GB),
            // XXX needed to prioritize this agent in the allocation algorithm
            Device("dev-4", "uuid-extra1-1", "rack-1", 10_GB),
            Device("dev-5", "uuid-extra1-2", "rack-1", 10_GB),
        });

        const auto agent2 = CreateAgentConfig("agent-2", {
            Device("dev-1", "uuid-4", "rack-2", 10_GB),
            Device("dev-2", "uuid-5", "rack-2", 10_GB),
            Device("dev-3", "uuid-6", "rack-2", 10_GB),
            // XXX needed to prioritize this agent in the allocation algorithm
            Device("dev-4", "uuid-extra2-1", "rack-2", 10_GB),
            Device("dev-5", "uuid-extra2-2", "rack-2", 10_GB),
        });

        const auto agent3 = CreateAgentConfig("agent-3", {
            Device("dev-1", "uuid-7", "rack-3", 10_GB),
            Device("dev-2", "uuid-8", "rack-3", 10_GB),
            Device("dev-3", "uuid-9", "rack-3", 10_GB),
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent1, agent2, agent3 })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(
            CreateRegistryConfig(0, { agent1, agent2, agent3 }));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 3);
        WaitForAgents(*runtime, 3);
        // XXX this free-func approach is broken - we can only spot the events
        // that are sent after we call this func, but some secure erase events
        // can happen before that
        WaitForSecureErase(*runtime, { agent1, agent2, agent3 });
        // in our case we are only able to spot the last agent
        // WaitForSecureErase(*runtime, { agent3 });

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
                "uuid-4",
                msg.GetReplicas(0).GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-5",
                msg.GetReplicas(0).GetDevices(1).GetDeviceUUID());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetDevices(0).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetDevices(1).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetDevices(0).GetBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetDevices(1).GetBlocksCount());

            UNIT_ASSERT_VALUES_UNEQUAL(
                0,
                msg.GetDevices(0).GetNodeId());

            // from same node
            UNIT_ASSERT_VALUES_EQUAL(
                msg.GetDevices(0).GetNodeId(),
                msg.GetDevices(1).GetNodeId());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetReplicas(0).GetDevices(0).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetReplicas(0).GetDevices(1).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetReplicas(0).GetDevices(0).GetBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetReplicas(0).GetDevices(1).GetBlocksCount());

            UNIT_ASSERT_VALUES_UNEQUAL(
                0,
                msg.GetReplicas(0).GetDevices(0).GetNodeId());

            // from same node
            UNIT_ASSERT_VALUES_EQUAL(
                msg.GetReplicas(0).GetDevices(0).GetNodeId(),
                msg.GetReplicas(0).GetDevices(1).GetNodeId());
        }

        {
            diskRegistry.SendAllocateDiskRequest("disk-1", 30_GB);
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(
                E_INVALID_STATE,
                response->GetStatus());
        }

        {
            diskRegistry.SendAllocateDiskRequest(
                "disk-1",
                20_GB,
                DefaultLogicalBlockSize,
                "", // placementGroupId
                "", // cloudId
                "", // folderId
                1   // replicaCount
            );
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(
                // XXX should be S_ALREADY,
                S_OK,
                response->GetStatus());
        }

        {
            diskRegistry.SendAllocateDiskRequest(
                "disk-1",
                30_GB,
                DefaultLogicalBlockSize,
                "", // placementGroupId
                "", // cloudId
                "", // folderId
                1   // replicaCount
            );
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(
                S_OK,
                response->GetStatus());

            auto& msg = response->Record;
            SortBy(*msg.MutableDevices(), TByUUID());

            UNIT_ASSERT_VALUES_EQUAL(3, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-1",
                msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-2",
                msg.GetDevices(1).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-3",
                msg.GetDevices(2).GetDeviceUUID());

            UNIT_ASSERT_VALUES_EQUAL(1, msg.ReplicasSize());

            SortBy(*msg.MutableReplicas(0)->MutableDevices(), TByUUID());

            UNIT_ASSERT_VALUES_EQUAL(3, msg.GetReplicas(0).DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-4",
                msg.GetReplicas(0).GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-5",
                msg.GetReplicas(0).GetDevices(1).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-6",
                msg.GetReplicas(0).GetDevices(2).GetDeviceUUID());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetDevices(0).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetDevices(1).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetDevices(2).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetDevices(0).GetBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetDevices(1).GetBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetDevices(2).GetBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetReplicas(0).GetDevices(0).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetReplicas(0).GetDevices(1).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetReplicas(0).GetDevices(2).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetReplicas(0).GetDevices(0).GetBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetReplicas(0).GetDevices(1).GetBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetReplicas(0).GetDevices(2).GetBlocksCount());
        }

        diskRegistry.ChangeAgentState(
            "agent-1",
            NProto::EAgentState::AGENT_STATE_UNAVAILABLE);

        {
            auto response = diskRegistry.AllocateDisk(
                "disk-1",
                30_GB,
                DefaultLogicalBlockSize,
                "", // placementGroupId
                "", // cloudId
                "", // folderId
                1   // replicaCount
            );

            auto& msg = response->Record;
            SortBy(*msg.MutableDevices(), TByUUID());

            UNIT_ASSERT_VALUES_EQUAL(3, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-7",
                msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-8",
                msg.GetDevices(1).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-9",
                msg.GetDevices(2).GetDeviceUUID());

            UNIT_ASSERT_VALUES_EQUAL(3, msg.DeviceReplacementUUIDsSize());

            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-7",
                msg.GetDeviceReplacementUUIDs(0));
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-8",
                msg.GetDeviceReplacementUUIDs(1));
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-9",
                msg.GetDeviceReplacementUUIDs(2));

            UNIT_ASSERT_VALUES_EQUAL(1, msg.ReplicasSize());

            SortBy(*msg.MutableReplicas(0)->MutableDevices(), TByUUID());

            UNIT_ASSERT_VALUES_EQUAL(3, msg.GetReplicas(0).DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-4",
                msg.GetReplicas(0).GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-5",
                msg.GetReplicas(0).GetDevices(1).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                "uuid-6",
                msg.GetReplicas(0).GetDevices(2).GetDeviceUUID());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetDevices(0).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetDevices(1).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetDevices(2).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetDevices(0).GetBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetDevices(1).GetBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetDevices(2).GetBlocksCount());

            UNIT_ASSERT_VALUES_UNEQUAL(
                0,
                msg.GetDevices(0).GetNodeId());

            // from same node
            UNIT_ASSERT_VALUES_EQUAL(
                msg.GetDevices(0).GetNodeId(),
                msg.GetDevices(1).GetNodeId());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetReplicas(0).GetDevices(0).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetReplicas(0).GetDevices(1).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetReplicas(0).GetDevices(2).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetReplicas(0).GetDevices(0).GetBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetReplicas(0).GetDevices(1).GetBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetReplicas(0).GetDevices(2).GetBlocksCount());

            UNIT_ASSERT_VALUES_UNEQUAL(
                0,
                msg.GetReplicas(0).GetDevices(0).GetNodeId());

            // from same node
            UNIT_ASSERT_VALUES_EQUAL(
                msg.GetReplicas(0).GetDevices(0).GetNodeId(),
                msg.GetReplicas(0).GetDevices(1).GetNodeId());
        }
    }
}

}   // namespace NCloud::NBlockStore::NStorage
