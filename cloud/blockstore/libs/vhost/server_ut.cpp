#include "server.h"

#include "vhost_test.h"

#include <cloud/blockstore/libs/common/sglist_test.h>
#include <cloud/blockstore/libs/diagnostics/critical_events.h>
#include <cloud/blockstore/libs/diagnostics/server_stats_test.h>
#include <cloud/blockstore/libs/diagnostics/volume_stats_test.h>
#include <cloud/blockstore/libs/service/device_handler.h>
#include <cloud/blockstore/libs/service/storage_test.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/path.h>
#include <util/thread/lfqueue.h>

namespace NCloud::NBlockStore::NVhost {

using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TTestRequest
{
    EBlockStoreRequest Type = EBlockStoreRequest::ReadBlocks;
    ui64 StartIndex = 0;
    ui64 BlocksCount = 0;
    TSgList SgList;
};

////////////////////////////////////////////////////////////////////////////////

class TTestEnvironment
{
private:
    const size_t ThreadsCount = 2;

    const TFsPath SocketPath = TFsPath("./testSocketPath");
    const ui32 VhostQueuesCount = 1;
    const ui32 BlockSize;
    const ui64 BlocksCount = 256;

    IServerPtr VhostServer;
    std::shared_ptr<TTestStorage> TestStorage;
    std::shared_ptr<ITestVhostDevice> VhostDevice;
    std::shared_ptr<TTestVhostQueueFactory> VhostQueueFactory;
    TLockFreeQueue<TTestRequest> RequestQueue;

    TAtomic ServiceFrozen = 0;
    TLockFreeQueue<TPromise<void>> FrozenPromises;

public:
    TTestEnvironment(ui32 blockSize)
        : BlockSize(blockSize)
    {
        InitVhostDeviceEnvironment();
    }

    ~TTestEnvironment()
    {
        UninitVhostDeviceEnvironment();
    }

    void StopVhostServer()
    {
        VhostServer->Stop();
        VhostServer.reset();
    }

    std::shared_ptr<ITestVhostDevice> GetVhostDevice()
    {
        return VhostDevice;
    }

    TTestVhostQueueFactory& GetVhostQueueFactory()
    {
        return *VhostQueueFactory;
    }

    bool DequeueRequest(TTestRequest& request)
    {
        return RequestQueue.Dequeue(&request);
    }

    void FreezeService(bool freeze)
    {
        AtomicSet(ServiceFrozen, freeze ? 1 : 0);

        if (!freeze) {
            TPromise<void> promise;
            while (FrozenPromises.Dequeue(&promise)) {
                promise.SetValue();
            }
        }
    }

private:
    void InitVhostDeviceEnvironment()
    {
        TestStorage = std::make_shared<TTestStorage>();
        TestStorage->WriteBlocksLocalHandler =
            [&] (TCallContextPtr ctx, std::shared_ptr<NProto::TWriteBlocksLocalRequest> request) {
                Y_UNUSED(ctx);

                auto guard = request->Sglist.Acquire();
                UNIT_ASSERT(guard);
                auto sglist = guard.Get();
                UNIT_ASSERT(request->BlocksCount * BlockSize == SgListGetSize(sglist));

                RequestQueue.Enqueue({
                    EBlockStoreRequest::WriteBlocks,
                    request->GetStartIndex(),
                    request->BlocksCount,
                    std::move(sglist)});

                if (AtomicGet(ServiceFrozen) == 1) {
                    auto promise = NewPromise<void>();
                    auto future = promise.GetFuture();
                    FrozenPromises.Enqueue(std::move(promise));
                    return future.Apply([=] (const auto& future) {
                        Y_UNUSED(future);
                        return NProto::TWriteBlocksLocalResponse();
                    });
                }

                return MakeFuture(NProto::TWriteBlocksLocalResponse());
            };
        TestStorage->ReadBlocksLocalHandler =
            [&] (TCallContextPtr ctx, std::shared_ptr<NProto::TReadBlocksLocalRequest> request) {
                Y_UNUSED(ctx);

                auto guard = request->Sglist.Acquire();
                UNIT_ASSERT(guard);
                auto sglist = guard.Get();
                UNIT_ASSERT(request->GetBlocksCount() * BlockSize == SgListGetSize(sglist));

                RequestQueue.Enqueue({
                    EBlockStoreRequest::ReadBlocks,
                    request->GetStartIndex(),
                    request->GetBlocksCount(),
                    std::move(sglist)});

                if (AtomicGet(ServiceFrozen) == 1) {
                    auto promise = NewPromise<void>();
                    auto future = promise.GetFuture();
                    FrozenPromises.Enqueue(std::move(promise));
                    return future.Apply([=] (const auto& future) {
                        Y_UNUSED(future);
                        return NProto::TReadBlocksLocalResponse();
                    });
                }

                return MakeFuture(NProto::TReadBlocksLocalResponse());
            };

        VhostQueueFactory = std::make_shared<TTestVhostQueueFactory>();

        TServerConfig serverConfig;
        serverConfig.ThreadsCount = ThreadsCount;

        VhostServer = CreateServer(
            CreateLoggingService("console"),
            CreateServerStatsStub(),
            VhostQueueFactory,
            CreateDefaultDeviceHandlerFactory(),
            serverConfig,
            NRdma::TRdmaHandler());

        VhostServer->Start();
        Sleep(TDuration::MilliSeconds(300));
        UNIT_ASSERT(VhostQueueFactory->Queues.size() == ThreadsCount);
        auto firstQueue = VhostQueueFactory->Queues.at(0);
        UNIT_ASSERT(firstQueue->IsRun());

        {
            TStorageOptions options;
            options.DiskId = "TestDiskId";
            options.BlockSize = BlockSize;
            options.BlocksCount = BlocksCount;
            options.VhostQueuesCount = VhostQueuesCount;
            options.UnalignedRequestsDisabled = false;

            auto future = VhostServer->StartEndpoint(
                SocketPath.GetPath(),
                TestStorage,
                options);
            const auto& error = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(!HasError(error), error);
        }
        UNIT_ASSERT(firstQueue->GetDevices().size() == 1);
        VhostDevice = firstQueue->GetDevices().at(0);
    }

    void UninitVhostDeviceEnvironment()
    {
        if (VhostServer) {
            auto future = VhostServer->StopEndpoint(SocketPath.GetPath());
            const auto& error = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(!HasError(error), error);
        }

        Sleep(TDuration::MilliSeconds(300));
        UNIT_ASSERT(VhostDevice->IsStopped());

        if (VhostServer) {
            VhostServer->Stop();
        }
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TServerTest)
{
    Y_UNIT_TEST(ShouldStartStopVhostEndpoint)
    {
        auto logging = CreateLoggingService("console");
        InitVhostLog(logging);

        auto vhostQueueFactory = CreateVhostQueueFactory();

        auto vhostServer = CreateServer(
            logging,
            CreateServerStatsStub(),
            vhostQueueFactory,
            CreateDefaultDeviceHandlerFactory(),
            TServerConfig(),
            NRdma::TRdmaHandler());

        vhostServer->Start();

        const TFsPath socket("./testSocketPath");

        {
            TStorageOptions options;
            options.DiskId = "TestDiskId";
            options.BlockSize = DefaultBlockSize;
            options.BlocksCount = 42;
            options.VhostQueuesCount = 1;
            options.UnalignedRequestsDisabled = false;

            auto future = vhostServer->StartEndpoint(
                socket.GetPath(),
                std::make_shared<TTestStorage>(),
                options);
            const auto& error = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(!HasError(error), error);
        }

        UNIT_ASSERT(socket.Exists());

        {
            auto future = vhostServer->StopEndpoint(socket.GetPath());
            const auto& error = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(!HasError(error), error);
        }

        vhostServer->Stop();
    }

    Y_UNIT_TEST(ShouldStopVhostServerWithStartedEndpoints)
    {
        auto logging = CreateLoggingService("console");
        InitVhostLog(logging);

        auto vhostQueueFactory = CreateVhostQueueFactory();

        auto vhostServer = CreateServer(
            logging,
            CreateServerStatsStub(),
            vhostQueueFactory,
            CreateDefaultDeviceHandlerFactory(),
            TServerConfig(),
            NRdma::TRdmaHandler());

        vhostServer->Start();

        TStorageOptions options;
        options.DiskId = "TestDiskId";
        options.BlockSize = DefaultBlockSize;
        options.BlocksCount = 42;
        options.VhostQueuesCount = 1;
        options.UnalignedRequestsDisabled = false;

        TString socketPath = "./testSocketPath";

        const size_t endpointCount = 8;
        TString sockets[endpointCount];

        for (size_t i = 0; i < endpointCount; ++i) {
            char ch = '0' + i;
            sockets[i] = socketPath + ch;

            auto future = vhostServer->StartEndpoint(
                sockets[i],
                std::make_shared<TTestStorage>(),
                options);
            const auto& error = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(!HasError(error), error);
            UNIT_ASSERT(TFsPath(sockets[i]).Exists());
        }

        vhostServer->Stop();
    }

    Y_UNIT_TEST(ShouldHandleVhostReadWriteRequests)
    {
        const ui32 blockSize = 4096;
        const ui64 firstSector = 8;
        const ui64 totalSectors = 32;
        const ui64 sectorSize = 512;

        UNIT_ASSERT(totalSectors * sectorSize % blockSize == 0);

        auto environment = TTestEnvironment(blockSize);
        auto device = environment.GetVhostDevice();

        TVector<TString> blocks;
        auto sgList = ResizeBlocks(
            blocks,
            totalSectors * sectorSize / blockSize,
            TString(blockSize, 'f'));

        {
            auto future = device->SendTestRequest(
                EBlockStoreRequest::WriteBlocks,
                firstSector * sectorSize,
                totalSectors * sectorSize,
                sgList);
            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(response == TVhostRequest::SUCCESS);

            TTestRequest request;
            bool res = environment.DequeueRequest(request);
            UNIT_ASSERT(res);
            UNIT_ASSERT(request.Type == EBlockStoreRequest::WriteBlocks);
            UNIT_ASSERT(request.StartIndex * blockSize == firstSector * sectorSize);
            UNIT_ASSERT(request.BlocksCount * blockSize == totalSectors * sectorSize);
            UNIT_ASSERT_VALUES_EQUAL(request.SgList, sgList);
            UNIT_ASSERT(!environment.DequeueRequest(request));
        }

        {
            auto future = device->SendTestRequest(
                EBlockStoreRequest::ReadBlocks,
                firstSector * sectorSize,
                totalSectors * sectorSize,
                sgList);
            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(response == TVhostRequest::SUCCESS);

            TTestRequest request;
            bool res = environment.DequeueRequest(request);
            UNIT_ASSERT(res);
            UNIT_ASSERT(request.Type == EBlockStoreRequest::ReadBlocks);
            UNIT_ASSERT(request.StartIndex * blockSize == firstSector * sectorSize);
            UNIT_ASSERT(request.BlocksCount * blockSize == totalSectors * sectorSize);
            UNIT_ASSERT_VALUES_EQUAL(request.SgList, sgList);
            UNIT_ASSERT(!environment.DequeueRequest(request));
        }
    }

    Y_UNIT_TEST(ShouldThrowCriticalEventIfFailedRequestQueueRunning)
    {
        NMonitoring::TDynamicCountersPtr counters = new NMonitoring::TDynamicCounters();
        InitCriticalEventsCounter(counters);
        auto configCounter =
            counters->GetCounter("AppCriticalEvents/VhostQueueRunningError", true);

        auto environment = TTestEnvironment(DefaultBlockSize);

        UNIT_ASSERT_VALUES_EQUAL(0, static_cast<int>(*configCounter));

        auto& factory = environment.GetVhostQueueFactory();
        factory.Queues.at(0)->Break();

        factory.FailedEvent.Reset();
        factory.FailedEvent.WaitT(TDuration::Seconds(1));
        factory.FailedEvent.Reset();
        factory.FailedEvent.WaitT(TDuration::Seconds(1));

        UNIT_ASSERT_VALUES_EQUAL(1, static_cast<int>(*configCounter));
    }

    Y_UNIT_TEST(ShouldStartEndpointIfSocketAlreadyExists)
    {
        auto logging = CreateLoggingService("console");
        InitVhostLog(logging);

        auto vhostQueueFactory = CreateVhostQueueFactory();

        auto vhostServer = CreateServer(
            logging,
            CreateServerStatsStub(),
            vhostQueueFactory,
            CreateDefaultDeviceHandlerFactory(),
            TServerConfig(),
            NRdma::TRdmaHandler());

        vhostServer->Start();

        const TFsPath socket("./testSocketPath");
        socket.Touch();

        {
            TStorageOptions options;
            options.DiskId = "TestDiskId";
            options.BlockSize = DefaultBlockSize;
            options.BlocksCount = 42;
            options.VhostQueuesCount = 1;
            options.UnalignedRequestsDisabled = false;

            auto future = vhostServer->StartEndpoint(
                socket.GetPath(),
                std::make_shared<TTestStorage>(),
                options);
            const auto& error = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(!HasError(error), error);
        }

        vhostServer->Stop();
    }

    Y_UNIT_TEST(ShouldRemoveUnixSocketAfterStopEndpoint)
    {
        auto logging = CreateLoggingService("console");
        InitVhostLog(logging);

        auto vhostQueueFactory = CreateVhostQueueFactory();

        auto vhostServer = CreateServer(
            logging,
            CreateServerStatsStub(),
            vhostQueueFactory,
            CreateDefaultDeviceHandlerFactory(),
            TServerConfig(),
            NRdma::TRdmaHandler());

        vhostServer->Start();

        const TFsPath socket("./testSocketPath");

        {
            TStorageOptions options;
            options.DiskId = "TestDiskId";
            options.BlockSize = DefaultBlockSize;
            options.BlocksCount = 42;
            options.VhostQueuesCount = 1;
            options.UnalignedRequestsDisabled = false;

            auto future = vhostServer->StartEndpoint(
                socket.GetPath(),
                std::make_shared<TTestStorage>(),
                options);
            const auto& error = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(!HasError(error), error);
        }

        auto future = vhostServer->StopEndpoint(socket.GetPath());
        const auto& error = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_C(!HasError(error), error);
        UNIT_ASSERT(!socket.Exists());

        vhostServer->Stop();
    }

    Y_UNIT_TEST(ShouldNotRemoveUnixSocketAfterStopServer)
    {
        auto logging = CreateLoggingService("console");
        InitVhostLog(logging);

        auto vhostQueueFactory = CreateVhostQueueFactory();

        auto vhostServer = CreateServer(
            logging,
            CreateServerStatsStub(),
            vhostQueueFactory,
            CreateDefaultDeviceHandlerFactory(),
            TServerConfig(),
            NRdma::TRdmaHandler());

        vhostServer->Start();

        const TFsPath socket("./testSocketPath");

        {
            TStorageOptions options;
            options.DiskId = "TestDiskId";
            options.BlockSize = DefaultBlockSize;
            options.BlocksCount = 42;
            options.VhostQueuesCount = 1;
            options.UnalignedRequestsDisabled = false;

            auto future = vhostServer->StartEndpoint(
                socket.GetPath(),
                std::make_shared<TTestStorage>(),
                options);
            const auto& error = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(!HasError(error), error);
        }

        vhostServer->Stop();
        UNIT_ASSERT(socket.Exists());
    }

    Y_UNIT_TEST(ShouldCancelRequestsInFlightWhenStopEndpointOrStopServer)
    {
        TString unixSocketPath = "./testSocketPath";
        const ui32 blockSize = 4096;
        const ui64 startIndex = 3;
        const ui64 blocksCount = 41;

        auto promise = NewPromise<void>();

        auto testStorage = std::make_shared<TTestStorage>();
        testStorage->WriteBlocksLocalHandler =
            [&] (TCallContextPtr ctx, std::shared_ptr<NProto::TWriteBlocksLocalRequest> request) {
                Y_UNUSED(ctx);
                Y_UNUSED(request);
                return promise.GetFuture().Apply([] (const auto& f) {
                    Y_UNUSED(f);
                    return NProto::TWriteBlocksLocalResponse();
                });
            };
        testStorage->ReadBlocksLocalHandler =
            [&] (TCallContextPtr ctx, std::shared_ptr<NProto::TReadBlocksLocalRequest> request) {
                Y_UNUSED(ctx);
                Y_UNUSED(request);
                return promise.GetFuture().Apply([] (const auto& f) {
                    Y_UNUSED(f);
                    return NProto::TReadBlocksLocalResponse();
                });
            };

        auto queueFactory = std::make_shared<TTestVhostQueueFactory>();

        TServerConfig serverConfig;
        serverConfig.ThreadsCount = 2;

        auto server = CreateServer(
            CreateLoggingService("console"),
            CreateServerStatsStub(),
            queueFactory,
            CreateDefaultDeviceHandlerFactory(),
            serverConfig,
            NRdma::TRdmaHandler());

        server->Start();
        Sleep(TDuration::MilliSeconds(300));
        UNIT_ASSERT(queueFactory->Queues.size() == serverConfig.ThreadsCount);
        auto firstQueue = queueFactory->Queues.at(0);
        UNIT_ASSERT(firstQueue->IsRun());

        TStorageOptions options;
        options.DiskId = "testDiskId";
        options.BlockSize = blockSize;
        options.BlocksCount = 256;
        options.VhostQueuesCount = 1;
        options.UnalignedRequestsDisabled = false;

        {
            auto future = server->StartEndpoint(
                unixSocketPath,
                testStorage,
                options);
            const auto& error = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(!HasError(error), error);
        }
        UNIT_ASSERT(firstQueue->GetDevices().size() == 1);
        auto device = firstQueue->GetDevices().at(0);

        TVector<TString> blocks;
        auto sgList = ResizeBlocks(
            blocks,
            blocksCount,
            TString(blockSize, 'f'));

        auto writeFuture = device->SendTestRequest(
            EBlockStoreRequest::WriteBlocks,
            startIndex * blockSize,
            blocksCount * blockSize,
            sgList);

        auto readFuture = device->SendTestRequest(
            EBlockStoreRequest::ReadBlocks,
            startIndex * blockSize,
            blocksCount * blockSize,
            sgList);

        Sleep(TDuration::MilliSeconds(300));
        UNIT_ASSERT(!writeFuture.HasValue());
        UNIT_ASSERT(!readFuture.HasValue());

        {
            auto future = server->StopEndpoint(unixSocketPath);
            auto response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(!HasError(response));
        }

        auto writeResponse = writeFuture.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT(writeResponse == TVhostRequest::CANCELLED);
        auto readResponse = readFuture.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT(readResponse == TVhostRequest::CANCELLED);

        {
            auto future = server->StartEndpoint(
                unixSocketPath,
                testStorage,
                options);
            const auto& error = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(!HasError(error), error);
        }
        device.reset();
        UNIT_ASSERT(firstQueue->GetDevices().size() == 1);
        device = firstQueue->GetDevices().at(0);

        writeFuture = device->SendTestRequest(
            EBlockStoreRequest::WriteBlocks,
            startIndex * blockSize,
            blocksCount * blockSize,
            sgList);

        readFuture = device->SendTestRequest(
            EBlockStoreRequest::ReadBlocks,
            startIndex * blockSize,
            blocksCount * blockSize,
            sgList);

        Sleep(TDuration::MilliSeconds(300));
        UNIT_ASSERT(!writeFuture.HasValue());
        UNIT_ASSERT(!readFuture.HasValue());

        server->Stop();

        writeResponse = writeFuture.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT(writeResponse == TVhostRequest::CANCELLED);
        readResponse = readFuture.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT(readResponse == TVhostRequest::CANCELLED);
    }

    Y_UNIT_TEST(ShouldPassCorrectMetrics)
    {
        TString testDiskId = "testDiskId";
        const ui32 blockSize = 4096;
        const ui64 firstSector = 8;
        const ui64 totalSectors = 32;
        const ui64 sectorSize = 512;

        UNIT_ASSERT(totalSectors * sectorSize % blockSize == 0);

        auto serverStats = std::make_shared<TTestServerStats>();
        auto serverStatsStub = CreateServerStatsStub();

        ui32 requestCounter = 0;

        serverStats->PrepareMetricRequestHandler = [&] (
            TMetricRequest& metricRequest,
            TString clientId,
            TString diskId,
            ui64 startIndex,
            ui32 requestBytes)
        {
            Y_UNUSED(clientId);

            UNIT_ASSERT(diskId == testDiskId);
            metricRequest.DiskId = std::move(diskId);

            switch (metricRequest.RequestType)
            {
                case EBlockStoreRequest::ReadBlocks:
                case EBlockStoreRequest::WriteBlocks:
                case EBlockStoreRequest::ZeroBlocks:
                    UNIT_ASSERT_VALUES_EQUAL(startIndex, firstSector * sectorSize / blockSize);
                    UNIT_ASSERT_VALUES_EQUAL(requestBytes, totalSectors * sectorSize);
                    break;
                case EBlockStoreRequest::MountVolume:
                    break;
                case EBlockStoreRequest::UnmountVolume:
                    break;
                default:
                    UNIT_FAIL("Unexpected request");
                    break;
            }

            ++requestCounter;
        };

        auto testStorage = std::make_shared<TTestStorage>();
        testStorage->WriteBlocksLocalHandler =
            [&] (TCallContextPtr ctx, std::shared_ptr<NProto::TWriteBlocksLocalRequest> request) {
                Y_UNUSED(ctx);
                Y_UNUSED(request);
                return MakeFuture(NProto::TWriteBlocksLocalResponse());
            };
        testStorage->ReadBlocksLocalHandler =
            [&] (TCallContextPtr ctx, std::shared_ptr<NProto::TReadBlocksLocalRequest> request) {
                Y_UNUSED(ctx);
                Y_UNUSED(request);
                return MakeFuture(NProto::TReadBlocksLocalResponse());
            };

        auto queueFactory = std::make_shared<TTestVhostQueueFactory>();

        TServerConfig serverConfig;
        serverConfig.ThreadsCount = 2;

        auto server = CreateServer(
            CreateLoggingService("console"),
            serverStats,
            queueFactory,
            CreateDefaultDeviceHandlerFactory(),
            serverConfig,
            NRdma::TRdmaHandler());

        server->Start();
        Sleep(TDuration::MilliSeconds(300));
        UNIT_ASSERT(queueFactory->Queues.size() == serverConfig.ThreadsCount);
        auto firstQueue = queueFactory->Queues.at(0);
        UNIT_ASSERT(firstQueue->IsRun());

        {
            TStorageOptions options;
            options.DiskId = testDiskId;
            options.BlockSize = blockSize;
            options.BlocksCount = 256;
            options.VhostQueuesCount = 1;
            options.UnalignedRequestsDisabled = false;

            auto future = server->StartEndpoint(
                "./testSocketPath",
                testStorage,
                options);
            const auto& error = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT_C(!HasError(error), error);
        }
        UNIT_ASSERT(firstQueue->GetDevices().size() == 1);
        auto device = firstQueue->GetDevices().at(0);

        TVector<TString> blocks;
        auto sgList = ResizeBlocks(
            blocks,
            totalSectors * sectorSize / blockSize,
            TString(blockSize, 'f'));

        {
            auto future = device->SendTestRequest(
                EBlockStoreRequest::WriteBlocks,
                firstSector * sectorSize,
                totalSectors * sectorSize,
                sgList);
            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(response == TVhostRequest::SUCCESS);
            UNIT_ASSERT_VALUES_EQUAL(requestCounter, 1);
        }

        {
            auto future = device->SendTestRequest(
                EBlockStoreRequest::ReadBlocks,
                firstSector * sectorSize,
                totalSectors * sectorSize,
                sgList);
            const auto& response = future.GetValue(TDuration::Seconds(5));
            UNIT_ASSERT(response == TVhostRequest::SUCCESS);
            UNIT_ASSERT_VALUES_EQUAL(requestCounter, 2);
        }
    }
}

}   // namespace NCloud::NBlockStore::NVhost
