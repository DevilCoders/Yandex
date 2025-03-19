#include "durable.h"

#include "config.h"

#include <cloud/filestore/libs/service/context.h>
#include <cloud/filestore/libs/service/filestore_test.h>

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/scheduler.h>
#include <cloud/storage/core/libs/common/scheduler_test.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <util/datetime/base.h>
#include <util/generic/scope.h>

namespace NCloud::NFileStore::NClient {

using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr TDuration WaitTimeout = TDuration::Seconds(5);
constexpr size_t MaxRetryCount = 3;

////////////////////////////////////////////////////////////////////////////////

struct TTestBootstrap
{
    ITimerPtr Timer = CreateWallClockTimer();
    ISchedulerPtr Scheduler = CreateScheduler();
    ILoggingServicePtr Logging = CreateLoggingService("console");
    TClientConfigPtr Config = std::make_shared<TClientConfig>(NProto::TClientConfig{});
    IRetryPolicyPtr Policy = CreateRetryPolicy(Config);

    std::shared_ptr<TFileStoreTest> Client = std::make_shared<TFileStoreTest>();
    IFileStoreServicePtr Durable;

    TTestBootstrap(
            ITimerPtr timer = CreateWallClockTimer(),
            ISchedulerPtr scheduler = CreateScheduler())
        : Timer(std::move(timer))
        , Scheduler(std::move(scheduler))
    {
        Durable = CreateDurableClient(
            Logging,
            Timer,
            Scheduler,
            Policy,
            Client);

        Start();
    }

    TTestBootstrap(TClientConfigPtr config)
        : Config(std::move(config))
    {
        Durable = CreateDurableClient(
            Logging,
            Timer,
            Scheduler,
            Policy,
            Client);

        Start();
    }

    ~TTestBootstrap()
    {
        Stop();
    }

    void Start()
    {
        if (Scheduler) {
            Scheduler->Start();
        }

        if (Logging) {
            Logging->Start();
        }

        if (Durable) {
            Durable->Start();
        }
    }

    void Stop()
    {
        if (Durable) {
            Durable->Stop();
        }

        if (Logging) {
            Logging->Stop();
        }

        if (Scheduler) {
            Scheduler->Stop();
        }
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TDurableClientTest)
{
    Y_UNIT_TEST(ShouldRetryUndeliveredRequests)
    {
        TTestBootstrap bootstrap;

        size_t requestsCount = 0;
        bootstrap.Client->PingHandler =
            [&] (std::shared_ptr<NProto::TPingRequest> request) {
                Y_UNUSED(request);

                NProto::TPingResponse response;
                if (++requestsCount < MaxRetryCount) {
                    auto& error = *response.MutableError();
                    error.SetCode(E_GRPC_ABORTED);
                }

                return MakeFuture(std::move(response));
            };

        auto future = bootstrap.Durable->Ping(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TPingRequest>());

        const auto& response = future.GetValue(WaitTimeout);
        UNIT_ASSERT(!HasError(response));
        UNIT_ASSERT_EQUAL(requestsCount, MaxRetryCount);
    }

    Y_UNIT_TEST(ShouldNotRetryNonRetriableRequests)
    {
        TTestBootstrap bootstrap;

        size_t requestsCount = 0;
        bootstrap.Client->PingHandler =
            [&] (std::shared_ptr<NProto::TPingRequest> request) {
                Y_UNUSED(request);
                ++requestsCount;

                NProto::TPingResponse response;

                auto& error = *response.MutableError();
                error.SetCode(E_FAIL);

                return MakeFuture(std::move(response));
            };

        auto future = bootstrap.Durable->Ping(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TPingRequest>());

        const auto& response = future.GetValue(WaitTimeout);
        UNIT_ASSERT(HasError(response));
        UNIT_ASSERT_EQUAL(requestsCount, 1);
    }

    Y_UNIT_TEST(ShouldHandleInFlightRequestsAfterDurableClientStop)
    {
        TTestBootstrap bootstrap;

        auto promise = NewPromise<NProto::TPingResponse>();
        bootstrap.Client->PingHandler = [&] (std::shared_ptr<NProto::TPingRequest>) {
            return promise;
        };

        TFuture<NProto::TPingResponse> future;
        {
            IFileStoreServicePtr durable = std::move(bootstrap.Durable);
            UNIT_ASSERT(!bootstrap.Durable);

            future = durable->Ping(
                MakeIntrusive<TCallContext>(),
                std::make_shared<NProto::TPingRequest>());

            durable->Stop();
        }

        UNIT_ASSERT(!future.HasValue());

        NProto::TPingResponse resp;
        promise.SetValue(resp);

        const auto& response = future.GetValue(WaitTimeout);
        UNIT_ASSERT(HasError(response));
    }

    Y_UNIT_TEST(ShouldHandleWaitingRequestsAfterDurableClientStop)
    {
        auto scheduler = std::make_shared<TTestScheduler>();
        TTestBootstrap bootstrap(CreateWallClockTimer(), scheduler);

        bootstrap.Client->PingHandler =
            [&] (std::shared_ptr<NProto::TPingRequest> request) {
                Y_UNUSED(request);

                NProto::TPingResponse response;
                auto& error = *response.MutableError();
                error.SetCode(E_GRPC_ABORTED);

                return MakeFuture(std::move(response));
            };

        TFuture<NProto::TPingResponse> future;
        {
            IFileStoreServicePtr durable = std::move(bootstrap.Durable);
            UNIT_ASSERT(!bootstrap.Durable);

            future = durable->Ping(
                MakeIntrusive<TCallContext>(),
                std::make_shared<NProto::TPingRequest>());

            durable->Stop();
        }

        UNIT_ASSERT(!future.HasValue());
        scheduler->RunAllScheduledTasks();

        const auto& response = future.GetValue(WaitTimeout);
        UNIT_ASSERT(HasError(response));
        UNIT_ASSERT(response.GetError().GetCode() != E_GRPC_ABORTED);
    }

    Y_UNIT_TEST(ShouldCalculateCorrectTimeout)
    {
        NProto::TClientConfig proto;
        proto.SetRetryTimeoutIncrement(2'000);
        proto.SetConnectionErrorMaxRetryTimeout(3'000);

        auto config = std::make_shared<TClientConfig>(proto);
        auto policy = CreateRetryPolicy(config);
        {
            TRetryState state;

            bool retry = policy->ShouldRetry(state, MakeError(E_REJECTED));
            UNIT_ASSERT(retry);
            UNIT_ASSERT_VALUES_EQUAL(TDuration::Seconds(2), state.Backoff);

            retry = policy->ShouldRetry(state, MakeError(E_REJECTED));
            UNIT_ASSERT(retry);
            UNIT_ASSERT_VALUES_EQUAL(TDuration::Seconds(4), state.Backoff);

            retry = policy->ShouldRetry(state, MakeError(E_GRPC_UNAVAILABLE));
            UNIT_ASSERT(retry);
            UNIT_ASSERT_VALUES_EQUAL(TDuration::Seconds(3), state.Backoff);

            retry = policy->ShouldRetry(state, MakeError(E_REJECTED));
            UNIT_ASSERT(retry);
            UNIT_ASSERT_VALUES_EQUAL(TDuration::Seconds(6), state.Backoff);
        }

        {
            TRetryState state;

            bool retry = policy->ShouldRetry(state, MakeError(S_OK));
            UNIT_ASSERT(!retry);
            UNIT_ASSERT_VALUES_EQUAL(TDuration::Zero(), state.Backoff);

            retry = policy->ShouldRetry(state, MakeError(E_GRPC_UNAVAILABLE));
            UNIT_ASSERT(retry);
            UNIT_ASSERT_VALUES_EQUAL(TDuration::Seconds(2), state.Backoff);

            retry = policy->ShouldRetry(state, MakeError(E_GRPC_UNAVAILABLE));
            UNIT_ASSERT(retry);
            UNIT_ASSERT_VALUES_EQUAL(TDuration::Seconds(3), state.Backoff);
        }
    }

    Y_UNIT_TEST(ShouldReturnNonRetriableErrorAfterRetryTimeout)
    {
        NProto::TClientConfig proto;
        proto.SetRetryTimeout(300);

        auto config = std::make_shared<TClientConfig>(proto);

        TTestBootstrap bootstrap(config);
        bootstrap.Client->PingHandler =
            [&] (std::shared_ptr<NProto::TPingRequest> request) {
                Y_UNUSED(request);

                return MakeFuture<NProto::TPingResponse>(
                    TErrorResponse(E_REJECTED));
            };

        auto future = bootstrap.Durable->Ping(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TPingRequest>());

        const auto& response = future.GetValue(TDuration::Seconds(5));
        UNIT_ASSERT_C(
            EErrorKind::ErrorRetriable != GetErrorKind(response.GetError()),
            response.GetError());

        TRetryState retryState;
        auto retry = bootstrap.Policy->ShouldRetry(retryState, response.GetError());
        UNIT_ASSERT(!retry);
    }
}

}   // namespace NCloud::NFileStore::NClient
