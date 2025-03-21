#include "listener.h"
#include "service.h"

#include <cloud/filestore/libs/service/context.h>
#include <cloud/filestore/libs/service/endpoint.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/scheduler_test.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/keyring/endpoints.h>
#include <cloud/storage/core/libs/keyring/endpoints_test.h>

#include <library/cpp/testing/unittest/registar.h>

#include <google/protobuf/util/message_differencer.h>

#include <util/generic/guid.h>
#include <util/generic/scope.h>

namespace NCloud::NFileStore::NServer {

using namespace NThreading;

namespace {
////////////////////////////////////////////////////////////////////////////////

constexpr TDuration WaitTimeout = TDuration::Seconds(5);

////////////////////////////////////////////////////////////////////////////////

struct TTestEndpoint:
    public IEndpoint
{
    NProto::TEndpointConfig Config;

    TPromise<NProto::TError> Start = NewPromise<NProto::TError>();
    TPromise<void> Stop = NewPromise();

    bool Ready = true;

    TTestEndpoint(const NProto::TEndpointConfig& config, bool ready = true)
        : Config(config)
        , Ready(ready)
    {}

    ~TTestEndpoint()
    {
        if (!Start.HasValue()) {
            Start.SetValue({});
        }

        if (!Stop.HasValue()) {
            Stop.SetValue();
        }
    }

    TFuture<NProto::TError> StartAsync() override
    {
        if (!Ready) {
            return Start;
        }

        return MakeFuture<NProto::TError>();
    }

    TFuture<void> StopAsync() override
    {
        if (!Ready) {
            return Stop;
        }

        return MakeFuture();
    }

    TFuture<void> SuspendAsync() override
    {
        if (!Ready) {
            return Stop;
        }

        return MakeFuture();
    }

    TVector<TIncompleteRequest> GetIncompleteRequests() override
    {
        return {};
    }
};

using TTestEndpointPtr = std::shared_ptr<TTestEndpoint>;

////////////////////////////////////////////////////////////////////////////////

struct TTestEndpointListener final
    : public IEndpointListener
{
    using TCreateEndpointHandler = std::function<
        IEndpointPtr(const NProto::TEndpointConfig& config)
        >;

    TCreateEndpointHandler CreateEndpointHandler;
    TVector<TTestEndpointPtr> Endpoints;

    IEndpointPtr CreateEndpoint(
        const NProto::TEndpointConfig& config) override
    {
        if (CreateEndpointHandler) {
            return CreateEndpointHandler(config);
        }

        Endpoints.emplace_back(std::make_shared<TTestEndpoint>(config));
        return Endpoints.back();
    }
};

////////////////////////////////////////////////////////////////////////////////

NProto::TKickEndpointResponse KickEndpoint(IEndpointManager& service, ui32 keyringId)
{
    auto request = std::make_shared<NProto::TKickEndpointRequest>();
    request->SetKeyringId(keyringId);

    auto future = service.KickEndpoint(
        MakeIntrusive<TCallContext>(),
        std::move(request));

     return future.GetValue(WaitTimeout);
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TServiceEndpointTest)
{
    Y_UNIT_TEST(ShouldHandleKickEndpoint)
    {
        TString id = "id";
        TString unixSocket = "testSocket";

        NProto::TStartEndpointRequest start;
        auto config = start.MutableEndpoint();
        config->SetFileSystemId(id);
        config->SetSocketPath(unixSocket);
        config->SetClientId("client");

        const TString dirPath = "./" + CreateGuidAsString();
        auto endpointStorage = CreateFileEndpointStorage(dirPath);
        auto mutableStorage = CreateFileMutableEndpointStorage(dirPath);

        auto initError = mutableStorage->Init();
        UNIT_ASSERT_C(!HasError(initError), initError);

        Y_DEFER {
            auto error = mutableStorage->Remove();
            UNIT_ASSERT_C(!HasError(error), error);
        };

        auto endpoint = std::make_shared<TTestEndpoint>(*config, false);
        auto listener = std::make_shared<TTestEndpointListener>();
        listener->CreateEndpointHandler =
            [&] (const NProto::TEndpointConfig&) {
                listener->Endpoints.push_back(endpoint);
                return endpoint;
            };

        auto service = CreateEndpointManager(
            CreateLoggingService("console"),
            endpointStorage,
            listener);
        service->Start();

        auto keyOrError = mutableStorage->AddEndpoint(
            id,
            SerializeEndpoint(start));
        UNIT_ASSERT_C(!HasError(keyOrError), keyOrError.GetError());

        auto request = std::make_shared<NProto::TKickEndpointRequest>();
        request->SetKeyringId(keyOrError.GetResult());

        auto future = service->KickEndpoint(
            MakeIntrusive<TCallContext>(),
            request);

        UNIT_ASSERT(!future.HasValue());
        endpoint->Start.SetValue(NProto::TError{});
        UNIT_ASSERT(future.Wait(WaitTimeout));

        auto response = future.GetValue();
        UNIT_ASSERT_C(!HasError(response), response.ShortDebugString());

        UNIT_ASSERT_VALUES_EQUAL(listener->Endpoints.size(), 1);
        const auto& created = *listener->Endpoints[0];

        google::protobuf::util::MessageDifferencer comparator;
        UNIT_ASSERT(comparator.Equals(created.Config, *config));

        // kick unexisting
        auto wrongKeyringId = keyOrError.GetResult() + 42;
        response = KickEndpoint(*service, wrongKeyringId);

        UNIT_ASSERT(HasError(response));
        UNIT_ASSERT_VALUES_EQUAL(response.GetError().GetCode(), E_INVALID_STATE);
        UNIT_ASSERT_VALUES_EQUAL(listener->Endpoints.size(), 1);

        endpoint->Stop.SetValue();

        UNIT_ASSERT_NO_EXCEPTION(service->Stop());
    }
}

}   // namespace NCloud::NFileStore::NServer
