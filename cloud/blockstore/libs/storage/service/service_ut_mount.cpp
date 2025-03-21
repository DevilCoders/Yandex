#include "service_ut.h"

#include "service_events_private.h"

#include <cloud/blockstore/libs/encryption/encryption_test.h>
#include <cloud/blockstore/libs/encryption/encryptor.h>
#include <cloud/blockstore/libs/storage/api/disk_registry.h>
#include <cloud/blockstore/libs/storage/api/ss_proxy.h>
#include <cloud/blockstore/libs/storage/api/stats_service.h>
#include <cloud/blockstore/libs/storage/api/volume.h>
#include <cloud/blockstore/libs/storage/api/volume_proxy.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/disk_counters.h>
#include <cloud/blockstore/libs/storage/core/mount_token.h>

#include <cloud/storage/core/libs/api/hive_proxy.h>

#include <util/generic/guid.h>
#include <util/generic/size_literals.h>
#include <util/string/escape.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

using namespace NCloud::NStorage;

namespace {

////////////////////////////////////////////////////////////////////////////////

using EChangeBindingOp = TEvService::TEvChangeVolumeBindingRequest::EChangeBindingOp;

} // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TServiceMountVolumeTest)
{
    void DoTestShouldMountVolume(TTestEnv& env, ui32 nodeIdx)
    {
        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume();

        TString sessionId;
        {
            auto response = service.MountVolume();
            sessionId = response->Record.GetSessionId();
        }

        service.UnmountVolume(DefaultDiskId, sessionId);
    }

    Y_UNIT_TEST(ShouldMountVolume)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        DoTestShouldMountVolume(env, nodeIdx);
    }

    Y_UNIT_TEST(ShouldPerformReadWriteInRepairMode)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume();

        TString sessionId;
        {
            auto response = service.MountVolume(
                DefaultDiskId,
                "", // instanceId
                "", // token
                NProto::IPC_GRPC,
                NProto::VOLUME_ACCESS_REPAIR,
                NProto::VOLUME_MOUNT_LOCAL);
            sessionId = response->Record.GetSessionId();
        }

        service.WriteBlocks(DefaultDiskId, TBlockRange64(0), sessionId, 1);
        {
            auto response = service.ReadBlocks(DefaultDiskId, 0, sessionId);

            const auto& blocks = response->Record.GetBlocks();
            UNIT_ASSERT_EQUAL(1, blocks.BuffersSize());
            UNIT_ASSERT_VALUES_EQUAL(
                TString(DefaultBlockSize, 1),
                blocks.GetBuffers(0)
            );
        }

        service.UnmountVolume(DefaultDiskId, sessionId);
    }

    Y_UNIT_TEST(ShouldDisallowMultipleLocalVolumeMount)
    {
        TTestEnv env(1, 2);
        ui32 nodeIdx1 = SetupTestEnv(env);
        ui32 nodeIdx2 = SetupTestEnv(env);

        TServiceClient service1(env.GetRuntime(), nodeIdx1);
        service1.CreateVolume();
        service1.AssignVolume(DefaultDiskId, "foo", "bar");
        service1.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_LOCAL);

        for (ui32 nodeIdx: {nodeIdx1, nodeIdx2}) {
            TServiceClient service2(env.GetRuntime(), nodeIdx);
            service2.SendMountVolumeRequest(
                DefaultDiskId,
                "foo",
                "bar",
                NProto::IPC_GRPC,
                NProto::VOLUME_ACCESS_READ_ONLY,
                NProto::VOLUME_MOUNT_LOCAL);

            auto response = service2.RecvMountVolumeResponse();
            UNIT_ASSERT_C(
                FAILED(response->GetStatus()),
                "Mount volume request unexpectedly succeeded");
        }
    }

    Y_UNIT_TEST(ShouldDisallowMultipleMountWithReadWriteAccess)
    {
        TTestEnv env(1, 2);
        ui32 nodeIdx1 = SetupTestEnv(env);
        ui32 nodeIdx2 = SetupTestEnv(env);

        TServiceClient service1(env.GetRuntime(), nodeIdx1);
        service1.CreateVolume();
        service1.AssignVolume(DefaultDiskId, "foo", "bar");
        service1.MountVolume(DefaultDiskId, "foo", "bar");

        for (ui32 nodeIdx: {nodeIdx1, nodeIdx2}) {
            TServiceClient service2(env.GetRuntime(), nodeIdx);
            service2.SendMountVolumeRequest(
                DefaultDiskId,
                "foo",
                "bar",
                NProto::IPC_GRPC,
                NProto::VOLUME_ACCESS_READ_WRITE,
                NProto::VOLUME_MOUNT_REMOTE);

            auto response = service2.RecvMountVolumeResponse();
            UNIT_ASSERT_C(
                FAILED(response->GetStatus()),
                "Mount volume request unexpectedly succeeded");
        }
    }

    Y_UNIT_TEST(ShouldAllowReadWriteMountIfVolumeIsReadOnlyMounted)
    {
        TTestEnv env(1, 2);
        ui32 nodeIdx1 = SetupTestEnv(env);
        ui32 nodeIdx2 = SetupTestEnv(env);

        TServiceClient service1(env.GetRuntime(), nodeIdx1);
        service1.CreateVolume();
        service1.AssignVolume(DefaultDiskId, "foo", "bar");
        service1.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_LOCAL);

        for (ui32 nodeIdx: {nodeIdx1, nodeIdx2}) {
            for (auto accessMode: {NProto::VOLUME_ACCESS_READ_WRITE, NProto::VOLUME_ACCESS_REPAIR}) {
                TServiceClient service2(env.GetRuntime(), nodeIdx);
                auto response = service2.MountVolume(
                    DefaultDiskId,
                    "foo",
                    "bar",
                    NProto::IPC_GRPC,
                    accessMode,
                    NProto::VOLUME_MOUNT_REMOTE);
                auto sessionId = response->Record.GetSessionId();
                service2.UnmountVolume(DefaultDiskId, sessionId);
            }
        }
    }

    void DoTestShouldKeepSessionsAfterTabletRestart(
        TTestEnv& env,
        ui32 nodeIdx1,
        ui32 nodeIdx2)
    {
        auto& runtime = env.GetRuntime();

        TServiceClient service1(runtime, nodeIdx1);
        service1.CreateVolume();
        service1.AssignVolume(DefaultDiskId, "foo", "bar");
        auto response = service1.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL);
        TString sessionId = response->Record.GetSessionId();

        runtime.UpdateCurrentTime(runtime.GetCurrentTime() + TDuration::Seconds(10));

        service1.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL);

        TServiceClient service2(runtime, nodeIdx2);
        service2.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_REMOTE);

        service1.WriteBlocks(DefaultDiskId, TBlockRange64(0), sessionId);
    }

    Y_UNIT_TEST(ShouldKeepSessionsAfterTabletRestart)
    {
        TTestEnv env(1, 2);
        auto unmountClientsTimeout = TDuration::Seconds(10);
        ui32 nodeIdx1 = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);
        ui32 nodeIdx2 = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);
        DoTestShouldKeepSessionsAfterTabletRestart(env, nodeIdx1, nodeIdx2);
    }

    Y_UNIT_TEST(ShouldRemoveClientIfClientDoesNotReconnectAfterPipeReset)
    {
        TTestEnv env(1, 2);
        auto unmountClientsTimeout = TDuration::Seconds(9);
        ui32 nodeIdx1 = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);
        ui32 nodeIdx2 = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);

        ui64 volumeTabletId = 0;
        auto& runtime = env.GetRuntime();

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvSSProxy::EvDescribeVolumeResponse: {
                        auto* msg = event->Get<TEvSSProxy::TEvDescribeVolumeResponse>();
                        const auto& volumeDescription =
                            msg->PathDescription.GetBlockStoreVolumeDescription();
                        volumeTabletId = volumeDescription.GetVolumeTabletId();
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        TServiceClient service1(runtime, nodeIdx1);
        service1.CreateVolume();
        service1.AssignVolume(DefaultDiskId, "foo", "bar");

        service1.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_REMOTE);

        if (!volumeTabletId) {
            TDispatchOptions options;
            options.FinalEvents.emplace_back(TEvSSProxy::EvDescribeVolumeResponse);
            runtime.DispatchEvents(options);
        }

        UNIT_ASSERT(volumeTabletId);

        RebootTablet(runtime, volumeTabletId, service1.GetSender(), nodeIdx1);

        TServiceClient service2(runtime, nodeIdx2);

        // Shouldn't be able to mount the same volume with read-write
        // access immediately after the volume was rebooted
        service2.SendMountVolumeRequest(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_REMOTE);
        {
            auto response = service2.RecvMountVolumeResponse();
            UNIT_ASSERT(FAILED(response->GetStatus()));
        }

        runtime.UpdateCurrentTime(runtime.GetCurrentTime() + unmountClientsTimeout);

        // Should now be able to mount the volume as the first service
        // should have timed out
        service2.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_REMOTE);
    }

    Y_UNIT_TEST(ShouldAllowMultipleReadOnlyMountIfOnlyOneClientHasLocalMount)
    {
        TTestEnv env(1, 2);
        ui32 nodeIdx1 = SetupTestEnv(env);
        ui32 nodeIdx2 = SetupTestEnv(env);

        TServiceClient service1(env.GetRuntime(), nodeIdx1);
        service1.CreateVolume();
        service1.AssignVolume(DefaultDiskId, "foo", "bar");
        service1.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_LOCAL);

        for (ui32 nodeIdx: {nodeIdx1, nodeIdx2}) {
            TServiceClient service2(env.GetRuntime(), nodeIdx);
            auto response = service2.MountVolume(
                DefaultDiskId,
                "foo",
                "bar",
                NProto::IPC_GRPC,
                NProto::VOLUME_ACCESS_READ_ONLY,
                NProto::VOLUME_MOUNT_REMOTE);
            auto sessionId = response->Record.GetSessionId();
            service2.UnmountVolume(DefaultDiskId, sessionId);
        }
    }

    Y_UNIT_TEST(ShouldBumpClientsLastActivityTimeOnMountRequest)
    {
        static constexpr TDuration mountVolumeTimeout = TDuration::Seconds(3);

        NProto::TStorageServiceConfig storageServiceConfig;
        storageServiceConfig.SetInactiveClientsTimeout(mountVolumeTimeout.MilliSeconds());

        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env, std::move(storageServiceConfig));

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();
        service.AssignVolume();

        TString sessionId;
        {
            auto response = service.MountVolume();
            sessionId = response->Record.GetSessionId();
        }

        runtime.UpdateCurrentTime(runtime.GetCurrentTime() + mountVolumeTimeout);
        service.MountVolume();
        runtime.UpdateCurrentTime(runtime.GetCurrentTime() + mountVolumeTimeout * 0.05);

        // Wait for timeout check
        {
            TDispatchOptions options;
            options.FinalEvents.emplace_back(TEvServicePrivate::EvInactiveClientsTimeout);
            runtime.DispatchEvents(options);
        }

        // Should receive the same session id as before because volume
        // was not unmounted via timeout due to present ping
        TString newSessionId;
        {
            auto response = service.MountVolume();
            newSessionId = response->Record.GetSessionId();
        }

        UNIT_ASSERT(sessionId == newSessionId);
    }

    Y_UNIT_TEST(ShouldMountVolumeWhenFirstMounterTimesOutOnVolumeSide)
    {
        TTestEnv env(1, 2);
        auto unmountClientsTimeout = TDuration::Seconds(9);
        ui32 nodeIdx1 = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);
        ui32 nodeIdx2 = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);

        auto& runtime = env.GetRuntime();

        ui64 volumeTabletId = 0;
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvSSProxy::EvDescribeVolumeResponse: {
                        auto* msg = event->Get<TEvSSProxy::TEvDescribeVolumeResponse>();
                        const auto& volumeDescription =
                            msg->PathDescription.GetBlockStoreVolumeDescription();
                        volumeTabletId = volumeDescription.GetVolumeTabletId();
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        TServiceClient service1(runtime, nodeIdx1);
        service1.CreateVolume();
        service1.AssignVolume(DefaultDiskId, "foo", "bar");

        service1.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_REMOTE);

        if (!volumeTabletId) {
            TDispatchOptions options;
            options.FinalEvents.emplace_back(TEvSSProxy::EvDescribeVolumeResponse);
            runtime.DispatchEvents(options);
        }

        UNIT_ASSERT(volumeTabletId);

        RebootTablet(runtime, volumeTabletId, service1.GetSender(), nodeIdx1);

        TServiceClient service2(runtime, nodeIdx2);

        // Shouldn't be able to mount the same volume with read-write
        // access immediately after the volume was rebooted
        service2.SendMountVolumeRequest(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_REMOTE);
        {
            auto response = service2.RecvMountVolumeResponse();
            UNIT_ASSERT(FAILED(response->GetStatus()));
        }

        runtime.UpdateCurrentTime(runtime.GetCurrentTime() + unmountClientsTimeout);

        // Should now be able to mount the volume as the first service
        // should have timed out
        service2.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_REMOTE);
    }

    void DoTestShouldCauseRemountWhenVolumeIsStolen(TTestEnv& env, ui32 nodeIdx)
    {
        auto& runtime = env.GetRuntime();

        TActorId volumeActorId;
        TActorId startVolumeActorId;
        ui64 volumeTabletId = 0;
        bool startVolumeActorStopped = false;

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvServicePrivate::EvVolumeTabletStatus: {
                        auto* msg = event->Get<TEvServicePrivate::TEvVolumeTabletStatus>();
                        volumeActorId = msg->VolumeActor;
                        volumeTabletId = msg->TabletId;
                        startVolumeActorId = event->Sender;
                        break;
                    }
                    case TEvServicePrivate::EvStartVolumeActorStopped:
                        startVolumeActorStopped = true;
                        break;
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();
        service.AssignVolume(DefaultDiskId, "foo", "bar");

        TString sessionId;
        {
            auto response = service.MountVolume(DefaultDiskId, "foo", "bar");
            sessionId = response->Record.GetSessionId();
        }

        // Lock tablet to a different owner in hive
        {
            TActorId edge = runtime.AllocateEdgeActor();
            runtime.SendToPipe(env.GetHive(), edge, new TEvHive::TEvLockTabletExecution(volumeTabletId));
            auto reply = runtime.GrabEdgeEvent<TEvHive::TEvLockTabletExecutionResult>(edge);
            UNIT_ASSERT_VALUES_EQUAL(reply->Get()->Record.GetStatus(), NKikimrProto::OK);
        }

        // Wait until start volume actor is stopped
        if (!startVolumeActorStopped) {
            TDispatchOptions options;
            options.FinalEvents.emplace_back(TEvServicePrivate::EvStartVolumeActorStopped);
            runtime.DispatchEvents(options);
            UNIT_ASSERT(startVolumeActorStopped);
        }

        // Next write request should cause invalid session response
        {
            service.SendWriteBlocksRequest(
                DefaultDiskId,
                TBlockRange64(0, 1023),
                sessionId,
                char(1));
            auto response = service.RecvWriteBlocksResponse();
            UNIT_ASSERT_VALUES_EQUAL_C(
                E_BS_INVALID_SESSION,
                response->GetStatus(),
                response->GetErrorReason()
            );
        }

        // Should work again after remount
        {
            auto response = service.MountVolume(DefaultDiskId, "foo", "bar");
            sessionId = response->Record.GetSessionId();
        }

        service.WriteBlocks(DefaultDiskId, TBlockRange64(0, 1023), sessionId);
        service.UnmountVolume(DefaultDiskId, sessionId);
    }

    Y_UNIT_TEST(ShouldCauseRemountWhenVolumeIsStolen)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        DoTestShouldCauseRemountWhenVolumeIsStolen(env, nodeIdx);
    }

    Y_UNIT_TEST(ShouldDisallowMountByAnotherClient)
    {
        static constexpr TDuration mountVolumeTimeout = TDuration::Seconds(3);

        NProto::TStorageServiceConfig storageServiceConfig;
        storageServiceConfig.SetInactiveClientsTimeout(
            mountVolumeTimeout.MilliSeconds());

        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env, std::move(storageServiceConfig));

        auto& runtime = env.GetRuntime();

        TServiceClient service1(runtime, nodeIdx);
        service1.CreateVolume();
        service1.AssignVolume(DefaultDiskId, "foo", "bar");
        service1.MountVolume(DefaultDiskId, "foo", "bar");

        TServiceClient service2(runtime, nodeIdx);
        service2.SendMountVolumeRequest(DefaultDiskId, "foo", "bar");
        auto response = service2.RecvMountVolumeResponse();
        UNIT_ASSERT(response->GetStatus() == E_BS_MOUNT_CONFLICT);
    }

    Y_UNIT_TEST(ShouldProcessMountAndUnmountRequestsSequentially)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TVector<TServiceClient> services;
        size_t numServices = 16;
        services.reserve(numServices);

        enum class EEventType
        {
            ADD_CLIENT_REQUEST,
            ADD_CLIENT_RESPONSE,
            REMOVE_CLIENT_REQUEST,
            REMOVE_CLIENT_RESPONSE
        };

        TVector<EEventType> events;
        THashSet<TActorId> addClientRequestSenders;
        THashSet<TActorId> removeClientRequestSenders;

        // Collect events (considering deduplication of forwarding through pipe)
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvVolume::EvAddClientRequest: {
                        events.emplace_back(EEventType::ADD_CLIENT_REQUEST);
                        addClientRequestSenders.emplace(event->Sender);
                        break;
                    }
                    case TEvVolume::EvAddClientResponse: {
                        if (addClientRequestSenders.contains(event->Recipient)) {
                            events.emplace_back(EEventType::ADD_CLIENT_RESPONSE);
                        }
                        break;
                    }
                    case TEvVolume::EvRemoveClientRequest: {
                        events.emplace_back(EEventType::REMOVE_CLIENT_REQUEST);
                        removeClientRequestSenders.emplace(event->Sender);
                        break;
                    }
                    case TEvVolume::EvRemoveClientResponse: {
                        if (removeClientRequestSenders.contains(event->Recipient)) {
                            events.emplace_back(EEventType::REMOVE_CLIENT_RESPONSE);
                        }
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        // Send mount requests from half of services, then start sending
        // mount requests from the other half and also sending unmount requests
        // from the first half of services
        TString sessionId;
        for (size_t i = 0; i < numServices; ++i) {
            TServiceClient service(runtime, nodeIdx);
            if (i == 0) {
                service.CreateVolume();
                service.AssignVolume(DefaultDiskId, "foo", "bar");
                service.WaitForVolume();
            }

            service.SendMountVolumeRequest(
                DefaultDiskId,
                "foo",
                "bar",
                NProto::IPC_GRPC,
                NProto::VOLUME_ACCESS_READ_ONLY,
                NProto::VOLUME_MOUNT_REMOTE);

            services.emplace_back(service);

            if (i < numServices / 2) {
                continue;
            }

            // Need to receive at least one mount response for session id
            // before sending unmount requests
            if (!sessionId) {
                auto response = services[0].RecvMountVolumeResponse();
                UNIT_ASSERT_C(
                    SUCCEEDED(response->GetStatus()),
                    response->GetErrorReason());
                sessionId = response->Record.GetSessionId();
            }

            auto& prevService = services[i-numServices/2];
            prevService.SendUnmountVolumeRequest(DefaultDiskId, sessionId);
        }

        // Send unmount requests from the second half of services
        for (size_t i = numServices / 2; i < numServices; ++i) {
            auto& service = services[i];
            service.SendUnmountVolumeRequest(DefaultDiskId, sessionId);
        }

        // Receive mount and unmount responses
        for (size_t i = 0; i < numServices; ++i) {
            if (i != 0) {
                auto response = services[i].RecvMountVolumeResponse();
                UNIT_ASSERT_C(
                    SUCCEEDED(response->GetStatus()),
                    response->GetErrorReason());
            }

            auto response = services[i].RecvUnmountVolumeResponse();
            UNIT_ASSERT_C(
                SUCCEEDED(response->GetStatus()),
                response->GetErrorReason());
        }
    }

    Y_UNIT_TEST(ShouldFailVolumeMountIfDescribeVolumeFails)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();

        auto error = MakeError(E_ARGUMENT, "Error");
        ui32 counter = 0;

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvSSProxy::EvDescribeVolumeRequest: {
                        if (++counter != 2) {
                            break;
                        }
                        auto response = std::make_unique<TEvSSProxy::TEvDescribeVolumeResponse>(
                            error);
                        runtime.Send(
                            new IEventHandle(
                                event->Sender,
                                event->Recipient,
                                response.release(),
                                0, // flags
                                event->Cookie),
                            nodeIdx);
                        return TTestActorRuntime::EEventAction::DROP;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service.SendMountVolumeRequest();
        auto response = service.RecvMountVolumeResponse();
        UNIT_ASSERT(response->GetStatus() == error.GetCode());
        UNIT_ASSERT(response->GetErrorReason() == error.GetMessage());
    }

    Y_UNIT_TEST(ShouldFailMountVolumeIfUnableToSetupVolumeClient)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();

        auto error = MakeError(E_ARGUMENT, "Error");

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvSSProxy::EvDescribeVolumeRequest: {
                        auto response = std::make_unique<TEvSSProxy::TEvDescribeVolumeResponse>(
                            error);
                        runtime.Send(
                            new IEventHandle(
                                event->Sender,
                                event->Recipient,
                                response.release(),
                                0, // flags
                                event->Cookie),
                            nodeIdx);
                        return TTestActorRuntime::EEventAction::DROP;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service.SendMountVolumeRequest();
        auto response = service.RecvMountVolumeResponse();
        UNIT_ASSERT(FAILED(response->GetStatus()));
    }

    Y_UNIT_TEST(ShouldFailPendingMountVolumeIfUnableToSetupVolumeClient)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();

        auto error = MakeError(E_ARGUMENT, "Error");
        TActorId target;
        TActorId source;
        ui64 cookie = 0;
        ui32 eventCounter = 0;

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvSSProxy::EvDescribeVolumeRequest: {
                        target = event->Sender;
                        source = event->Recipient;
                        cookie = event->Cookie;
                        return TTestActorRuntime::EEventAction::DROP;
                    }
                    case TEvService::EvMountVolumeRequest: {
                        if (++eventCounter == 6) {
                            auto response = std::make_unique<TEvSSProxy::TEvDescribeVolumeResponse>(
                                error);
                            runtime.Send(
                                new IEventHandle(
                                    target,
                                    source,
                                    response.release(),
                                    0, // flags
                                    cookie),
                                nodeIdx);
                        }
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service.SendMountVolumeRequest();
        service.SendMountVolumeRequest();
        service.SendMountVolumeRequest();

        auto response1 = service.RecvMountVolumeResponse();
        UNIT_ASSERT(response1->GetStatus() == error.GetCode());
        UNIT_ASSERT(response1->GetErrorReason() == error.GetMessage());

        auto response2 = service.RecvMountVolumeResponse();
        UNIT_ASSERT(response2->GetStatus() == error.GetCode());
        UNIT_ASSERT(response2->GetErrorReason() == error.GetMessage());

        auto response3 = service.RecvMountVolumeResponse();
        UNIT_ASSERT(FAILED(response3->GetStatus()));
    }

    void FailVolumeMountIfDescribeVolumeReturnsWrongInfoCommon(
        std::function<void(NKikimrSchemeOp::TPathDescription&)> mutator)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();

        runtime.SetObserverFunc(
            [=] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvSSProxy::EvDescribeVolumeResponse: {
                        auto* msg = event->Get<TEvSSProxy::TEvDescribeVolumeResponse>();
                        auto& pathDescription = const_cast<NKikimrSchemeOp::TPathDescription&>(msg->PathDescription);
                        mutator(pathDescription);
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service.SendMountVolumeRequest();
        auto response = service.RecvMountVolumeResponse();
        UNIT_ASSERT(FAILED(response->GetStatus()));
        UNIT_ASSERT(response->GetErrorReason());
    }

    Y_UNIT_TEST(ShouldFailVolumeMountIfDescribeVolumeReturnsUnparsableMountToken)
    {
        FailVolumeMountIfDescribeVolumeReturnsWrongInfoCommon(
            [] (NKikimrSchemeOp::TPathDescription& pathDescription) {
                auto& volumeDescription = *pathDescription.MutableBlockStoreVolumeDescription();
                volumeDescription.SetMountToken("some random string");
            });
    }

    Y_UNIT_TEST(ShouldFailVolumeMountIfWrongMountTokenIsUsed)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();

        service.AssignVolume(DefaultDiskId, TString(), "some mount token");

        service.SendMountVolumeRequest(DefaultDiskId, TString(), "other mount token");
        auto response = service.RecvMountVolumeResponse();
        UNIT_ASSERT(response->GetStatus() == E_ARGUMENT);
        UNIT_ASSERT(response->GetErrorReason().Contains("Mount token"));
    }

    Y_UNIT_TEST(ShouldAllowMountRequestsFromCtrlPlaneWithoutMountToken)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service(
            runtime,
            nodeIdx,
            NProto::SOURCE_SECURE_CONTROL_CHANNEL);
        service.CreateVolume();

        service.AssignVolume(DefaultDiskId, TString(), "some mount token");

        service.MountVolume(DefaultDiskId);
    }

    Y_UNIT_TEST(ShouldRejectMountRequestsFromCtrlPlaneWithMountToken)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service(
            runtime,
            nodeIdx,
            NProto::SOURCE_SECURE_CONTROL_CHANNEL);
        service.CreateVolume();

        service.AssignVolume(DefaultDiskId, TString(), "some mount token");

        service.SendMountVolumeRequest(DefaultDiskId, TString(), "some mount token");
        auto response = service.RecvMountVolumeResponse();
        UNIT_ASSERT(response->GetStatus() == E_ARGUMENT);
        UNIT_ASSERT(response->GetErrorReason().Contains("Mount token"));
    }

    Y_UNIT_TEST(ShouldAllowUnmountOfNonMountedVolume)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        TServiceClient service11(env.GetRuntime(), nodeIdx);
        service11.CreateVolume();
        service11.WaitForVolume();

        {
            service11.SendUnmountVolumeRequest();
            auto response = service11.RecvUnmountVolumeResponse();
            UNIT_ASSERT_VALUES_EQUAL_C(
                S_ALREADY,
                response->GetStatus(),
                response->GetErrorReason()
            );
        }

        TString sessionId11;
        {
            auto response = service11.MountVolume();
            sessionId11 = response->Record.GetSessionId();

            service11.ReadBlocks(DefaultDiskId, 0, sessionId11);
        }

        TServiceClient service12(env.GetRuntime(), nodeIdx);

        TString sessionId12;
        {
            auto response = service12.MountVolume(
                    DefaultDiskId,
                    TString(),
                    TString(),
                    NProto::IPC_GRPC,
                    NProto::VOLUME_ACCESS_READ_ONLY,
                    NProto::VOLUME_MOUNT_REMOTE);
            sessionId12 = response->Record.GetSessionId();

            service12.ReadBlocks(DefaultDiskId, 0, sessionId12);
        }

        service11.UnmountVolume();
        service11.WaitForVolume();

        {
            service11.SendUnmountVolumeRequest();
            auto response = service11.RecvUnmountVolumeResponse();
            UNIT_ASSERT_VALUES_EQUAL_C(
                S_ALREADY,
                response->GetStatus(),
                response->GetErrorReason()
            );
        }

        service12.ReadBlocks(DefaultDiskId, 0, sessionId12);
    }

    Y_UNIT_TEST(ShouldFailUnmountIfRemoveClientFails)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();
        auto sessionId = service.MountVolume()->Record.GetSessionId();

        auto error = MakeError(E_ARGUMENT, "Error");

        runtime.SetObserverFunc(
            [=] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvVolume::EvRemoveClientResponse: {
                        auto response = std::make_unique<TEvVolume::TEvRemoveClientResponse>(
                            error);
                        runtime.Send(
                            new IEventHandle(
                                event->Recipient,
                                event->Sender,
                                response.release(),
                                0, // flags,
                                event->Cookie),
                            nodeIdx);
                        return TTestActorRuntime::EEventAction::DROP;
                    };
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service.SendUnmountVolumeRequest(DefaultDiskId, sessionId);
        auto response = service.RecvUnmountVolumeResponse();
        UNIT_ASSERT_VALUES_EQUAL_C(
            error.GetCode(),
            response->GetStatus(),
            response->GetErrorReason()
        );
        UNIT_ASSERT_VALUES_EQUAL(
            error.GetMessage(),
            response->GetErrorReason()
        );
    }

    Y_UNIT_TEST(ShouldFailUnmountIfStopVolumeFails)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();
        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();
        auto sessionId = service.MountVolume()->Record.GetSessionId();

        auto error = MakeError(E_ARGUMENT, "Error");

        runtime.SetObserverFunc(
            [=] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvServicePrivate::EvStopVolumeResponse: {
                        auto response = std::make_unique<TEvServicePrivate::TEvStopVolumeResponse>(
                            error);
                        runtime.Send(
                            new IEventHandle(
                                event->Recipient,
                                event->Sender,
                                response.release(),
                                0, // flags,
                                event->Cookie),
                            nodeIdx);
                        return TTestActorRuntime::EEventAction::DROP;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service.SendUnmountVolumeRequest(DefaultDiskId, sessionId);
        auto response = service.RecvUnmountVolumeResponse();
        UNIT_ASSERT_VALUES_EQUAL_C(
            error.GetCode(),
            response->GetStatus(),
            response->GetErrorReason()
        );
        UNIT_ASSERT_VALUES_EQUAL(
            error.GetMessage(),
            response->GetErrorReason()
        );
    }

    Y_UNIT_TEST(ShouldHandleMountAfterUnmountInFlight)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();

        TAutoPtr<IEventHandle> unmountProcessedEvent;
        ui32 mountVolumeRequestsCount = 0;
        ui32 unmountVolumeRequestsCount = 0;
        bool onceUnmountProcessedEventSent = false;

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvService::EvMountVolumeRequest: {
                        ++mountVolumeRequestsCount;
                        break;
                    }
                    case TEvService::EvUnmountVolumeRequest: {
                        ++unmountVolumeRequestsCount;
                        break;
                    }
                    case TEvServicePrivate::EvUnmountRequestProcessed: {
                        if (!onceUnmountProcessedEventSent && !unmountProcessedEvent) {
                            unmountProcessedEvent = event;
                            return TTestActorRuntime::EEventAction::DROP;
                        }
                        break;
                    }
                }
                if (!onceUnmountProcessedEventSent &&
                    mountVolumeRequestsCount == 4 &&
                    unmountVolumeRequestsCount == 2 &&
                    unmountProcessedEvent)
                {
                    onceUnmountProcessedEventSent = true;
                    runtime.Send(unmountProcessedEvent.Release(), nodeIdx);
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service.SendMountVolumeRequest();
        service.SendUnmountVolumeRequest();
        service.SendMountVolumeRequest();

        {
            auto response = service.RecvMountVolumeResponse();
            UNIT_ASSERT_VALUES_EQUAL_C(
                S_OK,
                response->GetStatus(),
                response->GetErrorReason()
            );
        }

        {
            auto response = service.RecvUnmountVolumeResponse();
            UNIT_ASSERT_VALUES_EQUAL_C(
                S_OK,
                response->GetStatus(),
                response->GetErrorReason()
            );
        }

        {
            auto response = service.RecvMountVolumeResponse();
            UNIT_ASSERT_VALUES_EQUAL_C(
                S_OK,
                response->GetStatus(),
                response->GetErrorReason()
            );
        }
    }

    Y_UNIT_TEST(ShouldReturnAlreadyMountedOnRemountIfNoMountOptionsChanged)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume();
        service.MountVolume();

        {
            auto response = service.MountVolume();
            // XXX this behaviour doesn't seem to be correct (qkrorlqr@)
            UNIT_ASSERT_VALUES_EQUAL_C(
                S_OK,   // not 'already' because of a pipe reset event
                        // that happens after the first AddClient for local
                        // mounts
                response->GetStatus(),
                response->GetErrorReason()
            );
        }

        {
            auto response = service.MountVolume();
            UNIT_ASSERT_VALUES_EQUAL_C(
                S_ALREADY,
                response->GetStatus(),
                response->GetErrorReason()
            );
        }
    }

    Y_UNIT_TEST(ShouldProcessRemountFromLocalToRemote)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();
        service.MountVolume();

        auto response = service.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_REMOTE);
        UNIT_ASSERT_VALUES_EQUAL_C(
            S_OK,
            response->GetStatus(),
            response->GetErrorReason()
        );  // Must not be S_ALREADY
    }

    Y_UNIT_TEST(ShouldProcessRemountFromReadWriteToReadOnly)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();
        service.MountVolume();

        auto response = service.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_LOCAL);
        UNIT_ASSERT_VALUES_EQUAL_C(
            S_OK,   // Must not be S_ALREADY
            response->GetStatus(),
            response->GetErrorReason()
        );
    }

    Y_UNIT_TEST(ShouldProcessRemountFromRemoteToLocalWithoutOtherClients)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();
        service.AssignVolume(DefaultDiskId, "foo", "bar");

        auto readWriteAccessOptions = {
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_ACCESS_REPAIR,
        };

        for (auto readWriteAccessOption: readWriteAccessOptions) {
            service.MountVolume(
                DefaultDiskId,
                "foo",
                "bar",
                NProto::IPC_GRPC,
                readWriteAccessOption,
                NProto::VOLUME_MOUNT_REMOTE);

            service.WaitForVolume();

            auto response = service.MountVolume(
                DefaultDiskId,
                "foo",
                "bar",
                NProto::IPC_GRPC,
                readWriteAccessOption,
                NProto::VOLUME_MOUNT_LOCAL);
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());  // Must not be S_ALREADY
        }
    }

    Y_UNIT_TEST(ShouldRejectRemountFromRemoteToLocalWithAnotherLocalMounter)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service1(runtime, nodeIdx);
        service1.CreateVolume();
        service1.AssignVolume(DefaultDiskId, "foo", "bar");
        service1.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_REMOTE);

        TServiceClient service2(runtime, nodeIdx);
        service2.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL);

        service1.SendMountVolumeRequest(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_LOCAL);
        auto response = service1.RecvMountVolumeResponse();
        UNIT_ASSERT(response->GetStatus() == E_BS_MOUNT_CONFLICT);
    }

    Y_UNIT_TEST(ShouldProcessRemountFromReadOnlyToReadWriteWithoutOtherClients)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();
        service.AssignVolume(DefaultDiskId, "foo", "bar");

        auto localMountOptions = {
            NProto::VOLUME_MOUNT_LOCAL,
            NProto::VOLUME_MOUNT_REMOTE
        };

        for (auto localMountOption: localMountOptions) {
            {
                auto mountResponse = service.MountVolume(
                    DefaultDiskId,
                    "foo",
                    "bar",
                    NProto::IPC_GRPC,
                    NProto::VOLUME_ACCESS_READ_ONLY,
                    localMountOption);

                if (localMountOption == NProto::VOLUME_MOUNT_REMOTE) {
                    service.WaitForVolume();
                }

                service.SendWriteBlocksRequest(
                    DefaultDiskId,
                    TBlockRange64(0, 1023),
                    mountResponse->Record.GetSessionId());

                auto writeResponse = service.RecvWriteBlocksResponse();
                UNIT_ASSERT_VALUES_EQUAL_C(
                    E_ARGUMENT,
                    writeResponse->GetStatus(),
                    writeResponse->GetErrorReason()
                );
            }

            TString sessionId;
            {
                auto mountResponse = service.MountVolume(
                    DefaultDiskId,
                    "foo",
                    "bar",
                    NProto::IPC_GRPC,
                    NProto::VOLUME_ACCESS_READ_WRITE,
                    localMountOption
                );

                service.SendWriteBlocksRequest(
                    DefaultDiskId,
                    TBlockRange64(0, 1023),
                    mountResponse->Record.GetSessionId());

                auto writeResponse = service.RecvWriteBlocksResponse();
                UNIT_ASSERT_VALUES_EQUAL_C(
                    S_OK,
                    writeResponse->GetStatus(),
                    writeResponse->GetErrorReason()
                );

                sessionId = mountResponse->Record.GetSessionId();
            }

            service.UnmountVolume(DefaultDiskId, sessionId);
        }
    }

    Y_UNIT_TEST(ShouldRejectRemountFromRemoteToLocalWithAnotherWriter)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service1(runtime, nodeIdx);
        service1.CreateVolume();
        service1.AssignVolume(DefaultDiskId, "foo", "bar");

        service1.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_REMOTE);

        TServiceClient service2(runtime, nodeIdx);
        service2.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_REMOTE);

        service1.SendMountVolumeRequest(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_REMOTE);
        auto response = service1.RecvMountVolumeResponse();
        UNIT_ASSERT(response->GetStatus() == E_BS_MOUNT_CONFLICT);
    }

    Y_UNIT_TEST(ShouldRejectRemountFromReadOnlyToReadWriteWithAnotherWriter)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service1(runtime, nodeIdx);
        service1.CreateVolume();
        service1.AssignVolume(DefaultDiskId, "foo", "bar");

        service1.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_REMOTE);

        TServiceClient service2(runtime, nodeIdx);
        service2.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_REMOTE);

        service1.SendMountVolumeRequest(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_REMOTE);
        auto response = service1.RecvMountVolumeResponse();
        UNIT_ASSERT(response->GetStatus() == E_BS_MOUNT_CONFLICT);
    }

    Y_UNIT_TEST(ShouldStopVolumeTabletAfterLocalStartBecauseOfTimedoutAddClient)
    {
        TTestEnv env(1, 2);
        auto unmountClientsTimeout = TDuration::Seconds(10);
        ui32 nodeIdx1 = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);
        auto& runtime = env.GetRuntime();

        ui32 numCalls = 0;
        ui32 stopSeen = 0;
        ui32 startSeen = 0;
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvVolume::EvAddClientRequest: {
                        if (!numCalls++) {
                            return TTestActorRuntime::EEventAction::DROP;
                        }
                        break;
                    }
                    case TEvServicePrivate::EvStopVolumeRequest: {
                        ++stopSeen;
                        break;
                    }
                    case TEvServicePrivate::EvStartVolumeRequest: {
                        ++startSeen;
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        TServiceClient service1(runtime, nodeIdx1);
        service1.CreateVolume();
        service1.AssignVolume(DefaultDiskId, "foo", "bar");

        auto response1 = service1.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_REMOTE);
        TString sessionId1 = response1->Record.GetSessionId();
        UNIT_ASSERT(startSeen == 1);
        UNIT_ASSERT(stopSeen == 1);

        service1.WaitForVolume(DefaultDiskId);
        service1.ReadBlocks(DefaultDiskId, 0, sessionId1);
    }

    Y_UNIT_TEST(ShouldReleaseVolumeTabletIfRemountFromLocalToRemote)
    {
        TTestEnv env;
        auto unmountClientsTimeout = TDuration::Seconds(10);
        ui32 nodeIdx = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();
        service.MountVolume();

        bool stopResponseSeen = false;
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvServicePrivate::EvStopVolumeResponse: {
                        stopResponseSeen = true;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        auto response = service.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_REMOTE);
        TString sessionId = response->Record.GetSessionId();
        UNIT_ASSERT(response->GetStatus() == S_OK);
        UNIT_ASSERT(stopResponseSeen == true);
    }

    Y_UNIT_TEST(ShouldHandleLocalMigrationScenario)
    {
        TTestEnv env;
        auto unmountClientsTimeout = TDuration::Seconds(10);
        ui32 nodeIdx = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);

        auto& runtime = env.GetRuntime();

        TServiceClient service_source(runtime, nodeIdx);
        TServiceClient service_target(runtime, nodeIdx);
        service_source.CreateVolume();
        service_source.MountVolume();

        auto response = service_source.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_REMOTE);
        TString sessionId = response->Record.GetSessionId();
        UNIT_ASSERT(response->GetStatus() == S_OK);

        service_source.UnmountVolume(DefaultDiskId, sessionId);

        response = service_target.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_REMOTE);
        UNIT_ASSERT(response->Record.GetVolume().GetBlocksCount() == DefaultBlocksCount);

        response = service_target.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL);
        UNIT_ASSERT(response->Record.GetVolume().GetBlocksCount() == DefaultBlocksCount);
    }

    Y_UNIT_TEST(ShouldHandleInterNodeMigrationScenario)
    {
        TTestEnv env(1, 2);
        auto unmountClientsTimeout = TDuration::Seconds(10);
        ui32 nodeIdx1 = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);
        ui32 nodeIdx2 = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);

        auto& runtime = env.GetRuntime();

        TServiceClient service_source(runtime, nodeIdx1);
        TServiceClient service_target(runtime, nodeIdx2);
        service_source.CreateVolume();
        service_source.MountVolume();

        auto response = service_source.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_REMOTE);
        TString sessionId = response->Record.GetSessionId();
        UNIT_ASSERT(response->GetStatus() == S_OK);
        UNIT_ASSERT(response->Record.GetVolume().GetBlocksCount() == DefaultBlocksCount);

        response = service_target.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_REMOTE);
        UNIT_ASSERT(response->Record.GetVolume().GetBlocksCount() == DefaultBlocksCount);

        service_source.UnmountVolume(DefaultDiskId, sessionId);

        response= service_target.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL);
        UNIT_ASSERT(response->Record.GetVolume().GetBlocksCount() == DefaultBlocksCount);
    }

    Y_UNIT_TEST(ShouldReturnTabletBackIfLocalMountedAndTabletWasStolen)
    {
        TTestEnv env;
        auto unmountClientsTimeout = TDuration::Seconds(10);
        ui32 nodeIdx = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);

        ui64 volumeTabletId = 0;
        bool startVolumeActorStopped = false;

        auto& runtime = env.GetRuntime();
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvServicePrivate::EvVolumeTabletStatus: {
                        auto *msg = event->Get<TEvServicePrivate::TEvVolumeTabletStatus>();
                        volumeTabletId = msg->TabletId;
                        break;
                    }
                    case TEvServicePrivate::EvStartVolumeActorStopped: {
                        startVolumeActorStopped = true;
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        auto fakeLocalMounter = runtime.AllocateEdgeActor();

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();
        service.MountVolume();

        UNIT_ASSERT(volumeTabletId);
        runtime.SendToPipe(env.GetHive(), fakeLocalMounter, new TEvHive::TEvLockTabletExecution(volumeTabletId));

        // Wait until start volume actor is stopped
        {
            TDispatchOptions options;
            options.FinalEvents.emplace_back(TEvServicePrivate::EvStartVolumeActorStopped);
            runtime.DispatchEvents(options);
            UNIT_ASSERT(startVolumeActorStopped);
        }

        bool volumeStarted = false;
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvServicePrivate::EvStartVolumeResponse: {
                        volumeStarted = true;
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service.MountVolume();
        UNIT_ASSERT(volumeStarted);
    }

    Y_UNIT_TEST(ShouldnotRevomeVolumeIfLastClientUnmountedWithError)
    {
        TTestEnv env;
        auto unmountClientsTimeout = TDuration::Seconds(10);
        ui32 nodeIdx = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();
        service.SetClientId("0");
        service.MountVolume();

        // Collect events (considering deduplication of forwarding through pipe)
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvVolume::EvRemoveClientRequest: {
                        auto error = MakeError(E_REJECTED, "");
                        auto response = std::make_unique<TEvVolume::TEvRemoveClientResponse>(error);
                        runtime.Send(
                            new IEventHandle(
                                event->Sender,
                                event->Recipient,
                                response.release(),
                                0, // flags
                                event->Cookie),
                            nodeIdx);
                        return TTestActorRuntime::EEventAction::DROP;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });


        auto response = service.MountVolume(DefaultDiskId);
        UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());
        UNIT_ASSERT_VALUES_EQUAL(DefaultBlocksCount, response->Record.GetVolume().GetBlocksCount());

        service.SendUnmountVolumeRequest(
            DefaultDiskId,
            response->Record.GetSessionId());

        auto unmountResponse = service.RecvUnmountVolumeResponse();
        UNIT_ASSERT(FAILED(unmountResponse->GetStatus()));

        service.SetClientId("1");

        service.SendMountVolumeRequest(DefaultDiskId);

        response = service.RecvMountVolumeResponse();
        UNIT_ASSERT(FAILED(response->GetStatus()));
        UNIT_ASSERT_VALUES_EQUAL(
            "Volume already has connection with read-write access: 0",
            response->GetErrorReason());
    }

    Y_UNIT_TEST(ShouldResetMountSeqNumberOnUnmount)
    {
        TTestEnv env(1, 2);
        auto unmountClientsTimeout = TDuration::Seconds(10);
        ui32 nodeIdx1 = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);
        ui32 nodeIdx2 = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);

        auto& runtime = env.GetRuntime();

        TServiceClient service1(runtime, nodeIdx1);
        TServiceClient service2(runtime, nodeIdx2);
        service1.CreateVolume();
        service1.MountVolume();

        auto response1 = service1.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL,
            true,
            0
        );
        UNIT_ASSERT_VALUES_EQUAL_C(
            S_OK,
            response1->GetStatus(),
            response1->GetErrorReason()
        );
        UNIT_ASSERT_VALUES_EQUAL(
            DefaultBlocksCount,
            response1->Record.GetVolume().GetBlocksCount()
        );

        service1.WriteBlocks(
            DefaultDiskId,
            TBlockRange64(0, 1023),
            response1->Record.GetSessionId());

        auto response2 = service2.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL,
            true,
            1);
        UNIT_ASSERT_VALUES_EQUAL(
            DefaultBlocksCount,
            response2->Record.GetVolume().GetBlocksCount()
        );

        service2.WriteBlocks(
            DefaultDiskId,
            TBlockRange64(0, 1023),
            response2->Record.GetSessionId());

        {
            service1.SendWriteBlocksRequest(
                DefaultDiskId,
                TBlockRange64(0, 1023),
                response1->Record.GetSessionId(),
                char(1));
            auto response = service1.RecvWriteBlocksResponse();
            UNIT_ASSERT_C(
                FAILED(response->GetStatus()),
                response->GetErrorReason()
            );
        }

        {
            service1.SendMountVolumeRequest(
                DefaultDiskId,
                "foo",
                "bar",
                NProto::IPC_GRPC,
                NProto::VOLUME_ACCESS_READ_WRITE,
                NProto::VOLUME_MOUNT_LOCAL);
            {
                auto response = service1.RecvMountVolumeResponse();
                UNIT_ASSERT_C(
                    FAILED(response->GetStatus()),
                    response->GetErrorReason()
                );
            }
        }

        service2.UnmountVolume(DefaultDiskId, response2->Record.GetSessionId());

        response1 = service1.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL,
            true,
            0);
        UNIT_ASSERT_VALUES_EQUAL(
            DefaultBlocksCount,
            response1->Record.GetVolume().GetBlocksCount()
        );

        service1.WriteBlocks(
            DefaultDiskId,
            TBlockRange64(0, 1023),
            response1->Record.GetSessionId()
        );
    }

    Y_UNIT_TEST(ShouldHandleMountRequestWithMountSeqNumber)
    {
        TTestEnv env(1, 2);
        auto unmountClientsTimeout = TDuration::Seconds(10);
        ui32 nodeIdx1 = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);
        ui32 nodeIdx2 = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);

        auto& runtime = env.GetRuntime();

        TServiceClient service1(runtime, nodeIdx1);
        TServiceClient service2(runtime, nodeIdx2);
        service1.CreateVolume();
        service1.MountVolume();

        auto response1 = service1.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL,
            true,
            0);
        UNIT_ASSERT_VALUES_EQUAL_C(
            S_OK,
            response1->GetStatus(),
            response1->GetErrorReason()
        );
        UNIT_ASSERT_VALUES_EQUAL(
            DefaultBlocksCount,
            response1->Record.GetVolume().GetBlocksCount()
        );

        service1.WriteBlocks(
            DefaultDiskId,
            TBlockRange64(0, 1023),
            response1->Record.GetSessionId()
        );

        auto response2 = service2.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL,
            true,
            1
        );
        UNIT_ASSERT_VALUES_EQUAL(
            DefaultBlocksCount,
            response2->Record.GetVolume().GetBlocksCount()
        );

        service2.WriteBlocks(
            DefaultDiskId,
            TBlockRange64(0, 1023),
            response2->Record.GetSessionId()
        );

        {
            service1.SendWriteBlocksRequest(
                DefaultDiskId,
                TBlockRange64(0, 1023),
                response1->Record.GetSessionId(),
                char(1)
            );
            auto response = service1.RecvWriteBlocksResponse();
            UNIT_ASSERT_C(
                FAILED(response->GetStatus()),
                response->GetErrorReason()
            );
        }

        {
            service1.SendMountVolumeRequest(
                DefaultDiskId,
                "foo",
                "bar",
                NProto::IPC_GRPC,
                NProto::VOLUME_ACCESS_READ_WRITE,
                NProto::VOLUME_MOUNT_LOCAL
            );

            {
                auto response = service1.RecvMountVolumeResponse();
                UNIT_ASSERT_C(
                    FAILED(response->GetStatus()),
                    response->GetErrorReason()
                );
            }
        }
    }

    Y_UNIT_TEST(ShouldReportMountSeqNumberInStatVolumeResponse)
    {
        TTestEnv env;
        auto unmountClientsTimeout = TDuration::Seconds(10);
        ui32 nodeIdx = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();
        service.MountVolume();

        auto response = service.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL,
            true,
            1
        );
        UNIT_ASSERT_VALUES_EQUAL_C(
            S_OK,
            response->GetStatus(),
            response->GetErrorReason()
        );
        UNIT_ASSERT_VALUES_EQUAL(
            DefaultBlocksCount,
            response->Record.GetVolume().GetBlocksCount()
        );

        auto statResponse = service.StatVolume(DefaultDiskId);
        UNIT_ASSERT_VALUES_EQUAL_C(
            1,
            statResponse->Record.GetMountSeqNumber(),
            statResponse->GetErrorReason()
        );
    }

    void ShouldProperlyReactToAcquireDiskError(NProto::EVolumeMountMode mountMode) {
        TTestEnv env;
        NProto::TStorageServiceConfig config;
        config.SetAllocationUnitNonReplicatedSSD(1);
        config.SetAcquireNonReplicatedDevices(true);
        ui32 nodeIdx = SetupTestEnv(env, config);
        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume(
            DefaultDiskId,
            1_GB / DefaultBlockSize,
            DefaultBlockSize,
            TString(), // folderId
            TString(), // cloudId
            NCloud::NProto::STORAGE_MEDIA_SSD_NONREPLICATED
        );

        int acquireResps = 0;
        env.GetRuntime().SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvDiskRegistry::EvAcquireDiskResponse: {
                        ++acquireResps;
                        auto* msg = event->Get<TEvDiskRegistry::TEvAcquireDiskResponse>();
                        msg->Record.Clear();
                        msg->Record.MutableError()->SetCode(E_FAIL);
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            }
        );

        {
            service.SendMountVolumeRequest(
                DefaultDiskId,
                "", // instanceId
                "", // token
                NProto::IPC_GRPC,
                NProto::VOLUME_ACCESS_READ_WRITE,
                mountMode
            );

            auto response = service.RecvMountVolumeResponse();
            UNIT_ASSERT_VALUES_EQUAL(1, acquireResps);
            UNIT_ASSERT_VALUES_EQUAL(E_FAIL, response->GetStatus());
        }

        env.GetRuntime().SetObserverFunc(TTestActorRuntime::DefaultObserverFunc);

        {
            service.SendMountVolumeRequest(
                DefaultDiskId,
                "", // instanceId
                "", // token
                NProto::IPC_GRPC,
                NProto::VOLUME_ACCESS_READ_WRITE,
                mountMode
            );

            auto response = service.RecvMountVolumeResponse();
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());
            const auto& volume = response->Record.GetVolume();
            UNIT_ASSERT_VALUES_EQUAL(8, volume.DevicesSize());
            UNIT_ASSERT_VALUES_EQUAL(
                "transport0",
                volume.GetDevices(0).GetTransportId()
            );
            UNIT_ASSERT_VALUES_EQUAL(
                128_MB / DefaultBlockSize,
                volume.GetDevices(0).GetBlockCount()
            );
        }
    }

    Y_UNIT_TEST(ShouldProperlyReactToAcquireDiskErrorLocal)
    {
        ShouldProperlyReactToAcquireDiskError(NProto::VOLUME_MOUNT_LOCAL);
    }

    Y_UNIT_TEST(ShouldProperlyReactToAcquireDiskErrorRemote)
    {
        ShouldProperlyReactToAcquireDiskError(NProto::VOLUME_MOUNT_REMOTE);
    }

    void DoTestShouldChangeVolumeBindingForMountedVolume(TTestEnv& env, ui32 nodeIdx)
    {
        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume();

        TString sessionId;
        {
            auto response = service.MountVolume();
            sessionId = response->Record.GetSessionId();
        }

        service.WaitForVolume(DefaultDiskId);
        service.ReadBlocks(DefaultDiskId, 0, sessionId);

        {
            auto request = std::make_unique<TEvService::TEvChangeVolumeBindingRequest>(
                DefaultDiskId,
                EChangeBindingOp::RELEASE_TO_HIVE,
                NProto::EPreemptionSource::SOURCE_INITIAL_MOUNT);
            service.SendRequest(MakeStorageServiceId(), std::move(request));
            auto response = service.RecvResponse<TEvService::TEvChangeVolumeBindingResponse>();
        }

        service.WaitForVolume(DefaultDiskId);
        service.ReadBlocks(DefaultDiskId, 0, sessionId);

        service.UnmountVolume(DefaultDiskId, sessionId);
    }

    Y_UNIT_TEST(ShouldChangeVolumeBindingForMountedVolume)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        DoTestShouldChangeVolumeBindingForMountedVolume(env, nodeIdx);
    }

    void DoTestShouldRejectChangeVolumeBindingForUnknownVolume(TTestEnv& env, ui32 nodeIdx)
    {
        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume();

        TString sessionId;
        {
            auto response = service.MountVolume();
            sessionId = response->Record.GetSessionId();
        }

        service.ReadBlocks(DefaultDiskId, 0, sessionId);

        {
            auto request = std::make_unique<TEvService::TEvChangeVolumeBindingRequest>(
                "Unknown",
                EChangeBindingOp::RELEASE_TO_HIVE,
                NProto::EPreemptionSource::SOURCE_INITIAL_MOUNT);
            service.SendRequest(MakeStorageServiceId(), std::move(request));
            auto response = service.RecvResponse<TEvService::TEvChangeVolumeBindingResponse>();
            UNIT_ASSERT_C(
                FAILED(response->GetStatus()),
                "ChangeVolumeBindingRequest request unexpectedly succeeded");
        }
        service.UnmountVolume(DefaultDiskId, sessionId);
    }

    Y_UNIT_TEST(ShouldRejectChangeVolumeBindingForUnknownVolume)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        DoTestShouldRejectChangeVolumeBindingForUnknownVolume(env, nodeIdx);
    }

    void DoTestShouldBringRemotelyMountedVolumeBack(TTestEnv& env, ui32 nodeIdx)
    {
        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume();

        TString sessionId;
        {
            auto response = service.MountVolume();
            sessionId = response->Record.GetSessionId();
        }

        service.WaitForVolume(DefaultDiskId);
        service.ReadBlocks(DefaultDiskId, 0, sessionId);

        {
            auto request = std::make_unique<TEvService::TEvChangeVolumeBindingRequest>(
                DefaultDiskId,
                EChangeBindingOp::RELEASE_TO_HIVE,
                NProto::EPreemptionSource::SOURCE_INITIAL_MOUNT);
            service.SendRequest(MakeStorageServiceId(), std::move(request));
            auto response = service.RecvResponse<TEvService::TEvChangeVolumeBindingResponse>();
        }

        service.WaitForVolume(DefaultDiskId);
        service.ReadBlocks(DefaultDiskId, 0, sessionId);

        {
            auto request = std::make_unique<TEvService::TEvChangeVolumeBindingRequest>(
                DefaultDiskId,
                EChangeBindingOp::ACQUIRE_FROM_HIVE,
                NProto::EPreemptionSource::SOURCE_INITIAL_MOUNT);
            service.SendRequest(MakeStorageServiceId(), std::move(request));
            auto response = service.RecvResponse<TEvService::TEvChangeVolumeBindingResponse>();
        }

        service.UnmountVolume(DefaultDiskId, sessionId);
    }

    Y_UNIT_TEST(ShouldBringRemotelyMountedVolumeBack)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        DoTestShouldBringRemotelyMountedVolumeBack(env, nodeIdx);
    }

    void DoTestShouldSucceedSettingRemoteBindingForRemotelyMountedVolume(
        TTestEnv& env,
        ui32 nodeIdx)
    {
        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume();

        TString sessionId;
        {
            auto response = service.MountVolume();
            sessionId = response->Record.GetSessionId();
        }

        service.WaitForVolume(DefaultDiskId);
        service.ReadBlocks(DefaultDiskId, 0, sessionId);

        {
            auto request = std::make_unique<TEvService::TEvChangeVolumeBindingRequest>(
                DefaultDiskId,
                EChangeBindingOp::RELEASE_TO_HIVE,
                NProto::EPreemptionSource::SOURCE_INITIAL_MOUNT);
            service.SendRequest(MakeStorageServiceId(), std::move(request));
            auto response = service.RecvResponse<TEvService::TEvChangeVolumeBindingResponse>();
        }

        service.WaitForVolume(DefaultDiskId);
        service.ReadBlocks(DefaultDiskId, 0, sessionId);

        {
            auto request = std::make_unique<TEvService::TEvChangeVolumeBindingRequest>(
                DefaultDiskId,
                EChangeBindingOp::ACQUIRE_FROM_HIVE,
                NProto::EPreemptionSource::SOURCE_INITIAL_MOUNT);
            service.SendRequest(MakeStorageServiceId(), std::move(request));
            auto response = service.RecvResponse<TEvService::TEvChangeVolumeBindingResponse>();
        }

        service.UnmountVolume(DefaultDiskId, sessionId);
    }

    void DoTestShouldAllowRemoteMountIfBindingSetToLocal(TTestEnv& env, ui32 nodeIdx)
    {
        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume();
        service.AssignVolume(DefaultDiskId, "foo", "bar");
        auto response = service.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL);

        {
            auto request = std::make_unique<TEvService::TEvChangeVolumeBindingRequest>(
                DefaultDiskId,
                EChangeBindingOp::ACQUIRE_FROM_HIVE,
                NProto::EPreemptionSource::SOURCE_BALANCER);
            service.SendRequest(MakeStorageServiceId(), std::move(request));
            auto response = service.RecvResponse<TEvService::TEvChangeVolumeBindingResponse>();
        }

        response = service.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_REMOTE);

        auto sessionId = response->Record.GetSessionId();
        service.UnmountVolume(DefaultDiskId, sessionId);

        response = service.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_REMOTE);

        response = service.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL);
    }

    Y_UNIT_TEST(ShouldAllowRemoteMountIfBindingSetToLocal)
    {
        TTestEnv env(1, 2);
        ui32 nodeIdx = SetupTestEnv(env);

        DoTestShouldAllowRemoteMountIfBindingSetToLocal(env, nodeIdx);
    }

    void DoTestShouldAllowLocalMountIfBindingSetToRemote(TTestEnv& env, ui32 nodeIdx)
    {
        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume();
        service.AssignVolume(DefaultDiskId, "foo", "bar");
        auto response = service.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_REMOTE);

        {
            auto request = std::make_unique<TEvService::TEvChangeVolumeBindingRequest>(
                DefaultDiskId,
                EChangeBindingOp::RELEASE_TO_HIVE,
                NProto::EPreemptionSource::SOURCE_BALANCER);
            service.SendRequest(MakeStorageServiceId(), std::move(request));
            auto response = service.RecvResponse<TEvService::TEvChangeVolumeBindingResponse>();
        }

        response = service.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL);

        auto sessionId = response->Record.GetSessionId();
        service.UnmountVolume(DefaultDiskId, sessionId);

        response = service.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_REMOTE);

        response = service.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL);
    }

    Y_UNIT_TEST(ShouldAllowLocalMountIfBindingSetToRemote)
    {
        TTestEnv env(1, 2);
        ui32 nodeIdx = SetupTestEnv(env);

        DoTestShouldAllowLocalMountIfBindingSetToRemote(env, nodeIdx);
    }

    Y_UNIT_TEST(ShouldUseVolumeClientForRequestsIfVolumeWasMounted)
    {
        TTestEnv env;
        auto unmountClientsTimeout = TDuration::Seconds(9);
        ui32 nodeIdx = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);

        ui64 volumeTabletId = 0;
        TActorId volumeActorId;
        auto& runtime = env.GetRuntime();

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvServicePrivate::EvVolumeTabletStatus: {
                        auto* msg = event->Get<TEvServicePrivate::TEvVolumeTabletStatus>();
                        volumeActorId = msg->VolumeActor;
                        volumeTabletId = msg->TabletId;
                        break;
                    }
                    case TEvSSProxy::EvDescribeVolumeResponse: {
                        auto* msg = event->Get<TEvSSProxy::TEvDescribeVolumeResponse>();
                        const auto& volumeDescription =
                            msg->PathDescription.GetBlockStoreVolumeDescription();
                        volumeTabletId = volumeDescription.GetVolumeTabletId();
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();
        service.AssignVolume(DefaultDiskId, "foo", "bar");

        TString sessionId;
        service.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL);

        if (!volumeTabletId) {
            TDispatchOptions options;
            options.FinalEvents.emplace_back(TEvSSProxy::EvDescribeVolumeResponse);
            runtime.DispatchEvents(options);
        }

        UNIT_ASSERT(volumeTabletId);
        UNIT_ASSERT(volumeActorId);

        runtime.Send(new IEventHandle(volumeActorId, service.GetSender(), new TEvents::TEvPoisonPill()), nodeIdx);
        TDispatchOptions rebootOptions;
        rebootOptions.FinalEvents.push_back(TDispatchOptions::TFinalEventCondition(TEvTablet::EvBoot, 1));
        runtime.DispatchEvents(rebootOptions);

        service.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL);

        service.GetChangedBlocks(DefaultDiskId, 0, 1);
    }

    Y_UNIT_TEST(ShouldUseVolumeClientForRequestsIfVolumeWasMountedRemotely)
    {
        TTestEnv env(1, 2);
        auto unmountClientsTimeout = TDuration::Seconds(9);
        ui32 nodeIdx1 = SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);

        SetupTestEnvWithMultipleMount(
            env,
            unmountClientsTimeout);

        ui64 volumeTabletId = 0;
        auto& runtime = env.GetRuntime();

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvSSProxy::EvDescribeVolumeResponse: {
                        auto* msg = event->Get<TEvSSProxy::TEvDescribeVolumeResponse>();
                        const auto& volumeDescription =
                            msg->PathDescription.GetBlockStoreVolumeDescription();
                        volumeTabletId = volumeDescription.GetVolumeTabletId();
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        TServiceClient service(runtime, nodeIdx1);
        service.CreateVolume();
        service.AssignVolume(DefaultDiskId, "foo", "bar");

        TString sessionId;
        service.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_REMOTE);

        if (!volumeTabletId) {
            TDispatchOptions options;
            options.FinalEvents.emplace_back(TEvSSProxy::EvDescribeVolumeResponse);
            runtime.DispatchEvents(options);
        }

        UNIT_ASSERT(volumeTabletId);

        RebootTablet(runtime, volumeTabletId, service.GetSender(), nodeIdx1);

        service.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_REMOTE);

        service.WaitForVolume(DefaultDiskId);
        service.GetChangedBlocks(DefaultDiskId, 0, 1);
    }

    Y_UNIT_TEST(ShouldFailVolumeMountIfWrongEncryptionKeyHashIsUsed)
    {
        TEncryptionKeyFile keyFile(
            "0123456789012345678901234567890123456789012345678901234567890123");

        NProto::TEncryptionSpec encryptionSpec;
        auto& encryptionKey = *encryptionSpec.MutableKey();
        encryptionKey.SetMode(NProto::ENCRYPTION_AES_XTS);
        encryptionKey.MutableKeyPath()->SetFilePath(keyFile.GetPath());

        auto keyHashOrError = ComputeEncryptionKeyHash(encryptionKey);
        UNIT_ASSERT_C(!HasError(keyHashOrError), keyHashOrError.GetError());
        const auto& encryptionKeyHash = keyHashOrError.GetResult();

        TTestEnv env;
        NProto::TStorageServiceConfig config;
        config.SetDefaultTabletVersion(2);
        ui32 nodeIdx = SetupTestEnv(env, std::move(config));

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);

        {
            service.SendCreateVolumeRequest(
                DefaultDiskId,
                DefaultBlocksCount,
                DefaultBlockSize,
                "",
                "",
                NCloud::NProto::STORAGE_MEDIA_DEFAULT,
                NProto::TVolumePerformanceProfile(),
                "",
                0,
                encryptionSpec);
            auto response = service.RecvCreateVolumeResponse();
            UNIT_ASSERT_VALUES_EQUAL(S_OK, response->GetStatus());
        }

        {
            service.SendMountVolumeRequest(
                DefaultDiskId,
                "",
                "",
                NProto::IPC_GRPC,
                NProto::VOLUME_ACCESS_READ_WRITE,
                NProto::VOLUME_MOUNT_LOCAL,
                false,
                0,
                "WrongTestKeyHash");
            auto response = service.RecvMountVolumeResponse();
            UNIT_ASSERT_VALUES_EQUAL_C(
                E_ARGUMENT,
                response->GetStatus(),
                response->GetErrorReason()
            );
            UNIT_ASSERT_C(
                response->GetErrorReason().Contains("Encryption key hash"),
                response->GetErrorReason()
            );
        }

        {
            service.SendMountVolumeRequest(
                DefaultDiskId,
                "",
                "",
                NProto::IPC_GRPC,
                NProto::VOLUME_ACCESS_READ_WRITE,
                NProto::VOLUME_MOUNT_LOCAL,
                false,
                0,
                encryptionKeyHash);
            auto response = service.RecvMountVolumeResponse();
            UNIT_ASSERT_VALUES_EQUAL_C(
                S_OK,
                response->GetStatus(),
                response->GetErrorReason()
            );
        }
    }

    Y_UNIT_TEST(ShouldRemoveVolumeFromServiceEvenIfUnmountFailed)
    {
        static constexpr TDuration mountVolumeTimeout = TDuration::Seconds(1);

        NProto::TStorageServiceConfig storageServiceConfig;
        storageServiceConfig.SetInactiveClientsTimeout(mountVolumeTimeout.MilliSeconds());

        TTestEnv env;
        ui32 nodeIdx1 = SetupTestEnv(env, std::move(storageServiceConfig));

        auto& runtime = env.GetRuntime();
        bool dieSeen = false;
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvVolume::EvRemoveClientRequest: {
                        auto msg = std::make_unique<TEvVolume::TEvRemoveClientResponse>(
                            MakeError(E_REJECTED, "Tablet is dead"));
                        runtime.Send(
                            new IEventHandle(
                                event->Sender,
                                event->Recipient,
                                msg.release(),
                                0, // flags
                                event->Cookie),
                            nodeIdx1);
                        return TTestActorRuntime::EEventAction::DROP;
                    }
                    case TEvServicePrivate::EvSessionActorDied: {
                        dieSeen = true;
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        TServiceClient service1(env.GetRuntime(), nodeIdx1);
        service1.CreateVolume(
            DefaultDiskId,
            512,
            DefaultBlockSize,
            "foo",
            "bar"
        );

        service1.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_LOCAL
        );

        service1.DestroyVolume(DefaultDiskId);

        {
            TDispatchOptions options;
            options.FinalEvents.emplace_back(
                TEvService::EvUnregisterVolume);
            UNIT_ASSERT_VALUES_EQUAL(
                true,
                runtime.DispatchEvents(options));
        }

        {
            TDispatchOptions options;
            options.FinalEvents.emplace_back(
                TEvService::EvUnmountVolumeRequest);
            options.FinalEvents.emplace_back(
                TEvServicePrivate::EvSessionActorDied);
            UNIT_ASSERT_VALUES_EQUAL(
                true,
                runtime.DispatchEvents(options));
        }

        UNIT_ASSERT_VALUES_EQUAL(
            true,
            dieSeen);
    }

    Y_UNIT_TEST(ShouldAllowToIncreaseSeqNumWithNonLocalReadOnlyMounts)
    {
        TTestEnv env(1, 2);
        ui32 nodeIdx1 = SetupTestEnv(env);
        ui32 nodeIdx2 = SetupTestEnv(env);

        TServiceClient service1(env.GetRuntime(), nodeIdx1);
        TServiceClient service2(env.GetRuntime(), nodeIdx2);

        service1.CreateVolume(
            DefaultDiskId,
            512,
            DefaultBlockSize,
            "foo",
            "bar"
        );

        auto response1 = service1.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL
        );
        auto session1 = response1->Record.GetSessionId();

        service1.WriteBlocks(DefaultDiskId, TBlockRange64(0), session1, 1);

        auto response2 = service2.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_REMOTE,
            false,
            1);
        auto session2 = response2->Record.GetSessionId();

        service2.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL,
            false,
            1);

        service2.WriteBlocks(DefaultDiskId, TBlockRange64(0), session2, 1);
    }

    Y_UNIT_TEST(ShouldReportRemoteVolumeStatsToBalancer)
    {
        static constexpr TDuration mountVolumeTimeout = TDuration::Seconds(100);

        NProto::TStorageServiceConfig storageServiceConfig;
        storageServiceConfig.SetInactiveClientsTimeout(mountVolumeTimeout.MilliSeconds());

        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env, std::move(storageServiceConfig));

        auto& runtime = env.GetRuntime();
        runtime.SetRegistrationObserverFunc([] (auto& runtime, const auto& parentId, const auto& actorId)
        {
            Y_UNUSED(parentId);
            runtime.EnableScheduleForActor(actorId);
        });

        TServiceClient service(env.GetRuntime(), nodeIdx);

        service.CreateVolume("vol1");
        auto resp2 = service.MountVolume("vol1");
        service.WaitForVolume("vol1");

        {
            auto request = std::make_unique<TEvService::TEvChangeVolumeBindingRequest>(
                "vol1",
                EChangeBindingOp::RELEASE_TO_HIVE,
                NProto::EPreemptionSource::SOURCE_BALANCER);
            service.SendRequest(MakeStorageServiceId(), std::move(request));
            auto response = service.RecvResponse<TEvService::TEvChangeVolumeBindingResponse>();
        }

        service.WaitForVolume("vol1");

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvStatsService::EvVolumePartCounters: {
                        auto* msg = event->Get<TEvStatsService::TEvVolumePartCounters>();
                        msg->VolumeSystemCpu = 100500;
                        msg->VolumeSystemCpu = 100500;
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service.ReadBlocks("vol1", 0, resp2->Record.GetSessionId());

        runtime.UpdateCurrentTime(runtime.GetCurrentTime() + TDuration::Seconds(15));
        runtime.DispatchEvents({}, TDuration::Seconds(1));

        auto response = service.GetVolumeStats();

        UNIT_ASSERT_VALUES_EQUAL(1, response->VolumeStats.size());
        const auto& d1 = response->VolumeStats[0];
        UNIT_ASSERT_VALUES_EQUAL("vol1", d1.GetDiskId());
        UNIT_ASSERT_VALUES_EQUAL(100500, d1.GetSystemCpu());
        UNIT_ASSERT_VALUES_UNEQUAL(100500, d1.GetUserCpu());
    }

    Y_UNIT_TEST(ShouldReportLocalVolumeStatsToBalancer)
    {
        static constexpr TDuration mountVolumeTimeout = TDuration::Seconds(100);

        NProto::TStorageServiceConfig storageServiceConfig;
        storageServiceConfig.SetInactiveClientsTimeout(mountVolumeTimeout.MilliSeconds());

        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env, std::move(storageServiceConfig));

        auto& runtime = env.GetRuntime();
        runtime.SetRegistrationObserverFunc([] (auto& runtime, const auto& parentId, const auto& actorId)
        {
            Y_UNUSED(parentId);
            runtime.EnableScheduleForActor(actorId);
        });

        TServiceClient service(env.GetRuntime(), nodeIdx);
        service.CreateVolume("vol0");
        auto resp1 = service.MountVolume("vol0");
        service.WaitForVolume("vol0");

        service.ReadBlocks("vol0", 0, resp1->Record.GetSessionId());

        runtime.UpdateCurrentTime(runtime.GetCurrentTime() + TDuration::Seconds(15));
        runtime.DispatchEvents({}, TDuration::Seconds(1));
        runtime.UpdateCurrentTime(runtime.GetCurrentTime() + TDuration::Seconds(15));
        runtime.DispatchEvents({}, TDuration::Seconds(1));

        auto response = service.GetVolumeStats();

        UNIT_ASSERT_VALUES_EQUAL(1, response->VolumeStats.size());
        const auto& d1 = response->VolumeStats[0];
        UNIT_ASSERT_VALUES_EQUAL("vol0", d1.GetDiskId());
        UNIT_ASSERT_VALUES_UNEQUAL(0, d1.GetSystemCpu());
        UNIT_ASSERT_VALUES_UNEQUAL(0, d1.GetUserCpu());
    }

    Y_UNIT_TEST(ShouldNotUnmountInactiveClientIfThereArePendingMountUnmountRequests)
    {
        TTestEnv env;
        NProto::TStorageServiceConfig storageServiceConfig;
        storageServiceConfig.SetInitialAddClientTimeout(
            TDuration::Seconds(5).MilliSeconds()
        );

        ui32 nodeIdx1 = SetupTestEnv(env, std::move(storageServiceConfig));

        TServiceClient service1(env.GetRuntime(), nodeIdx1);
        TServiceClient service2(env.GetRuntime(), nodeIdx1);

        ui64 volumeTabletId = 0;
        auto& runtime = env.GetRuntime();

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvSSProxy::EvDescribeVolumeResponse: {
                        auto* msg = event->Get<TEvSSProxy::TEvDescribeVolumeResponse>();
                        const auto& volumeDescription =
                            msg->PathDescription.GetBlockStoreVolumeDescription();
                        volumeTabletId = volumeDescription.GetVolumeTabletId();
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service1.CreateVolume();

        auto sessionId1 = service1.MountVolume()->Record.GetSessionId();
        auto sessionId2 = service2.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_REMOTE)->Record.GetSessionId();

        RebootTablet(runtime, volumeTabletId, service1.GetSender(), nodeIdx1);

        runtime.AdvanceCurrentTime(TDuration::Seconds(4));

        service1.SendMountVolumeRequest();

        TAutoPtr<IEventHandle> savedEvent;
        bool unmountSeen = false;
        bool responseCatched = false;

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvVolume::EvAddClientResponse: {
                        if (!savedEvent && !responseCatched) {
                            savedEvent = event;
                            responseCatched = true;
                            return TTestActorRuntime::EEventAction::DROP;
                        }
                        break;
                    }
                    case TEvService::EvUnmountVolumeRequest: {
                        unmountSeen = true;
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service2.SendMountVolumeRequest(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_REMOTE);

        runtime.AdvanceCurrentTime(TDuration::Seconds(6));

        TDispatchOptions options;
        options.FinalEvents.emplace_back(TEvVolume::EvAddClientRequest);
        runtime.DispatchEvents(options);

        UNIT_ASSERT(savedEvent);
        runtime.Send(savedEvent.Release(), nodeIdx1);

        service1.RecvMountVolumeResponse();
        service2.RecvMountVolumeResponse();

        UNIT_ASSERT_VALUES_EQUAL(false, unmountSeen);

        service1.UnmountVolume(DefaultDiskId, sessionId1);
        service2.UnmountVolume(DefaultDiskId, sessionId2);
    }

    Y_UNIT_TEST(ShouldNotRunTwoStageLocalMountAfterTwoStageLocalMountDuration)
    {
        static constexpr TDuration twoStageLocalMountDuration = TDuration::Seconds(10);

        NProto::TStorageServiceConfig storageServiceConfig;
        storageServiceConfig.SetTwoStageLocalMountDuration(twoStageLocalMountDuration.MilliSeconds());
        storageServiceConfig.SetTwoStageLocalMountEnabled(true);

        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env, std::move(storageServiceConfig));

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);

        auto startTime = runtime.GetCurrentTime();

        service.CreateVolume();
        service.AssignVolume();

        runtime.UpdateCurrentTime(startTime + twoStageLocalMountDuration);

        ui32 chBindingSeen = 0;

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvService::EvChangeVolumeBindingRequest: {
                        ++chBindingSeen;
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service.SendMountVolumeRequest();

        TDispatchOptions options;
        options.FinalEvents.emplace_back(TEvServicePrivate::EvStartVolumeResponse);
        runtime.DispatchEvents(options);

        UNIT_ASSERT_VALUES_EQUAL(0, chBindingSeen);
    }

    Y_UNIT_TEST(ShouldNotRunVolumesLocallyIfRemoteMountOnly)
    {
        NProto::TStorageServiceConfig storageServiceConfig;
        storageServiceConfig.SetRemoteMountOnly(true);

        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env, std::move(storageServiceConfig));

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);

        service.CreateVolume();
        service.AssignVolume();

        ui32 localStartSeen = 0;

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvServicePrivate::EvStartVolumeResponse: {
                        ++localStartSeen;
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service.MountVolume();

        UNIT_ASSERT_VALUES_EQUAL(0, localStartSeen);
    }

    Y_UNIT_TEST(ShouldExtendAddClientTimeoutForVolumesPreviouslyMountedLocal)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);

        service.CreateVolume();
        service.AssignVolume();

        ui64 volumeTabletId = 0;

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvSSProxy::EvDescribeVolumeResponse: {
                        auto* msg = event->Get<TEvSSProxy::TEvDescribeVolumeResponse>();
                        const auto& volumeDescription =
                            msg->PathDescription.GetBlockStoreVolumeDescription();
                        volumeTabletId = volumeDescription.GetVolumeTabletId();
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service.MountVolume();

        bool startVolumeSeen = false;

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvVolume::EvAddClientRequest: {
                        return TTestActorRuntime::EEventAction::DROP;
                    }
                    case TEvServicePrivate::EvStartVolumeRequest: {
                        startVolumeSeen = true;
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service.SendMountVolumeRequest();

        TDispatchOptions options;
        runtime.DispatchEvents(options, TDuration::Seconds(2));

        UNIT_ASSERT_VALUES_EQUAL(0, startVolumeSeen);
    }

    Y_UNIT_TEST(ShouldNotSendVolumeMountedForPingMounts)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);

        service.CreateVolume("vol1");
        service.AssignVolume("vol1");

        bool addClientSeen = false;

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvVolume::EvAddClientRequest: {
                        addClientSeen = true;
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        // test that we don't exceed limit during mount

        service.MountVolume("vol1");
        service.MountVolume("vol1");

        addClientSeen = false;
        service.MountVolume("vol1");
        UNIT_ASSERT_VALUES_EQUAL(false, addClientSeen);
    }

    Y_UNIT_TEST(ShouldNotExceedThresholdForNumberOfLocallyMountedVolumes)
    {
        NProto::TStorageServiceConfig storageServiceConfig;
        storageServiceConfig.SetMaxLocalVolumes(2);

        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env, std::move(storageServiceConfig));

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);

        service.CreateVolume("vol1");
        service.AssignVolume("vol1");

        service.CreateVolume("vol2");
        service.AssignVolume("vol2");

        service.CreateVolume("vol3");
        service.AssignVolume("vol3");

        service.CreateVolume("vol4-remote");
        service.AssignVolume("vol4-remote");

        bool startVolumeSeen = false;

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvServicePrivate::EvStartVolumeRequest: {
                        startVolumeSeen = true;
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        // test that we don't exceed limit during mount

        auto sessionId1 = service.MountVolume("vol1")->Record.GetSessionId();
        UNIT_ASSERT_VALUES_EQUAL(true, startVolumeSeen);

        UNIT_ASSERT_VALUES_EQUAL(
            sessionId1,
            service.MountVolume("vol1")->Record.GetSessionId());

        startVolumeSeen = false;
        service.MountVolume(
            "vol4-remote",
            "",
            "",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_REMOTE);
        UNIT_ASSERT_VALUES_EQUAL(false, startVolumeSeen);

        startVolumeSeen = false;
        auto sessionId2 = service.MountVolume("vol2")->Record.GetSessionId();
        UNIT_ASSERT_VALUES_EQUAL(true, startVolumeSeen);

        startVolumeSeen = false;
        auto sessionId3 = service.MountVolume("vol3")->Record.GetSessionId();
        UNIT_ASSERT_VALUES_EQUAL(false, startVolumeSeen);

        // test that subsequent mounts does not change situation
        startVolumeSeen = false;
        UNIT_ASSERT_VALUES_EQUAL(
            sessionId3,
            service.MountVolume("vol3")->Record.GetSessionId());
        UNIT_ASSERT_VALUES_EQUAL(false, startVolumeSeen);

        // test that service tracks current number of locally mounted volumes

        service.UnmountVolume("vol2", sessionId2);

        startVolumeSeen = false;
        UNIT_ASSERT_VALUES_EQUAL(
            sessionId3,
            service.MountVolume("vol3")->Record.GetSessionId());
        UNIT_ASSERT_VALUES_EQUAL(true, startVolumeSeen);

        startVolumeSeen = false;
        sessionId2 = service.MountVolume("vol2")->Record.GetSessionId();
        UNIT_ASSERT_VALUES_EQUAL(false, startVolumeSeen);
    }

    Y_UNIT_TEST(ShouldAllowSeveralVolumePipesFromSameClient)
    {
        TTestEnv env(1, 3);
        ui32 nodeIdx1 = SetupTestEnv(env);
        ui32 nodeIdx2 = SetupTestEnv(env);
        ui32 nodeIdx3 = SetupTestEnv(env);

        TServiceClient service1(env.GetRuntime(), nodeIdx1);
        TServiceClient service2(env.GetRuntime(), nodeIdx2);
        TServiceClient service3(env.GetRuntime(), nodeIdx3);

        service2.SetClientId(service1.GetClientId());
        service3.SetClientId(service1.GetClientId());

        service1.CreateVolume(
            DefaultDiskId,
            512,
            DefaultBlockSize,
            "foo",
            "bar"
        );

        auto response1 = service1.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL
        );
        auto session1 = response1->Record.GetSessionId();

        service1.WriteBlocks(DefaultDiskId, TBlockRange64(0), session1, 1);

        auto response2 = service2.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_REMOTE,
            false,
            1);
        auto session2 = response2->Record.GetSessionId();

        service2.WriteBlocks(DefaultDiskId, TBlockRange64(0), session2, 1);

        {
            service1.SendWriteBlocksRequest(
                DefaultDiskId,
                TBlockRange64(0),
                session1,
                1);

            auto response = service1.RecvWriteBlocksResponse();
            UNIT_ASSERT_C(
                FAILED(response->GetStatus()),
                "WriteBlocks request unexpectedly succeeded");
        }

        auto response3 = service3.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_REMOTE
        );
        auto session3 = response3->Record.GetSessionId();

        service3.WriteBlocks(DefaultDiskId, TBlockRange64(0), session3, 1);

        {
            service2.SendWriteBlocksRequest(
                DefaultDiskId,
                TBlockRange64(0),
                session2,
                1);

            auto response = service2.RecvWriteBlocksResponse();
            UNIT_ASSERT_C(
                FAILED(response->GetStatus()),
                "WriteBlocks request unexpectedly succeeded");
        }
    }

    Y_UNIT_TEST(ShouldSuccessfullyUnmountIfVolumeDestroyed)
    {
        TTestEnv env(1, 2);
        ui32 nodeIdx1 = SetupTestEnv(env);
        ui32 nodeIdx2 = SetupTestEnv(env);

        TServiceClient service1(env.GetRuntime(), nodeIdx1);
        TServiceClient service2(env.GetRuntime(), nodeIdx2);

        service1.CreateVolume(
            DefaultDiskId,
            512,
            DefaultBlockSize,
            "foo",
            "bar"
        );

        auto response1 = service1.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL
        );

        auto sessionId = response1->Record.GetSessionId();

        bool tabletDeadSeen = false;
        env.GetRuntime().SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvTablet::EvTabletDead: {
                        tabletDeadSeen = true;
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service2.DestroyVolume();

        if (!tabletDeadSeen) {
            TDispatchOptions options;
            options.FinalEvents.emplace_back(TEvTablet::EvTabletDead);
            env.GetRuntime().DispatchEvents(options);
        }

        auto response2 = service1.UnmountVolume(
            DefaultDiskId,
            sessionId);
        UNIT_ASSERT_VALUES_EQUAL(S_ALREADY, response2->GetStatus());
        UNIT_ASSERT_VALUES_EQUAL(
            "Volume is already destroyed",
            response2->Record.GetError().GetMessage());
    }

    Y_UNIT_TEST(ShouldRetryUnmountForServiceInitiatedUnmountsWithRetriableError)
    {
        static constexpr TDuration mountVolumeTimeout = TDuration::Seconds(3);

        NProto::TStorageServiceConfig storageServiceConfig;
        storageServiceConfig.SetInactiveClientsTimeout(mountVolumeTimeout.MilliSeconds());

        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env, std::move(storageServiceConfig));
        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);

        service.CreateVolume();
        auto response = service.MountVolume();
        auto sessionId = response->Record.GetSessionId();

        bool reject = true;
        NProto::TError lastError;
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvVolume::EvRemoveClientRequest: {
                        if (!reject) {
                            break;
                        }
                        reject = false;
                        auto error = MakeError(E_REJECTED, "Something went wrong:(");
                        auto response = std::make_unique<TEvVolume::TEvRemoveClientResponse>(
                            error);
                        runtime.Send(
                            new IEventHandle(
                                event->Sender,
                                event->Recipient,
                                response.release(),
                                0, // flags,
                                event->Cookie),
                            nodeIdx);
                        return TTestActorRuntime::EEventAction::DROP;
                    };
                    case TEvServicePrivate::EvUnmountRequestProcessed: {
                        const auto* msg = event->Get<TEvServicePrivate::TEvUnmountRequestProcessed>();
                        lastError = msg->Error;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        runtime.UpdateCurrentTime(runtime.GetCurrentTime() + mountVolumeTimeout);
        // wait for rejected unmount
        {
            TDispatchOptions options;
            options.FinalEvents.emplace_back(TEvVolume::EvRemoveClientResponse);
            options.FinalEvents.emplace_back(TEvService::EvUnmountVolumeResponse);
            runtime.DispatchEvents(options);
        }

        UNIT_ASSERT_VALUES_EQUAL(E_REJECTED, lastError.GetCode());
        UNIT_ASSERT_VALUES_EQUAL(
            "Something went wrong:(",
            lastError.GetMessage());

        {
            TDispatchOptions options;
            options.FinalEvents.emplace_back(TEvService::EvUnmountVolumeResponse);
            options.FinalEvents.emplace_back(TEvServicePrivate::EvSessionActorDied);
            runtime.DispatchEvents(options);
        }

        UNIT_ASSERT_VALUES_EQUAL(S_OK, lastError.GetCode());
    }

    Y_UNIT_TEST(ShouldReplyWithRejectedWhenAddClientTimesOutAndConfigRequiresIt)
    {
        NProto::TStorageServiceConfig storageConfig;
        NProto::TFeaturesConfig featuresConfig;

        storageConfig.SetInitialAddClientTimeout(TDuration::Seconds(2).MilliSeconds());
        storageConfig.SetRejectMountOnAddClientTimeout(true);

        TTestEnv env;

        ui32 nodeIdx = SetupTestEnv(env, storageConfig, {});
        auto& runtime = env.GetRuntime();

        bool rejectAddClient = true;
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvVolume::EvAddClientRequest: {
                        if (rejectAddClient) {
                            return TTestActorRuntime::EEventAction::DROP;
                        }
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();

        service.SendMountVolumeRequest();
        auto response = service.RecvMountVolumeResponse();

        UNIT_ASSERT_VALUES_EQUAL(E_REJECTED, response->GetStatus());

        {
            rejectAddClient = false;
            auto response = service.MountVolume();
            auto sessionId = response->Record.GetSessionId();
            service.WaitForVolume(DefaultDiskId);
            service.ReadBlocks(DefaultDiskId, 0, sessionId);
        }
    }

    Y_UNIT_TEST(ShouldStopLocalTabletIfLocalMounterUnableToDescribeVolume)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service(runtime, nodeIdx);
        service.CreateVolume();
        service.AssignVolume(DefaultDiskId, "foo", "bar");

        ui64 volumeTabletId = 0;
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvSSProxy::EvDescribeVolumeResponse: {
                        auto* msg = event->Get<TEvSSProxy::TEvDescribeVolumeResponse>();
                        const auto& volumeDescription =
                            msg->PathDescription.GetBlockStoreVolumeDescription();
                        volumeTabletId = volumeDescription.GetVolumeTabletId();
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL);

        bool check = false;
        bool unlockSeen = false;
        bool failDescribe = true;
        bool sessionDeathSeen = false;
        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvSSProxy::EvDescribeVolumeResponse: {
                        if (failDescribe) {
                            auto* msg = event->Get<TEvSSProxy::TEvDescribeVolumeResponse>();
                            const_cast<NProto::TError&>(msg->Error) =
                                MakeError(E_REJECTED, "SS is dead");
                            }
                        break;
                    }
                    case TEvHiveProxy::EvUnlockTabletRequest: {
                        if (check) {
                            auto* msg = event->Get<TEvHiveProxy::TEvUnlockTabletRequest>();
                            if (msg->TabletId == volumeTabletId) {
                                unlockSeen = true;
                            }
                        }
                        break;
                    }
                    case TEvServicePrivate::EvSessionActorDied: {
                        sessionDeathSeen = true;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        service.SendMountVolumeRequest(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_LOCAL);

        check = true;
        auto response = service.RecvMountVolumeResponse();
        UNIT_ASSERT_VALUES_EQUAL(E_REJECTED, response->GetStatus());
        UNIT_ASSERT_VALUES_EQUAL(true, unlockSeen);
        UNIT_ASSERT_VALUES_EQUAL(true, sessionDeathSeen);

        failDescribe = false;
        auto mountResponse = service.MountVolume(
            DefaultDiskId,
            "foo",
            "bar",
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_LOCAL);

        service.ReadBlocks(DefaultDiskId, 0, mountResponse->Record.GetSessionId());
    }

    Y_UNIT_TEST(ShouldRestartVolumeClientIfUnmountComesFromMonitoring)
    {
        TTestEnv env;
        ui32 nodeIdx = SetupTestEnv(env);

        auto& runtime = env.GetRuntime();

        TServiceClient service1(env.GetRuntime(), nodeIdx);
        TServiceClient service2(env.GetRuntime(), nodeIdx);
        TServiceClient service3(env.GetRuntime(), nodeIdx);

        service1.CreateVolume(
            DefaultDiskId,
            512,
            DefaultBlockSize,
            "foo",
            "bar"
        );

        TActorId volumeClient1;
        TActorId volumeClient2;
        bool volumeClientPillSeen = false;
        ui32 addCnt = 0;

        runtime.SetObserverFunc(
            [&] (TTestActorRuntimeBase& runtime, TAutoPtr<IEventHandle>& event) {
                switch (event->GetTypeRewrite()) {
                    case TEvVolume::EvAddClientRequest: {
                        if (!volumeClient1 || volumeClientPillSeen) {
                            ++addCnt;
                            if (addCnt == 1) {
                                if (!volumeClient1) {
                                    volumeClient1 = event->Recipient;
                                } else {
                                    volumeClient2 = event->Recipient;
                                }
                            }
                        }
                        break;
                    }
                    case TEvents::TSystem::PoisonPill: {
                        if (volumeClient1 && volumeClient1 == event->Recipient) {
                            volumeClientPillSeen = true;
                            addCnt = 0;
                        }
                        break;
                    }
                }
                return TTestActorRuntime::DefaultObserverFunc(runtime, event);
            });

        auto response1 = service1.MountVolume(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_WRITE,
            NProto::VOLUME_MOUNT_LOCAL
        );

        auto sessionId = response1->Record.GetSessionId();

        service1.SendUnmountVolumeRequest(
            DefaultDiskId,
            sessionId,
            NProto::SOURCE_SERVICE_MONITORING);

        service2.SendMountVolumeRequest(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_REMOTE
        );

        service3.SendMountVolumeRequest(
            DefaultDiskId,
            TString(),
            TString(),
            NProto::IPC_GRPC,
            NProto::VOLUME_ACCESS_READ_ONLY,
            NProto::VOLUME_MOUNT_REMOTE
        );

        service1.RecvUnmountVolumeResponse();
        UNIT_ASSERT_VALUES_EQUAL(true, volumeClientPillSeen);

        service2.RecvMountVolumeResponse();
        UNIT_ASSERT_VALUES_UNEQUAL(volumeClient1, volumeClient2);

        service3.RecvMountVolumeResponse();
    }
}

}   // namespace NCloud::NBlockStore::NStorage
