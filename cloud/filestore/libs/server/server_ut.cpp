#include "server.h"

#include "config.h"

#include <cloud/filestore/libs/client/client.h>
#include <cloud/filestore/libs/client/config.h>
#include <cloud/filestore/libs/diagnostics/profile_log.h>
#include <cloud/filestore/libs/diagnostics/request_stats.h>
#include <cloud/filestore/libs/service/context.h>
#include <cloud/filestore/libs/service/filestore_test.h>

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <library/cpp/monlib/dynamic_counters/counters.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <util/datetime/base.h>

namespace NCloud::NFileStore::NServer {

using namespace NThreading;

using namespace NCloud::NFileStore::NClient;

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr TDuration WaitTimeout = TDuration::Seconds(5);

template <typename F>
bool WaitFor(F&& event)
{
    TInstant deadline = WaitTimeout.ToDeadLine();
    while (TInstant::Now() < deadline) {
        if (event()) {
            return true;
        }
        Sleep(TDuration::MilliSeconds(100));
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////

template <typename T>
class TAtomicStorage
{
private:
    TMaybe<T> Value;
    TMutex Lock;

public:
    void Set(T value)
    {
        with_lock (Lock) {
            Value = std::move(value);
        }
    }

    T Get() const
    {
        with_lock (Lock) {
            return Value.GetRef();
        }
    }

    bool Empty() const
    {
        with_lock (Lock) {
            return Value.Empty();
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TBootstrap
{
    NMonitoring::TDynamicCountersPtr Counters;

    IServerPtr Server;
    IFileStoreServicePtr Client;
    std::shared_ptr<TFileStoreTest> Service;
    bool Stopped = false;

public:
    TBootstrap()
        : Counters{MakeIntrusive<NMonitoring::TDynamicCounters>()}
        , Service{std::make_shared<TFileStoreTest>()}
    {
        auto logging = CreateLoggingService("console");
        auto registry = CreateRequestStatsRegistry(
            "server_ut",
            {},
            Counters,
            CreateWallClockTimer()
        );

        auto [clientConfig, serverConfig] = CreateConfigs();
        Server = CreateServer(
            serverConfig,
            logging,
            registry->GetServerStats(),
            CreateProfileLogStub(),
            Service);

        Client = CreateFileStoreClient(clientConfig, logging);
    }

    ~TBootstrap()
    {
        Stop();
    }

    void Start()
    {
        Server->Start();
        Client->Start();
    }

    void Stop()
    {
        if (!Stopped) {
            Server->Stop();
            Client->Stop();
            Stopped = true;
        }
    }

private:
    TPortManager PortManager;

    std::pair<TClientConfigPtr, TServerConfigPtr> CreateConfigs()
    {
        NProto::TClientConfig client;
        client.SetPort(PortManager.GetPort(9021));

        NProto::TServerConfig server;
        server.SetPort(client.GetPort());

        return {
            std::make_shared<TClientConfig>(client),
            std::make_shared<TServerConfig>(server),
        };
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TServerTest)
{
    Y_UNIT_TEST(ShouldHandleRequests)
    {
        TBootstrap bootstrap;

        bootstrap.Service->PingHandler = [] (std::shared_ptr<NProto::TPingRequest> request) {
            Y_UNUSED(request);
            return MakeFuture<NProto::TPingResponse>();
        };

        bootstrap.Start();

        auto context = MakeIntrusive<TCallContext>();
        auto request = std::make_shared<NProto::TPingRequest>();

        auto future = bootstrap.Client->Ping(
            std::move(context),
            std::move(request));

        const auto& response = future.GetValue(WaitTimeout);
        UNIT_ASSERT_C(!HasError(response), FormatError(response.GetError()));
    }

    Y_UNIT_TEST(ShouldGetSessionEventsStream)
    {
        TAtomicStorage<IResponseHandlerPtr<NProto::TGetSessionEventsResponse>> storage;

        TBootstrap bootstrap;
        bootstrap.Service->GetSessionEventsStreamHandler =
            [&] (std::shared_ptr<NProto::TGetSessionEventsRequest> request,
                 IResponseHandlerPtr<NProto::TGetSessionEventsResponse> responseHandler)
        {
            Y_UNUSED(request);
            storage.Set(responseHandler);
        };

        bootstrap.Start();

        auto context = MakeIntrusive<TCallContext>();
        auto request = std::make_shared<NProto::TGetSessionEventsRequest>();
        auto responseHandler = std::make_shared<TResponseHandler>();

        bootstrap.Client->GetSessionEventsStream(
            std::move(context),
            std::move(request),
            responseHandler);
        UNIT_ASSERT(WaitFor([&] { return !storage.Empty(); }));

        auto serverHandler = storage.Get();

        NProto::TGetSessionEventsResponse respose;
        auto* event = respose.AddEvents();
        event->SetSeqNo(1);

        serverHandler->HandleResponse(respose);
        UNIT_ASSERT(WaitFor([=] { return responseHandler->GotResponse(); }));

        serverHandler->HandleCompletion({});
        UNIT_ASSERT(WaitFor([=] { return responseHandler->GotCompletion(); }));
    }

    Y_UNIT_TEST(ShouldHitErrorMetricOnFailure)
    {
        TBootstrap bootstrap;
        bootstrap.Service->CreateNodeHandler = [] (auto) {
            return MakeFuture<NProto::TCreateNodeResponse>(TErrorResponse(E_IO, ""));
        };

        bootstrap.Start();

        auto context = MakeIntrusive<TCallContext>();
        auto request = std::make_shared<NProto::TCreateNodeRequest>();
        auto future = bootstrap.Client->CreateNode(
            std::move(context),
            std::move(request));

        auto response = future.GetValueSync();
        UNIT_ASSERT(HasError(response));
        UNIT_ASSERT_EQUAL(E_IO, response.GetError().GetCode());
        bootstrap.Stop();

        auto counters = bootstrap.Counters
            ->FindSubgroup("component", "server_ut")
            ->FindSubgroup("request", "CreateNode");
        UNIT_ASSERT_EQUAL(1, counters->GetCounter("Errors")->GetAtomic());
        UNIT_ASSERT_EQUAL(1, counters->GetCounter("Errors/Fatal")->GetAtomic());
    }

}

}   // namespace NCloud::NFileStore::NServer
