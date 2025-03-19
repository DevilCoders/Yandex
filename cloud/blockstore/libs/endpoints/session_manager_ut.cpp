#include "session_manager.h"

#include <cloud/blockstore/libs/diagnostics/request_stats.h>
#include <cloud/blockstore/libs/diagnostics/server_stats_test.h>
#include <cloud/blockstore/libs/diagnostics/volume_stats.h>
#include <cloud/blockstore/libs/service/service_test.h>
#include <cloud/blockstore/libs/service/storage_provider.h>
#include <cloud/storage/core/libs/common/scheduler_test.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/coroutine/executor.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/scope.h>

namespace NCloud::NBlockStore::NServer {

using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TBootstrap
{
    const ILoggingServicePtr Logging = CreateLoggingService("console");
    const TExecutorPtr Executor = TExecutor::Create("TestService");

    TBootstrap()
    {}

    ~TBootstrap()
    {
        Stop();
    }

    void Start()
    {
        if (Logging) {
            Logging->Start();
        }

        if (Executor) {
            Executor->Start();
        }
    }

    void Stop()
    {
        if (Executor) {
            Executor->Stop();
        }

        if (Logging) {
            Logging->Stop();
        }
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TSessionManagerTest)
{
    void ServerStatsShouldMountVolumeWhenEndpointIsStarted(
        NProto::EClientIpcType ipcType)
    {
        TString socketPath = "testSocket";
        TString diskId = "testDiskId";
        ui32 serverStatsMountCounter = 0;

        auto scheduler = std::make_shared<TTestScheduler>();

        auto serverStats = std::make_shared<TTestServerStats>();
        serverStats->MountVolumeHandler = [&] (
                const NProto::TVolume& volume,
                const TString& clientId,
                const TString& instanceId)
            {
                Y_UNUSED(clientId);
                Y_UNUSED(instanceId);

                UNIT_ASSERT_VALUES_EQUAL(diskId, volume.GetDiskId());
                ++serverStatsMountCounter;
                return true;
            };
        serverStats->UnmountVolumeHandler = [&] (
                const TString& unmountDiskId,
                const TString& clientId)
            {
                Y_UNUSED(clientId);
                UNIT_ASSERT_VALUES_EQUAL(diskId, unmountDiskId);
            };

        auto service = std::make_shared<TTestService>();
        service->DescribeVolumeHandler =
            [&] (std::shared_ptr<NProto::TDescribeVolumeRequest> request) {
                Y_UNUSED(request);
                return MakeFuture(NProto::TDescribeVolumeResponse());
            };
        service->MountVolumeHandler =
            [&] (std::shared_ptr<NProto::TMountVolumeRequest> request) {
                NProto::TMountVolumeResponse response;
                response.MutableVolume()->SetDiskId(request->GetDiskId());
                response.SetInactiveClientsTimeout(100);
                return MakeFuture(response);
            };
        service->UnmountVolumeHandler =
            [&] (std::shared_ptr<NProto::TUnmountVolumeRequest> request) {
                Y_UNUSED(request);
                return MakeFuture(NProto::TUnmountVolumeResponse());
            };

        auto executor = TExecutor::Create("TestService");

        auto sessionManager = CreateSessionManager(
            CreateWallClockTimer(),
            scheduler,
            CreateLoggingService("console"),
            CreateMonitoringServiceStub(),
            CreateRequestStatsStub(),
            CreateVolumeStatsStub(),
            serverStats,
            service,
            CreateDefaultStorageProvider(service),
            executor,
            TSessionManagerOptions());

        executor->Start();
        Y_DEFER {
            executor->Stop();
        };

        NProto::TStartEndpointRequest request;
        request.SetUnixSocketPath(socketPath);
        request.SetDiskId(diskId);
        request.SetClientId("testClientId");
        request.SetIpcType(ipcType);

        {
            auto future = sessionManager->CreateSession(
                MakeIntrusive<TCallContext>(),
                request);

            auto sessionOrError = future.GetValue(TDuration::Seconds(3));
            UNIT_ASSERT_C(!HasError(sessionOrError), sessionOrError.GetError());
        }

        ui32 expectedCount = 1;
        UNIT_ASSERT_VALUES_EQUAL(expectedCount, serverStatsMountCounter);

        for (size_t i = 0; i < 10; ++i) {
            scheduler->RunAllScheduledTasks();
            ++expectedCount;
            UNIT_ASSERT_VALUES_EQUAL(expectedCount, serverStatsMountCounter);
        }

        {
            auto future = sessionManager->RemoveSession(
                MakeIntrusive<TCallContext>(),
                socketPath,
                request.GetHeaders());
            auto error = future.GetValue(TDuration::Seconds(3));
            UNIT_ASSERT_C(!HasError(error), error);
        }

        for (size_t i = 0; i < 10; ++i) {
            scheduler->RunAllScheduledTasks();
            UNIT_ASSERT_VALUES_EQUAL(expectedCount, serverStatsMountCounter);
        }
    }

    Y_UNIT_TEST(ServerStatsShouldMountVolumeWhenEndpointIsStarted_grpc)
    {
        ServerStatsShouldMountVolumeWhenEndpointIsStarted(NProto::IPC_GRPC);
    }

    Y_UNIT_TEST(ServerStatsShouldMountVolumeWhenEndpointIsStarted_nbd)
    {
        ServerStatsShouldMountVolumeWhenEndpointIsStarted(NProto::IPC_NBD);
    }

    Y_UNIT_TEST(ServerStatsShouldMountVolumeWhenEndpointIsStarted_vhost)
    {
        ServerStatsShouldMountVolumeWhenEndpointIsStarted(NProto::IPC_VHOST);
    }
}

}   // namespace NCloud::NBlockStore::NServer
