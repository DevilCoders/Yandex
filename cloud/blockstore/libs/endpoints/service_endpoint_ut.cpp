#include "service_endpoint.h"

#include "endpoint_manager.h"

#include <cloud/blockstore/libs/diagnostics/critical_events.h>
#include <cloud/blockstore/libs/diagnostics/request_stats.h>
#include <cloud/blockstore/libs/diagnostics/server_stats.h>
#include <cloud/blockstore/libs/diagnostics/volume_stats.h>
#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/service/service.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/scheduler_test.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/keyring/endpoints.h>
#include <cloud/storage/core/libs/keyring/endpoints_test.h>

#include <library/cpp/monlib/dynamic_counters/counters.h>
#include <library/cpp/testing/unittest/registar.h>

#include <google/protobuf/util/message_differencer.h>

#include <util/generic/guid.h>
#include <util/generic/scope.h>

namespace NCloud::NBlockStore::NServer {

using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TTestEndpointManager final
    : public IEndpointManager
{
    using TStartEndpointHandler = std::function<
        NThreading::TFuture<NProto::TStartEndpointResponse>(
            std::shared_ptr<NProto::TStartEndpointRequest> request)
        >;

    using TStopEndpointHandler = std::function<
        NThreading::TFuture<NProto::TStopEndpointResponse>(
            std::shared_ptr<NProto::TStopEndpointRequest> request)
        >;

    using TListEndpointsHandler = std::function<
        NThreading::TFuture<NProto::TListEndpointsResponse>(
            std::shared_ptr<NProto::TListEndpointsRequest> request)
        >;

    using TDescribeEndpointHandler = std::function<
        NThreading::TFuture<NProto::TDescribeEndpointResponse>(
            std::shared_ptr<NProto::TDescribeEndpointRequest> request)
        >;

    TStartEndpointHandler StartEndpointHandler;
    TStopEndpointHandler StopEndpointHandler;
    TListEndpointsHandler ListEndpointsHandler;
    TDescribeEndpointHandler DescribeEndpointHandler;

    TFuture<NProto::TStartEndpointResponse> StartEndpoint(
        TCallContextPtr ctx,
        std::shared_ptr<NProto::TStartEndpointRequest> request) override
    {
        Y_UNUSED(ctx);
        return StartEndpointHandler(std::move(request));
    }

    TFuture<NProto::TStopEndpointResponse> StopEndpoint(
        TCallContextPtr ctx,
        std::shared_ptr<NProto::TStopEndpointRequest> request) override
    {
        Y_UNUSED(ctx);
        return StopEndpointHandler(std::move(request));
    }

    TFuture<NProto::TListEndpointsResponse> ListEndpoints(
        TCallContextPtr ctx,
        std::shared_ptr<NProto::TListEndpointsRequest> request) override
    {
        Y_UNUSED(ctx);
        return ListEndpointsHandler(std::move(request));
    }

    TFuture<NProto::TDescribeEndpointResponse> DescribeEndpoint(
        TCallContextPtr ctx,
        std::shared_ptr<NProto::TDescribeEndpointRequest> request) override
    {
        Y_UNUSED(ctx);
        return DescribeEndpointHandler(std::move(request));
    }
};

////////////////////////////////////////////////////////////////////////////////

NProto::TKickEndpointResponse KickEndpoint(
    IBlockStore& service,
    ui32 keyringId,
    ui32 requestId = 42)
{
    auto request = std::make_shared<NProto::TKickEndpointRequest>();
    request->MutableHeaders()->SetRequestId(requestId);
    request->SetKeyringId(keyringId);

    auto future = service.KickEndpoint(
        MakeIntrusive<TCallContext>(),
        std::move(request));

     return future.GetValue(TDuration::Seconds(5));
}


}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TServiceEndpointTest)
{
    Y_UNIT_TEST(ShouldHandleKickEndpoint)
    {
        const TString dirPath = "./" + CreateGuidAsString();
        auto endpointStorage = CreateFileEndpointStorage(dirPath);
        auto mutableStorage = CreateFileMutableEndpointStorage(dirPath);

        auto initError = mutableStorage->Init();
        UNIT_ASSERT_C(!HasError(initError), initError);

        Y_DEFER {
            auto error = mutableStorage->Remove();
            UNIT_ASSERT_C(!HasError(error), error);
        };

        TString unixSocket = "testSocket";
        TString diskId = "testDiskId";
        auto ipcType = NProto::IPC_GRPC;

        TVector<std::shared_ptr<NProto::TStartEndpointRequest>> endpoints;
        auto endpointManager = std::make_shared<TTestEndpointManager>();
        endpointManager->StartEndpointHandler = [&] (
            std::shared_ptr<NProto::TStartEndpointRequest> request)
        {
            endpoints.push_back(std::move(request));
            return MakeFuture(NProto::TStartEndpointResponse());
        };

        auto endpointService = CreateMultipleEndpointService(
            nullptr,
            CreateWallClockTimer(),
            CreateSchedulerStub(),
            CreateLoggingService("console"),
            CreateRequestStatsStub(),
            CreateVolumeStatsStub(),
            CreateServerStatsStub(),
            endpointStorage,
            endpointManager,
            {});

        NProto::TStartEndpointRequest request;
        request.SetUnixSocketPath(unixSocket);
        request.SetDiskId(diskId);
        request.SetClientId("testClientId");
        request.SetIpcType(ipcType);

        auto keyOrError = mutableStorage->AddEndpoint(
            diskId,
            SerializeEndpoint(request));
        UNIT_ASSERT_C(!HasError(keyOrError), keyOrError.GetError());

        {
            ui32 requestId = 325;
            auto response = KickEndpoint(
                *endpointService,
                keyOrError.GetResult(),
                requestId);
            UNIT_ASSERT(!HasError(response));

            UNIT_ASSERT_VALUES_EQUAL(1, endpoints.size());
            const auto& endpoint = *endpoints[0];

            google::protobuf::util::MessageDifferencer comparator;
            request.MutableHeaders()->SetRequestId(requestId);
            UNIT_ASSERT(comparator.Equals(endpoint, request));
        }

        {
            auto wrongKeyringId = keyOrError.GetResult() + 42;
            auto response = KickEndpoint(*endpointService, wrongKeyringId);
            UNIT_ASSERT(HasError(response)
                && response.GetError().GetCode() == E_INVALID_STATE);
        }
    }

    Y_UNIT_TEST(ShouldTimeoutFrozenRequest)
    {
        const TString dirPath = "./" + CreateGuidAsString();
        auto endpointStorage = CreateFileEndpointStorage(dirPath);
        auto mutableStorage = CreateFileMutableEndpointStorage(dirPath);

        auto initError = mutableStorage->Init();
        UNIT_ASSERT_C(!HasError(initError), initError);

        Y_DEFER {
            auto error = mutableStorage->Remove();
            UNIT_ASSERT_C(!HasError(error), error);
        };

        auto testScheduler = std::make_shared<TTestScheduler>();

        auto endpointManager = std::make_shared<TTestEndpointManager>();
        endpointManager->StartEndpointHandler = [&] (
            std::shared_ptr<NProto::TStartEndpointRequest> request)
        {
            Y_UNUSED(request);
            return NewPromise<NProto::TStartEndpointResponse>();
        };
        endpointManager->StopEndpointHandler = [&] (
            std::shared_ptr<NProto::TStopEndpointRequest> request)
        {
            Y_UNUSED(request);
            return NewPromise<NProto::TStopEndpointResponse>();
        };
        endpointManager->ListEndpointsHandler = [&] (
            std::shared_ptr<NProto::TListEndpointsRequest> request)
        {
            Y_UNUSED(request);
            return NewPromise<NProto::TListEndpointsResponse>();
        };
        endpointManager->DescribeEndpointHandler = [&] (
            std::shared_ptr<NProto::TDescribeEndpointRequest> request)
        {
            Y_UNUSED(request);
            return NewPromise<NProto::TDescribeEndpointResponse>();
        };

        auto endpointService = CreateMultipleEndpointService(
            nullptr,
            CreateWallClockTimer(),
            testScheduler,
            CreateLoggingService("console"),
            CreateRequestStatsStub(),
            CreateVolumeStatsStub(),
            CreateServerStatsStub(),
            endpointStorage,
            endpointManager,
            {});

        {
            auto request = std::make_shared<NProto::TStartEndpointRequest>();
            request->MutableHeaders()->SetRequestTimeout(100);

            auto future = endpointService->StartEndpoint(
                MakeIntrusive<TCallContext>(),
                request);

            testScheduler->RunAllScheduledTasks();

            auto response = future.GetValue(TDuration::Seconds(3));
            UNIT_ASSERT_VALUES_EQUAL_C(
                E_TIMEOUT,
                response.GetError().GetCode(),
                response);
        }

        {
            auto request = std::make_shared<NProto::TStopEndpointRequest>();
            request->MutableHeaders()->SetRequestTimeout(100);

            auto future = endpointService->StopEndpoint(
                MakeIntrusive<TCallContext>(),
                request);

            testScheduler->RunAllScheduledTasks();

            auto response = future.GetValue(TDuration::Seconds(3));
            UNIT_ASSERT_VALUES_EQUAL_C(
                E_TIMEOUT,
                response.GetError().GetCode(),
                response);
        }

        {
            auto request = std::make_shared<NProto::TListEndpointsRequest>();
            request->MutableHeaders()->SetRequestTimeout(100);

            auto future = endpointService->ListEndpoints(
                MakeIntrusive<TCallContext>(),
                request);

            testScheduler->RunAllScheduledTasks();

            auto response = future.GetValue(TDuration::Seconds(3));
            UNIT_ASSERT_VALUES_EQUAL_C(
                E_TIMEOUT,
                response.GetError().GetCode(),
                response);
        }

        {
            auto request = std::make_shared<NProto::TDescribeEndpointRequest>();
            request->MutableHeaders()->SetRequestTimeout(100);

            auto future = endpointService->DescribeEndpoint(
                MakeIntrusive<TCallContext>(),
                request);

            testScheduler->RunAllScheduledTasks();

            auto response = future.GetValue(TDuration::Seconds(3));
            UNIT_ASSERT_VALUES_EQUAL_C(
                E_TIMEOUT,
                response.GetError().GetCode(),
                response);
        }

        {
            auto diskId = "testDiskId";
            NProto::TStartEndpointRequest startRequest;
            startRequest.SetDiskId(diskId);

            auto keyOrError = mutableStorage->AddEndpoint(
                diskId,
                SerializeEndpoint(startRequest));
            UNIT_ASSERT_C(!HasError(keyOrError), keyOrError.GetError());

            auto request = std::make_shared<NProto::TKickEndpointRequest>();
            request->MutableHeaders()->SetRequestTimeout(100);
            request->SetKeyringId(keyOrError.GetResult());

            auto future = endpointService->KickEndpoint(
                MakeIntrusive<TCallContext>(),
                request);

            testScheduler->RunAllScheduledTasks();

            auto response = future.GetValue(TDuration::Seconds(3));
            UNIT_ASSERT_VALUES_EQUAL_C(
                E_TIMEOUT,
                response.GetError().GetCode(),
                response);
        }
    }

    Y_UNIT_TEST(ShouldThrowCriticalEventIfFailedToRestoreEndpoint)
    {
        const TString wrongSocketPath = "wrong.socket";
        size_t startedEndpointCount = 0;

        auto endpointManager = std::make_shared<TTestEndpointManager>();
        endpointManager->StartEndpointHandler = [&] (
            std::shared_ptr<NProto::TStartEndpointRequest> request)
        {
            if (request->GetUnixSocketPath() == wrongSocketPath) {
                return MakeFuture<NProto::TStartEndpointResponse>(
                    TErrorResponse(E_FAIL));
            }

            ++startedEndpointCount;
            return MakeFuture(NProto::TStartEndpointResponse());
        };

        NMonitoring::TDynamicCountersPtr counters = new NMonitoring::TDynamicCounters();
        InitCriticalEventsCounter(counters);
        auto configCounter =
            counters->GetCounter("AppCriticalEvents/EndpointRestoringError", true);

        UNIT_ASSERT_VALUES_EQUAL(0, static_cast<int>(*configCounter));

        const TString dirPath = "./" + CreateGuidAsString();
        auto endpointStorage = CreateFileEndpointStorage(dirPath);
        auto mutableStorage = CreateFileMutableEndpointStorage(dirPath);

        auto initError = mutableStorage->Init();
        UNIT_ASSERT_C(!HasError(initError), initError);

        Y_DEFER {
            auto error = mutableStorage->Remove();
            UNIT_ASSERT_C(!HasError(error), error);
        };

        size_t wrongDataCount = 3;
        size_t wrongSocketCount = 4;
        size_t correctCount = 5;
        size_t num = 0;

        for (size_t i = 0; i < wrongDataCount; ++i) {
            auto keyOrError = mutableStorage->AddEndpoint(
                TStringBuilder() << "endpoint" << ++num,
                "invalid proto request data");
            UNIT_ASSERT_C(!HasError(keyOrError), keyOrError.GetError());
        }

        for (size_t i = 0; i < wrongSocketCount; ++i) {
            NProto::TStartEndpointRequest request;
            request.SetUnixSocketPath(wrongSocketPath);

            auto keyOrError = mutableStorage->AddEndpoint(
                TStringBuilder() << "endpoint" << ++num,
                SerializeEndpoint(request));
            UNIT_ASSERT_C(!HasError(keyOrError), keyOrError.GetError());
        }

        for (size_t i = 0; i < correctCount; ++i) {
            NProto::TStartEndpointRequest request;
            request.SetUnixSocketPath("endpoint.sock");

            auto keyOrError = mutableStorage->AddEndpoint(
                TStringBuilder() << "endpoint" << ++num,
                SerializeEndpoint(request));
            UNIT_ASSERT_C(!HasError(keyOrError), keyOrError.GetError());
        }

        auto endpointService = CreateMultipleEndpointService(
            nullptr,
            CreateWallClockTimer(),
            CreateSchedulerStub(),
            CreateLoggingService("console"),
            CreateRequestStatsStub(),
            CreateVolumeStatsStub(),
            CreateServerStatsStub(),
            endpointStorage,
            endpointManager,
            {});

        endpointService->RestoreEndpoints().Wait();

        UNIT_ASSERT_VALUES_EQUAL(
            wrongDataCount + wrongSocketCount,
            static_cast<int>(*configCounter));
        UNIT_ASSERT_VALUES_EQUAL(correctCount, startedEndpointCount);
    }

    Y_UNIT_TEST(ShouldThrowCriticalEventIfNotFoundEndpointStorage)
    {
        NMonitoring::TDynamicCountersPtr counters = new NMonitoring::TDynamicCounters();
        InitCriticalEventsCounter(counters);
        auto configCounter =
            counters->GetCounter("AppCriticalEvents/EndpointRestoringError", true);

        UNIT_ASSERT_VALUES_EQUAL(0, static_cast<int>(*configCounter));

        const TString dirPath = "./invalidEndpointStoragePath";
        auto endpointStorage = CreateFileEndpointStorage(dirPath);

        auto endpointService = CreateMultipleEndpointService(
            nullptr,
            CreateWallClockTimer(),
            CreateSchedulerStub(),
            CreateLoggingService("console"),
            CreateRequestStatsStub(),
            CreateVolumeStatsStub(),
            CreateServerStatsStub(),
            endpointStorage,
            std::make_shared<TTestEndpointManager>(),
            {});

        endpointService->RestoreEndpoints().Wait();

        UNIT_ASSERT_VALUES_EQUAL(1, static_cast<int>(*configCounter));
    }

    Y_UNIT_TEST(ShouldNotThrowCriticalEventIfKeyringDescIsEmpty)
    {
        NMonitoring::TDynamicCountersPtr counters = new NMonitoring::TDynamicCounters();
        InitCriticalEventsCounter(counters);
        auto configCounter =
            counters->GetCounter("AppCriticalEvents/EndpointRestoringError", true);

        UNIT_ASSERT_VALUES_EQUAL(0, static_cast<int>(*configCounter));

        auto endpointStorage = CreateKeyringEndpointStorage("nbs", "");

        auto endpointService = CreateMultipleEndpointService(
            nullptr,
            CreateWallClockTimer(),
            CreateSchedulerStub(),
            CreateLoggingService("console"),
            CreateRequestStatsStub(),
            CreateVolumeStatsStub(),
            CreateServerStatsStub(),
            endpointStorage,
            std::make_shared<TTestEndpointManager>(),
            {});

        endpointService->RestoreEndpoints().Wait();

        UNIT_ASSERT_VALUES_EQUAL(0, static_cast<int>(*configCounter));
    }

    Y_UNIT_TEST(ShouldHandleRestoreFlagInListEndpointsResponse)
    {
        auto trigger = NewPromise<NProto::TStartEndpointResponse>();

        auto endpointManager = std::make_shared<TTestEndpointManager>();
        endpointManager->StartEndpointHandler = [&] (
            std::shared_ptr<NProto::TStartEndpointRequest> request)
        {
            Y_UNUSED(request);
            return trigger;
        };

        endpointManager->ListEndpointsHandler = [&] (
            std::shared_ptr<NProto::TListEndpointsRequest> request)
        {
            Y_UNUSED(request);
            return MakeFuture(NProto::TListEndpointsResponse());
        };

        const TString dirPath = "./" + CreateGuidAsString();
        auto endpointStorage = CreateFileEndpointStorage(dirPath);
        auto mutableStorage = CreateFileMutableEndpointStorage(dirPath);

        auto initError = mutableStorage->Init();
        UNIT_ASSERT_C(!HasError(initError), initError);

        Y_DEFER {
            auto error = mutableStorage->Remove();
            UNIT_ASSERT_C(!HasError(error), error);
        };

        size_t endpointCount = 5;

        for (size_t i = 0; i < endpointCount; ++i) {
            NProto::TStartEndpointRequest request;
            request.SetUnixSocketPath("endpoint.sock");

            auto keyOrError = mutableStorage->AddEndpoint(
                TStringBuilder() << "endpoint" << i,
                SerializeEndpoint(request));
            UNIT_ASSERT_C(!HasError(keyOrError), keyOrError.GetError());
        }

        auto endpointService = CreateMultipleEndpointService(
            nullptr,
            CreateWallClockTimer(),
            CreateSchedulerStub(),
            CreateLoggingService("console"),
            CreateRequestStatsStub(),
            CreateVolumeStatsStub(),
            CreateServerStatsStub(),
            endpointStorage,
            endpointManager,
            {});

        auto restoreFuture = endpointService->RestoreEndpoints();
        UNIT_ASSERT(!restoreFuture.HasValue());

        {
            auto future = endpointService->ListEndpoints(
                MakeIntrusive<TCallContext>(),
                std::make_shared<NProto::TListEndpointsRequest>());
            auto response = future.GetValue(TDuration::Seconds(1));
            UNIT_ASSERT(!response.GetEndpointsWereRestored());
        }

        trigger.SetValue(NProto::TStartEndpointResponse());
        restoreFuture.Wait();

        {
            auto future = endpointService->ListEndpoints(
                MakeIntrusive<TCallContext>(),
                std::make_shared<NProto::TListEndpointsRequest>());
            auto response = future.GetValue(TDuration::Seconds(1));
            UNIT_ASSERT(response.GetEndpointsWereRestored());
        }
    }

    Y_UNIT_TEST(ShouldListKeyrings)
    {
        const TString dirPath = "./" + CreateGuidAsString();
        auto endpointStorage = CreateFileEndpointStorage(dirPath);
        auto mutableStorage = CreateFileMutableEndpointStorage(dirPath);

        auto initError = mutableStorage->Init();
        UNIT_ASSERT_C(!HasError(initError), initError);

        Y_DEFER {
            auto error = mutableStorage->Remove();
            UNIT_ASSERT_C(!HasError(error), error);
        };

        const ui32 endpointCount = 42;
        THashMap<ui32, NProto::TStartEndpointRequest> requests;

        for (size_t i = 0; i < endpointCount; ++i) {
            NProto::TStartEndpointRequest request;
            request.SetUnixSocketPath("testSocket" + ToString(i + 1));
            request.SetDiskId("testDiskId" + ToString(i + 1));
            request.SetIpcType(NProto::IPC_GRPC);

            auto keyOrError = mutableStorage->AddEndpoint(
                request.GetDiskId(),
                SerializeEndpoint(request));
            UNIT_ASSERT_C(!HasError(keyOrError), keyOrError.GetError());

            requests.emplace(keyOrError.GetResult(), request);
        }

        auto endpointService = CreateMultipleEndpointService(
            nullptr,
            CreateWallClockTimer(),
            CreateSchedulerStub(),
            CreateLoggingService("console"),
            CreateRequestStatsStub(),
            CreateVolumeStatsStub(),
            CreateServerStatsStub(),
            endpointStorage,
            std::make_shared<TTestEndpointManager>(),
            {});

        auto future = endpointService->ListKeyrings(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TListKeyringsRequest>());

        auto response = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_C(!HasError(response), response);

        auto& endpoints = response.GetEndpoints();
        UNIT_ASSERT_VALUES_EQUAL(endpointCount, endpoints.size());

        for (const auto& endpoint: endpoints) {
            auto it = requests.find(endpoint.GetKeyringId());
            UNIT_ASSERT(it != requests.end());

            google::protobuf::util::MessageDifferencer comparator;
            UNIT_ASSERT(comparator.Equals(it->second, endpoint.GetRequest()));
        }
    }
}

}   // namespace NCloud::NBlockStore::NServer
