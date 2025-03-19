#include "disk_registry.h"
#include "disk_registry_actor.h"

#include <cloud/blockstore/config/disk.pb.h>

#include <cloud/blockstore/libs/diagnostics/critical_events.h>
#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/storage/api/disk_agent.h>
#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/api/volume_proxy.h>
#include <cloud/blockstore/libs/storage/disk_registry/testlib/test_env.h>
#include <cloud/blockstore/libs/storage/disk_registry/testlib/test_logbroker.h>
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
    Y_UNIT_TEST(ShouldRecoverStateOnReboot)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB),
            Device("dev-3", "uuid-3", "rack-1", 10_GB),
            Device("dev-4", "uuid-4", "rack-1", 10_GB),
            Device("dev-5", "uuid-5", "rack-1", 10_GB)
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent1 })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, {agent1}));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 1);
        WaitForAgents(*runtime, 1);
        WaitForSecureErase(*runtime, {agent1});

        diskRegistry.AllocateDisk("disk-1", 20_GB);
        diskRegistry.AllocateDisk("disk-2", 20_GB);
        diskRegistry.AllocateDisk("disk-3", 10_GB);

        diskRegistry.DeallocateDisk("disk-2", true);

        diskRegistry.RebootTablet();
        diskRegistry.WaitReady();

        diskRegistry.AcquireDisk("disk-1", "session-1");
        diskRegistry.AcquireDisk("disk-3", "session-2");

        diskRegistry.SendAcquireDiskRequest("disk-2", "session-3");
        auto response = diskRegistry.RecvAcquireDiskResponse();
        UNIT_ASSERT_VALUES_EQUAL(response->GetStatus(), E_NOT_FOUND);
    }

    Y_UNIT_TEST(ShouldCleanupDisks)
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

        TSSProxyClient ss(*runtime);

        ss.CreateVolume("vol");
        ss.CreateVolume("nonrepl-vol");

        diskRegistry.AllocateDisk("nonrepl-vol", 10_GB);
        diskRegistry.AllocateDisk("nonrepl-garbage", 10_GB);

        UNIT_ASSERT(diskRegistry.Exists("nonrepl-vol"));
        UNIT_ASSERT(diskRegistry.Exists("nonrepl-garbage"));

        diskRegistry.MarkDiskForCleanup("nonrepl-vol");
        diskRegistry.MarkDiskForCleanup("nonrepl-garbage");

        diskRegistry.CleanupDisks();

        UNIT_ASSERT(diskRegistry.Exists("nonrepl-vol"));
        UNIT_ASSERT(!diskRegistry.Exists("nonrepl-garbage"));
    }

    Y_UNIT_TEST(ShouldCleanupMirroredDisks)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB),
        });

        const auto agent2 = CreateAgentConfig("agent-2", {
            Device("dev-1", "uuid-3", "rack-2", 10_GB),
            Device("dev-2", "uuid-4", "rack-2", 10_GB),
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

        TSSProxyClient ss(*runtime);

        ss.CreateVolume("vol");
        ss.CreateVolume("mirrored-vol");

        diskRegistry.AllocateDisk(
            "mirrored-vol",
            10_GB,
            DefaultLogicalBlockSize,
            "", // placementGroupId
            "", // cloudId
            "", // folderId
            1   // replicaCount
        );
        diskRegistry.AllocateDisk(
            "mirrored-garbage",
            10_GB,
            DefaultLogicalBlockSize,
            "", // placementGroupId
            "", // cloudId
            "", // folderId
            1   // replicaCount
        );

        // checking that our volumes are actually mirrored
        {
            auto response = diskRegistry.DescribeDisk("mirrored-vol");
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());
            auto& r = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(1, r.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(1, r.ReplicasSize());
            UNIT_ASSERT_VALUES_EQUAL(1, r.GetReplicas(0).DevicesSize());
        }

        {
            auto response = diskRegistry.DescribeDisk("mirrored-garbage");
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());
            auto& r = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(1, r.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(1, r.ReplicasSize());
            UNIT_ASSERT_VALUES_EQUAL(1, r.GetReplicas(0).DevicesSize());
        }

        UNIT_ASSERT(diskRegistry.Exists("mirrored-vol"));
        UNIT_ASSERT(diskRegistry.Exists("mirrored-garbage"));

        diskRegistry.MarkDiskForCleanup("mirrored-vol");
        diskRegistry.MarkDiskForCleanup("mirrored-garbage");

        diskRegistry.CleanupDisks();

        UNIT_ASSERT(diskRegistry.Exists("mirrored-vol"));
        UNIT_ASSERT(!diskRegistry.Exists("mirrored-garbage"));
    }

    Y_UNIT_TEST(ShouldRepairVolume)
    {
        const auto agent = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB)
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

        TSSProxyClient ss(*runtime);

        ss.CreateVolume("vol");
        ss.CreateVolume("nonrepl-vol-1");
        ss.CreateVolume("nonrepl-vol-2");
        diskRegistry.AllocateDisk("nonrepl-vol-2", 10_GB);

        UNIT_ASSERT(!diskRegistry.Exists("nonrepl-vol-1"));

        diskRegistry.CleanupDisks();

        // UNIT_ASSERT(diskRegistry.Exists("nonrepl-vol-1"));
    }

    Y_UNIT_TEST(ShouldNotCleanupUnmarkedDisks)
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

        TSSProxyClient ss(*runtime);

        ss.CreateVolume("vol");
        ss.CreateVolume("nonrepl-vol");

        diskRegistry.AllocateDisk("nonrepl-vol", 10_GB);
        diskRegistry.AllocateDisk("nonrepl-garbage", 10_GB);

        diskRegistry.CleanupDisks();

        UNIT_ASSERT(diskRegistry.Exists("nonrepl-vol"));
        UNIT_ASSERT(diskRegistry.Exists("nonrepl-garbage"));
    }

    Y_UNIT_TEST(ShouldNotCleanupAliveDisks)
    {
        const auto agent = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB),
            Device("dev-3", "uuid-3", "rack-1", 10_GB),
            Device("dev-4", "uuid-4", "rack-1", 10_GB)
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

        TSSProxyClient ss(*runtime);

        ss.CreateVolume("nonrepl-vol-11");
        ss.CreateVolume("nonrepl-vol-x");
        ss.CreateVolume("nonrepl-vol-2");
        ss.CreateVolume("nonrepl-vol-10");

        diskRegistry.AllocateDisk("nonrepl-vol-2", 10_GB);
        diskRegistry.AllocateDisk("nonrepl-vol-10", 10_GB);
        diskRegistry.AllocateDisk("nonrepl-vol-x", 10_GB);
        diskRegistry.AllocateDisk("nonrepl-vol-11", 10_GB);

        size_t toRemove = 0;

        runtime->SetObserverFunc([&](TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
            switch (event->GetTypeRewrite()) {
                case TEvDiskRegistry::EvDeallocateDiskRequest:
                    toRemove++;
            }

            return TTestActorRuntime::DefaultObserverFunc(runtime, event);
        });

        diskRegistry.CleanupDisks();

        UNIT_ASSERT_VALUES_EQUAL(0, toRemove);
    }

    Y_UNIT_TEST(ShouldCleanDirtyDevicesAfterReboot)
    {
        const auto agentConfig = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB)
        });

        auto agent = CreateTestDiskAgent(agentConfig);
        agent->HandleSecureEraseDeviceImpl = [] (const auto&, const auto&) {
            // ignore
            return true;
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, { agentConfig }));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 1);
        WaitForAgents(*runtime, 1);

        {
            // can't allocate new disk: all devices are dirty
            diskRegistry.SendAllocateDiskRequest("disk-2", 20_GB);
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(response->GetStatus(), E_BS_DISK_ALLOCATION_FAILED);
        }

        diskRegistry.RebootTablet();
        diskRegistry.WaitReady();

        {
            // still can't allocate new disk
            diskRegistry.SendAllocateDiskRequest("disk-2", 20_GB);
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(response->GetStatus(), E_BS_DISK_ALLOCATION_FAILED);
        }
    }

    Y_UNIT_TEST(ShouldRetryCleanDevices)
    {
        const auto agentConfig = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB)
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agentConfig })
            .Build();

        int dev1Requests = 0;
        int dev2Requests = 0;

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig({ agentConfig }));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 1);
        WaitForAgents(*runtime, 1);
        WaitForSecureErase(*runtime, {agentConfig});

        runtime->SetObserverFunc([&](TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
            switch (event->GetTypeRewrite()) {
                case TEvDiskAgent::EvSecureEraseDeviceRequest: {
                    auto& msg = *event->Get<TEvDiskAgent::TEvSecureEraseDeviceRequest>();
                    dev2Requests += msg.Record.GetDeviceUUID() == "uuid-2";

                    if (msg.Record.GetDeviceUUID() == "uuid-1") {
                        ++dev1Requests;
                        if (dev1Requests == 1) {
                            auto response = std::make_unique<TEvDiskAgent::TEvSecureEraseDeviceResponse>(
                                MakeError(E_REJECTED));

                            runtime.Send(
                                new IEventHandle(
                                    event->Sender,
                                    event->Recipient,
                                    response.release(),
                                    0, // flags
                                    event->Cookie));

                            return TTestActorRuntime::EEventAction::DROP;
                        }
                    }
                    break;
                }
            }

            return TTestActorRuntime::DefaultObserverFunc(runtime, event);
        });

        diskRegistry.AllocateDisk("disk-1", 20_GB);
        diskRegistry.DeallocateDisk("disk-1", true);

        UNIT_ASSERT_VALUES_EQUAL(0, dev1Requests);
        UNIT_ASSERT_VALUES_EQUAL(0, dev2Requests);

        runtime->AdvanceCurrentTime(TDuration::Seconds(6));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));
        UNIT_ASSERT_VALUES_EQUAL(1, dev1Requests);
        UNIT_ASSERT_VALUES_EQUAL(1, dev2Requests);

        runtime->AdvanceCurrentTime(TDuration::Seconds(6));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));
        UNIT_ASSERT_VALUES_EQUAL(2, dev1Requests);
        UNIT_ASSERT_VALUES_EQUAL(1, dev2Requests);

        TVector<NProto::TDeviceConfig> devices;
        auto& device = devices.emplace_back();
        device.SetNodeId(runtime->GetNodeId(0));
        device.SetDeviceUUID("uuid-1");
        diskRegistry.SecureErase(devices);

        UNIT_ASSERT_VALUES_EQUAL(dev1Requests, 3);
        UNIT_ASSERT_VALUES_EQUAL(dev2Requests, 1);
    }

    Y_UNIT_TEST(ShouldSecureErase)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB)
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent1 })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, { agent1 }));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 1);
        WaitForAgents(*runtime, 1);
        WaitForSecureErase(*runtime, {agent1});

        TVector<NProto::TDeviceConfig> devices;

        {
            auto& device = devices.emplace_back();
            device.SetNodeId(runtime->GetNodeId(0));
            device.SetDeviceUUID("uuid-1");
        }

        {
            auto& device = devices.emplace_back();
            device.SetNodeId(runtime->GetNodeId(0));
            device.SetDeviceUUID("uuid-2");
        }

        {
            auto& device = devices.emplace_back();
            device.SetNodeId(runtime->GetNodeId(0));
            device.SetDeviceUUID("uuid-3");
        }

        TVector<TString> cleanDevices;

        runtime->SetObserverFunc([&](TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
            switch (event->GetTypeRewrite()) {
                case TEvDiskRegistryPrivate::EvCleanupDevicesRequest: {
                    auto& msg = *event->Get<TEvDiskRegistryPrivate::TEvCleanupDevicesRequest>();
                    cleanDevices = msg.Devices;
                    break;
                }
            }

            return TTestActorRuntime::DefaultObserverFunc(runtime, event);
        });

        diskRegistry.SecureErase(devices);

        UNIT_ASSERT_VALUES_EQUAL(cleanDevices.size(), 3);
        UNIT_ASSERT_VALUES_EQUAL(cleanDevices[0], "uuid-1");
        UNIT_ASSERT_VALUES_EQUAL(cleanDevices[1], "uuid-2");
        UNIT_ASSERT_VALUES_EQUAL(cleanDevices[2], "uuid-3");
    }

    Y_UNIT_TEST(ShouldHandleUndeliveredSecureEraseRequests)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1", "rack-1", 10_GB),
                Device("dev-2", "uuid-2", "rack-1", 10_GB)
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-3", "uuid-3", "rack-1", 10_GB),
                Device("dev-4", "uuid-4", "rack-1", 10_GB)
            })
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents(agents)
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, agents));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 2);
        WaitForAgents(*runtime, 2);
        WaitForSecureErase(*runtime, agents);

        KillAgent(*runtime, 0);
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        // secure erase

        TVector<TString> cleanDevices;

        runtime->SetObserverFunc([&](TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
            switch (event->GetTypeRewrite()) {
                case TEvDiskRegistryPrivate::EvCleanupDevicesRequest: {
                    auto& msg = *event->Get<TEvDiskRegistryPrivate::TEvCleanupDevicesRequest>();
                    cleanDevices = msg.Devices;
                    break;
                }
            }

            return TTestActorRuntime::DefaultObserverFunc(runtime, event);
        });

        TVector<NProto::TDeviceConfig> devices;

        {
            auto& device = devices.emplace_back();
            device.SetNodeId(runtime->GetNodeId(0));
            device.SetDeviceUUID("uuid-1");
        }

        {
            auto& device = devices.emplace_back();
            device.SetNodeId(runtime->GetNodeId(0));
            device.SetDeviceUUID("uuid-2");
        }

        {
            auto& device = devices.emplace_back();
            device.SetNodeId(runtime->GetNodeId(1));
            device.SetDeviceUUID("uuid-3");
        }

        {
            auto& device = devices.emplace_back();
            device.SetNodeId(runtime->GetNodeId(1));
            device.SetDeviceUUID("uuid-4");
        }

        diskRegistry.SecureErase(devices);

        Sort(cleanDevices);
        UNIT_ASSERT_VALUES_EQUAL(2, cleanDevices.size());
        UNIT_ASSERT_VALUES_EQUAL("uuid-3", cleanDevices[0]);
        UNIT_ASSERT_VALUES_EQUAL("uuid-4", cleanDevices[1]);
    }

    Y_UNIT_TEST(ShouldHandleSecureEraseRequestTimeout)
    {
        const auto agentConfig1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB)
        });

        const auto agentConfig2 = CreateAgentConfig("agent-2", {
            Device("dev-3", "uuid-3", "rack-1", 10_GB),
            Device("dev-4", "uuid-4", "rack-1", 10_GB)
        });

        auto agent = CreateTestDiskAgent(agentConfig1);
        agent->HandleSecureEraseDeviceImpl = [] (const auto&, const auto&) {
            // ignore
            return true;
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent, CreateTestDiskAgent(agentConfig2) })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(
            CreateRegistryConfig(0, { agentConfig1, agentConfig2 }));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 2);
        WaitForAgents(*runtime, 2);

        // secure erase

        TVector<TString> cleanDevices;

        runtime->SetObserverFunc([&](TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
            switch (event->GetTypeRewrite()) {
                case TEvDiskRegistryPrivate::EvCleanupDevicesRequest: {
                    auto& msg = *event->Get<TEvDiskRegistryPrivate::TEvCleanupDevicesRequest>();
                    cleanDevices = msg.Devices;
                    break;
                }
            }

            return TTestActorRuntime::DefaultObserverFunc(runtime, event);
        });

        TVector<NProto::TDeviceConfig> devices;

        {
            auto& device = devices.emplace_back();
            device.SetNodeId(runtime->GetNodeId(0));
            device.SetDeviceUUID("uuid-1");
        }

        {
            auto& device = devices.emplace_back();
            device.SetNodeId(runtime->GetNodeId(0));
            device.SetDeviceUUID("uuid-2");
        }

        {
            auto& device = devices.emplace_back();
            device.SetNodeId(runtime->GetNodeId(1));
            device.SetDeviceUUID("uuid-3");
        }

        {
            auto& device = devices.emplace_back();
            device.SetNodeId(runtime->GetNodeId(1));
            device.SetDeviceUUID("uuid-4");
        }

        diskRegistry.SecureErase(devices, TDuration::Seconds(1));

        Sort(cleanDevices);
        UNIT_ASSERT_VALUES_EQUAL(2, cleanDevices.size());
        UNIT_ASSERT_VALUES_EQUAL("uuid-3", cleanDevices[0]);
        UNIT_ASSERT_VALUES_EQUAL("uuid-4", cleanDevices[1]);
    }

    Y_UNIT_TEST(ShouldUpdateNodeId)
    {
        auto agent = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB)
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, { agent }));
        diskRegistry.AllowDiskAllocation();

        agent.SetNodeId(42);
        diskRegistry.RegisterAgent(agent);
        diskRegistry.CleanupDevices(TVector<TString>{"uuid-1"});

        diskRegistry.AllocateDisk("disk-1", 10_GB);
        diskRegistry.SendAcquireDiskRequest("disk-1", "session-1");

        auto response = diskRegistry.RecvAcquireDiskResponse();
        UNIT_ASSERT_VALUES_EQUAL(E_REJECTED, response->GetStatus());
        UNIT_ASSERT_VALUES_EQUAL("not delivered", response->GetErrorReason());

        RegisterAgents(*runtime, 1);
        WaitForAgents(*runtime, 1);

        {
            TDiskRegistryClient diskRegistry(*runtime);
            diskRegistry.WaitReady();
            diskRegistry.AcquireDisk("disk-1", "session-1");
        }
    }

    Y_UNIT_TEST(ShouldNotifyDisks)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB),
            Device("dev-3", "uuid-3", "rack-1", 10_GB),
            Device("dev-4", "uuid-4", "rack-1", 10_GB),
        });

        const auto agent2 = CreateAgentConfig("agent-2", {
            Device("dev-1", "uuid-5", "rack-2", 10_GB),
            Device("dev-2", "uuid-6", "rack-2", 10_GB),
            Device("dev-3", "uuid-7", "rack-2", 10_GB),
            Device("dev-4", "uuid-8", "rack-2", 10_GB),
        });

        const auto newAgent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB),
            Device("dev-3", "uuid-3", "rack-1", 10_GB),
            Device("dev-4", "uuid-4", "rack-1", 10_GB),
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent1, agent2, newAgent1 })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(
            0,
            {
                agent1,
                agent2,
            }
        ));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 2);
        WaitForAgents(*runtime, 2);
        WaitForSecureErase(*runtime, {agent1, agent2});

        diskRegistry.AllocateDisk("disk-1", 10_GB, DefaultLogicalBlockSize);
        diskRegistry.AllocateDisk("disk-2", 40_GB, DefaultLogicalBlockSize);
        diskRegistry.AllocateDisk("disk-3", 10_GB, DefaultLogicalBlockSize);
        diskRegistry.AllocateDisk("disk-4", 20_GB, DefaultLogicalBlockSize);

        TVector<TString> notifiedDiskIds;
        TList<std::unique_ptr<IEventHandle>> notifications;
        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                if (event->GetTypeRewrite() == TEvVolume::EvReallocateDiskRequest
                        && event->Recipient == MakeVolumeProxyServiceId())
                {
                    auto* msg = event->Get<TEvVolume::TEvReallocateDiskRequest>();
                    notifiedDiskIds.push_back(msg->Record.GetDiskId());
                    notifications.emplace_back(event.Release());
                    return TTestActorRuntime::EEventAction::DROP;
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        RegisterAgent(*runtime, 2);

        {
            auto response = diskRegistry.ListDisksToNotify();
            UNIT_ASSERT_VALUES_EQUAL(3, response->DiskIds.size());
            Sort(response->DiskIds);
            UNIT_ASSERT_VALUES_EQUAL("disk-1", response->DiskIds[0]);
            UNIT_ASSERT_VALUES_EQUAL("disk-3", response->DiskIds[1]);
            UNIT_ASSERT_VALUES_EQUAL("disk-4", response->DiskIds[2]);
        }

        runtime->AdvanceCurrentTime(TDuration::Seconds(5));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));
        UNIT_ASSERT_VALUES_EQUAL(3, notifications.size());
        for (auto& notification: notifications) {
            runtime->Send(notification.release());
        }

        UNIT_ASSERT_VALUES_EQUAL(3, notifiedDiskIds.size());
        Sort(notifiedDiskIds.begin(), notifiedDiskIds.end());
        UNIT_ASSERT_VALUES_EQUAL("disk-1", notifiedDiskIds[0]);
        UNIT_ASSERT_VALUES_EQUAL("disk-3", notifiedDiskIds[1]);
        UNIT_ASSERT_VALUES_EQUAL("disk-4", notifiedDiskIds[2]);

        runtime->AdvanceCurrentTime(TDuration::Seconds(1));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        {
            auto response = diskRegistry.ListDisksToNotify();
            UNIT_ASSERT_VALUES_EQUAL(0, response->DiskIds.size());
        }
    }

    Y_UNIT_TEST(ShouldRetryTimedOutNotifications)
    {
        TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1.1", "rack-1", 10_GB),
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-1", "uuid-2.1", "rack-2", 10_GB),
            })
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents(agents)
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, agents));
        diskRegistry.AllowDiskAllocation();

        RegisterAndWaitForAgents(*runtime, agents);

        diskRegistry.AllocateDisk("disk-1", 20_GB);

        int requests = 0;
        runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                if (event->GetTypeRewrite() == TEvVolume::EvReallocateDiskRequest
                        && event->Recipient == MakeVolumeProxyServiceId())
                {
                    ++requests;
                    return TTestActorRuntime::EEventAction::DROP;
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        diskRegistry.ChangeAgentState(
            "agent-1",
            NProto::EAgentState::AGENT_STATE_WARNING);

        UNIT_ASSERT_VALUES_EQUAL(1, requests);

        {
            auto response = diskRegistry.ListDisksToNotify();
            UNIT_ASSERT_VALUES_EQUAL(1, response->DiskIds.size());
        }

        UNIT_ASSERT_VALUES_EQUAL(1, requests);

        runtime->AdvanceCurrentTime(TDuration::Seconds(5));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        UNIT_ASSERT_VALUES_EQUAL(1, requests);

        runtime->AdvanceCurrentTime(TDuration::Seconds(30));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        UNIT_ASSERT_VALUES_EQUAL(2, requests);

        {
            auto response = diskRegistry.ListDisksToNotify();
            UNIT_ASSERT_VALUES_EQUAL(1, response->DiskIds.size());
        }
    }

    Y_UNIT_TEST(ShouldReplaceDevice)
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

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, { agent1, agent2 }));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 2);
        WaitForSecureErase(*runtime, {agent1, agent2});

        TVector<TString> devices;

        {
            auto response = diskRegistry.AllocateDisk(
                "disk-1", 30_GB, DefaultLogicalBlockSize);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());
            auto& r = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(3, r.DevicesSize());
            devices = {
                r.GetDevices(0).GetDeviceUUID(),
                r.GetDevices(1).GetDeviceUUID(),
                r.GetDevices(2).GetDeviceUUID()
            };
            Sort(devices);
        }

        diskRegistry.ReplaceDevice("disk-1", devices[0]);

        TVector<TString> newDevices;

        {
            auto response = diskRegistry.DescribeDisk("disk-1");
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());
            auto& r = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(3, r.DevicesSize());
            newDevices = {
                r.GetDevices(0).GetDeviceUUID(),
                r.GetDevices(1).GetDeviceUUID(),
                r.GetDevices(2).GetDeviceUUID()
            };
            Sort(newDevices);
        }

        TVector<TString> diff;

        std::set_difference(
            devices.cbegin(),
            devices.cend(),
            newDevices.cbegin(),
            newDevices.cend(),
            std::back_inserter(diff));

        UNIT_ASSERT_VALUES_EQUAL(1, diff.size());
        UNIT_ASSERT_VALUES_EQUAL(devices[0], diff[0]);

        {
            diskRegistry.SendAllocateDiskRequest(
                "disk-2", 10_GB, DefaultLogicalBlockSize);
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, response->GetStatus());
        }
    }

    Y_UNIT_TEST(ShouldRejectDisconnectedAgent)
    {
        const auto agent = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
        });

        auto runtime = TTestRuntimeBuilder().Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, { agent }));
        diskRegistry.AllowDiskAllocation();

        auto sender = runtime->AllocateEdgeActor(0);

        auto registerAgent = [&] {
            auto pipe = runtime->ConnectToPipe(
                TestTabletId,
                sender,
                0,
                NKikimr::GetPipeConfigWithRetries());

            auto request = std::make_unique<TEvDiskRegistry::TEvRegisterAgentRequest>();
            *request->Record.MutableAgentConfig() = agent;
            request->Record.MutableAgentConfig()->SetNodeId(runtime->GetNodeId(0));

            auto pipeEv = new IEventHandle(pipe, sender, request.release(), 0, 0);
            pipeEv->Rewrite(NKikimr::TEvTabletPipe::EvSend, pipe);

            runtime->Send(pipeEv, 0, true);

            return pipe;
        };

        auto disconnectAgent = [&](auto pipe) {
            runtime->Send(new IEventHandle(
                pipe, sender, new TEvTabletPipe::TEvShutdown()), 0, true);
        };

        auto pipe = registerAgent();

        runtime->AdvanceCurrentTime(TDuration::Seconds(60));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));
        disconnectAgent(pipe);

        runtime->AdvanceCurrentTime(TDuration::Seconds(1));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        pipe = registerAgent();

        runtime->AdvanceCurrentTime(TDuration::Seconds(13));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        disconnectAgent(pipe);

        runtime->AdvanceCurrentTime(TDuration::Seconds(2));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        runtime->AdvanceCurrentTime(TDuration::Seconds(1));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        pipe = registerAgent();

        runtime->AdvanceCurrentTime(TDuration::Seconds(10));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        {
            diskRegistry.CleanupDevices(TVector<TString>{"uuid-1"});

            diskRegistry.SendAllocateDiskRequest("nonrepl-vol-1", 10_GB);
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());
        }
    }

    Y_UNIT_TEST(ShouldFailCleanupIfDiskRegistryRestarts)
    {
        const auto agent = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB)
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

        TSSProxyClient ss(*runtime);

        ss.CreateVolume("vol");
        ss.CreateVolume("nonrepl-vol");
        diskRegistry.AllocateDisk("nonrepl-vol", 10_GB);
        diskRegistry.AllocateDisk("nonrepl-garbage", 10_GB);

        UNIT_ASSERT(diskRegistry.Exists("nonrepl-vol"));
        UNIT_ASSERT(diskRegistry.Exists("nonrepl-garbage"));

        runtime->SetObserverFunc(
        [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
            switch (event->GetTypeRewrite()) {
                case TEvSSProxy::EvDescribeVolumeRequest: {
                    return TTestActorRuntime::EEventAction::DROP;
                }
            }

            return TTestActorRuntime::DefaultObserverFunc(runtime, event);
        });

        diskRegistry.SendCleanupDisksRequest();

        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));
        diskRegistry.RebootTablet();
        diskRegistry.WaitReady();

        auto response = diskRegistry.RecvCleanupDisksResponse();
        UNIT_ASSERT_VALUES_EQUAL(
            response->GetStatus(),
            E_REJECTED);
    }

    Y_UNIT_TEST(ShouldNotReuseBrokenDevice)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1.1", "rack-1", 10_GB),
            Device("dev-2", "uuid-1.2", "rack-1", 10_GB)
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent1 })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, { agent1 }));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 1);
        WaitForAgents(*runtime, 1);
        WaitForSecureErase(*runtime, {agent1});

        {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL("uuid-1.1", msg.GetDevices(0).GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1.2", msg.GetDevices(1).GetDeviceUUID());
        }

        diskRegistry.ChangeDeviceState("uuid-1.1", NProto::DEVICE_STATE_WARNING);
        diskRegistry.DeallocateDisk("disk-1", true);

        // wait for cleanup devices
        {
            TDispatchOptions options;
            options.FinalEvents = {
                TDispatchOptions::TFinalEventCondition(
                    TEvDiskRegistryPrivate::EvCleanupDevicesResponse)
            };

            runtime->DispatchEvents(options);
        }

        {
            diskRegistry.SendAllocateDiskRequest("disk-2", 20_GB);
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(response->GetStatus(), E_BS_DISK_ALLOCATION_FAILED);
        }
    }

    Y_UNIT_TEST(ShouldPublishDiskState)
    {
        TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1.1", "rack-1", 10_GB),
                Device("dev-2", "uuid-1.2", "rack-1", 10_GB),
                Device("dev-3", "uuid-1.3", "rack-1", 10_GB)
            })
        };

        NProto::TStorageServiceConfig config;
        // disable recycling
        config.SetNonReplicatedDiskRecyclingPeriod(Max<ui32>());
        config.SetAllocationUnitNonReplicatedSSD(10);

        auto logbrokerService = std::make_shared<TTestLogbrokerService>();

        auto runtime = TTestRuntimeBuilder()
            .With(config)
            .With(logbrokerService)
            .WithAgents(agents)
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, agents));
        diskRegistry.AllowDiskAllocation();

        RegisterAndWaitForAgents(*runtime, agents);

        diskRegistry.AllocateDisk("vol0", 30_GB);

        UNIT_ASSERT_VALUES_EQUAL(0, logbrokerService->GetItemCount());

        auto removeHost = [&] (auto host) {
            NProto::TAction action;
            action.SetHost(host);
            action.SetType(NProto::TAction::REMOVE_HOST);

            diskRegistry.CmsAction(TVector<NProto::TAction>{ action });
        };

        auto waitForItems = [&] {
            // speed up test
            runtime->AdvanceCurrentTime(TDuration::Seconds(10));

            if (logbrokerService->GetItemCount()) {
                return;
            }

            TDispatchOptions options;
            options.CustomFinalCondition = [&] {
                return logbrokerService->GetItemCount();
            };

            runtime->DispatchEvents(options);
        };

        // warning
        removeHost("agent-1");
        waitForItems();

        ui64 maxSeqNo = 0;
        {
            auto items = logbrokerService->ExtractItems();
            UNIT_ASSERT_VALUES_EQUAL(1, items.size());

            auto& item = items[0];
            maxSeqNo = item.SeqNo;

            UNIT_ASSERT_VALUES_EQUAL("vol0", item.DiskId);
            UNIT_ASSERT_EQUAL(
                NProto::DISK_STATE_ONLINE, // migration
                item.State);
            UNIT_ASSERT_VALUES_UNEQUAL("", item.Message);
        }

        diskRegistry.ChangeDeviceState("uuid-1.2", NProto::DEVICE_STATE_ERROR);
        waitForItems();

        {
            auto items = logbrokerService->ExtractItems();
            UNIT_ASSERT_VALUES_EQUAL(1, items.size());
            auto& item = items[0];

            UNIT_ASSERT_LT(maxSeqNo, item.SeqNo);
            maxSeqNo = item.SeqNo;

            UNIT_ASSERT_VALUES_EQUAL("vol0", item.DiskId);
            UNIT_ASSERT_EQUAL(
                NProto::DISK_STATE_ERROR,
                item.State);
            UNIT_ASSERT_VALUES_EQUAL("", item.Message);
        }

        // unavailable
        runtime->AdvanceCurrentTime(TDuration::Days(1));
        removeHost("agent-1");
        diskRegistry.ChangeAgentState(
            "agent-1",
            NProto::EAgentState::AGENT_STATE_UNAVAILABLE);

        // waste some time
        runtime->AdvanceCurrentTime(TDuration::Minutes(1));
        runtime->DispatchEvents(TDispatchOptions(), TDuration::MilliSeconds(10));

        // vol0 already broken
        UNIT_ASSERT_VALUES_EQUAL(0, logbrokerService->GetItemCount());
    }

    Y_UNIT_TEST(ShouldRestoreCountersAfterReboot)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB)
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({agent1})
            .Build();

        auto getOnlineDisks = [&] {
            return runtime->GetAppData(0).Counters
                ->GetSubgroup("counters", "blockstore")
                ->GetSubgroup("component", "disk_registry")
                ->GetCounter("DisksInOnlineState")
                ->Val();
        };

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        UNIT_ASSERT_VALUES_EQUAL(0, getOnlineDisks());

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, {agent1}));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 1);
        WaitForAgents(*runtime, 1);
        WaitForSecureErase(*runtime, {agent1});

        diskRegistry.AllocateDisk("disk-1", 10_GB);

        runtime->AdvanceCurrentTime(UpdateCountersInterval);
        runtime->DispatchEvents(TDispatchOptions(), TDuration::MilliSeconds(10));

        UNIT_ASSERT_VALUES_EQUAL(1, getOnlineDisks());

        diskRegistry.RebootTablet();
        diskRegistry.WaitReady();

        UNIT_ASSERT_VALUES_EQUAL(1, getOnlineDisks());
    }

    Y_UNIT_TEST(ShouldSkipSecureEraseForUnavailableDevices)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1", "rack-1", 10_GB),
                Device("dev-2", "uuid-2", "rack-1", 10_GB),
                Device("dev-3", "uuid-3", "rack-1", 10_GB),
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-1", "uuid-4", "rack-2", 10_GB),
                Device("dev-2", "uuid-5", "rack-2", 10_GB),
                Device("dev-3", "uuid-6", "rack-2", 10_GB)
            })
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents(agents)
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();
        diskRegistry.UpdateConfig(CreateRegistryConfig(agents));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, agents.size());
        WaitForAgents(*runtime, agents.size());
        WaitForSecureErase(*runtime, agents);

        diskRegistry.AllocateDisk("disk-1", 60_GB);

        diskRegistry.ChangeDeviceState("uuid-2", NProto::DEVICE_STATE_ERROR);
        diskRegistry.ChangeAgentState("agent-2", NProto::AGENT_STATE_UNAVAILABLE);

        TVector<TString> toErase;

        runtime->SetObserverFunc([&](TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
            switch (event->GetTypeRewrite()) {
                case TEvDiskRegistryPrivate::EvSecureEraseRequest: {
                    auto& msg = *event->Get<TEvDiskRegistryPrivate::TEvSecureEraseRequest>();
                    for (auto& d: msg.DirtyDevices) {
                        toErase.push_back(d.GetDeviceUUID());
                    }
                    break;
                }
            }
            return TTestActorRuntime::DefaultObserverFunc(runtime, event);
        });

        auto waitForErase = [&] {
            runtime->AdvanceCurrentTime(TDuration::Seconds(5));
            TDispatchOptions options;
            options.FinalEvents = {
                TDispatchOptions::TFinalEventCondition(
                    TEvDiskRegistryPrivate::EvSecureEraseRequest)
            };
            runtime->DispatchEvents(options);
            TVector<TString> tmp;
            std::swap(toErase, tmp);
            Sort(tmp);
            return tmp;
        };

        diskRegistry.DeallocateDisk("disk-1", true);

        {
            auto uuids = waitForErase();
            UNIT_ASSERT_VALUES_EQUAL(2, uuids.size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-1", uuids[0]);
            UNIT_ASSERT_VALUES_EQUAL("uuid-3", uuids[1]);
        }

        diskRegistry.ChangeAgentState("agent-2", NProto::EAgentState::AGENT_STATE_ONLINE);
        {
            auto uuids = waitForErase();
            UNIT_ASSERT_VALUES_EQUAL(3, uuids.size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-4", uuids[0]);
            UNIT_ASSERT_VALUES_EQUAL("uuid-5", uuids[1]);
            UNIT_ASSERT_VALUES_EQUAL("uuid-6", uuids[2]);
        }

        diskRegistry.ChangeDeviceState("uuid-2", NProto::DEVICE_STATE_ONLINE);
        {
            auto uuids = waitForErase();
            UNIT_ASSERT_VALUES_EQUAL(1, uuids.size());
            UNIT_ASSERT_VALUES_EQUAL("uuid-2", uuids[0]);
        }
    }

    Y_UNIT_TEST(ShouldDetectBrokenDevices)
    {
        const auto agent = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB),
            Device("dev-3", "uuid-3", "rack-1", 10_GB),
            Device("dev-4", "uuid-4", "rack-1", 10_GB)
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({agent})
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);

        diskRegistry.WaitReady();
        diskRegistry.UpdateConfig(CreateRegistryConfig(0, {agent}));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 1);
        WaitForAgents(*runtime, 1);
        WaitForSecureErase(*runtime, {agent});

        TSet<TString> diskDevices;
        TSet<TString> freeDevices { "uuid-1", "uuid-2", "uuid-3", "uuid-4" };

        {
            auto response = diskRegistry.AllocateDisk("vol0", 20_GB);
            auto& r = response->Record;
            UNIT_ASSERT_EQUAL(NProto::VOLUME_IO_OK, r.GetIOMode());
            UNIT_ASSERT_UNEQUAL(0, r.GetIOModeTs());
            UNIT_ASSERT_VALUES_EQUAL(2, r.DevicesSize());
            for (const auto& d: r.GetDevices()) {
                diskDevices.insert(d.GetDeviceUUID());
                freeDevices.erase(d.GetDeviceUUID());
            }
        }

        UNIT_ASSERT_VALUES_EQUAL(2, diskDevices.size());
        UNIT_ASSERT_VALUES_EQUAL(2, freeDevices.size());

        {
            NProto::TAgentStats agentStats;
            agentStats.SetNodeId(runtime->GetNodeId(0));
            auto& device1 = *agentStats.AddDeviceStats();
            device1.SetDeviceUUID(*diskDevices.begin());
            device1.SetErrors(1); // break the disk
            auto& device2 = *agentStats.AddDeviceStats();
            device2.SetDeviceUUID(*std::next(diskDevices.begin()));
            device2.SetErrors(0);
            auto& device3 = *agentStats.AddDeviceStats();
            device3.SetDeviceUUID(*freeDevices.begin());
            device3.SetErrors(1);
            auto& device4 = *agentStats.AddDeviceStats();
            device4.SetDeviceUUID(*std::next(freeDevices.begin()));
            device4.SetErrors(0);
            diskRegistry.UpdateAgentStats(std::move(agentStats));
        }

        runtime->DispatchEvents(TDispatchOptions(), TDuration::Seconds(1));
        {
            // can't allocate disk with two devices
            diskRegistry.SendAllocateDiskRequest("vol1", 20_GB);
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(response->GetStatus(), E_BS_DISK_ALLOCATION_FAILED);
        }

        diskRegistry.AllocateDisk("vol1", 10_GB);
        {
            diskRegistry.SendAllocateDiskRequest("vol3", 10_GB);
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(response->GetStatus(), E_BS_DISK_ALLOCATION_FAILED);
        }

        {
            auto response = diskRegistry.DescribeDisk("vol0");
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());
            auto& r = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(2, r.DevicesSize());
            UNIT_ASSERT_EQUAL(NProto::DISK_STATE_ERROR, r.GetState());
        }
    }

    Y_UNIT_TEST(ShouldCountPublishDiskStateErrors)
    {
        TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1.1", "rack-1", 10_GB),
            }),
        };

        auto logbrokerService = std::make_shared<TTestLogbrokerService>(
            MakeError(E_FAIL, "Test")
        );

        auto runtime = TTestRuntimeBuilder()
            .With(logbrokerService)
            .WithAgents(agents)
            .Build();

        NMonitoring::TDynamicCountersPtr counters = new NMonitoring::TDynamicCounters();
        InitCriticalEventsCounter(counters);
        auto publishDiskStateError =
            counters->GetCounter("AppCriticalEvents/PublishDiskStateError", true);

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, agents));
        diskRegistry.AllowDiskAllocation();

        RegisterAndWaitForAgents(*runtime, agents);

        diskRegistry.AllocateDisk("disk-1", 10_GB);

        UNIT_ASSERT_VALUES_EQUAL(0, logbrokerService->GetItemCount());
        UNIT_ASSERT_VALUES_EQUAL(0, publishDiskStateError->Val());

        {
            NProto::TAction action;
            action.SetHost("agent-1");
            action.SetType(NProto::TAction::REMOVE_HOST);

            diskRegistry.CmsAction(TVector{ action });
        }

        // speed up test
        runtime->AdvanceCurrentTime(TDuration::Seconds(10));

        if (!logbrokerService->GetItemCount()) {

            TDispatchOptions options;
            options.CustomFinalCondition = [&] {
                return logbrokerService->GetItemCount();
            };

            runtime->DispatchEvents(options);
        }

        UNIT_ASSERT_VALUES_UNEQUAL(0, logbrokerService->GetItemCount());

        UNIT_ASSERT_VALUES_EQUAL(
            logbrokerService->GetItemCount(),
            publishDiskStateError->Val()
        );
    }

    Y_UNIT_TEST(ShouldRegisterUnavailableAgentAfterTimeout)
    {
        auto defaultConfig = CreateDefaultStorageConfig();

        auto agent = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB)
        });
        agent.SetSeqNumber(1);
        agent.SetNodeId(42);

        auto runtime = TTestRuntimeBuilder()
            .With(defaultConfig)
            .Build();

        {
            TDiskRegistryClient diskRegistry(*runtime);
            diskRegistry.WaitReady();

            diskRegistry.UpdateConfig(CreateRegistryConfig(0, { agent }));
            diskRegistry.AllowDiskAllocation();

            diskRegistry.SendRegisterAgentRequest(agent);
            auto response = diskRegistry.RecvRegisterAgentResponse();
            UNIT_ASSERT_C(SUCCEEDED(response->GetStatus()), response->GetErrorReason());
        }
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        runtime->AdvanceCurrentTime(TDuration::MilliSeconds(
            defaultConfig.GetNonReplicatedAgentTimeout() / 2));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();
        agent.SetSeqNumber(0);
        agent.SetNodeId(100);

        {
            diskRegistry.SendRegisterAgentRequest(agent);
            auto response = diskRegistry.RecvRegisterAgentResponse();
            UNIT_ASSERT_C(FAILED(response->GetStatus()), response->GetErrorReason());
        }
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        runtime->AdvanceCurrentTime(TDuration::MilliSeconds(
            defaultConfig.GetNonReplicatedAgentTimeout() / 2));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        {
            diskRegistry.SendRegisterAgentRequest(agent);
            auto response = diskRegistry.RecvRegisterAgentResponse();
            UNIT_ASSERT_C(SUCCEEDED(response->GetStatus()), response->GetErrorReason());
        }
    }

    Y_UNIT_TEST(ShouldNotUnregisterNewAgentAfterKillPreviousAgent)
    {
        auto defaultConfig = CreateDefaultStorageConfig();

        auto agent = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB)
        });
        agent.SetSeqNumber(0);
        agent.SetNodeId(42);

        auto runtime = TTestRuntimeBuilder()
            .With(defaultConfig)
            .Build();

        auto diskRegistry0 = std::make_unique<TDiskRegistryClient>(*runtime);
        diskRegistry0->WaitReady();

        diskRegistry0->UpdateConfig(CreateRegistryConfig(0, { agent }));
        diskRegistry0->AllowDiskAllocation();

        {
            diskRegistry0->SendRegisterAgentRequest(agent);
            auto response = diskRegistry0->RecvRegisterAgentResponse();
            UNIT_ASSERT_C(SUCCEEDED(response->GetStatus()), response->GetErrorReason());
        }
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        TDiskRegistryClient diskRegistry1(*runtime);
        diskRegistry1.WaitReady();
        agent.SetSeqNumber(1);
        agent.SetNodeId(100);

        {
            diskRegistry1.SendRegisterAgentRequest(agent);
            auto response = diskRegistry1.RecvRegisterAgentResponse();
            UNIT_ASSERT_C(SUCCEEDED(response->GetStatus()), response->GetErrorReason());
        }
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        diskRegistry0.reset();
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        runtime->AdvanceCurrentTime(TDuration::MilliSeconds(
            defaultConfig.GetNonReplicatedAgentTimeout()));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        agent.SetSeqNumber(0);
        agent.SetNodeId(42);
        {
            TDiskRegistryClient diskRegistry(*runtime);
            diskRegistry.WaitReady();
            diskRegistry.SendRegisterAgentRequest(agent);
            auto response = diskRegistry.RecvRegisterAgentResponse();
            UNIT_ASSERT_C(FAILED(response->GetStatus()), response->GetErrorReason());
        }
    }

    Y_UNIT_TEST(ShouldKillPreviousAgentAfterRegisterNewAgent)
    {
        auto agent = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB)
        });
        agent.SetSeqNumber(0);
        agent.SetNodeId(42);

        auto runtime = TTestRuntimeBuilder()
            .Build();

        size_t poisonPillCount = 0;
        runtime->SetObserverFunc([&](TTestActorRuntimeBase& rt, TAutoPtr<IEventHandle>& event) {
            switch (event->GetTypeRewrite()) {
                case TEvents::TSystem::PoisonPill:
                    ++poisonPillCount;
                    break;
            }

            return TTestActorRuntime::DefaultObserverFunc(rt, event);
        });

        TDiskRegistryClient diskRegistry0(*runtime);
        diskRegistry0.WaitReady();

        diskRegistry0.UpdateConfig(CreateRegistryConfig(0, { agent }));
        diskRegistry0.AllowDiskAllocation();

        {
            diskRegistry0.SendRegisterAgentRequest(agent);
            auto response = diskRegistry0.RecvRegisterAgentResponse();
            UNIT_ASSERT_C(SUCCEEDED(response->GetStatus()), response->GetErrorReason());
        }
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        TDiskRegistryClient diskRegistry1(*runtime);
        diskRegistry1.WaitReady();
        agent.SetSeqNumber(1);
        agent.SetNodeId(100);

        {
            poisonPillCount = 0;
            diskRegistry1.SendRegisterAgentRequest(agent);
            auto response = diskRegistry1.RecvRegisterAgentResponse();
            UNIT_ASSERT_C(SUCCEEDED(response->GetStatus()), response->GetErrorReason());
            UNIT_ASSERT_VALUES_EQUAL(1, poisonPillCount);
        }
    }

    Y_UNIT_TEST(ShouldMuteIOErrorsForTempUnavailableDisk)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1", "rack-1", 10_GB),
                Device("dev-2", "uuid-2", "rack-1", 10_GB)
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-3", "uuid-3", "rack-1", 10_GB),
                Device("dev-4", "uuid-4", "rack-1", 10_GB)
            })
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents(agents)
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, agents));
        diskRegistry.AllowDiskAllocation();

        RegisterAndWaitForAgents(*runtime, agents);

        {
            auto response = diskRegistry.AllocateDisk("disk-1", 40_GB);
            auto& r = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(4, r.DevicesSize());
            UNIT_ASSERT_EQUAL(NProto::VOLUME_IO_OK, r.GetIOMode());
            UNIT_ASSERT_VALUES_EQUAL(false, r.GetMuteIOErrors());
        }

        diskRegistry.ChangeAgentState(
            agents[0].GetAgentId(),
            NProto::AGENT_STATE_UNAVAILABLE);

        {
            auto response = diskRegistry.AllocateDisk("disk-1", 40_GB);
            auto& r = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(4, r.DevicesSize());
            UNIT_ASSERT_EQUAL(NProto::VOLUME_IO_OK, r.GetIOMode());
            UNIT_ASSERT_VALUES_EQUAL(true, r.GetMuteIOErrors());
        }
    }
}

}   // namespace NCloud::NBlockStore::NStorage
