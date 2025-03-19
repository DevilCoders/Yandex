#include "server.h"

#include "vhost.h"

#include <cloud/blockstore/libs/diagnostics/critical_events.h>
#include <cloud/blockstore/libs/diagnostics/server_stats.h>
#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/service/device_handler.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/service/storage.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/thread.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <util/folder/path.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>
#include <util/system/mutex.h>
#include <util/system/thread.h>

namespace NCloud::NBlockStore::NVhost {

using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr ui32 MaxZeroRequestSize = 32*1024*1024;
constexpr TDuration ShutdownTimeout = TDuration::Seconds(5);

////////////////////////////////////////////////////////////////////////////////

struct TReadBlocksLocalMethod
{
    static TFuture<NProto::TReadBlocksLocalResponse> Execute(
        IDeviceHandler& deviceHandler,
        TCallContextPtr ctx,
        TVhostRequest& vhostRequest)
    {
        TString checkpointId;
        return deviceHandler.Read(
            std::move(ctx),
            vhostRequest.From,
            vhostRequest.Length,
            vhostRequest.SgList,
            checkpointId);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TWriteBlocksLocalMethod
{
    static TFuture<NProto::TWriteBlocksLocalResponse> Execute(
        IDeviceHandler& deviceHandler,
        TCallContextPtr ctx,
        TVhostRequest& vhostRequest)
    {
        return deviceHandler.Write(
            std::move(ctx),
            vhostRequest.From,
            vhostRequest.Length,
            vhostRequest.SgList);
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TRequest
    : public TIntrusiveListItem<TRequest>
    , TAtomicRefCount<TRequest>
{
    TVhostRequestPtr VhostRequest;
    TCallContextPtr CallContext;
    TMetricRequest MetricRequest;

    TAtomic Completed = 0;

    TRequest(ui64 requestId, TVhostRequestPtr vhostRequest)
        : VhostRequest(std::move(vhostRequest))
        , CallContext(MakeIntrusive<TCallContext>(requestId))
        , MetricRequest(VhostRequest->Type)
    {}
};

using TRequestPtr = TIntrusivePtr<TRequest>;

////////////////////////////////////////////////////////////////////////////////

struct TAppContext
{
    IServerStatsPtr ServerStats;
    IVhostQueueFactoryPtr VhostQueueFactory;
    IDeviceHandlerFactoryPtr DeviceHandlerFactory;
    TServerConfig Config;
    NRdma::TRdmaHandler Rdma;

    TLog Log;

    TAtomic ShouldStop = 0;
};

////////////////////////////////////////////////////////////////////////////////

class TEndpoint final
    : public std::enable_shared_from_this<TEndpoint>
{
private:
    TAppContext& AppCtx;
    const IDeviceHandlerPtr DeviceHandler;
    const TString SocketPath;
    const TStorageOptions Options;
    IVhostDevicePtr VhostDevice;

    TIntrusiveList<TRequest> RequestsInFlight;
    TAdaptiveLock RequestsLock;

    TAtomic Stopped = 0;

public:
    TEndpoint(
            TAppContext& appCtx,
            IDeviceHandlerPtr deviceHandler,
            TString socketPath,
            const TStorageOptions& options)
        : AppCtx(appCtx)
        , DeviceHandler(std::move(deviceHandler))
        , SocketPath(std::move(socketPath))
        , Options(options)
    {}

    void SetVhostDevice(IVhostDevicePtr vhostDevice)
    {
        Y_VERIFY(VhostDevice == nullptr);
        VhostDevice = std::move(vhostDevice);
    }

    NProto::TError Start()
    {
        TFsPath(SocketPath).DeleteIfExists();

        bool started = VhostDevice->Start();

        if (!started) {
            NProto::TError error;
            error.SetCode(E_FAIL);
            error.SetMessage(TStringBuilder()
                << "could not register block device "
                << SocketPath.Quote());
            return error;
        }

        int mode = S_IRGRP | S_IWGRP | S_IRUSR | S_IWUSR;
        auto err = Chmod(SocketPath.c_str(), mode);

        if (err != 0) {
            NProto::TError error;
            error.SetCode(MAKE_SYSTEM_ERROR(err));
            error.SetMessage(TStringBuilder()
                << "failed to chmod socket "
                << SocketPath.Quote());
            return error;
        }

        return NProto::TError();
    }

    TFuture<NProto::TError> Stop(bool deleteSocket)
    {
        if (AtomicSwap(&Stopped, 1) == 1) {
            return MakeFuture(MakeError(S_ALREADY));
        }

        auto future = VhostDevice->Stop();

        auto cancelError = MakeError(E_CANCELLED, "Request cancelled");
        with_lock (RequestsLock) {
            for (auto& request: RequestsInFlight) {
                CompleteRequest(request, cancelError);
            }
            RequestsInFlight.Clear();
        }

        if (deleteSocket) {
            future = future.Apply([socketPath = SocketPath] (const auto& f) {
                TFsPath(socketPath).DeleteIfExists();
                return f.GetValue();
            });
        }

        return future;
    }

    ui32 GetVhostQueuesCount() const
    {
        return Options.VhostQueuesCount;
    }

    TVector<TIncompleteRequest> GetIncompleteRequests()
    {
        TIncompleteRequestStats<BlockStoreRequestsCount> stats;
        ui64 now = GetCycleCount();

        with_lock (RequestsLock) {
            for (auto& request: RequestsInFlight) {
                auto& req = request.MetricRequest;
                ui64 started = AtomicGet(request.CallContext->RequestStarted);

                if (started && started < now) {
                    auto requestTime = request.CallContext->CalcExecutionTime(
                        started,
                        now);
                    stats.Add(
                        req.MediaKind,
                        static_cast<TDiagnosticsRequestType>(req.RequestType),
                        requestTime);
                }
            }
        }

        return stats.Finish();
    }

    void ProcessRequest(TVhostRequestPtr vhostRequest)
    {
        const auto requestType = vhostRequest->Type;
        auto request = RegisterRequest(std::move(vhostRequest));

        switch (requestType) {
            case EBlockStoreRequest::WriteBlocks:
                ProcessRequest<TWriteBlocksLocalMethod>(std::move(request));
                break;
            case EBlockStoreRequest::ReadBlocks:
                ProcessRequest<TReadBlocksLocalMethod>(std::move(request));
                break;
            default:
                Y_FAIL("Unexpected request type: %d",
                    static_cast<int>(requestType));
                break;
        }
    }

private:
    template <typename TMethod>
    void ProcessRequest(TRequestPtr request)
    {
        auto future = TMethod::Execute(
            *DeviceHandler,
            request->CallContext,
            *request->VhostRequest);

        auto weakPtr = weak_from_this();
        future.Subscribe([weakPtr, req = std::move(request)] (const auto& f) {
            const auto& response = f.GetValue();
            if (auto p = weakPtr.lock()) {
                p->CompleteRequest(*req, response.GetError());
                p->UnregisterRequest(*req);
            }
        });
    }

    TRequestPtr RegisterRequest(TVhostRequestPtr vhostRequest)
    {
        auto startIndex = vhostRequest->From / Options.BlockSize;
        auto endIndex = (vhostRequest->From + vhostRequest->Length) / Options.BlockSize;
        if (endIndex * Options.BlockSize < vhostRequest->From + vhostRequest->Length) {
            ++endIndex;
        }

        auto request = MakeIntrusive<TRequest>(
            CreateRequestId(),
            std::move(vhostRequest));

        const ui32 blockSize = AppCtx.ServerStats->GetBlockSize(Options.DiskId);

        AppCtx.ServerStats->PrepareMetricRequest(
            request->MetricRequest,
            Options.ClientId,
            Options.DiskId,
            startIndex,
            blockSize * (endIndex - startIndex));

        AppCtx.ServerStats->RequestStarted(
            AppCtx.Log,
            request->MetricRequest,
            *request->CallContext);

        with_lock (RequestsLock) {
            RequestsInFlight.PushBack(request.Get());
        }
        return request;
    }

    void CompleteRequest(TRequest& request, const NProto::TError& error)
    {
        if (AtomicSwap(&request.Completed, 1) == 1) {
            return;
        }

        AppCtx.ServerStats->RequestCompleted(
            AppCtx.Log,
            request.MetricRequest,
            *request.CallContext,
            error);

        request.VhostRequest->Complete(GetResult(error));
    }

    void UnregisterRequest(TRequest& request)
    {
        with_lock (RequestsLock) {
            request.Unlink();
        }
    }

    TVhostRequest::EResult GetResult(const NProto::TError& error)
    {
        if (!HasError(error)) {
            return TVhostRequest::SUCCESS;
        }

        if (AtomicGet(AppCtx.ShouldStop) == 1 ||
            AtomicGet(Stopped) == 1)
        {
            if (error.GetCode() == E_CANCELLED) {
                return TVhostRequest::CANCELLED;
            }

            if (GetErrorKind(error) == EErrorKind::ErrorRetriable) {
                return TVhostRequest::CANCELLED;
            }
        }

        return TVhostRequest::IOERR;
    }
};

using TEndpointPtr = std::shared_ptr<TEndpoint>;

////////////////////////////////////////////////////////////////////////////////

class TExecutor final
    : public ISimpleThread
{
private:
    TAppContext& AppCtx;
    const TString Name;
    TExecutorCounters::TExecutorScope ExecutorScope;
    const IVhostQueuePtr VhostQueue;
    TAffinity Affinity;

    TMap<TString, TEndpointPtr> Endpoints;

public:
    TExecutor(
            TAppContext& appCtx,
            TString name,
            IVhostQueuePtr vhostQueue,
            const TAffinity& affinity)
        : AppCtx(appCtx)
        , Name(std::move(name))
        , ExecutorScope(AppCtx.ServerStats->StartExecutor())
        , VhostQueue(std::move(vhostQueue))
        , Affinity(affinity)
    {}

    void Shutdown()
    {
        VhostQueue->Stop();
        Join();
    }

    TVector<TIncompleteRequest> GetIncompleteRequests()
    {
        TVector<TIncompleteRequest> requestsInFlight;

        for (auto& it: Endpoints) {
            auto requests = it.second->GetIncompleteRequests();
            requestsInFlight.insert(
                requestsInFlight.end(),
                requests.begin(),
                requests.end());
        }

        return requestsInFlight;
    }

    TEndpointPtr CreateEndpoint(
        const TString& socketPath,
        const TStorageOptions& options,
        IStoragePtr storage)
    {
        auto deviceHandler = AppCtx.DeviceHandlerFactory->CreateDeviceHandler(
            std::move(storage),
            options.ClientId,
            options.BlockSize,
            MaxZeroRequestSize / options.BlockSize,
            options.UnalignedRequestsDisabled,
            AppCtx.ServerStats->GetUnalignedRequestCounter());

        auto endpoint = std::make_shared<TEndpoint>(
            AppCtx,
            std::move(deviceHandler),
            socketPath,
            options);

        auto vhostDevice = VhostQueue->CreateDevice(
            socketPath,
            options.DeviceName.empty() ? options.DiskId : options.DeviceName,
            options.BlockSize,
            options.BlocksCount,
            options.VhostQueuesCount,
            endpoint.get(),
            AppCtx.Rdma.RegisterMemory,
            AppCtx.Rdma.UnregisterMemory);
        endpoint->SetVhostDevice(std::move(vhostDevice));

        return endpoint;
    }

    void AddEndpoint(const TString& socketPath, TEndpointPtr endpoint)
    {
        auto [it, inserted] = Endpoints.emplace(
            socketPath,
            std::move(endpoint));
        Y_VERIFY(inserted);
    }

    TEndpointPtr RemoveEndpoint(const TString& socketPath)
    {
        auto it = Endpoints.find(socketPath);
        Y_VERIFY(it != Endpoints.end());

        auto endpoint = std::move(it->second);
        Endpoints.erase(it);

        return endpoint;
    }

    ui32 GetVhostQueuesCount() const
    {
        ui32 queuesCount = 0;
        for (const auto& it: Endpoints) {
            queuesCount += it.second->GetVhostQueuesCount();
        }
        return queuesCount;
    }

private:
    void* ThreadProc() override
    {
        TAffinityGuard affinityGuard(Affinity);

        ::NCloud::SetCurrentThreadName(Name);

        TLog& Log = AppCtx.Log;

        while (true) {
            int res = RunRequestQueue();
            if (res != -EAGAIN) {
                if (res < 0) {
                    ReportVhostQueueRunningError();
                    STORAGE_ERROR(
                        "Failed to run vhost request queue. Return code: " << -res);
                }
                break;
            }

            while (auto req = VhostQueue->DequeueRequest()) {
                ProcessRequest(std::move(req));
            }
        }

        return nullptr;
    }

    int RunRequestQueue()
    {
        auto activity = ExecutorScope.StartWait();

        return VhostQueue->Run();
    }

    void ProcessRequest(TVhostRequestPtr vhostRequest)
    {
        auto activity = ExecutorScope.StartExecute();

        auto* endpoint = reinterpret_cast<TEndpoint*>(vhostRequest->Cookie);
        endpoint->ProcessRequest(std::move(vhostRequest));
    }
};

using TExecutorPtr = std::unique_ptr<TExecutor>;

////////////////////////////////////////////////////////////////////////////////

class TServer final
    : public TAppContext
    , public IServer
    , public std::enable_shared_from_this<TServer>
{
private:
    TMutex Lock;

    TVector<TExecutorPtr> Executors;

    TMap<TString, TExecutor*> EndpointMap;

public:
    TServer(
        ILoggingServicePtr logging,
        IServerStatsPtr serverStats,
        IVhostQueueFactoryPtr vhostQueueFactory,
        IDeviceHandlerFactoryPtr deviceHandlerFactory,
        const TServerConfig& config,
        NRdma::TRdmaHandler rdma);

    ~TServer();

    void Start() override;
    void Stop() override;

    TVector<TIncompleteRequest> GetIncompleteRequests() override;

    TFuture<NProto::TError> StartEndpoint(
        TString socketPath,
        IStoragePtr storage,
        const TStorageOptions& options) override;

    TFuture<NProto::TError> StopEndpoint(const TString& socketPath) override;

private:
    void InitExecutors();

    TExecutor* PickExecutor() const;

    void StopAllEndpoints();
};

////////////////////////////////////////////////////////////////////////////////

TServer::TServer(
    ILoggingServicePtr logging,
    IServerStatsPtr serverStats,
    IVhostQueueFactoryPtr vhostQueueFactory,
    IDeviceHandlerFactoryPtr deviceHandlerFactory,
    const TServerConfig& config,
    NRdma::TRdmaHandler rdma)
{
    Log = logging->CreateLog("BLOCKSTORE_VHOST");
    ServerStats = std::move(serverStats);
    VhostQueueFactory = std::move(vhostQueueFactory);
    DeviceHandlerFactory = std::move(deviceHandlerFactory);
    Config = config;
    Rdma = rdma;

    InitExecutors();
}

TServer::~TServer()
{
    Stop();
}

void TServer::Start()
{
    STORAGE_INFO("Start");

    for (auto& executor: Executors) {
        executor->Start();
    }
}

void TServer::Stop()
{
    if (AtomicSwap(&ShouldStop, 1) == 1) {
        return;
    }

    STORAGE_INFO("Shutting down");

    StopAllEndpoints();

    for (auto& executor: Executors) {
        executor->Shutdown();
    }
}

TVector<TIncompleteRequest> TServer::GetIncompleteRequests()
{
    TVector<TIncompleteRequest> requestsInFlight;

    with_lock (Lock) {
        for (auto& executor: Executors) {
            auto requests = executor->GetIncompleteRequests();
            requestsInFlight.insert(
                requestsInFlight.end(),
                requests.begin(),
                requests.end());
        }
    }

    return requestsInFlight;
}

TFuture<NProto::TError> TServer::StartEndpoint(
    TString socketPath,
    IStoragePtr storage,
    const TStorageOptions& options)
{
    if (AtomicGet(ShouldStop) == 1) {
        NProto::TError error;
        error.SetCode(E_FAIL);
        error.SetMessage("Vhost server is stopped");
        return MakeFuture(error);
    }

    TExecutor* executor;

    with_lock (Lock) {
        auto it = EndpointMap.find(socketPath);
        if (it != EndpointMap.end()) {
            NProto::TError error;
            error.SetCode(E_FAIL);
            error.SetMessage(TStringBuilder()
                << "endpoint " << socketPath.Quote()
                << " has already been started");
            return MakeFuture(error);
        }

        executor = PickExecutor();
        Y_VERIFY(executor);
    }

    auto endpoint = executor->CreateEndpoint(
        socketPath,
        options,
        std::move(storage));

    auto error = SafeExecute<NProto::TError>([&] {
        return endpoint->Start();
    });
    if (HasError(error)) {
        return MakeFuture(error);
    }

    with_lock (Lock) {
        executor->AddEndpoint(socketPath, std::move(endpoint));

        auto [it, inserted] = EndpointMap.emplace(
            std::move(socketPath),
            executor);
        Y_VERIFY(inserted);
    }

    return MakeFuture<NProto::TError>();
}

TFuture<NProto::TError> TServer::StopEndpoint(const TString& socketPath)
{
    if (AtomicGet(ShouldStop) == 1) {
        NProto::TError error;
        error.SetCode(E_FAIL);
        error.SetMessage("Vhost server is stopped");
        return MakeFuture(error);
    }

    TEndpointPtr endpoint;

    with_lock (Lock) {
        auto it = EndpointMap.find(socketPath);
        if (it == EndpointMap.end()) {
            NProto::TError error;
            error.SetCode(S_ALREADY);
            error.SetMessage(TStringBuilder()
                << "endpoint " << socketPath.Quote()
                << " has already been stopped");
            return MakeFuture(error);
        }

        auto* executor = it->second;
        EndpointMap.erase(it);

        endpoint = executor->RemoveEndpoint(socketPath);
    }

    return endpoint->Stop(true);
}

void TServer::StopAllEndpoints()
{
    TVector<TString> sockets;
    TVector<TFuture<NProto::TError>> futures;

    with_lock (Lock) {
        for (const auto& it: EndpointMap) {
            const auto& socketPath = it.first;
            auto* executor = it.second;

            auto endpoint = executor->RemoveEndpoint(socketPath);
            auto future = endpoint->Stop(false);
            sockets.push_back(socketPath);
            futures.push_back(future);
        }

        EndpointMap.clear();
    }

    WaitAll(futures).Wait(ShutdownTimeout);

    for (size_t i = 0; i < sockets.size(); ++i) {
        const auto& socketPath = sockets[i];
        const auto& future = futures[i];

        NProto::TError error;
        if (!future.HasValue()) {
            error = TErrorResponse(E_TIMEOUT, "Timeout");
        } else {
            error = future.GetValue();
        }

        if (HasError(error)) {
            STORAGE_ERROR("Failed to stop endpoint: "
                << socketPath.Quote()
                << ". Error: " << error);
        }
    }
}

void TServer::InitExecutors()
{
    for (size_t i = 1; i <= Config.ThreadsCount; ++i) {
        auto vhostQueue = VhostQueueFactory->CreateQueue();

        auto executor = std::make_unique<TExecutor>(
            *this,
            TStringBuilder() << "VHOST" << i,
            std::move(vhostQueue),
            Config.Affinity);

        Executors.push_back(std::move(executor));
    }
}

TExecutor* TServer::PickExecutor() const
{
    TExecutor* result = nullptr;

    for (const auto& executor: Executors) {
        if (result == nullptr ||
            executor->GetVhostQueuesCount() < result->GetVhostQueuesCount())
        {
            result = executor.get();
        }
    }

    return result;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IServerPtr CreateServer(
    ILoggingServicePtr logging,
    IServerStatsPtr serverStats,
    IVhostQueueFactoryPtr vhostQueueFactory,
    IDeviceHandlerFactoryPtr deviceHandlerFactory,
    const TServerConfig& config,
    NRdma::TRdmaHandler rdma)
{
    return std::make_shared<TServer>(
        std::move(logging),
        std::move(serverStats),
        std::move(vhostQueueFactory),
        std::move(deviceHandlerFactory),
        config,
        rdma);
}

}   // namespace NCloud::NBlockStore::NVhost
