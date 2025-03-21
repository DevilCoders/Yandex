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

#include <atomic>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;
using namespace NKikimr;
using namespace NDiskRegistryTest;

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TDiskRegistryTest)
{
    Y_UNIT_TEST(ShouldPassAccessModeAndMountSeqNumberInAcquireDevicesRequest)
    {
        const auto agentConfig = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB),
            Device("dev-3", "uuid-3", "rack-1", 10_GB),
            Device("dev-4", "uuid-4", "rack-1", 10_GB),
            Device("dev-5", "uuid-5", "rack-1", 10_GB)
        });

        THashSet<TString> acquiredDevices;

        auto agent = CreateTestDiskAgent(agentConfig);

        NProto::EVolumeAccessMode accessMode = NProto::VOLUME_ACCESS_READ_WRITE;
        ui64 mountSeqNumber = 0;

        agent->HandleAcquireDevicesImpl = [&] (
            const TEvDiskAgent::TEvAcquireDevicesRequest::TPtr& ev,
            const TActorContext& ctx)
        {
            Y_UNUSED(ctx);

            const auto& record = ev->Get()->Record;
            accessMode = record.GetAccessMode();
            mountSeqNumber = record.GetMountSeqNumber();

            NCloud::Reply(
                ctx,
                *ev,
                std::make_unique<TEvDiskAgent::TEvAcquireDevicesResponse>()
            );

            return true;
        };

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, {agentConfig}));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 1);
        WaitForAgents(*runtime, 1);
        WaitForSecureErase(*runtime, {agentConfig});

        diskRegistry.AllocateDisk("disk-1", 10_GB);

        diskRegistry.AcquireDisk(
            "disk-1",
            "session-1",
            NProto::VOLUME_ACCESS_READ_ONLY,
            1
        );

        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<int>(NProto::VOLUME_ACCESS_READ_ONLY),
            static_cast<int>(accessMode)
        );
        UNIT_ASSERT_VALUES_EQUAL(1, mountSeqNumber);

        diskRegistry.AcquireDisk(
            "disk-1",
            "session-1",
            NProto::VOLUME_ACCESS_READ_WRITE,
            2
        );

        UNIT_ASSERT_VALUES_EQUAL(
            static_cast<int>(NProto::VOLUME_ACCESS_READ_WRITE),
            static_cast<int>(accessMode)
        );
        UNIT_ASSERT_VALUES_EQUAL(2, mountSeqNumber);
    }

    Y_UNIT_TEST(ShouldCancelAcquireDisk)
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
            .WithAgents({
                CreateTestDiskAgent(agent1),
                CreateBrokenTestDiskAgent(agent2)
             })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, {agent1, agent2}));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 2);
        WaitForAgents(*runtime, 2);
        diskRegistry.CleanupDevices(TVector<TString>{
            "uuid-1", "uuid-2",
            "uuid-3", "uuid-4"
        });

        diskRegistry.AllocateDisk("disk-1", 40_GB);

        diskRegistry.SendAcquireDiskRequest("disk-1", "session-1");
        auto response = diskRegistry.RecvAcquireDiskResponse();
        UNIT_ASSERT_VALUES_EQUAL(response->GetStatus(), E_BS_INVALID_SESSION);
    }

    Y_UNIT_TEST(ShouldRespectAgentRequestTimeoutDuringAcquire)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB)
        });

        const auto agent2 = CreateAgentConfig("agent-2", {
            Device("dev-1", "uuid-3", "rack-1", 10_GB),
            Device("dev-2", "uuid-4", "rack-1", 10_GB)
        });

        auto active = std::make_shared<std::atomic<bool>>();
        active->store(true);

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({
                CreateTestDiskAgent(agent1),
                CreateSuspendedTestDiskAgent(agent2, active),
             })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, {agent1, agent2}));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 2);
        WaitForAgents(*runtime, 2);
        WaitForSecureErase(*runtime, {agent1, agent2});

        diskRegistry.AllocateDisk("disk-1", 40_GB);

        active->store(false);
        diskRegistry.SendAcquireDiskRequest("disk-1", "session-1");
        {
            auto response = diskRegistry.RecvAcquireDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(response->GetStatus(), E_REJECTED);
        }

        active->store(true);
        diskRegistry.SendAcquireDiskRequest("disk-1", "session-1");
        {
            auto response = diskRegistry.RecvAcquireDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(response->GetStatus(), S_OK);
        }
    }

    Y_UNIT_TEST(ShouldCancelPendingSessionsOnReboot)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
            Device("dev-2", "uuid-2", "rack-1", 10_GB)
        });

        auto active = std::make_shared<std::atomic<bool>>();

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({
                CreateSuspendedTestDiskAgent(agent1, active)
             })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();
        diskRegistry.UpdateConfig(CreateRegistryConfig(0, {agent1}));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 1);
        WaitForAgents(*runtime, 1);
        WaitForSecureErase(*runtime, {agent1});

        diskRegistry.AllocateDisk("disk-1", 20_GB);
        diskRegistry.SendAcquireDiskRequest("disk-1", "session-1");

        runtime->AdvanceCurrentTime(TDuration::Seconds(1));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        diskRegistry.RebootTablet();
        diskRegistry.WaitReady();

        runtime->AdvanceCurrentTime(TDuration::Seconds(1));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));
        *active = 1;

        auto response = diskRegistry.RecvAcquireDiskResponse();
        UNIT_ASSERT_VALUES_EQUAL(
            response->GetStatus(),
            E_REJECTED);

        diskRegistry.AcquireDisk("disk-1", "session-1");
    }

    Y_UNIT_TEST(ShouldCancelSessionOnReRegisterAgent)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB)
        });

        const auto agent2 = CreateAgentConfig("agent-2", {
            Device("dev-1", "uuid-2", "rack-1", 10_GB)
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

        diskRegistry.AllocateDisk("disk-1", 20_GB);
        diskRegistry.AcquireDisk("disk-1", "session-1");

        RegisterAgent(*runtime, 0);

        diskRegistry.AcquireDisk("disk-1", "session-2"); // OK: session-1 has been released
    }

    Y_UNIT_TEST(ShouldAcquireDisk)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("test", "uuid-1", "rack-1", 10_GB)
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

        {
            auto response = diskRegistry.AllocateDisk("disk-1", 10_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(msg.DevicesSize(), 1);
            UNIT_ASSERT_VALUES_EQUAL(
                msg.GetDevices(0).GetNodeId(),
                runtime->GetNodeId(0));
        }

        {
            auto response = diskRegistry.AcquireDisk("disk-1", "session-1");
            const auto& msg = response->Record;

            UNIT_ASSERT_VALUES_EQUAL(1, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetDevices(0).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetDevices(0).GetBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(
                msg.GetDevices(0).GetNodeId(),
                runtime->GetNodeId(0));
        }

        diskRegistry.ReleaseDisk("disk-1", "session-1");
    }

    Y_UNIT_TEST(ShouldNotSendAcquireReleaseRequestsToUnavailableAgents)
    {
        const auto agent1 = CreateAgentConfig("agent-1", {
            Device("test", "uuid-1", "rack-1", 10_GB)
        });

        const auto agent2 = CreateAgentConfig("agent-2", {
            Device("test", "uuid-2", "rack-2", 10_GB)
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

        {
            auto response = diskRegistry.AllocateDisk("disk-1", 20_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(msg.DevicesSize(), 2);
            UNIT_ASSERT_VALUES_EQUAL(
                msg.GetDevices(0).GetNodeId(),
                runtime->GetNodeId(0));

            UNIT_ASSERT_VALUES_EQUAL(
                msg.GetDevices(1).GetNodeId(),
                runtime->GetNodeId(1));
        }

        auto breakAgent = [&] (const TString& agentId) {
            diskRegistry.ChangeAgentState(
                agentId,
                NProto::AGENT_STATE_UNAVAILABLE
            );
            runtime->AdvanceCurrentTime(TDuration::Seconds(20));
            runtime->DispatchEvents({}, TDuration::MilliSeconds(10));
        };

        breakAgent("agent-1");

        {
            auto response = diskRegistry.AcquireDisk("disk-1", "session-1");
            const auto& msg = response->Record;

            UNIT_ASSERT_VALUES_EQUAL(1, msg.DevicesSize());

            UNIT_ASSERT_VALUES_EQUAL(
                DefaultLogicalBlockSize,
                msg.GetDevices(0).GetBlockSize());

            UNIT_ASSERT_VALUES_EQUAL(
                10_GB / DefaultLogicalBlockSize,
                msg.GetDevices(0).GetBlocksCount());

            UNIT_ASSERT_VALUES_EQUAL(
                msg.GetDevices(0).GetNodeId(),
                runtime->GetNodeId(1));
        }

        diskRegistry.ReleaseDisk("disk-1", "session-1");

        breakAgent("agent-2");

        {
            auto response = diskRegistry.AcquireDisk("disk-1", "session-1");
            const auto& msg = response->Record;

            UNIT_ASSERT_VALUES_EQUAL(0, msg.DevicesSize());
        }

        diskRegistry.ReleaseDisk("disk-1", "session-1");
    }

    Y_UNIT_TEST(ShouldAcquireReleaseSession)
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

        diskRegistry.AllocateDisk("disk-1", 20_GB);

        {
            auto response = diskRegistry.StartAcquireDisk("disk-1", "session-1");
            UNIT_ASSERT(!HasError(response->GetError()));
            UNIT_ASSERT_VALUES_EQUAL(response->Devices.size(), 2);
        }

        {
            auto response = diskRegistry.FinishAcquireDisk("disk-1", "session-1");
            UNIT_ASSERT(!HasError(response->GetError()));
        }

        {
            auto response = diskRegistry.RemoveDiskSession("disk-1", "session-1");
            UNIT_ASSERT(!HasError(response->GetError()));
        }
    }

    Y_UNIT_TEST(ShouldHandleUndeliveredAcquire)
    {
        auto agent = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB)
        });
        agent.SetNodeId(42);

        auto runtime = TTestRuntimeBuilder().Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig({agent}));
        diskRegistry.AllowDiskAllocation();

        diskRegistry.RegisterAgent(agent);
        diskRegistry.CleanupDevices(TVector<TString>{"uuid-1"});

        diskRegistry.AllocateDisk("disk-1", 10_GB);
        diskRegistry.SendAcquireDiskRequest("disk-1", "session-1");

        auto response = diskRegistry.RecvAcquireDiskResponse();
        UNIT_ASSERT_VALUES_EQUAL(
            response->GetStatus(),
            E_REJECTED);
    }

    Y_UNIT_TEST(ShouldHandleTimeoutedStartSession)
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

        diskRegistry.AllocateDisk("disk-1", 20_GB);

        TAutoPtr<IEventHandle> startAcquireDiskResp;
        TAutoPtr<IEventHandle> finishAcquireDiskReq;

        auto observerFunc = runtime->SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvDiskRegistryPrivate::EvStartAcquireDiskResponse: {
                        startAcquireDiskResp = std::move(event);
                        return TTestActorRuntime::EEventAction::DROP;
                    }
                    case TEvDiskRegistryPrivate::EvFinishAcquireDiskRequest: {
                        finishAcquireDiskReq = std::move(event);
                        return TTestActorRuntime::EEventAction::DROP;
                    }
                }

                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        diskRegistry.SendAcquireDiskRequest("disk-1", "session-1");
        runtime->AdvanceCurrentTime(TDuration::Seconds(1));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        while (!finishAcquireDiskReq) {
            runtime->AdvanceCurrentTime(TDuration::Seconds(1));
            runtime->DispatchEvents({}, TDuration::MilliSeconds(10));
        }

        runtime->SetObserverFunc(observerFunc);

        runtime->Send(startAcquireDiskResp.Release());
        runtime->Send(finishAcquireDiskReq.Release());

        auto response = diskRegistry.RecvAcquireDiskResponse();
        UNIT_ASSERT_VALUES_EQUAL(E_REJECTED, response->GetError().GetCode());
    }

    Y_UNIT_TEST(ShouldFailReleaseSessionIfDiskRegistryRestarts)
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

        diskRegistry.AllocateDisk("disk-1", 20_GB);

        {
            auto response = diskRegistry.StartAcquireDisk("disk-1", "session-1");
            UNIT_ASSERT(!HasError(response->GetError()));
            UNIT_ASSERT_VALUES_EQUAL(response->Devices.size(), 2);
        }

        {
            auto response = diskRegistry.FinishAcquireDisk("disk-1", "session-1");
            UNIT_ASSERT(!HasError(response->GetError()));
        }

        auto observerFunc = runtime->SetObserverFunc(
        [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
            switch (event->GetTypeRewrite()) {
                case TEvDiskAgent::EvReleaseDevicesRequest: {
                    return TTestActorRuntime::EEventAction::DROP;
                }
            }

            return TTestActorRuntime::DefaultObserverFunc(runtime, event);
        });

        diskRegistry.SendReleaseDiskRequest("disk-1", "session-1");

        runtime->AdvanceCurrentTime(TDuration::Seconds(1));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));
        diskRegistry.RebootTablet();
        diskRegistry.WaitReady();

        auto response = diskRegistry.RecvReleaseDiskResponse();
        UNIT_ASSERT_VALUES_EQUAL(
            response->GetStatus(),
            E_REJECTED);
    }

    Y_UNIT_TEST(ShouldRejectTimedOutReleaseDisk)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1.1", "rack-1", 10_GB),
                Device("dev-2", "uuid-1.2", "rack-1", 10_GB)
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-1", "uuid-2.1", "rack-1", 10_GB),
                Device("dev-2", "uuid-2.2", "rack-1", 10_GB)
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

        diskRegistry.AllocateDisk("disk-1", 40_GB);
        diskRegistry.AcquireDisk("disk-1", "session-1");

        auto observerFunc = runtime->SetObserverFunc(
        [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
            switch (event->GetTypeRewrite()) {
                case TEvDiskAgent::EvReleaseDevicesRequest: {
                    return TTestActorRuntime::EEventAction::DROP;
                }
            }

            return TTestActorRuntime::DefaultObserverFunc(runtime, event);
        });

        diskRegistry.SendReleaseDiskRequest("disk-1", "session-1");

        runtime->AdvanceCurrentTime(TDuration::Seconds(5));
        runtime->DispatchEvents({}, TDuration::MilliSeconds(10));

        auto response = diskRegistry.RecvReleaseDiskResponse();
        UNIT_ASSERT_VALUES_EQUAL(
            response->GetStatus(),
            E_TIMEOUT);

        runtime->SetObserverFunc(observerFunc);
        diskRegistry.ReleaseDisk("disk-1", "session-1");
    }
}

}   // namespace NCloud::NBlockStore::NStorage
