#include "endpoint_manager.h"

#include "endpoint_listener.h"
#include "session_manager.h"

#include <cloud/blockstore/config/server.pb.h>

#include <cloud/blockstore/libs/client/config.h>
#include <cloud/blockstore/libs/client/session.h>
#include <cloud/blockstore/libs/common/iovector.h>
#include <cloud/blockstore/libs/common/sglist_test.h>
#include <cloud/blockstore/libs/diagnostics/request_stats.h>
#include <cloud/blockstore/libs/diagnostics/server_stats_test.h>
#include <cloud/blockstore/libs/diagnostics/volume_stats.h>
#include <cloud/blockstore/libs/endpoints_grpc/socket_endpoint_listener.h>
#include <cloud/blockstore/libs/server/client_acceptor.h>
#include <cloud/blockstore/libs/service/service_test.h>
#include <cloud/blockstore/libs/service/storage_provider.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/scheduler.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/coroutine/executor.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>

#include <library/cpp/testing/unittest/registar.h>

#include <google/protobuf/util/message_differencer.h>

#include <util/folder/path.h>
#include <util/generic/scope.h>

namespace NCloud::NBlockStore::NServer {

using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

static constexpr TDuration TestRequestTimeout = TDuration::Seconds(42);
static const TString TestClientId = "testClientId";

////////////////////////////////////////////////////////////////////////////////

struct TTestSessionManager final
    : public ISessionManager
{
    ui32 CreateSessionCounter = 0;
    NProto::TStartEndpointRequest LastCreateSesionRequest;

    ui32 AlterSessionCounter = 0;
    TString LastAlterSocketPath;
    NProto::EVolumeAccessMode LastAlterAccessMode;
    NProto::EVolumeMountMode LastAlterMountMode;
    ui64 LastAlterMountSeqNumber;

    TFuture<TSessionOrError> CreateSession(
        TCallContextPtr ctx,
        const NProto::TStartEndpointRequest& request) override
    {
        Y_UNUSED(ctx);

        ++CreateSessionCounter;
        LastCreateSesionRequest = request;
        return MakeFuture<TSessionOrError>(TSessionInfo());
    }

    TFuture<NProto::TError> RemoveSession(
        TCallContextPtr ctx,
        const TString& socketPath,
        const NProto::THeaders& headers) override
    {
        Y_UNUSED(ctx);
        Y_UNUSED(socketPath);
        Y_UNUSED(headers);
        return MakeFuture(NProto::TError());
    }

    TFuture<NProto::TError> AlterSession(
        TCallContextPtr ctx,
        const TString& socketPath,
        NProto::EVolumeAccessMode accessMode,
        NProto::EVolumeMountMode mountMode,
        ui64 mountSeqNumber,
        const NProto::THeaders& headers) override
    {
        Y_UNUSED(ctx);
        Y_UNUSED(headers);
        ++AlterSessionCounter;
        LastAlterSocketPath = socketPath;
        LastAlterAccessMode = accessMode;
        LastAlterMountMode = mountMode;
        LastAlterMountSeqNumber = mountSeqNumber;
        return MakeFuture(NProto::TError());
    }

    TFuture<NProto::TDescribeEndpointResponse> DescribeSession(
        const TString& socketPath) override
    {
        Y_UNUSED(socketPath);
        return MakeFuture(NProto::TDescribeEndpointResponse());
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TTestEndpoint
{
    NProto::TStartEndpointRequest Request;
    NClient::ISessionPtr Session;
};

////////////////////////////////////////////////////////////////////////////////

class TTestEndpointListener final
    : public IEndpointListener
{
private:
    const TFuture<NProto::TError> Result;

    TMap<TString, TTestEndpoint> Endpoints;

public:
    TTestEndpointListener(
            TFuture<NProto::TError> result = MakeFuture<NProto::TError>())
        : Result(std::move(result))
    {}

    TFuture<NProto::TError> StartEndpoint(
        const NProto::TStartEndpointRequest& request,
        const NProto::TVolume& volume,
        NClient::ISessionPtr session) override
    {
        Y_UNUSED(volume);

        UNIT_ASSERT(!Endpoints.contains(request.GetUnixSocketPath()));

        TTestEndpoint endpoint;
        endpoint.Request = request;

        Endpoints.emplace(
            request.GetUnixSocketPath(),
            TTestEndpoint {
                .Request = request,
                .Session = std::move(session)
            });

        return Result;
    }

    TFuture<NProto::TError> StopEndpoint(const TString& socketPath) override
    {
        Endpoints.erase(socketPath);

        return Result;
    }

    TMap<TString, TTestEndpoint> GetEndpoints() const
    {
        return Endpoints;
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TBootstrap
{
    const ILoggingServicePtr Logging = CreateLoggingService("console");
    const IBlockStorePtr Service;
    const TExecutorPtr Executor = TExecutor::Create("TestService");

    TBootstrap(IBlockStorePtr service)
        : Service(std::move(service))
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

        if (Service) {
            Service->Start();
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

        if (Service) {
            Service->Stop();
        }

        if (Logging) {
            Logging->Stop();
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

void SetDefaultHeaders(NProto::TStartEndpointRequest& request)
{
    request.MutableHeaders()->SetRequestTimeout(
        TestRequestTimeout.MilliSeconds());
}

void CheckRequestHeaders(const NProto::THeaders& headers)
{
    UNIT_ASSERT_VALUES_EQUAL(
        TestRequestTimeout.MilliSeconds(),
        headers.GetRequestTimeout());

    UNIT_ASSERT_VALUES_EQUAL(TestClientId, headers.GetClientId());
}

////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<TTestService> CreateTestService(
    TMap<TString, NProto::TMountVolumeRequest>& mountedVolumes,
    TFuture<void> mountResult = MakeFuture(),
    TFuture<void> unmountResult = MakeFuture(),
    TFuture<void> storageResult = MakeFuture())
{
    auto service = std::make_shared<TTestService>();
    service->DescribeVolumeHandler =
        [&] (std::shared_ptr<NProto::TDescribeVolumeRequest> request) {
            Y_DEFER {
                request->Clear();
            };

            UNIT_ASSERT_VALUES_EQUAL(
                TestRequestTimeout.MilliSeconds(),
                request->GetHeaders().GetRequestTimeout());

            NProto::TDescribeVolumeResponse response;
            response.MutableVolume()->SetDiskId(request->GetDiskId());
            response.MutableVolume()->SetBlockSize(DefaultBlockSize);
            return MakeFuture(response);
        };
    service->MountVolumeHandler =
        [&] (std::shared_ptr<NProto::TMountVolumeRequest> request) {
            Y_DEFER {
                request->Clear();
            };

            CheckRequestHeaders(request->GetHeaders());

            mountedVolumes[request->GetDiskId()] = *request;

            NProto::TMountVolumeResponse response;
            response.MutableVolume()->SetDiskId(request->GetDiskId());
            response.MutableVolume()->SetBlockSize(DefaultBlockSize);
            return mountResult.Apply([=] (const auto&) {
                return response;
            });
        };
    service->UnmountVolumeHandler =
        [&] (std::shared_ptr<NProto::TUnmountVolumeRequest> request) {
            Y_DEFER {
                request->Clear();
            };

            CheckRequestHeaders(request->GetHeaders());

            mountedVolumes.erase(request->GetDiskId());

            return unmountResult.Apply([=] (const auto&) {
                return NProto::TUnmountVolumeResponse();
            });
        };
    service->WriteBlocksLocalHandler =
        [&] (std::shared_ptr<NProto::TWriteBlocksLocalRequest> request) {
            Y_DEFER {
                request->Clear();
            };

            CheckRequestHeaders(request->GetHeaders());

            return storageResult.Apply([=] (const auto&) {
                return NProto::TWriteBlocksLocalResponse();
            });
        };
    service->ReadBlocksLocalHandler =
        [&] (std::shared_ptr<NProto::TReadBlocksLocalRequest> request) {
            Y_DEFER {
                request->Clear();
            };

            CheckRequestHeaders(request->GetHeaders());

            return storageResult.Apply([=] (const auto&) {
                return NProto::TReadBlocksLocalResponse();
            });
        };
    service->ZeroBlocksHandler =
        [&] (std::shared_ptr<NProto::TZeroBlocksRequest> request) {
            Y_DEFER {
                request->Clear();
            };

            CheckRequestHeaders(request->GetHeaders());

            return storageResult.Apply([=] (const auto&) {
                return NProto::TZeroBlocksResponse();
            });
        };

    return service;
}

////////////////////////////////////////////////////////////////////////////////

IEndpointManagerPtr CreateEndpointManager(
    TBootstrap& bootstrap,
    THashMap<NProto::EClientIpcType, IEndpointListenerPtr> endpointListeners,
    IServerStatsPtr serverStats = CreateServerStatsStub(),
    TString nbdSocketSuffix = "")
{
    TSessionManagerOptions sessionManagerOptions;
    sessionManagerOptions.DefaultClientConfig.SetRequestTimeout(
        TestRequestTimeout.MilliSeconds());

    auto sessionManager = CreateSessionManager(
        CreateWallClockTimer(),
        CreateSchedulerStub(),
        bootstrap.Logging,
        CreateMonitoringServiceStub(),
        CreateRequestStatsStub(),
        CreateVolumeStatsStub(),
        serverStats,
        bootstrap.Service,
        CreateDefaultStorageProvider(bootstrap.Service),
        bootstrap.Executor,
        sessionManagerOptions);

    return NServer::CreateEndpointManager(
        bootstrap.Logging,
        serverStats,
        bootstrap.Executor,
        std::move(sessionManager),
        std::move(endpointListeners),
        std::move(nbdSocketSuffix));
}

////////////////////////////////////////////////////////////////////////////////

TFuture<NProto::TStartEndpointResponse> StartEndpoint(
    IEndpointManager& endpointManager,
    const NProto::TStartEndpointRequest& request)
{
    return endpointManager.StartEndpoint(
        MakeIntrusive<TCallContext>(),
        std::make_shared<NProto::TStartEndpointRequest>(request));
}

TFuture<NProto::TStopEndpointResponse> StopEndpoint(
    IEndpointManager& endpointManager,
    const TString& unixSocketPath)
{
    auto request = std::make_shared<NProto::TStopEndpointRequest>();
    request->SetUnixSocketPath(unixSocketPath);

    return endpointManager.StopEndpoint(
        MakeIntrusive<TCallContext>(),
        std::move(request));
}

TFuture<NProto::TListEndpointsResponse> ListEndpoints(
    IEndpointManager& endpointManager)
{
    return endpointManager.ListEndpoints(
        MakeIntrusive<TCallContext>(),
        std::make_shared<NProto::TListEndpointsRequest>());
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TEndpointManagerTest)
{
    Y_UNIT_TEST(ShouldHandleStartStopEndpoint)
    {
        TString unixSocket = "testSocket";
        TString diskId = "testDiskId";
        auto ipcType = NProto::IPC_GRPC;

        TMap<TString, NProto::TMountVolumeRequest> mountedVolumes;
        TBootstrap bootstrap(CreateTestService(mountedVolumes));

        auto listener = std::make_shared<TTestEndpointListener>();
        auto manager = CreateEndpointManager(
            bootstrap,
            {{ ipcType, listener }});

        bootstrap.Start();

        {
            NProto::TStartEndpointRequest request;
            SetDefaultHeaders(request);
            request.SetUnixSocketPath(unixSocket);
            request.SetDiskId(diskId);
            request.SetClientId(TestClientId);
            request.SetIpcType(ipcType);

            auto future = StartEndpoint(*manager, request);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));

            UNIT_ASSERT(mountedVolumes.contains(diskId));
            UNIT_ASSERT(listener->GetEndpoints().contains(unixSocket));
        }

        {
            auto future = StopEndpoint(*manager, unixSocket);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));

            UNIT_ASSERT(mountedVolumes.empty());
            UNIT_ASSERT(listener->GetEndpoints().empty());
        }
    }

    Y_UNIT_TEST(ShouldChangeMountModesUsingStartEndpoint)
    {
        auto unixSocket = "testSocket";
        auto ipcType = NProto::IPC_GRPC;
        TString diskId = "testDiskId";

        TMap<TString, NProto::TMountVolumeRequest> mountedVolumes;
        TBootstrap bootstrap(CreateTestService(mountedVolumes));

        auto manager = CreateEndpointManager(
            bootstrap,
            {{ ipcType, std::make_shared<TTestEndpointListener>() }});

        bootstrap.Start();

        auto accessMode = NProto::VOLUME_ACCESS_READ_ONLY;
        auto mountMode = NProto::VOLUME_MOUNT_REMOTE;
        ui64 mountSeqNumber = 2;

        {
            NProto::TStartEndpointRequest request;
            SetDefaultHeaders(request);
            request.SetUnixSocketPath(unixSocket);
            request.SetDiskId(diskId);
            request.SetClientId(TestClientId);
            request.SetIpcType(ipcType);
            request.SetVolumeAccessMode(accessMode);
            request.SetVolumeMountMode(mountMode);
            request.SetMountSeqNumber(mountSeqNumber);

            auto future = StartEndpoint(*manager, request);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));

            const auto& mountRequest = mountedVolumes.find(diskId)->second;
            UNIT_ASSERT(mountRequest.GetVolumeAccessMode() == accessMode);
            UNIT_ASSERT(mountRequest.GetVolumeMountMode() == mountMode);
            UNIT_ASSERT(mountRequest.GetMountSeqNumber() == mountSeqNumber);
        }

        {
            accessMode = NProto::VOLUME_ACCESS_READ_WRITE;
            mountMode = NProto::VOLUME_MOUNT_LOCAL;
            ++mountSeqNumber;

            NProto::TStartEndpointRequest request;
            SetDefaultHeaders(request);
            request.SetUnixSocketPath(unixSocket);
            request.SetDiskId(diskId);
            request.SetClientId(TestClientId);
            request.SetIpcType(ipcType);
            request.SetVolumeAccessMode(accessMode);
            request.SetVolumeMountMode(mountMode);
            request.SetMountSeqNumber(mountSeqNumber);

            auto future = StartEndpoint(*manager, request);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));

            const auto& mountRequest = mountedVolumes.find(diskId)->second;
            UNIT_ASSERT(mountRequest.GetVolumeAccessMode() == accessMode);
            UNIT_ASSERT(mountRequest.GetVolumeMountMode() == mountMode);
            UNIT_ASSERT(mountRequest.GetMountSeqNumber() == mountSeqNumber);
        }

        {
            auto future = StopEndpoint(*manager, unixSocket);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));

            UNIT_ASSERT(mountedVolumes.empty());
        }
    }

    Y_UNIT_TEST(ShouldHandleListEndpoints)
    {
        TMap<TString, NProto::TMountVolumeRequest> mountedVolumes;
        TBootstrap bootstrap(CreateTestService(mountedVolumes));

        auto manager = CreateEndpointManager(
            bootstrap,
            {
                { NProto::IPC_GRPC, std::make_shared<TTestEndpointListener>() },
                { NProto::IPC_NBD, std::make_shared<TTestEndpointListener>() },
            });

        bootstrap.Start();

        NProto::TStartEndpointRequest request1;
        SetDefaultHeaders(request1);
        request1.SetUnixSocketPath("testSocket1");
        request1.SetDiskId("testDiskId1");
        request1.SetClientId(TestClientId);
        request1.SetIpcType(NProto::IPC_GRPC);

        NProto::TStartEndpointRequest request2;
        SetDefaultHeaders(request2);
        request2.SetUnixSocketPath("testSocket2");
        request2.SetDiskId("testDiskId2");
        request2.SetClientId(TestClientId);
        request2.SetIpcType(NProto::IPC_NBD);

        {
            auto future = StartEndpoint(*manager, request1);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));
        }

        {
            auto future = StartEndpoint(*manager, request2);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));
        }

        {
            auto future = ListEndpoints(*manager);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));

            UNIT_ASSERT(response.EndpointsSize() == 2);

            auto endpoint1 = response.GetEndpoints().Get(0);
            auto endpoint2 = response.GetEndpoints().Get(1);
            if (endpoint1.GetUnixSocketPath() != request1.GetUnixSocketPath()) {
                endpoint1 = response.GetEndpoints().Get(1);
                endpoint2 = response.GetEndpoints().Get(0);
            }

            UNIT_ASSERT(endpoint1.GetUnixSocketPath() == request1.GetUnixSocketPath());
            UNIT_ASSERT(endpoint1.GetIpcType() == request1.GetIpcType());
            UNIT_ASSERT(endpoint1.GetDiskId() == request1.GetDiskId());

            UNIT_ASSERT(endpoint2.GetUnixSocketPath() == request2.GetUnixSocketPath());
            UNIT_ASSERT(endpoint2.GetIpcType() == request2.GetIpcType());
            UNIT_ASSERT(endpoint2.GetDiskId() == request2.GetDiskId());
        }
    }

    Y_UNIT_TEST(ShouldNotStartStopEndpointTwice)
    {
        TMap<TString, NProto::TMountVolumeRequest> mountedVolumes;
        TBootstrap bootstrap(CreateTestService(mountedVolumes));

        auto listener = std::make_shared<TTestEndpointListener>();
        auto manager = CreateEndpointManager(
            bootstrap,
            {{ NProto::IPC_GRPC, listener }});

        bootstrap.Start();

        auto socketPath = "testSocketPath";
        auto diskId = "testDiskId";

        NProto::TStartEndpointRequest startRequest;
        SetDefaultHeaders(startRequest);
        startRequest.SetUnixSocketPath(socketPath);
        startRequest.SetDiskId(diskId);
        startRequest.SetClientId(TestClientId);
        startRequest.SetIpcType(NProto::IPC_GRPC);

        {
            auto future = StartEndpoint(*manager, startRequest);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(response.GetError().GetCode() == S_OK);
        }

        {
            auto future = StartEndpoint(*manager, startRequest);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(response.GetError().GetCode() == S_ALREADY);
        }

        UNIT_ASSERT(mountedVolumes.contains(diskId));
        UNIT_ASSERT(listener->GetEndpoints().contains(socketPath));

        {
            auto future = StopEndpoint(*manager, socketPath);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(response.GetError().GetCode() == S_OK);
        }

        {
            auto future = StopEndpoint(*manager, socketPath);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(response.GetError().GetCode() == S_FALSE);
        }

        UNIT_ASSERT(mountedVolumes.empty());
        UNIT_ASSERT(listener->GetEndpoints().empty());
    }

    Y_UNIT_TEST(ShouldNotStartBusyEndpoint)
    {
        TMap<TString, NProto::TMountVolumeRequest> mountedVolumes;
        TBootstrap bootstrap(CreateTestService(mountedVolumes));

        auto grpcListener = std::make_shared<TTestEndpointListener>();
        auto nbdListener = std::make_shared<TTestEndpointListener>();

        auto manager = CreateEndpointManager(
            bootstrap,
            {
                { NProto::IPC_GRPC, grpcListener },
                { NProto::IPC_NBD, nbdListener },
            });

        bootstrap.Start();

        auto socketPath = "testSocketPath";

        {
            NProto::TStartEndpointRequest request;
            SetDefaultHeaders(request);
            request.SetUnixSocketPath(socketPath);
            request.SetDiskId("testDiskId1");
            request.SetClientId(TestClientId);
            request.SetIpcType(NProto::IPC_GRPC);

            auto future = StartEndpoint(*manager, request);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));
            UNIT_ASSERT(grpcListener->GetEndpoints().size() == 1);
            UNIT_ASSERT(nbdListener->GetEndpoints().size() == 0);
        }

        {
            NProto::TStartEndpointRequest request;
            SetDefaultHeaders(request);
            request.SetUnixSocketPath(socketPath);
            request.SetDiskId("testDiskId2");
            request.SetClientId(TestClientId);
            request.SetIpcType(NProto::IPC_NBD);

            auto future = StartEndpoint(*manager, request);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(response.GetError().GetCode() == E_INVALID_STATE);
            UNIT_ASSERT(grpcListener->GetEndpoints().size() == 1);
            UNIT_ASSERT(nbdListener->GetEndpoints().size() == 0);
        }
    }

    Y_UNIT_TEST(ShouldNotMountDiskWhenStartEndpointFailed)
    {
        TMap<TString, NProto::TMountVolumeRequest> mountedVolumes;
        TBootstrap bootstrap(CreateTestService(mountedVolumes));

        auto error = TErrorResponse(E_FAIL, "Endpoint listener is broken");
        auto listener = std::make_shared<TTestEndpointListener>(
            MakeFuture<NProto::TError>(error));

        auto manager = CreateEndpointManager(
            bootstrap,
            {{ NProto::IPC_GRPC, listener }});

        bootstrap.Start();

        NProto::TStartEndpointRequest request;
        SetDefaultHeaders(request);
        request.SetUnixSocketPath("testSocket");
        request.SetDiskId("testDiskId");
        request.SetClientId(TestClientId);
        request.SetIpcType(NProto::IPC_GRPC);

        auto future = StartEndpoint(*manager, request);
        UNIT_ASSERT(HasError(future.GetValue(TDuration::Seconds(5))));
        UNIT_ASSERT(mountedVolumes.empty());
    }

    Y_UNIT_TEST(ShouldHandleParallelStartStopEndpoints)
    {
        TMap<TString, NProto::TMountVolumeRequest> mountedVolumes;
        auto mountPromise = NewPromise<void>();
        auto unmountPromise = NewPromise<void>();
        auto service = CreateTestService(
            mountedVolumes,
            mountPromise,
            unmountPromise);

        TBootstrap bootstrap(std::move(service));

        auto manager = CreateEndpointManager(
            bootstrap,
            {{ NProto::IPC_GRPC, std::make_shared<TTestEndpointListener>() }});

        bootstrap.Start();

        auto unixSocket = "testSocket";

        NProto::TStartEndpointRequest startRequest;
        SetDefaultHeaders(startRequest);
        startRequest.SetUnixSocketPath(unixSocket);
        startRequest.SetDiskId("testDiskId");
        startRequest.SetClientId(TestClientId);
        startRequest.SetIpcType(NProto::IPC_GRPC);

        NProto::TStartEndpointRequest otherStartRequest = startRequest;
        otherStartRequest.SetIpcType(NProto::IPC_VHOST);

        {
            auto future1 = StartEndpoint(*manager, startRequest);
            UNIT_ASSERT(!future1.HasValue());

            auto future2 = StartEndpoint(*manager, startRequest);
            UNIT_ASSERT(!future2.HasValue());

            auto future3 = StartEndpoint(*manager, otherStartRequest);
            auto response3 = future3.GetValue();
            UNIT_ASSERT(response3.GetError().GetCode() == E_REJECTED);

            auto future = StopEndpoint(*manager, unixSocket);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(response.GetError().GetCode() == E_REJECTED);

            mountPromise.SetValue();

            auto response1 = future1.GetValue(TDuration::Seconds(5));
            auto response2 = future2.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(response1.GetError().GetCode() == S_OK);
            UNIT_ASSERT(response2.GetError().GetCode() == S_OK);
        }

        {
            auto future1 = StopEndpoint(*manager, unixSocket);
            UNIT_ASSERT(!future1.HasValue());

            auto future2 = StopEndpoint(*manager, unixSocket);
            UNIT_ASSERT(!future2.HasValue());

            auto future3 = StartEndpoint(*manager, otherStartRequest);
            auto response3 = future3.GetValue();
            UNIT_ASSERT(response3.GetError().GetCode() == E_REJECTED);

            unmountPromise.SetValue();

            auto response1 = future1.GetValue(TDuration::Seconds(5));
            auto response2 = future2.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(response1.GetError().GetCode() == S_OK);
            UNIT_ASSERT(response2.GetError().GetCode() == S_OK);
        }
    }

    Y_UNIT_TEST(ShouldHandleLocalRequests)
    {
        TMap<TString, NProto::TMountVolumeRequest> mountedVolumes;
        TBootstrap bootstrap(CreateTestService(mountedVolumes));

        auto listener = std::make_shared<TTestEndpointListener>();
        auto manager = CreateEndpointManager(
            bootstrap,
            {{ NProto::IPC_GRPC, listener }});

        bootstrap.Start();

        auto unixSocket = "testSocket";

        {
            NProto::TStartEndpointRequest request;
            SetDefaultHeaders(request);
            request.SetUnixSocketPath(unixSocket);
            request.SetDiskId("testDiskId");
            request.SetClientId(TestClientId);
            request.SetIpcType(NProto::IPC_GRPC);

            auto future = StartEndpoint(*manager, request);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));
        }

        const auto& endpoints = listener->GetEndpoints();
        auto session = endpoints.find(unixSocket)->second.Session;

        const ui64 startIndex = 8;
        const ui64 blocksCount = 32;

        TVector<TString> blocks;
        auto sglist = ResizeBlocks(
            blocks,
            blocksCount,
            TString(DefaultBlockSize, 'f'));

        {
            auto request = std::make_shared<NProto::TWriteBlocksLocalRequest>();
            request->SetStartIndex(startIndex);
            request->BlocksCount = blocksCount;
            request->BlockSize = DefaultBlockSize;
            request->Sglist = TGuardedSgList(sglist);

            auto future = session->WriteBlocksLocal(
                MakeIntrusive<TCallContext>(),
                std::move(request));

            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));
        }

        {
            auto request = std::make_shared<NProto::TReadBlocksLocalRequest>();
            request->SetStartIndex(startIndex);
            request->SetBlocksCount(blocksCount);
            request->BlockSize = DefaultBlockSize;
            request->Sglist = TGuardedSgList(sglist);

            auto future = session->ReadBlocksLocal(
                MakeIntrusive<TCallContext>(),
                std::move(request));

            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));
        }

        {
            auto request = std::make_shared<NProto::TZeroBlocksRequest>();
            request->SetStartIndex(startIndex);
            request->SetBlocksCount(blocksCount);

            auto future = session->ZeroBlocks(
                MakeIntrusive<TCallContext>(),
                std::move(request));

            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));
        }
    }

    Y_UNIT_TEST(ShouldMountUnmountVolumeForMetrics)
    {
        TString testDiskId = "testDiskId";

        auto serverStats = std::make_shared<TTestServerStats>();

        ui32 mountCounter = 0;
        ui32 unmountCounter = 0;

        serverStats->MountVolumeHandler = [&] (
                const NProto::TVolume& volume,
                const TString& clientId,
                const TString& instanceId)
            {
                Y_UNUSED(clientId);
                Y_UNUSED(instanceId);

                UNIT_ASSERT(volume.GetDiskId() == testDiskId);
                ++mountCounter;
                return true;
            };

        serverStats->UnmountVolumeHandler = [&] (
                const TString& diskId,
                const TString& clientId)
            {
                Y_UNUSED(clientId);
                UNIT_ASSERT(diskId == testDiskId);
                ++unmountCounter;
            };

        TMap<TString, NProto::TMountVolumeRequest> mountedVolumes;
        TBootstrap bootstrap(CreateTestService(mountedVolumes));

        auto listener = std::make_shared<TTestEndpointListener>();
        auto manager = CreateEndpointManager(
            bootstrap,
            {{ NProto::IPC_VHOST, listener }},
            serverStats);

        bootstrap.Start();

        auto unixSocket = "testSocket";

        {
            NProto::TStartEndpointRequest request;
            SetDefaultHeaders(request);
            request.SetUnixSocketPath(unixSocket);
            request.SetDiskId(testDiskId);
            request.SetClientId(TestClientId);
            request.SetIpcType(NProto::IPC_VHOST);

            auto future = StartEndpoint(*manager, request);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));
            UNIT_ASSERT(mountCounter == 1);
        }

        {
            auto future = StopEndpoint(*manager, unixSocket);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));
            UNIT_ASSERT(unmountCounter == 1);
        }
    }

    Y_UNIT_TEST(ShouldNotStartEndpointWithSocketPathLongerThanLimit)
    {
        TMap<TString, NProto::TMountVolumeRequest> mountedVolumes;
        TBootstrap bootstrap(CreateTestService(mountedVolumes));

        auto grpcListener = CreateSocketEndpointListener(bootstrap.Logging, 16);
        grpcListener->SetClientAcceptor(CreateClientAcceptorStub());

        auto manager = CreateEndpointManager(
            bootstrap,
            {{ NProto::IPC_GRPC, grpcListener }});

        bootstrap.Start();

        TString maxSocketPath(UnixSocketPathLengthLimit, 'x');

        NProto::TStartEndpointRequest startRequest;
        SetDefaultHeaders(startRequest);
        startRequest.SetDiskId("testDiskId");
        startRequest.SetClientId(TestClientId);
        startRequest.SetIpcType(NProto::IPC_GRPC);

        {
            startRequest.SetUnixSocketPath(maxSocketPath);
            auto future1 = StartEndpoint(*manager, startRequest);
            UNIT_ASSERT(!HasError(future1.GetValue(TDuration::Seconds(5))));

            UNIT_ASSERT(TFsPath(maxSocketPath).Exists());

            auto future2 = StopEndpoint(*manager, maxSocketPath);
            UNIT_ASSERT(!HasError(future2.GetValue(TDuration::Seconds(5))));

            UNIT_ASSERT(!TFsPath(maxSocketPath).Exists());
        }

        {
            startRequest.SetUnixSocketPath(maxSocketPath + 'x');

            auto future = StartEndpoint(*manager, startRequest);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(HasError(response)
                && response.GetError().GetCode() == E_ARGUMENT);

            UNIT_ASSERT(!TFsPath(startRequest.GetUnixSocketPath()).Exists());
            UNIT_ASSERT(!TFsPath(maxSocketPath).Exists());
        }
    }

    Y_UNIT_TEST(ShouldStartStopNbdEndpointWithGrpcEndpoint)
    {
        TString unixSocket = "testSocket";
        TString diskId = "testDiskId";
        TString nbdSocketSuffix = "_nbd";

        TMap<TString, NProto::TMountVolumeRequest> mountedVolumes;
        TBootstrap bootstrap(CreateTestService(mountedVolumes));

        auto grpcListener = std::make_shared<TTestEndpointListener>();
        auto nbdListener = std::make_shared<TTestEndpointListener>();

        auto manager = CreateEndpointManager(
            bootstrap,
            {
                { NProto::IPC_GRPC, grpcListener },
                { NProto::IPC_NBD, nbdListener },
            },
            CreateServerStatsStub(),
            nbdSocketSuffix);

        bootstrap.Start();

        {
            NProto::TStartEndpointRequest request;
            SetDefaultHeaders(request);
            request.SetUnixSocketPath(unixSocket);
            request.SetDiskId(diskId);
            request.SetClientId(TestClientId);
            request.SetIpcType(NProto::IPC_GRPC);

            auto future = StartEndpoint(*manager, request);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(!HasError(response), response.GetError());

            UNIT_ASSERT(mountedVolumes.contains(diskId));

            UNIT_ASSERT(grpcListener->GetEndpoints().contains(unixSocket));
            const auto& grpcEndpoints = grpcListener->GetEndpoints();
            auto grpcRequest = grpcEndpoints.find(unixSocket)->second.Request;
            UNIT_ASSERT(!grpcRequest.GetUnalignedRequestsDisabled());
            UNIT_ASSERT(!grpcRequest.GetSendNbdMinBlockSize());

            auto nbdUnixSocket = unixSocket + nbdSocketSuffix;

            UNIT_ASSERT(nbdListener->GetEndpoints().contains(nbdUnixSocket));
            const auto& nbdEndpoints = nbdListener->GetEndpoints();
            auto nbdRequest = nbdEndpoints.find(nbdUnixSocket)->second.Request;
            UNIT_ASSERT(nbdRequest.GetUnalignedRequestsDisabled());
            UNIT_ASSERT(nbdRequest.GetSendNbdMinBlockSize());
        }

        {
            auto future = StopEndpoint(*manager, unixSocket);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(!HasError(response), response.GetError());

            UNIT_ASSERT(mountedVolumes.empty());
            UNIT_ASSERT(grpcListener->GetEndpoints().empty());
            UNIT_ASSERT(nbdListener->GetEndpoints().empty());
        }
    }

    Y_UNIT_TEST(ShouldIgnoreInstanceIdWhenCompareStartEndpointRequests)
    {
        TMap<TString, NProto::TMountVolumeRequest> mountedVolumes;
        TBootstrap bootstrap(CreateTestService(mountedVolumes));

        auto sessionManager = std::make_shared<TTestSessionManager>();
        auto manager = NServer::CreateEndpointManager(
            bootstrap.Logging,
            CreateServerStatsStub(),
            bootstrap.Executor,
            sessionManager,
            {{ NProto::IPC_GRPC, std::make_shared<TTestEndpointListener>() }},
            ""  // NbdSocketSuffix
        );

        bootstrap.Start();

        NProto::TStartEndpointRequest request;
        SetDefaultHeaders(request);
        request.SetUnixSocketPath("testSocket");
        request.SetDiskId("testDiskId");
        request.SetClientId(TestClientId);
        request.SetInstanceId("testInstanceId");
        request.SetIpcType(NProto::IPC_GRPC);
        request.SetVolumeAccessMode(NProto::VOLUME_ACCESS_READ_ONLY);
        request.SetVolumeMountMode(NProto::VOLUME_MOUNT_REMOTE);
        request.SetMountSeqNumber(1);

        {
            auto future = StartEndpoint(*manager, request);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(
                S_OK == response.GetError().GetCode(),
                response.GetError());

            UNIT_ASSERT_VALUES_EQUAL(1, sessionManager->CreateSessionCounter);
            UNIT_ASSERT_VALUES_EQUAL(0, sessionManager->AlterSessionCounter);

            google::protobuf::util::MessageDifferencer comparator;
            UNIT_ASSERT(comparator.Equals(
                request,
                sessionManager->LastCreateSesionRequest));
        }

        {
            auto future = StartEndpoint(*manager, request);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(
                S_ALREADY == response.GetError().GetCode(),
                response.GetError());

            UNIT_ASSERT_VALUES_EQUAL(1, sessionManager->CreateSessionCounter);
            UNIT_ASSERT_VALUES_EQUAL(0, sessionManager->AlterSessionCounter);
        }

        {
            request.SetInstanceId("otherTestInstanceId");

            auto future = StartEndpoint(*manager, request);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(
                S_ALREADY == response.GetError().GetCode(),
                response.GetError());

            UNIT_ASSERT_VALUES_EQUAL(1, sessionManager->CreateSessionCounter);
            UNIT_ASSERT_VALUES_EQUAL(0, sessionManager->AlterSessionCounter);
        }

        {
            request.SetVolumeAccessMode(NProto::VOLUME_ACCESS_READ_WRITE);
            request.SetVolumeMountMode(NProto::VOLUME_MOUNT_LOCAL);
            request.SetMountSeqNumber(42);

            auto future = StartEndpoint(*manager, request);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(
                S_OK == response.GetError().GetCode(),
                response.GetError());

            UNIT_ASSERT_VALUES_EQUAL(1, sessionManager->CreateSessionCounter);
            UNIT_ASSERT_VALUES_EQUAL(1, sessionManager->AlterSessionCounter);

            UNIT_ASSERT_VALUES_EQUAL(
                request.GetUnixSocketPath(), sessionManager->LastAlterSocketPath);
            UNIT_ASSERT(
                NProto::VOLUME_ACCESS_READ_WRITE == sessionManager->LastAlterAccessMode);
            UNIT_ASSERT(
                NProto::VOLUME_MOUNT_LOCAL == sessionManager->LastAlterMountMode);
            UNIT_ASSERT_VALUES_EQUAL(42, sessionManager->LastAlterMountSeqNumber);
        }

        {
            request.SetClientId("otherTestClientId");

            auto future = StartEndpoint(*manager, request);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(HasError(response.GetError()), response.GetError());

            UNIT_ASSERT_VALUES_EQUAL(1, sessionManager->CreateSessionCounter);
            UNIT_ASSERT_VALUES_EQUAL(1, sessionManager->AlterSessionCounter);
        }
    }
}

}   // namespace NCloud::NBlockStore::NServer
