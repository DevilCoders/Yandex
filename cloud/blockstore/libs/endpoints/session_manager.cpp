#include "session_manager.h"

#include <cloud/blockstore/libs/client/client.h>
#include <cloud/blockstore/libs/client/config.h>
#include <cloud/blockstore/libs/client/durable.h>
#include <cloud/blockstore/libs/client/metric.h>
#include <cloud/blockstore/libs/client/session.h>
#include <cloud/blockstore/libs/client/throttling.h>
#include <cloud/blockstore/libs/diagnostics/server_stats.h>
#include <cloud/blockstore/libs/diagnostics/volume_stats.h>
#include <cloud/blockstore/libs/encryption/encryption_client.h>
#include <cloud/blockstore/libs/encryption/encryptor.h>
#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/service/service.h>
#include <cloud/blockstore/libs/service/storage_provider.h>
#include <cloud/blockstore/libs/validation/validation.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/coroutine/executor.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>

#include <util/generic/hash.h>
#include <util/string/builder.h>
#include <util/system/mutex.h>

namespace NCloud::NBlockStore::NServer {

using namespace NThreading;

using namespace NCloud::NBlockStore::NClient;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TEndpoint
{
private:
    TExecutor& Executor;
    const ISessionPtr Session;
    const IBlockStorePtr DataClient;
    const IVolumeStatsPtr VolumeStats;
    const TString ClientId;
    const NProto::TClientPerformanceProfile PerformanceProfile;

public:
    TEndpoint(
            TExecutor& executor,
            ISessionPtr session,
            IBlockStorePtr dataClient,
            IVolumeStatsPtr volumeStats,
            TString clientId,
            NProto::TClientPerformanceProfile performanceProfile)
        : Executor(executor)
        , Session(std::move(session))
        , DataClient(std::move(dataClient))
        , VolumeStats(std::move(volumeStats))
        , ClientId(std::move(clientId))
        , PerformanceProfile(std::move(performanceProfile))
    {}

    NProto::TError Start(TCallContextPtr callContext, NProto::THeaders headers)
    {
        DataClient->Start();

        headers.SetClientId(ClientId);
        auto future = Session->MountVolume(std::move(callContext), headers);
        auto response = Executor.WaitFor(future);

        if (HasError(response)) {
            DataClient->Stop();
        }

        return response.GetError();
    }

    NProto::TError Stop(TCallContextPtr callContext, NProto::THeaders headers)
    {
        headers.SetClientId(ClientId);
        auto future = Session->UnmountVolume(std::move(callContext), headers);
        auto response = Executor.WaitFor(future);

        DataClient->Stop();
        return response.GetError();
    }

    NProto::TError Alter(
        TCallContextPtr callContext,
        NProto::EVolumeAccessMode accessMode,
        NProto::EVolumeMountMode mountMode,
        ui64 mountSeqNumber,
        NProto::THeaders headers)
    {
        headers.SetClientId(ClientId);
        auto future = Session->MountVolume(
            accessMode,
            mountMode,
            mountSeqNumber,
            std::move(callContext),
            headers);
        auto response = Executor.WaitFor(future);
        return response.GetError();
    }

    ISessionPtr GetSession()
    {
        return Session;
    }

    NProto::TClientPerformanceProfile GetPerformanceProfile()
    {
        return PerformanceProfile;
    }
};

using TEndpointPtr = std::shared_ptr<TEndpoint>;

////////////////////////////////////////////////////////////////////////////////

class TClientBase
    : public IBlockStore
{
public:
    TClientBase() = default;

#define BLOCKSTORE_IMPLEMENT_METHOD(name, ...)                                 \
    TFuture<NProto::T##name##Response> name(                                   \
        TCallContextPtr callContext,                                           \
        std::shared_ptr<NProto::T##name##Request> request) override            \
    {                                                                          \
        Y_UNUSED(callContext);                                                 \
        Y_UNUSED(request);                                                     \
        const auto& type = GetBlockStoreRequestName(EBlockStoreRequest::name); \
        return MakeFuture<NProto::T##name##Response>(TErrorResponse(           \
            E_NOT_IMPLEMENTED,                                                 \
            TStringBuilder() << "Unsupported request " << type.Quote()));      \
    }                                                                          \
// BLOCKSTORE_IMPLEMENT_METHOD

    BLOCKSTORE_SERVICE(BLOCKSTORE_IMPLEMENT_METHOD)

#undef BLOCKSTORE_IMPLEMENT_METHOD
};

////////////////////////////////////////////////////////////////////////////////

class TStorageDataClient final
    : public TClientBase
{
private:
    const IStoragePtr Storage;
    const IBlockStorePtr Service;
    const IServerStatsPtr ServerStats;
    const TString ClientId;
    const TDuration RequestTimeout;

public:
    TStorageDataClient(
            IStoragePtr storage,
            IBlockStorePtr service,
            IServerStatsPtr serverStats,
            TString clientId,
            TDuration requestTimeout)
        : Storage(std::move(storage))
        , Service(std::move(service))
        , ServerStats(std::move(serverStats))
        , ClientId(std::move(clientId))
        , RequestTimeout(requestTimeout)
    {}

    void Start() override
    {}

    void Stop() override
    {}

    TStorageBuffer AllocateBuffer(size_t bytesCount) override
    {
        return Storage->AllocateBuffer(bytesCount);
    }

#define BLOCKSTORE_IMPLEMENT_METHOD(name, ...)                                 \
    TFuture<NProto::T##name##Response> name(                                   \
        TCallContextPtr callContext,                                           \
        std::shared_ptr<NProto::T##name##Request> request) override            \
    {                                                                          \
        PrepareRequestHeaders(*request->MutableHeaders(), *callContext);       \
        return Storage->name(std::move(callContext), std::move(request));      \
    }                                                                          \
// BLOCKSTORE_IMPLEMENT_METHOD

    BLOCKSTORE_IMPLEMENT_METHOD(ZeroBlocks)
    BLOCKSTORE_IMPLEMENT_METHOD(ReadBlocksLocal)
    BLOCKSTORE_IMPLEMENT_METHOD(WriteBlocksLocal)

#undef BLOCKSTORE_IMPLEMENT_METHOD

    TFuture<NProto::TMountVolumeResponse> MountVolume(
        TCallContextPtr callContext,
        std::shared_ptr<NProto::TMountVolumeRequest> request) override
    {
        const TString instanceId = request->GetInstanceId();
        PrepareRequestHeaders(*request->MutableHeaders(), *callContext);
        auto future = Service->MountVolume(
            std::move(callContext),
            std::move(request));

        const auto& serverStats = ServerStats;
        return future.Apply([=] (const auto& f) {
            const auto& response = f.GetValue();

            if (!HasError(response) && response.HasVolume()) {
                serverStats->MountVolume(
                    response.GetVolume(),
                    ClientId,
                    instanceId);
            }
            return f;
        });
    }

    TFuture<NProto::TUnmountVolumeResponse> UnmountVolume(
        TCallContextPtr callContext,
        std::shared_ptr<NProto::TUnmountVolumeRequest> request) override
    {
        auto diskId = request->GetDiskId();

        PrepareRequestHeaders(*request->MutableHeaders(), *callContext);
        auto future = Service->UnmountVolume(
            std::move(callContext),
            std::move(request));

        const auto& serverStats = ServerStats;
        return future.Apply([=, diskId = std::move(diskId)] (const auto& f) {
            const auto& response = f.GetValue();

            if (!HasError(response)) {
                serverStats->UnmountVolume(diskId, ClientId);
            }
            return f;
        });
    }

private:
    void PrepareRequestHeaders(
        NProto::THeaders& headers,
        const TCallContext& callContext)
    {
        headers.SetClientId(ClientId);

        if (!headers.GetRequestTimeout()) {
            headers.SetRequestTimeout(RequestTimeout.MilliSeconds());
        }

        if (!headers.GetTimestamp()) {
            headers.SetTimestamp(TInstant::Now().MicroSeconds());
        }

        if (!headers.GetRequestId()) {
            headers.SetRequestId(callContext.RequestId);
        }
    }

    template <typename TResponse, typename TRequest>
    TFuture<TResponse> CreateUnsupportedResponse(
        TCallContextPtr callContext,
        std::shared_ptr<TRequest> request)
    {
        Y_UNUSED(callContext);
        Y_UNUSED(request);

        auto requestType = GetBlockStoreRequest<TRequest>();
        const auto& requestName = GetBlockStoreRequestName(requestType);

        TErrorResponse response(
            E_FAIL,
            TStringBuilder()
                << "Unsupported storage request: "
                << requestName.Quote());

        return MakeFuture<TResponse>(std::move(response));
    }
};

////////////////////////////////////////////////////////////////////////////////

class TSessionManager final
    : public ISessionManager
    , public std::enable_shared_from_this<TSessionManager>
{
private:
    const ITimerPtr Timer;
    const ISchedulerPtr Scheduler;
    const ILoggingServicePtr Logging;
    const IMonitoringServicePtr Monitoring;
    const IRequestStatsPtr RequestStats;
    const IVolumeStatsPtr VolumeStats;
    const IServerStatsPtr ServerStats;
    const IBlockStorePtr Service;
    const IStorageProviderPtr StorageProvider;
    const IThrottlerProviderPtr ThrottlerProvider;
    const TExecutorPtr Executor;
    const NProto::TClientConfig DefaultClientConfig;
    const bool StrictContractValidation;

    TLog Log;

    TMutex EndpointLock;
    THashMap<TString, TEndpointPtr> Endpoints;

public:
    TSessionManager(
            ITimerPtr timer,
            ISchedulerPtr scheduler,
            ILoggingServicePtr logging,
            IMonitoringServicePtr monitoring,
            IRequestStatsPtr requestStats,
            IVolumeStatsPtr volumeStats,
            IServerStatsPtr serverStats,
            IBlockStorePtr service,
            IStorageProviderPtr storageProvider,
            IThrottlerProviderPtr throttlerProvider,
            TExecutorPtr executor,
            const NProto::TClientConfig& defaultClientConfig,
            bool strictContractValidation)
        : Timer(std::move(timer))
        , Scheduler(std::move(scheduler))
        , Logging(std::move(logging))
        , Monitoring(std::move(monitoring))
        , RequestStats(std::move(requestStats))
        , VolumeStats(std::move(volumeStats))
        , ServerStats(std::move(serverStats))
        , Service(std::move(service))
        , StorageProvider(std::move(storageProvider))
        , ThrottlerProvider(std::move(throttlerProvider))
        , Executor(std::move(executor))
        , DefaultClientConfig(defaultClientConfig)
        , StrictContractValidation(strictContractValidation)
    {
        Log = Logging->CreateLog("BLOCKSTORE_SERVER");
    }

    TFuture<TSessionOrError> CreateSession(
        TCallContextPtr callContext,
        const NProto::TStartEndpointRequest& request) override;

    TFuture<NProto::TError> RemoveSession(
        TCallContextPtr callContext,
        const TString& socketPath,
        const NProto::THeaders& headers) override;

    TFuture<NProto::TError> AlterSession(
        TCallContextPtr callContext,
        const TString& socketPath,
        NProto::EVolumeAccessMode accessMode,
        NProto::EVolumeMountMode mountMode,
        ui64 mountSeqNumber,
        const NProto::THeaders& headers) override;

    TFuture<NProto::TDescribeEndpointResponse> DescribeSession(
        const TString& socketPath) override;

private:
    TSessionOrError CreateSessionImpl(
        TCallContextPtr callContext,
        const NProto::TStartEndpointRequest& request);

    NProto::TError RemoveSessionImpl(
        TCallContextPtr callContext,
        const TString& socketPath,
        const NProto::THeaders& headers);

    NProto::TError AlterSessionImpl(
        TCallContextPtr callContext,
        const TString& socketPath,
        NProto::EVolumeAccessMode accessMode,
        NProto::EVolumeMountMode mountMode,
        ui64 mountSeqNumber,
        const NProto::THeaders& headers);

    NProto::TDescribeEndpointResponse DescribeSessionImpl(
        const TString& socketPath);

    NProto::TDescribeVolumeResponse DescribeVolume(
        TCallContextPtr callContext,
        const NProto::TStartEndpointRequest& request);

    TResultOrError<TEndpointPtr> CreateEndpoint(
        const NProto::TStartEndpointRequest& request,
        const NProto::TVolume& volume);

    TClientAppConfigPtr CreateClientConfig(
        const NProto::TStartEndpointRequest& request);

    static TSessionConfig CreateSessionConfig(
        const NProto::TStartEndpointRequest& request);
};

////////////////////////////////////////////////////////////////////////////////

TFuture<TSessionManager::TSessionOrError> TSessionManager::CreateSession(
    TCallContextPtr callContext,
    const NProto::TStartEndpointRequest& request)
{
    return Executor->Execute([=] () mutable {
        return CreateSessionImpl(std::move(callContext), request);
    });
}

TSessionManager::TSessionOrError TSessionManager::CreateSessionImpl(
    TCallContextPtr callContext,
    const NProto::TStartEndpointRequest& request)
{
    auto describeResponse = DescribeVolume(callContext, request);
    if (HasError(describeResponse)) {
        return TErrorResponse(describeResponse.GetError());
    }
    const auto& volume = describeResponse.GetVolume();

    auto result = CreateEndpoint(request, volume);
    if (HasError(result)) {
        return TErrorResponse(result.GetError());
    }
    const auto& endpoint = result.GetResult();

    auto error = endpoint->Start(std::move(callContext), request.GetHeaders());
    if (HasError(error)) {
        return TErrorResponse(error);
    }

    with_lock (EndpointLock) {
        auto [it, inserted] = Endpoints.emplace(
            request.GetUnixSocketPath(),
            endpoint);
        Y_VERIFY(inserted);
    }

    return TSessionInfo {
        .Volume = volume,
        .Session = endpoint->GetSession()
    };
}

NProto::TDescribeVolumeResponse TSessionManager::DescribeVolume(
    TCallContextPtr callContext,
    const NProto::TStartEndpointRequest& startRequest)
{
    auto describeRequest = std::make_shared<NProto::TDescribeVolumeRequest>();
    describeRequest->MutableHeaders()->CopyFrom(startRequest.GetHeaders());
    describeRequest->SetDiskId(startRequest.GetDiskId());

    auto future = Service->DescribeVolume(
        std::move(callContext),
        std::move(describeRequest));

    return Executor->WaitFor(future);
}

TFuture<NProto::TError> TSessionManager::RemoveSession(
    TCallContextPtr callContext,
    const TString& socketPath,
    const NProto::THeaders& headers)
{
    return Executor->Execute([=] () mutable {
        return RemoveSessionImpl(std::move(callContext), socketPath, headers);
    });
}

NProto::TError TSessionManager::RemoveSessionImpl(
    TCallContextPtr callContext,
    const TString& socketPath,
    const NProto::THeaders& headers)
{
    TEndpointPtr endpoint;

    with_lock (EndpointLock) {
        auto it = Endpoints.find(socketPath);
        Y_VERIFY(it != Endpoints.end());
        endpoint = std::move(it->second);
        Endpoints.erase(it);
    }

    auto error = endpoint->Stop(std::move(callContext), headers);

    endpoint.reset();
    ThrottlerProvider->Clean();

    return error;
}

TFuture<NProto::TError> TSessionManager::AlterSession(
    TCallContextPtr callContext,
    const TString& socketPath,
    NProto::EVolumeAccessMode accessMode,
    NProto::EVolumeMountMode mountMode,
    ui64 mountSeqNumber,
    const NProto::THeaders& headers)
{
    return Executor->Execute([=] () mutable {
        return AlterSessionImpl(
            std::move(callContext),
            socketPath,
            accessMode,
            mountMode,
            mountSeqNumber,
            headers);
    });
}

NProto::TError TSessionManager::AlterSessionImpl(
    TCallContextPtr callContext,
    const TString& socketPath,
    NProto::EVolumeAccessMode accessMode,
    NProto::EVolumeMountMode mountMode,
    ui64 mountSeqNumber,
    const NProto::THeaders& headers)
{
    TEndpointPtr endpoint;

    with_lock (EndpointLock) {
        auto it = Endpoints.find(socketPath);
        if (it == Endpoints.end()) {
            return TErrorResponse(
                E_INVALID_STATE,
                TStringBuilder()
                    << "endpoint " << socketPath.Quote()
                    << " hasn't been started");
        }
        endpoint = it->second;
    }

    return endpoint->Alter(
        std::move(callContext),
        accessMode,
        mountMode,
        mountSeqNumber,
        headers);
}

TFuture<NProto::TDescribeEndpointResponse> TSessionManager::DescribeSession(
    const TString& socketPath)
{
    return Executor->Execute([=] () mutable {
        return DescribeSessionImpl(socketPath);
    });
}

NProto::TDescribeEndpointResponse TSessionManager::DescribeSessionImpl(
    const TString& socketPath)
{
    TEndpointPtr endpoint;

    with_lock (EndpointLock) {
        auto it = Endpoints.find(socketPath);
        if (it == Endpoints.end()) {
            return TErrorResponse(
                E_INVALID_STATE,
                TStringBuilder()
                    << "endpoint " << socketPath.Quote()
                    << " hasn't been started");
        }
        endpoint = it->second;
    }

    NProto::TDescribeEndpointResponse response;
    response.MutablePerformanceProfile()->CopyFrom(
        endpoint->GetPerformanceProfile());
    return response;
}

TResultOrError<TEndpointPtr> TSessionManager::CreateEndpoint(
    const NProto::TStartEndpointRequest& request,
    const NProto::TVolume& volume)
{
    const auto clientId = request.GetClientId();
    auto accessMode = request.GetVolumeAccessMode();

    auto future = StorageProvider->CreateStorage(volume, clientId, accessMode)
        .Apply([] (const auto& f) {
            auto storage = f.GetValue();
            // TODO: StorageProvider should return TResultOrError<IStoragePtr>
            return TResultOrError(std::move(storage));
        });

    auto storageResult = Executor->WaitFor(future);
    auto storage = storageResult.GetResult();

    auto clientConfig = CreateClientConfig(request);

    IBlockStorePtr client = std::make_shared<TStorageDataClient>(
        std::move(storage),
        Service,
        ServerStats,
        clientId,
        clientConfig->GetRequestTimeout());

    auto retryPolicy = CreateRetryPolicy(clientConfig);

    if (volume.GetStorageMediaKind() != NProto::STORAGE_MEDIA_SSD_LOCAL) {
        client = CreateDurableClient(
            clientConfig,
            std::move(client),
            std::move(retryPolicy),
            Logging,
            Timer,
            Scheduler,
            RequestStats,
            VolumeStats);
    }

    auto clientOrError = TryToCreateEncryptionClient(
        std::move(client),
        Logging,
        request.GetEncryptionSpec());

    if (HasError(clientOrError)) {
        return clientOrError.GetError();
    }
    client = clientOrError.GetResult();

    auto performanceProfile = request.GetClientPerformanceProfile();
    auto throttler = ThrottlerProvider->GetThrottler(
        clientConfig->GetClientConfig(),
        request.GetClientProfile(),
        performanceProfile);

    if (throttler) {
        client = CreateThrottlingClient(
            std::move(client),
            std::move(throttler));
    } else {
        performanceProfile = NProto::TClientPerformanceProfile();
    }

    if (StrictContractValidation) {
        client = CreateValidationClient(
            Logging,
            Monitoring,
            std::move(client),
            CreateCrcDigestCalculator());
    }

    auto session = NClient::CreateSession(
        Timer,
        Scheduler,
        Logging,
        RequestStats,
        VolumeStats,
        client,
        std::move(clientConfig),
        CreateSessionConfig(request));

    return std::make_shared<TEndpoint>(
        *Executor,
        std::move(session),
        std::move(client),
        VolumeStats,
        clientId,
        performanceProfile);
}

TClientAppConfigPtr TSessionManager::CreateClientConfig(
    const NProto::TStartEndpointRequest& request)
{
    NProto::TClientAppConfig clientAppConfig;
    auto& config = *clientAppConfig.MutableClientConfig();

    config = DefaultClientConfig;
    config.SetClientId(request.GetClientId());
    config.SetInstanceId(request.GetInstanceId());

    if (request.GetRequestTimeout()) {
        config.SetRequestTimeout(request.GetRequestTimeout());
    }
    if (request.GetRetryTimeout()) {
        config.SetRetryTimeout(request.GetRetryTimeout());
    }
    if (request.GetRetryTimeoutIncrement()) {
        config.SetRetryTimeoutIncrement(request.GetRetryTimeoutIncrement());
    }

    return std::make_shared<TClientAppConfig>(std::move(clientAppConfig));
}

TSessionConfig TSessionManager::CreateSessionConfig(
    const NProto::TStartEndpointRequest& request)
{
    TSessionConfig config;
    config.DiskId = request.GetDiskId();
    config.InstanceId = request.GetInstanceId();
    config.AccessMode = request.GetVolumeAccessMode();
    config.MountMode = request.GetVolumeMountMode();
    config.MountFlags = request.GetMountFlags();
    config.IpcType = request.GetIpcType();
    config.ClientVersionInfo = request.GetClientVersionInfo();
    config.MountSeqNumber = request.GetMountSeqNumber();
    return config;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

ISessionManagerPtr CreateSessionManager(
    ITimerPtr timer,
    ISchedulerPtr scheduler,
    ILoggingServicePtr logging,
    IMonitoringServicePtr monitoring,
    IRequestStatsPtr requestStats,
    IVolumeStatsPtr volumeStats,
    IServerStatsPtr serverStats,
    IBlockStorePtr service,
    IStorageProviderPtr storageProvider,
    TExecutorPtr executor,
    const TSessionManagerOptions& options)
{
    auto throttlerProvider = CreateThrottlerProvider(
        options.HostProfile,
        logging,
        timer,
        scheduler,
        monitoring->GetCounters()->GetSubgroup("counters", "blockstore"),
        requestStats,
        volumeStats);

    return std::make_shared<TSessionManager>(
        std::move(timer),
        std::move(scheduler),
        std::move(logging),
        std::move(monitoring),
        std::move(requestStats),
        std::move(volumeStats),
        std::move(serverStats),
        std::move(service),
        std::move(storageProvider),
        std::move(throttlerProvider),
        std::move(executor),
        options.DefaultClientConfig,
        options.StrictContractValidation);
}

}   // namespace NCloud::NBlockStore::NServer
