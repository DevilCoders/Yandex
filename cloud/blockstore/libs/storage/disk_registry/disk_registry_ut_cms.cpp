#include "disk_registry.h"
#include "disk_registry_actor.h"

#include <cloud/blockstore/config/disk.pb.h>

#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/storage/api/disk_agent.h>
#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/api/volume_proxy.h>
#include <cloud/blockstore/libs/storage/disk_registry/testlib/test_env.h>
#include <cloud/blockstore/libs/storage/disk_registry/testlib/test_logbroker.h>
#include <cloud/blockstore/libs/storage/testlib/ss_proxy_client.h>
#include <cloud/blockstore/libs/storage/testlib/ut_helpers.h>

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
    Y_UNIT_TEST(ShouldRemoveAgentsUponCmsRequest)
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

        RegisterAndWaitForAgents(*runtime, {agent});

        TString freeDevice;
        TString diskDevice;
        {
            auto response = diskRegistry.AllocateDisk("vol1", 10_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(1, msg.DevicesSize());
            diskDevice = msg.GetDevices(0).GetDeviceUUID();

            freeDevice = diskDevice == "uuid-1"
                ? "uuid-2"
                : "uuid-1";
        }

        NProto::TAction action;
        action.SetHost("agent-1");
        action.SetType(NProto::TAction::REMOVE_HOST);
        TVector<NProto::TAction> actions;
        actions.push_back(action);

        ui32 cmsTimeout = 0;
        {
            auto response = diskRegistry.CmsAction(actions);
            UNIT_ASSERT_VALUES_EQUAL(
                E_TRY_AGAIN,
                response->Record.GetActionResults(0).GetResult().GetCode());
            cmsTimeout = response->Record.GetActionResults(0).GetTimeout();
            UNIT_ASSERT_VALUES_UNEQUAL(0, cmsTimeout);
        }

        // request will be ignored
        diskRegistry.ChangeAgentState("agent-1", NProto::EAgentState::AGENT_STATE_ONLINE);

        {
            diskRegistry.SendAllocateDiskRequest("vol2", 10_GB);
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, response->GetStatus());
        }

        runtime->AdvanceCurrentTime(TDuration::Days(1) / 2);

        {
            auto response = diskRegistry.CmsAction(actions);
            UNIT_ASSERT_VALUES_EQUAL(
                E_TRY_AGAIN,
                response->Record.GetActionResults(0).GetResult().GetCode());
            auto curTimeout = response->Record.GetActionResults(0).GetTimeout();
            UNIT_ASSERT_VALUES_UNEQUAL(0, curTimeout);
            UNIT_ASSERT_LT(curTimeout, cmsTimeout);
        }

        runtime->AdvanceCurrentTime(TDuration::Days(1) / 2);

        {
            auto response = diskRegistry.CmsAction(actions);
            UNIT_ASSERT_VALUES_EQUAL(
                E_TRY_AGAIN,
                response->Record.GetActionResults(0).GetResult().GetCode());
            auto curTimeout = response->Record.GetActionResults(0).GetTimeout();
            UNIT_ASSERT_VALUES_UNEQUAL(0, curTimeout);
            UNIT_ASSERT_VALUES_EQUAL(curTimeout, cmsTimeout);
        }

        diskRegistry.ChangeDeviceState(freeDevice, NProto::EDeviceState::DEVICE_STATE_ERROR);

        {
            auto response = diskRegistry.CmsAction(actions);
            UNIT_ASSERT_VALUES_EQUAL(
                E_TRY_AGAIN,
                response->Record.GetActionResults(0).GetResult().GetCode());
            auto curTimeout = response->Record.GetActionResults(0).GetTimeout();
            UNIT_ASSERT_VALUES_UNEQUAL(0, curTimeout);
        }

        diskRegistry.ChangeDeviceState(diskDevice, NProto::EDeviceState::DEVICE_STATE_ERROR);

        {
            auto response = diskRegistry.CmsAction(actions);
            UNIT_ASSERT_VALUES_EQUAL(
                S_OK,
                response->Record.GetActionResults(0).GetResult().GetCode());
            UNIT_ASSERT(response->Record.GetActionResults(0).GetTimeout() == 0);
        }

        diskRegistry.ChangeAgentState("agent-1", NProto::EAgentState::AGENT_STATE_UNAVAILABLE);
        diskRegistry.ChangeAgentState("agent-1", NProto::EAgentState::AGENT_STATE_ONLINE);

        diskRegistry.ChangeDeviceState("uuid-1", NProto::EDeviceState::DEVICE_STATE_ONLINE);
        diskRegistry.ChangeDeviceState("uuid-2", NProto::EDeviceState::DEVICE_STATE_ONLINE);

        diskRegistry.AllocateDisk("vol2", 10_GB);
    }

    Y_UNIT_TEST(ShouldFailCmsRequestIfActionIsUnknown)
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

        NProto::TAction action;
        action.SetHost("agent-1");
        action.SetType(NProto::TAction::UNKNOWN);
        TVector<NProto::TAction> actions;
        actions.push_back(action);
        auto response = diskRegistry.CmsAction(std::move(actions));
        UNIT_ASSERT_VALUES_EQUAL(
            E_ARGUMENT,
            response->Record.GetActionResults(0).GetResult().GetCode());
    }

    Y_UNIT_TEST(ShouldFailCmsRequestIfAgentIsNotFound)
    {
        auto runtime = TTestRuntimeBuilder()
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        NProto::TAction action;
        action.SetHost("agent-1");
        action.SetType(NProto::TAction::REMOVE_HOST);
        TVector<NProto::TAction> actions;
        actions.push_back(action);
        auto response = diskRegistry.CmsAction(std::move(actions));
        UNIT_ASSERT_VALUES_EQUAL(
            E_NOT_FOUND,
            response->Record.GetActionResults(0).GetResult().GetCode());
    }

    Y_UNIT_TEST(ShouldFailCmsRequestIfDeviceNotFound)
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

        {
            NProto::TAction action;
            action.SetHost("agent-2");
            action.SetType(NProto::TAction::REMOVE_DEVICE);
            action.SetDevice("dev-1");
            TVector<NProto::TAction> actions;
            actions.push_back(action);
            auto response = diskRegistry.CmsAction(std::move(actions));
            UNIT_ASSERT_VALUES_EQUAL(
                E_NOT_FOUND,
                response->Record.GetActionResults(0).GetResult().GetCode());
        }

        {
            NProto::TAction action;
            action.SetHost("agent-1");
            action.SetType(NProto::TAction::REMOVE_DEVICE);
            action.SetDevice("dev-10");
            TVector<NProto::TAction> actions;
            actions.push_back(action);
            auto response = diskRegistry.CmsAction(std::move(actions));
            UNIT_ASSERT_VALUES_EQUAL(
                E_NOT_FOUND,
                response->Record.GetActionResults(0).GetResult().GetCode());
        }
    }

    Y_UNIT_TEST(ShouldRemoveDeviceUponCmsRequest)
    {
        const auto agent = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
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

        diskRegistry.AllocateDisk("vol1", 10_GB);

        NProto::TAction action;
        action.SetHost("agent-1");
        action.SetDevice("dev-1");
        action.SetType(NProto::TAction::REMOVE_DEVICE);
        TVector<NProto::TAction> actions;
        actions.push_back(action);

        ui32 cmsTimeout = 0;
        {
            auto response = diskRegistry.CmsAction(actions);
            UNIT_ASSERT_VALUES_EQUAL(
                E_TRY_AGAIN,
                response->Record.GetActionResults(0).GetResult().GetCode());
            cmsTimeout = response->Record.GetActionResults(0).GetTimeout();
            UNIT_ASSERT_VALUES_UNEQUAL(0, cmsTimeout);
        }

        // request will be ignored
        diskRegistry.ChangeDeviceState("uuid-1", NProto::EDeviceState::DEVICE_STATE_ONLINE);

        {
            auto response = diskRegistry.AllocateDisk("vol1", 10_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(1, msg.DevicesSize());
            const auto& device = msg.GetDevices(0);

            UNIT_ASSERT_VALUES_EQUAL("uuid-1", device.GetDeviceUUID());
            UNIT_ASSERT_VALUES_EQUAL(
                static_cast<ui32>(NProto::DEVICE_STATE_WARNING),
                static_cast<ui32>(device.GetState()));
        }

        runtime->AdvanceCurrentTime(TDuration::Days(1) / 2);

        {
            auto response = diskRegistry.CmsAction(actions);
            UNIT_ASSERT_VALUES_EQUAL(
                E_TRY_AGAIN,
                response->Record.GetActionResults(0).GetResult().GetCode());
            auto curTimeout = response->Record.GetActionResults(0).GetTimeout();
            UNIT_ASSERT_VALUES_UNEQUAL(0, curTimeout);
            UNIT_ASSERT(curTimeout < cmsTimeout);
        }

        runtime->AdvanceCurrentTime(TDuration::Days(1) / 2);

        {
            auto response = diskRegistry.CmsAction(actions);
            UNIT_ASSERT_VALUES_EQUAL(
                E_TRY_AGAIN,
                response->Record.GetActionResults(0).GetResult().GetCode());
            cmsTimeout = response->Record.GetActionResults(0).GetTimeout();
            UNIT_ASSERT_VALUES_UNEQUAL(0, cmsTimeout);
        }

        diskRegistry.ChangeDeviceState("uuid-1", NProto::EDeviceState::DEVICE_STATE_ERROR);

        {
            auto response = diskRegistry.CmsAction(actions);
            UNIT_ASSERT_VALUES_EQUAL(
                S_OK,
                response->Record.GetActionResults(0).GetResult().GetCode());
            UNIT_ASSERT(response->Record.GetActionResults(0).GetTimeout() == 0);
        }

        diskRegistry.ChangeDeviceState("uuid-1", NProto::EDeviceState::DEVICE_STATE_ONLINE);

        {
            auto response = diskRegistry.AllocateDisk("vol1", 10_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(1, msg.DevicesSize());
            const auto& device = msg.GetDevices(0);

            UNIT_ASSERT_VALUES_EQUAL("uuid-1", device.GetDeviceUUID());
            UNIT_ASSERT_EQUAL(NProto::DEVICE_STATE_ONLINE, device.GetState());
        }
    }

    Y_UNIT_TEST(ShouldReturnOkIfRemovedDeviceHasNoDisks)
    {
        const auto agent = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB),
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

        NProto::TAction action;
        action.SetHost("agent-1");
        action.SetDevice("dev-1");
        action.SetType(NProto::TAction::REMOVE_DEVICE);
        TVector<NProto::TAction> actions;
        actions.push_back(action);

        ui32 cmsTimeout = 0;
        {
            auto response = diskRegistry.CmsAction(actions);
            UNIT_ASSERT_VALUES_EQUAL(
                S_OK,
                response->Record.GetActionResults(0).GetResult().GetCode());
            cmsTimeout = response->Record.GetActionResults(0).GetTimeout();
            UNIT_ASSERT_VALUES_EQUAL(0, cmsTimeout);
        }

        {
            diskRegistry.SendAllocateDiskRequest("vol1", 10_GB);
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, response->GetStatus());
        }

        runtime->AdvanceCurrentTime(TDuration::Days(1) / 2);
        {
            auto response = diskRegistry.CmsAction(actions);
            UNIT_ASSERT_VALUES_EQUAL(
                S_OK,
                response->Record.GetActionResults(0).GetResult().GetCode());
            cmsTimeout = response->Record.GetActionResults(0).GetTimeout();
            UNIT_ASSERT_VALUES_EQUAL(0, cmsTimeout);
        }

        diskRegistry.ChangeDeviceState("uuid-1", NProto::EDeviceState::DEVICE_STATE_ERROR);
        diskRegistry.ChangeDeviceState("uuid-1", NProto::EDeviceState::DEVICE_STATE_ONLINE);

        diskRegistry.AllocateDisk("vol1", 10_GB);
    }

    Y_UNIT_TEST(ShouldFailCmsRequestIfDiskRegistryRestarts)
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

        NProto::TAction action;
        action.SetHost("agent-2");
        action.SetType(NProto::TAction::REMOVE_DEVICE);
        action.SetDevice("dev-1");
        TVector<NProto::TAction> actions;
        actions.push_back(action);

        runtime->SetObserverFunc(
        [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
            switch (event->GetTypeRewrite()) {
                case TEvDiskRegistryPrivate::EvUpdateCmsHostDeviceStateRequest: {
                    return TTestActorRuntime::EEventAction::DROP;
                }
            }

            return TTestActorRuntime::DefaultObserverFunc(runtime, event);
        });

        diskRegistry.SendCmsActionRequest(std::move(actions));

        runtime->AdvanceCurrentTime(TDuration::Seconds(1));
        runtime->DispatchEvents({}, TDuration::MicroSeconds(40));
        diskRegistry.RebootTablet();
        diskRegistry.WaitReady();

        auto response = diskRegistry.RecvCmsActionResponse();

        UNIT_ASSERT_VALUES_EQUAL(
            E_REJECTED,
            response->Record.GetError().GetCode());
    }

    void ShouldRestoreCMSTimeoutAfterReboot(NProto::TAction action)
    {
        const auto agent = CreateAgentConfig("agent-1", {
            Device("dev-1", "uuid-1", "rack-1", 10_GB)
        });

        auto runtime = TTestRuntimeBuilder()
            .WithAgents({ agent })
            .Build();

        TDiskRegistryClient diskRegistry(*runtime);
        diskRegistry.WaitReady();

        diskRegistry.UpdateConfig(CreateRegistryConfig(0, { agent }));
        diskRegistry.AllowDiskAllocation();

        RegisterAgents(*runtime, 1);
        WaitForAgents(*runtime, 1);
        WaitForSecureErase(*runtime, {agent});

        diskRegistry.AllocateDisk("vol1", 10_GB);

        auto tryRemove = [&] {
            TVector<NProto::TAction> actions{ action };

            auto response = diskRegistry.CmsAction(actions);
            return response->Record.GetActionResults(0);
        };

        ui32 cmsTimeout = 0;
        {
            auto r = tryRemove();

            UNIT_ASSERT_VALUES_EQUAL(E_TRY_AGAIN, r.GetResult().GetCode());
            UNIT_ASSERT_VALUES_UNEQUAL(0, r.GetTimeout());

            cmsTimeout = r.GetTimeout();
        }

        runtime->AdvanceCurrentTime(TDuration::Days(1) / 2);

        {
            auto r = tryRemove();

            UNIT_ASSERT_VALUES_EQUAL(E_TRY_AGAIN, r.GetResult().GetCode());
            UNIT_ASSERT_VALUES_UNEQUAL(0, r.GetTimeout());
            UNIT_ASSERT_GT(cmsTimeout, r.GetTimeout());
        }

        diskRegistry.RebootTablet();
        diskRegistry.WaitReady();

        {
            auto r = tryRemove();

            UNIT_ASSERT_VALUES_EQUAL(E_TRY_AGAIN, r.GetResult().GetCode());
            UNIT_ASSERT_VALUES_UNEQUAL(0, r.GetTimeout());
            UNIT_ASSERT_GT(cmsTimeout, r.GetTimeout());
        }

        runtime->AdvanceCurrentTime(TDuration::Days(1) / 2);

        {
            auto r = tryRemove();

            UNIT_ASSERT_VALUES_EQUAL(E_TRY_AGAIN, r.GetResult().GetCode());
            UNIT_ASSERT_VALUES_UNEQUAL(0, r.GetTimeout());
            UNIT_ASSERT_VALUES_EQUAL(cmsTimeout, r.GetTimeout());
        }

        diskRegistry.DeallocateDisk("vol1", true);

        {
            auto r = tryRemove();

            UNIT_ASSERT_VALUES_EQUAL(S_OK, r.GetResult().GetCode());
            UNIT_ASSERT_VALUES_EQUAL(0, r.GetTimeout());
        }
    }

    Y_UNIT_TEST(ShouldRestoreAgentCMSTimeoutAfterReboot)
    {
        NProto::TAction action;
        action.SetHost("agent-1");
        action.SetType(NProto::TAction::REMOVE_HOST);

        ShouldRestoreCMSTimeoutAfterReboot(action);
    }

    Y_UNIT_TEST(ShouldRestoreDeviceCMSTimeoutAfterReboot)
    {
        NProto::TAction action;
        action.SetHost("agent-1");
        action.SetDevice("dev-1");
        action.SetType(NProto::TAction::REMOVE_DEVICE);

        ShouldRestoreCMSTimeoutAfterReboot(action);
    }

    Y_UNIT_TEST(ShouldReturnOkForCmsRequestIfAgentDoesnotHaveUserDisks)
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

        NProto::TAction action;
        action.SetHost("agent-1");
        action.SetType(NProto::TAction::REMOVE_HOST);
        TVector<NProto::TAction> actions;
        actions.push_back(action);

        ui32 cmsTimeout = 0;
        {
            auto response = diskRegistry.CmsAction(actions);
            UNIT_ASSERT_VALUES_EQUAL(
                S_OK,
                response->Record.GetActionResults(0).GetResult().GetCode());
            cmsTimeout = response->Record.GetActionResults(0).GetTimeout();
            UNIT_ASSERT_VALUES_EQUAL(0, cmsTimeout);
        }

        {
            diskRegistry.SendAllocateDiskRequest("vol1", 10_GB);
            auto response = diskRegistry.RecvAllocateDiskResponse();
            UNIT_ASSERT_VALUES_EQUAL(E_BS_DISK_ALLOCATION_FAILED, response->GetStatus());
        }

        runtime->AdvanceCurrentTime(TDuration::Days(1) / 2);
        {
            auto response = diskRegistry.CmsAction(actions);
            UNIT_ASSERT_VALUES_EQUAL(
                S_OK,
                response->Record.GetActionResults(0).GetResult().GetCode());
            cmsTimeout = response->Record.GetActionResults(0).GetTimeout();
            UNIT_ASSERT_VALUES_EQUAL(0, cmsTimeout);
        }

        diskRegistry.ChangeAgentState("agent-1", NProto::EAgentState::AGENT_STATE_UNAVAILABLE);
        diskRegistry.ChangeAgentState("agent-1", NProto::EAgentState::AGENT_STATE_ONLINE);

        diskRegistry.AllocateDisk("vol1", 10_GB);
    }

    Y_UNIT_TEST(ShouldReturnOkForCmsRequestIfDisksWereDeallocated)
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

        diskRegistry.AllocateDisk("vol1", 10_GB);

        NProto::TAction action;
        action.SetHost("agent-1");
        action.SetType(NProto::TAction::REMOVE_HOST);
        TVector<NProto::TAction> actions;
        actions.push_back(action);

        ui32 cmsTimeout = 0;
        {
            auto response = diskRegistry.CmsAction(actions);
            UNIT_ASSERT_VALUES_EQUAL(
                E_TRY_AGAIN,
                response->Record.GetActionResults(0).GetResult().GetCode());
            cmsTimeout = response->Record.GetActionResults(0).GetTimeout();
            UNIT_ASSERT_VALUES_UNEQUAL(0, cmsTimeout);
        }

        diskRegistry.DeallocateDisk("vol1", true);

        runtime->AdvanceCurrentTime(TDuration::Days(1) / 2);
        {
            auto response = diskRegistry.CmsAction(actions);
            UNIT_ASSERT_VALUES_EQUAL(
                S_OK,
                response->Record.GetActionResults(0).GetResult().GetCode());
            cmsTimeout = response->Record.GetActionResults(0).GetTimeout();
            UNIT_ASSERT_VALUES_EQUAL(0, cmsTimeout);
        }

        diskRegistry.ChangeAgentState("agent-1", NProto::EAgentState::AGENT_STATE_UNAVAILABLE);
        diskRegistry.ChangeAgentState("agent-1", NProto::EAgentState::AGENT_STATE_ONLINE);
    }

    Y_UNIT_TEST(ShouldAllowToTakeAwayAlreadyUnavailableAgents)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1", "rack-1", 10_GB)
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-1", "uuid-2", "rack-2", 10_GB)
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

        diskRegistry.AllocateDisk("vol1", 20_GB);
        diskRegistry.ChangeAgentState("agent-1", NProto::EAgentState::AGENT_STATE_UNAVAILABLE);

        TVector<NProto::TAction> actions {[] {
            NProto::TAction action;
            action.SetHost("agent-1");
            action.SetType(NProto::TAction::REMOVE_HOST);

            return action;
        }()};

        ui32 cmsTimeout = 0;
        {
            auto response = diskRegistry.CmsAction(actions);
            const auto& result = response->Record.GetActionResults(0);
            UNIT_ASSERT_VALUES_EQUAL(E_TRY_AGAIN, result.GetResult().GetCode());
            cmsTimeout = result.GetTimeout();
            UNIT_ASSERT_VALUES_UNEQUAL(0, cmsTimeout);
        }

        runtime->AdvanceCurrentTime(TDuration::Seconds(cmsTimeout / 2));

        {
            auto response = diskRegistry.CmsAction(actions);
            const auto& result = response->Record.GetActionResults(0);
            UNIT_ASSERT_VALUES_EQUAL(E_TRY_AGAIN, result.GetResult().GetCode());
            cmsTimeout = result.GetTimeout();
            UNIT_ASSERT_VALUES_UNEQUAL(0, cmsTimeout);
        }

        runtime->AdvanceCurrentTime(TDuration::Seconds(cmsTimeout + 1));

        {
            auto response = diskRegistry.CmsAction(actions);
            const auto& result = response->Record.GetActionResults(0);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, result.GetResult().GetCode());
            cmsTimeout = result.GetTimeout();
            UNIT_ASSERT_VALUES_EQUAL(0, cmsTimeout);
        }

        // Take away empty agent without timeout

        diskRegistry.ChangeAgentState("agent-2", NProto::EAgentState::AGENT_STATE_UNAVAILABLE);

        actions = {[] {
            NProto::TAction action;
            action.SetHost("agent-2");
            action.SetType(NProto::TAction::REMOVE_HOST);

            return action;
        }()};

        {
            auto response = diskRegistry.CmsAction(actions);
            const auto& result = response->Record.GetActionResults(0);
            UNIT_ASSERT_VALUES_EQUAL(E_TRY_AGAIN, result.GetResult().GetCode());
            cmsTimeout = result.GetTimeout();
            UNIT_ASSERT_VALUES_UNEQUAL(0, cmsTimeout);
        }

        diskRegistry.DeallocateDisk("vol1", true);

        {
            auto response = diskRegistry.CmsAction(actions);
            const auto& result = response->Record.GetActionResults(0);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, result.GetResult().GetCode());
            cmsTimeout = result.GetTimeout();
            UNIT_ASSERT_VALUES_EQUAL(0, cmsTimeout);
        }
    }

    Y_UNIT_TEST(ShouldAllowToTakeAwayAlreadyUnavailableDevice)
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

        diskRegistry.AllocateDisk("vol1", 20_GB);
        diskRegistry.ChangeDeviceState("uuid-1", NProto::EDeviceState::DEVICE_STATE_ERROR);

        NProto::TAction action;
        action.SetHost("agent-1");
        action.SetDevice("dev-1");
        action.SetType(NProto::TAction::REMOVE_DEVICE);
        TVector<NProto::TAction> actions;
        actions.push_back(action);

        ui32 cmsTimeout = 0;
        {
            auto response = diskRegistry.CmsAction(actions);
            UNIT_ASSERT_VALUES_EQUAL(
                S_OK,
                response->Record.GetActionResults(0).GetResult().GetCode());
            cmsTimeout = response->Record.GetActionResults(0).GetTimeout();
            UNIT_ASSERT_VALUES_EQUAL(0, cmsTimeout);
        }

        diskRegistry.ChangeDeviceState("uuid-1", NProto::EDeviceState::DEVICE_STATE_ONLINE);
    }

    Y_UNIT_TEST(ShouldGetDependentDisks)
    {
        const TVector agents {
            CreateAgentConfig("agent-1", {
                Device("dev-1", "uuid-1", "rack-1", 10_GB),
                Device("dev-2", "uuid-2", "rack-1", 10_GB),
            }),
            CreateAgentConfig("agent-2", {
                Device("dev-1", "uuid-3", "rack-2", 10_GB),
                Device("dev-2", "uuid-4", "rack-2", 10_GB),
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

        TString vol1Device;
        TString vol2Device;
        TString vol3Device;
        TString vol4Device;
        TVector<TString> agent1Disks;
        TVector<TString> agent2Disks;

        auto allocateDisk = [&] (const TString& diskId, TString& devName) {
            auto response = diskRegistry.AllocateDisk(diskId, 10_GB);
            const auto& msg = response->Record;
            UNIT_ASSERT_VALUES_EQUAL(1, msg.DevicesSize());
            devName = msg.GetDevices(0).GetDeviceName();
            if (msg.GetDevices(0).GetAgentId() == "agent-1") {
                agent1Disks.push_back(diskId);
            } else {
                agent2Disks.push_back(diskId);
            }
        };

        allocateDisk("vol1", vol1Device);
        allocateDisk("vol2", vol2Device);
        allocateDisk("vol3", vol3Device);
        allocateDisk("vol4", vol4Device);

        TVector<NProto::TAction> actions;
        {
            NProto::TAction action;
            action.SetHost("agent-1");
            action.SetType(NProto::TAction::GET_DEPENDENT_DISKS);
            actions.push_back(action);
        }

        {
            auto response = diskRegistry.CmsAction(actions);
            const auto& result = response->Record.GetActionResults(0);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, result.GetResult().GetCode());
            const auto& diskIds = result.GetDependentDisks();
            ASSERT_VECTOR_CONTENTS_EQUAL(diskIds, agent1Disks);
        }

        actions.back().SetHost("agent-2");

        {
            auto response = diskRegistry.CmsAction(actions);
            const auto& result = response->Record.GetActionResults(0);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, result.GetResult().GetCode());
            const auto& diskIds = result.GetDependentDisks();
            ASSERT_VECTOR_CONTENTS_EQUAL(diskIds, agent2Disks);
        }

        actions.back().SetHost("agent-3");

        {
            auto response = diskRegistry.CmsAction(actions);
            const auto& result = response->Record.GetActionResults(0);
            UNIT_ASSERT_VALUES_EQUAL(E_NOT_FOUND, result.GetResult().GetCode());
        }

        actions.back().SetHost("agent-1");
        actions.back().SetDevice(vol1Device);

        {
            auto response = diskRegistry.CmsAction(actions);
            const auto& result = response->Record.GetActionResults(0);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, result.GetResult().GetCode());
            const auto& diskIds = result.GetDependentDisks();
            ASSERT_VECTOR_CONTENTS_EQUAL(diskIds, TVector<TString>{"vol1"});
        }
    }
}

}   // namespace NCloud::NBlockStore::NStorage
