#include "service_endpoint.h"

#include "endpoint_manager.h"

#include <cloud/blockstore/libs/client/config.h>
#include <cloud/blockstore/libs/client/durable.h>
#include <cloud/blockstore/libs/client/metric.h>
#include <cloud/blockstore/libs/diagnostics/critical_events.h>
#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/service/service.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/scheduler.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/keyring/endpoints.h>

#include <util/generic/guid.h>
#include <util/generic/map.h>

namespace NCloud::NBlockStore::NServer {

using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

template <typename TService>
class TServiceWrapper
    : public TService
{
private:
    IBlockStorePtr Service;

public:
    TServiceWrapper(IBlockStorePtr service)
        : Service(std::move(service))
    {}

    void Start() override
    {
        if (Service) {
            Service->Start();
        }
    }

    void Stop() override
    {
        if (Service) {
            Service->Stop();
        }
    }

    TStorageBuffer AllocateBuffer(size_t bytesCount) override
    {
        return Service->AllocateBuffer(bytesCount);
    }

#define BLOCKSTORE_IMPLEMENT_METHOD(name, ...)                                 \
    TFuture<NProto::T##name##Response> name(                                   \
        TCallContextPtr ctx,                                                   \
        std::shared_ptr<NProto::T##name##Request> request) override            \
    {                                                                          \
        return Service->name(std::move(ctx), std::move(request));              \
    }                                                                          \
// BLOCKSTORE_IMPLEMENT_METHOD

    BLOCKSTORE_SERVICE(BLOCKSTORE_IMPLEMENT_METHOD)

#undef BLOCKSTORE_IMPLEMENT_METHOD
};

////////////////////////////////////////////////////////////////////////////////

class TServiceAdapter final
    : public TServiceWrapper<IBlockStore>
{
private:
    const IEndpointManagerPtr EndpointManager;
    const TString ClientId;

public:
    TServiceAdapter(IEndpointManagerPtr endpointManager, TString clientId)
        : TServiceWrapper(nullptr)
        , EndpointManager(std::move(endpointManager))
        , ClientId(std::move(clientId))
    {}

    TFuture<NProto::TStartEndpointResponse> StartEndpoint(
        TCallContextPtr ctx,
        std::shared_ptr<NProto::TStartEndpointRequest> request) override
    {
        auto& headers = *request->MutableHeaders();
        headers.SetClientId(ClientId);

        auto requestId = headers.GetRequestId();
        if (!requestId) {
            headers.SetRequestId(CreateRequestId());
        }

        return EndpointManager->StartEndpoint(
            std::move(ctx),
            std::move(request));
    }
};

////////////////////////////////////////////////////////////////////////////////

class TMultipleEndpointService final
    : public TServiceWrapper<IEndpointService>
    , public std::enable_shared_from_this<TMultipleEndpointService>
{
private:
    const ITimerPtr Timer;
    const ISchedulerPtr Scheduler;
    const ILoggingServicePtr Logging;
    const IRequestStatsPtr RequestStats;
    const IVolumeStatsPtr VolumeStats;
    const IServerStatsPtr ServerStats;
    const IEndpointStoragePtr EndpointStorage;
    const IEndpointManagerPtr EndpointManager;
    const NProto::TClientConfig ClientConfig;

    TLog Log;
    IBlockStorePtr RestoringClient;
    IIncompleteRequestProviderPtr IncompleteRequestProvider;

    TAtomic Restored = 0;

public:
    TMultipleEndpointService(
            IBlockStorePtr service,
            ITimerPtr timer,
            ISchedulerPtr scheduler,
            ILoggingServicePtr logging,
            IRequestStatsPtr requestStats,
            IVolumeStatsPtr volumeStats,
            IServerStatsPtr serverStats,
            IEndpointStoragePtr endpointStorage,
            IEndpointManagerPtr endpointManager,
            NProto::TClientConfig clientConfig)
        : TServiceWrapper(std::move(service))
        , Timer(std::move(timer))
        , Scheduler(std::move(scheduler))
        , Logging(std::move(logging))
        , RequestStats(std::move(requestStats))
        , VolumeStats(std::move(volumeStats))
        , ServerStats(std::move(serverStats))
        , EndpointStorage(std::move(endpointStorage))
        , EndpointManager(std::move(endpointManager))
        , ClientConfig(std::move(clientConfig))
    {
        Log = Logging->CreateLog("BLOCKSTORE_SERVER");

        InitRestoringClient();
    }

    void Start() override
    {
        TServiceWrapper::Start();

        RestoringClient->Start();
    }

    void Stop() override
    {
        RestoringClient->Stop();

        TServiceWrapper::Stop();
    }

    TVector<TIncompleteRequest> GetIncompleteRequests() override
    {
        return IncompleteRequestProvider->GetIncompleteRequests();
    }

    TFuture<NProto::TStartEndpointResponse> StartEndpoint(
        TCallContextPtr ctx,
        std::shared_ptr<NProto::TStartEndpointRequest> request) override
    {
        auto timeout = TDuration::MilliSeconds(
            request->GetHeaders().GetRequestTimeout());

        auto future = EndpointManager->StartEndpoint(
            std::move(ctx),
            std::move(request));

        return CreateTimeoutFuture(future, timeout);
    }

    TFuture<NProto::TStopEndpointResponse> StopEndpoint(
        TCallContextPtr ctx,
        std::shared_ptr<NProto::TStopEndpointRequest> request) override
    {
        auto timeout = TDuration::MilliSeconds(
            request->GetHeaders().GetRequestTimeout());

        auto future = EndpointManager->StopEndpoint(
            std::move(ctx),
            std::move(request));

        return CreateTimeoutFuture(future, timeout);
    }

    TFuture<NProto::TListEndpointsResponse> ListEndpoints(
        TCallContextPtr ctx,
        std::shared_ptr<NProto::TListEndpointsRequest> request) override
    {
        auto timeout = TDuration::MilliSeconds(
            request->GetHeaders().GetRequestTimeout());

        auto future = EndpointManager->ListEndpoints(
            std::move(ctx),
            std::move(request));

        bool restored = AtomicGet(Restored);

        future = future.Apply([restored] (const auto& f) {
            auto response = f.GetValue();
            response.SetEndpointsWereRestored(restored);
            return response;
        });

        return CreateTimeoutFuture(future, timeout);
    }

    TFuture<NProto::TKickEndpointResponse> KickEndpoint(
        TCallContextPtr ctx,
        std::shared_ptr<NProto::TKickEndpointRequest> request) override
    {
        auto timeout = TDuration::MilliSeconds(
            request->GetHeaders().GetRequestTimeout());

        auto future = KickEndpointImpl(
            std::move(ctx),
            std::move(request));

        return CreateTimeoutFuture(future, timeout);
    }

    TFuture<NProto::TListKeyringsResponse> ListKeyrings(
        TCallContextPtr ctx,
        std::shared_ptr<NProto::TListKeyringsRequest> request) override
    {
        Y_UNUSED(ctx);
        Y_UNUSED(request);

        auto response = ListKeyringsImpl();
        return MakeFuture(std::move(response));
    }

    TFuture<NProto::TDescribeEndpointResponse> DescribeEndpoint(
        TCallContextPtr ctx,
        std::shared_ptr<NProto::TDescribeEndpointRequest> request) override
    {
        auto timeout = TDuration::MilliSeconds(
            request->GetHeaders().GetRequestTimeout());

        auto future = EndpointManager->DescribeEndpoint(
            std::move(ctx),
            std::move(request));

        return CreateTimeoutFuture(future, timeout);
    }

    TFuture<void> RestoreEndpoints() override
    {
        auto weakPtr = weak_from_this();
        return RestoreEndpointsImpl().Apply([weakPtr] (const auto& future) {
            if (auto ptr = weakPtr.lock()) {
                AtomicSet(ptr->Restored, 1);
            }
            return future.GetValue();
        });
    }

private:
    TFuture<NProto::TKickEndpointResponse> KickEndpointImpl(
        TCallContextPtr ctx,
        std::shared_ptr<NProto::TKickEndpointRequest> request)
    {
        auto requestOrError = EndpointStorage->GetEndpoint(
            request->GetKeyringId());

        if (HasError(requestOrError)) {
            return MakeFuture<NProto::TKickEndpointResponse>(
                TErrorResponse(requestOrError.GetError()));
        }

        auto startRequest = DeserializeEndpoint<NProto::TStartEndpointRequest>(
            requestOrError.GetResult());

        if (!startRequest) {
            NProto::TKickEndpointResponse response;
            *response.MutableError() = MakeError(E_INVALID_STATE, TStringBuilder()
                << "Failed to deserialize endpoint with key "
                << request->GetKeyringId());
            return MakeFuture(std::move(response));
        }

        if (GetRequestId(*startRequest) == 0) {
            startRequest->MutableHeaders()->SetRequestId(
                GetRequestId(*request));
        }

        STORAGE_INFO("Kick StartEndpoint request: " << *startRequest);
        auto future = StartEndpoint(
            std::move(ctx),
            std::move(startRequest));

        return future.Apply([] (const auto& f) {
            const auto& startResponse = f.GetValue();

            NProto::TKickEndpointResponse response;
            response.MutableError()->CopyFrom(startResponse.GetError());
            return response;
        });
    }

    void InitRestoringClient()
    {
        TString clientId = CreateGuidAsString() + "_bootstrap";

        NProto::TClientAppConfig clientAppConfig;
        auto& config = *clientAppConfig.MutableClientConfig();

        config = ClientConfig;
        config.SetClientId(clientId);

        auto clientConfig = std::make_shared<NClient::TClientAppConfig>(clientAppConfig);

        auto client = std::make_shared<TServiceAdapter>(
            EndpointManager,
            std::move(clientId));

        auto metricsClient = NClient::CreateMetricClient(
            std::move(client),
            Log,
            ServerStats);

        auto retryPolicy = CreateRetryPolicy(clientConfig);

        RestoringClient = CreateDurableClient(
            clientConfig,
            metricsClient,
            std::move(retryPolicy),
            Logging,
            Timer,
            Scheduler,
            RequestStats,
            VolumeStats);

        IncompleteRequestProvider = metricsClient;
    }

    template <typename T>
    TFuture<T> CreateTimeoutFuture(const TFuture<T>& future, TDuration timeout)
    {
        if (!timeout) {
            return future;
        }

        auto promise = NewPromise<T>();

        Scheduler->Schedule(Timer->Now() + timeout, [=] () mutable {
            promise.TrySetValue(TErrorResponse(E_TIMEOUT, "Timeout"));
        });

        future.Subscribe([=] (const auto& f) mutable {
            promise.TrySetValue(f.GetValue());
        });

        return promise;
    }

    NProto::TListKeyringsResponse ListKeyringsImpl();

    TFuture<void> RestoreEndpointsImpl();
};

////////////////////////////////////////////////////////////////////////////////

NProto::TListKeyringsResponse TMultipleEndpointService::ListKeyringsImpl()
{
    auto idsOrError = EndpointStorage->GetEndpointIds();
    if (HasError(idsOrError)) {
        return TErrorResponse(idsOrError.GetError());
    }

    NProto::TListKeyringsResponse response;
    auto& endpoints = *response.MutableEndpoints();

    const auto& storedIds = idsOrError.GetResult();
    endpoints.Reserve(storedIds.size());

    for (auto keyringId: storedIds) {
        auto& endpoint = *endpoints.Add();
        endpoint.SetKeyringId(keyringId);

        auto requestOrError = EndpointStorage->GetEndpoint(keyringId);
        if (HasError(requestOrError)) {
            continue;
        }

        auto request = DeserializeEndpoint<NProto::TStartEndpointRequest>(
            requestOrError.GetResult());

        if (!request) {
            continue;
        }

        endpoint.MutableRequest()->CopyFrom(*request);
    }

    return response;
}

TFuture<void> TMultipleEndpointService::RestoreEndpointsImpl()
{
    auto idsOrError = EndpointStorage->GetEndpointIds();
    if (HasError(idsOrError)) {
        STORAGE_ERROR("Failed to get endpoints from storage: "
            << FormatError(idsOrError.GetError()));
        ReportEndpointRestoringError();
        return MakeFuture();
    }

    const auto& storedIds = idsOrError.GetResult();
    STORAGE_INFO("Found " << storedIds.size() << " endpoints in storage");

    TVector<TFuture<void>> futures;

    for (auto keyringId: storedIds) {
        auto requestOrError = EndpointStorage->GetEndpoint(keyringId);
        if (HasError(requestOrError)) {
            ReportEndpointRestoringError();
            STORAGE_ERROR("Failed to restore endpoint. ID: " << keyringId
                << ", error: " << FormatError(requestOrError.GetError()));
            continue;
        }

        auto request = DeserializeEndpoint<NProto::TStartEndpointRequest>(
            requestOrError.GetResult());

        if (!request) {
            ReportEndpointRestoringError();
            STORAGE_ERROR("Failed to deserialize request. ID: " << keyringId);
            continue;
        }

        auto requestId = CreateRequestId();
        request->MutableHeaders()->SetRequestId(requestId);

        auto socketPath = request->GetUnixSocketPath();

        auto future = RestoringClient->StartEndpoint(
            MakeIntrusive<TCallContext>(requestId),
            std::move(request));

        future.Subscribe([=] (const auto& f) {
            const auto& response = f.GetValue();
            if (HasError(response)) {
                ReportEndpointRestoringError();
                STORAGE_ERROR("Failed to start endpoint " << socketPath.Quote()
                    << ", error:" << FormatError(response.GetError()));
            }
        });

        futures.push_back(future.IgnoreResult());
    }

    return WaitAll(futures);
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IEndpointServicePtr CreateMultipleEndpointService(
    IBlockStorePtr service,
    ITimerPtr timer,
    ISchedulerPtr scheduler,
    ILoggingServicePtr logging,
    IRequestStatsPtr requestStats,
    IVolumeStatsPtr volumeStats,
    IServerStatsPtr serverStats,
    IEndpointStoragePtr endpointStorage,
    IEndpointManagerPtr endpointManager,
    NProto::TClientConfig clientConfig)
{
    return std::make_shared<TMultipleEndpointService>(
        std::move(service),
        std::move(timer),
        std::move(scheduler),
        std::move(logging),
        std::move(requestStats),
        std::move(volumeStats),
        std::move(serverStats),
        std::move(endpointStorage),
        std::move(endpointManager),
        std::move(clientConfig));
}

}   // namespace NCloud::NBlockStore::NServer
