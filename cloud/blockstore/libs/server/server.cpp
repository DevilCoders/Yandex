#include "server.h"

#include "client_acceptor.h"
#include "config.h"
#include "endpoint_poller.h"

#include <cloud/blockstore/public/api/grpc/service.grpc.pb.h>

#include <cloud/blockstore/libs/diagnostics/config.h>
#include <cloud/blockstore/libs/diagnostics/server_stats.h>
#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/service/request.h>
#include <cloud/blockstore/libs/service/service.h>

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/thread.h>
#include <cloud/storage/core/libs/diagnostics/incomplete_requests.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/grpc/completion.h>
#include <cloud/storage/core/libs/grpc/credentials.h>
#include <cloud/storage/core/libs/grpc/initializer.h>
#include <cloud/storage/core/libs/grpc/keepalive.h>
#include <cloud/storage/core/libs/grpc/time.h>

#include <library/cpp/actors/prof/tag.h>

#include <contrib/libs/grpc/include/grpcpp/completion_queue.h>
#include <contrib/libs/grpc/include/grpcpp/resource_quota.h>
#include <contrib/libs/grpc/include/grpcpp/security/auth_metadata_processor.h>
#include <contrib/libs/grpc/include/grpcpp/security/server_credentials.h>
#include <contrib/libs/grpc/include/grpcpp/server.h>
#include <contrib/libs/grpc/include/grpcpp/server_builder.h>
#include <contrib/libs/grpc/include/grpcpp/server_context.h>
#include <contrib/libs/grpc/include/grpcpp/server_posix.h>
#include <contrib/libs/grpc/include/grpcpp/support/status.h>

#include <util/datetime/cputimer.h>
#include <util/folder/path.h>
#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/network/init.h>
#include <util/stream/file.h>
#include <util/string/join.h>
#include <util/system/file.h>
#include <util/system/mutex.h>
#include <util/system/spinlock.h>
#include <util/system/thread.h>

namespace NCloud::NBlockStore::NServer {

using namespace NMonitoring;
using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

namespace NHeaders {
    const grpc::string ClientId = "x-nbs-client-id";
    const grpc::string IdempotenceId = "x-nbs-idempotence-id";
    const grpc::string RequestId = "x-nbs-request-id";
    const grpc::string Timestamp = "x-nbs-timestamp";
    const grpc::string TraceId = "x-nbs-trace-id";
    const grpc::string RequestTimeout = "x-nbs-request-timeout";
}

////////////////////////////////////////////////////////////////////////////////

TString ReadFile(const TString& fileName)
{
    TFileInput in(fileName);
    return in.ReadAll();
}

////////////////////////////////////////////////////////////////////////////////

struct TRequestSourceKind
{
    grpc::string Name;
    NProto::ERequestSource Value;
};

const TRequestSourceKind RequestSourceKinds[] = {
    { "INSECURE_CONTROL_CHANNEL", NProto::SOURCE_INSECURE_CONTROL_CHANNEL },
    { "SECURE_CONTROL_CHANNEL",   NProto::SOURCE_SECURE_CONTROL_CHANNEL },
    { "TCP_DATA_CHANNEL",         NProto::SOURCE_TCP_DATA_CHANNEL },
    { "FD_DATA_CHANNEL",          NProto::SOURCE_FD_DATA_CHANNEL },
};

grpc::string_ref GetRequestSourceName(NProto::ERequestSource source)
{
    Y_VERIFY(source < NProto::ERequestSource_ARRAYSIZE);
    return RequestSourceKinds[(int)source].Name;
}

TMaybe<NProto::ERequestSource> ParseRequestSourceName(grpc::string_ref name)
{
    for (const auto& kind: RequestSourceKinds) {
        if (kind.Name == name) {
            return kind.Value;
        }
    }
    return {};
}

////////////////////////////////////////////////////////////////////////////////
// TODO: should rework all auth* details after talk to security guys.

const grpc::string AUTH_HEADER = "authorization";
const grpc::string AUTH_METHOD = "Bearer ";
const grpc::string AUTH_PROPERTY_SOURCE = "y-auth-source";

TString GetAuthToken(const std::multimap<grpc::string_ref, grpc::string_ref>& metadata)
{
    auto range = metadata.equal_range(AUTH_HEADER);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second.starts_with(AUTH_METHOD)) {
            auto token = it->second.substr(AUTH_METHOD.size());
            return { token.begin(), token.end() };
        }
    }

    return {};
}

class TAuthMetadataProcessor final
    : public grpc::AuthMetadataProcessor
{
private:
    const NProto::ERequestSource RequestSource;

public:
    explicit TAuthMetadataProcessor(NProto::ERequestSource source)
        : RequestSource(source)
    {}

    bool IsBlocking() const override
    {
        // unless we are doing something heavy/blocking
        // we could do it in the caller context.
        return false;
    }

    grpc::Status Process(
        const InputMetadata& authMetadata,
        grpc::AuthContext* context,
        OutputMetadata* consumedAuthMetadata,
        OutputMetadata* responseMetadata) override
    {
        Y_UNUSED(authMetadata);
        Y_UNUSED(consumedAuthMetadata);
        Y_UNUSED(responseMetadata);

        if (context->FindPropertyValues(AUTH_PROPERTY_SOURCE).empty()) {
            context->AddProperty(
                AUTH_PROPERTY_SOURCE,
                GetRequestSourceName(RequestSource));
        }

        return grpc::Status::OK;
    }

    static TMaybe<NProto::ERequestSource> GetRequestSource(
        const grpc::AuthContext& context)
    {
        auto values = context.FindPropertyValues(AUTH_PROPERTY_SOURCE);
        if (!values.empty()) {
            return ParseRequestSourceName(values[0]);
        }
        return {};
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TRequestHandlerBase
{
    TCallContextPtr CallContext = MakeIntrusive<TCallContext>();

    TMetricRequest MetricRequest;

    enum {
        WaitingForRequest = 0,
        ExecutingRequest = 1,
        ExecutionCompleted = 2,
        ExecutionCancelled = 3,
        SendingResponse = 4,
        RequestCompleted = 5,
    };

    TAtomic RefCount = 0;
    TAtomic RequestState = WaitingForRequest;

    NProto::TError Error;

    TRequestHandlerBase(EBlockStoreRequest requestType)
        : MetricRequest(requestType)
    {}

    virtual ~TRequestHandlerBase() = default;

    virtual void Process(bool ok) = 0;
    virtual void Cancel() = 0;

    void* AcquireCompletionTag()
    {
        AtomicIncrement(RefCount);
        return this;
    }

    bool ReleaseCompletionTag()
    {
        return AtomicDecrement(RefCount) == 0;
    }
};

using TRequestHandlerPtr = std::unique_ptr<TRequestHandlerBase>;

////////////////////////////////////////////////////////////////////////////////

class TRequestsInFlight
{
private:
    THashSet<TRequestHandlerBase*> Requests;
    TAdaptiveLock RequestsLock;

public:
    size_t GetCount() const
    {
        with_lock (RequestsLock) {
            return Requests.size();
        }
    }

    TVector<TIncompleteRequest> GetRequests() const
    {
        TIncompleteRequestStats<BlockStoreRequestsCount> stats;

        with_lock (RequestsLock) {
            ui64 now = GetCycleCount();
            for (auto* handler: Requests) {
                auto& req = handler->MetricRequest;
                ui64 started = AtomicGet(handler->CallContext->RequestStarted);

                if (started && started < now) {
                    auto requestTime = handler->CallContext->CalcExecutionTime(
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

    void Register(TRequestHandlerBase* handler)
    {
        with_lock (RequestsLock) {
            auto res = Requests.emplace(handler);
            Y_VERIFY(res.second);
        }
    }

    void Unregister(TRequestHandlerBase* handler)
    {
        with_lock (RequestsLock) {
            auto it = Requests.find(handler);
            Y_VERIFY(it != Requests.end());

            Requests.erase(it);
        }
    }

    void CancelAll()
    {
        with_lock (RequestsLock) {
            for (auto* handler: Requests) {
                handler->Cancel();
            }
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class TSessionStorage;

struct TAppContext
{
    TServerAppConfigPtr Config;
    ILoggingServicePtr Logging;
    TLog Log;
    IBlockStorePtr Service;

    std::unique_ptr<grpc::Server> Server;

    std::shared_ptr<TSessionStorage> SessionStorage;

    IServerStatsPtr ServerStats;

    TAtomic ShouldStop = 0;
};

////////////////////////////////////////////////////////////////////////////////

struct TExecutorContext
{
    std::unique_ptr<grpc::ServerCompletionQueue> CompletionQueue;
    TRequestsInFlight RequestsInFlight;
};

////////////////////////////////////////////////////////////////////////////////

template <typename TService>
constexpr bool IsDataService()
{
    return false;
}

template <>
constexpr bool IsDataService<NProto::TBlockStoreDataService::AsyncService>()
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_DECLARE_METHOD(name, ...)                                   \
    struct T##name##Method                                                     \
    {                                                                          \
        static constexpr EBlockStoreRequest Request = EBlockStoreRequest::name;\
                                                                               \
        using TRequest = NProto::T##name##Request;                             \
        using TResponse = NProto::T##name##Response;                           \
                                                                               \
        template <typename T, typename ...TArgs>                               \
        static void Prepare(T& service, TArgs&& ...args)                       \
        {                                                                      \
            service.Request##name(std::forward<TArgs>(args)...);               \
        }                                                                      \
                                                                               \
        template <typename T, typename ...TArgs>                               \
        static TFuture<TResponse> Execute(T& service, TArgs&& ...args)         \
        {                                                                      \
            return service.name(std::forward<TArgs>(args)...);                 \
        }                                                                      \
    };                                                                         \
// BLOCKSTORE_DECLARE_METHOD

BLOCKSTORE_GRPC_SERVICE(BLOCKSTORE_DECLARE_METHOD)

#undef BLOCKSTORE_DECLARE_METHOD

////////////////////////////////////////////////////////////////////////////////

class TSessionStorage final
    : public IClientAcceptor
{
    using TClientInfo = std::pair<ui32, IBlockStorePtr>;

private:
    TAppContext& AppCtx;

    TMutex Lock;
    THashMap<ui32, TClientInfo> ClientInfos;

public:
    TSessionStorage(TAppContext& appCtx)
        : AppCtx(appCtx)
    {}

    void Accept(
        const TSocketHolder& socket,
        IBlockStorePtr sessionService) override
    {
        if (AtomicGet(AppCtx.ShouldStop)) {
            return;
        }

        TSocketHolder dupSocket;

        with_lock (Lock) {
            auto it = FindClient(socket);
            Y_VERIFY(it == ClientInfos.end());

            dupSocket = SafeCreateDuplicate(socket);

            TClientInfo client = { (ui32)socket, std::move(sessionService) };
            auto res = ClientInfos.emplace((ui32)dupSocket, std::move(client));
            Y_VERIFY(res.second);
        }

        TLog& Log = AppCtx.Log;
        STORAGE_DEBUG("Accept client. Unix socket fd = " << (ui32)dupSocket);
        grpc::AddInsecureChannelFromFd(AppCtx.Server.get(), dupSocket.Release());
    }

    void Remove(const TSocketHolder& socket) override
    {
        if (AtomicGet(AppCtx.ShouldStop)) {
            return;
        }

        with_lock (Lock) {
            auto it = FindClient(socket);
            Y_VERIFY(it != ClientInfos.end());
            ClientInfos.erase(it);
        }
    }

    IBlockStorePtr GetSessionService(ui32 fd)
    {
        with_lock (Lock) {
            auto it = ClientInfos.find(fd);
            if (it != ClientInfos.end()) {
                const auto& clientInfo = it->second;
                return clientInfo.second;
            }
        }
        return nullptr;
    }

private:
    static TSocketHolder CreateDuplicate(const TSocketHolder& socket)
    {
        auto fileHandle = TFileHandle(socket);
        auto duplicateFd = fileHandle.Duplicate();
        fileHandle.Release();
        return TSocketHolder(duplicateFd);
    }

    // need for avoid race, see NBS-3325
    TSocketHolder SafeCreateDuplicate(const TSocketHolder& socket)
    {
        TList<TSocketHolder> holders;

        while (true) {
            TSocketHolder holder = CreateDuplicate(socket);
            if (ClientInfos.find(holder) == ClientInfos.end()) {
                return holder;
            }

            holders.push_back(std::move(holder));
        }
    }

    THashMap<ui32, TClientInfo>::iterator FindClient(const TSocketHolder& socket)
    {
        ui32 fd = socket;
        for (auto it = ClientInfos.begin(); it != ClientInfos.end(); ++it) {
            const auto& clientInfo = it->second;
            if (clientInfo.first == fd) {
                return it;
            }
        }
        return ClientInfos.end();
    }
};

////////////////////////////////////////////////////////////////////////////////

template<typename TMethod>
struct TRequestDataHolder
{};

template<>
struct TRequestDataHolder<TMountVolumeMethod>
{
    TString ClientId;
    TString InstanceId;
};

template<>
struct TRequestDataHolder<TUnmountVolumeMethod>
{
    TString DiskId;
    TString ClientId;
};

////////////////////////////////////////////////////////////////////////////////

template <typename TService, typename TMethod>
class TRequestHandler final
    : public TRequestHandlerBase
{
    using TRequest = typename TMethod::TRequest;
    using TResponse = typename TMethod::TResponse;

private:
    TAppContext& AppCtx;
    TExecutorContext& ExecCtx;
    TService& Service;

    std::unique_ptr<grpc::ServerContext> Context = std::make_unique<grpc::ServerContext>();
    grpc::ServerAsyncResponseWriter<TResponse> Writer;

    std::shared_ptr<TRequest> Request = std::make_shared<TRequest>();
    TFuture<TResponse> Response;

    static constexpr ui32 InvalidFd = INVALID_SOCKET;
    ui32 SourceFd = InvalidFd;

    [[no_unique_address]] TRequestDataHolder<TMethod> DataHolder;

public:
    TRequestHandler(
            TAppContext& appCtx,
            TExecutorContext& execCtx,
            TService& service)
        : TRequestHandlerBase(TMethod::Request)
        , AppCtx(appCtx)
        , ExecCtx(execCtx)
        , Service(service)
        , Writer(Context.get())
    {}

    static void Start(
        TAppContext& appCtx,
        TExecutorContext& execCtx,
        TService& service)
    {
        auto handler = std::make_unique<TRequestHandler<TService, TMethod>>(
            appCtx,
            execCtx,
            service);

        execCtx.RequestsInFlight.Register(handler.get());

        handler->PrepareRequest();
        handler.release();   // ownership transferred to CompletionQueue
    }

    void Process(bool ok) override
    {
        if (AtomicGet(AppCtx.ShouldStop) == 0 &&
            AtomicGet(RequestState) == WaitingForRequest)
        {
            // There always should be handler waiting for request.
            // Spawn new request only when handling request from server queue.
            Start(AppCtx, ExecCtx, Service);
        }

        if (!ok) {
            auto prevState = AtomicSwap(&RequestState, RequestCompleted);
            if (prevState != WaitingForRequest) {
                CompleteRequest();
            }
        }

        for (;;) {
            switch (AtomicGet(RequestState)) {
                case WaitingForRequest:
                    if (AtomicCas(&RequestState, ExecutingRequest, WaitingForRequest)) {
                        // fix NBS-2490
                        if (AtomicGet(AppCtx.ShouldStop)) {
                            Cancel();
                            return;
                        }

                        PrepareRequestContext();
                        ExecuteRequest();

                        // request is in progress now
                        return;
                    }
                    break;

                case ExecutionCompleted:
                    if (AtomicCas(&RequestState, SendingResponse, ExecutionCompleted)) {
                        try {
                            const auto& response = Response.GetValue();

                            // 'mute' fatal errors for stopped endpoints
                            if (HasError(response) && EndpointIsStopped()) {
                                SendResponse(TErrorResponse(E_GRPC_UNAVAILABLE, TStringBuilder()
                                    << "Endpoint has been stopped (fd = " << SourceFd << ")."
                                    << " Service error: " << response.GetError()));
                            } else {
                                SendResponse(response);
                            }
                        } catch (const TServiceError& e) {
                            SendResponse(GetErrorResponse(e));
                        } catch (...) {
                            SendResponse(GetErrorResponse(CurrentExceptionMessage()));
                        }

                        // request is in progress now
                        return;
                    }
                    break;

                case ExecutionCancelled:
                    if (AtomicCas(&RequestState, SendingResponse, ExecutionCancelled)) {
                        // cancel inflight requests due to server shutting down
                        SendError(grpc::Status(
                            grpc::StatusCode::UNAVAILABLE,
                            "Server shutting down"));

                        // request is in progress now
                        return;
                    }
                    break;

                case SendingResponse:
                    if (AtomicCas(&RequestState, RequestCompleted, SendingResponse)) {
                        CompleteRequest();
                    }
                    break;

                case RequestCompleted:
                    // request completed and could be safely destroyed
                    ExecCtx.RequestsInFlight.Unregister(this);

                    // free grpc server context as soon as we completed request
                    // because request handler may be destructed later than
                    // grpc shutdown
                    Context.reset();
                    return;
            }
        }
    }

    void Cancel() override
    {
        if (AtomicCas(&RequestState, ExecutionCancelled, ExecutingRequest)) {
            // will be processed on executor thread
            EnqueueCompletion(ExecCtx.CompletionQueue.get(), AcquireCompletionTag());
            return;
        }

        if (Context->c_call()) {
            Context->TryCancel();
        }
    }

private:
    void PrepareRequest()
    {
        TMethod::Prepare(
            Service,
            Context.get(),
            Request.get(),
            &Writer,
            ExecCtx.CompletionQueue.get(),
            ExecCtx.CompletionQueue.get(),
            AcquireCompletionTag());
    }

    void PrepareRequestContext()
    {
        auto& headers = *Request->MutableHeaders();

        const auto& metadata = Context->client_metadata();
        if (!metadata.empty()) {
            if (auto value = GetMetadata(metadata, NHeaders::ClientId)) {
                headers.SetClientId(TString{value});
            }
            if (auto value = GetMetadata(metadata, NHeaders::IdempotenceId)) {
                headers.SetIdempotenceId(TString{value});
            }
            if (auto value = GetMetadata(metadata, NHeaders::TraceId)) {
                headers.SetTraceId(TString{value});
            }
            if (auto value = GetMetadata(metadata, NHeaders::RequestId)) {
                ui64 requestId;
                if (TryFromString(value, requestId)) {
                    headers.SetRequestId(requestId);
                }
            }
            if (auto value = GetMetadata(metadata, NHeaders::Timestamp)) {
                ui64 timestamp;
                if (TryFromString(value, timestamp)) {
                    headers.SetTimestamp(timestamp);
                }
            }
            if (auto value = GetMetadata(metadata, NHeaders::RequestTimeout)) {
                ui32 requestTimeout;
                if (TryFromString(value, requestTimeout)) {
                    headers.SetRequestTimeout(requestTimeout);
                }
            }
        }

        auto now = TInstant::Now();

        auto timestamp = TInstant::MicroSeconds(headers.GetTimestamp());
        if (!timestamp || timestamp > now || now - timestamp > TDuration::Seconds(1)) {
            // fix request timestamp
            timestamp = now;
            headers.SetTimestamp(timestamp.MicroSeconds());
        }

        auto requestTimeout = TDuration::MilliSeconds(headers.GetRequestTimeout());
        if (!requestTimeout) {
            requestTimeout = AppCtx.Config->GetRequestTimeout();
            headers.SetRequestTimeout(requestTimeout.MilliSeconds());
        }

        CallContext->RequestId = headers.GetRequestId();
        if (!CallContext->RequestId) {
            CallContext->RequestId = CreateRequestId();
            headers.SetRequestId(CallContext->RequestId);
        }

        auto clientId = GetClientId(*Request);
        auto diskId = GetDiskId(*Request);
        ui64 startIndex = 0;
        ui64 requestBytes = 0;

        if (IsReadWriteRequest(MetricRequest.RequestType)) {
            startIndex = GetStartIndex(*Request);
            requestBytes = CalculateBytesCount(
                *Request,
                AppCtx.ServerStats->GetBlockSize(diskId));
        }

        AppCtx.ServerStats->PrepareMetricRequest(
            MetricRequest,
            std::move(clientId),
            std::move(diskId),
            startIndex,
            requestBytes);

         OnRequestPreparation(*Request, DataHolder);
    }

    void ValidateRequest()
    {
        auto authContext = Context->auth_context();
        Y_VERIFY(authContext);

        auto requestSource = TAuthMetadataProcessor::GetRequestSource(*authContext);

        if (!requestSource) {
            requestSource = NProto::SOURCE_FD_DATA_CHANNEL;
        }

        const bool isDataChannel = IsDataChannel(*requestSource);
        const bool isDataService = IsDataService<TService>();

        if (isDataChannel != isDataService) {
            ythrow TServiceError(E_GRPC_UNIMPLEMENTED)
                << "mismatched request channel";
        }

        // TODO: probably should fail request, if Internal was set by client
        auto& internal = *Request->MutableHeaders()->MutableInternal();
        internal.Clear();
        internal.SetRequestSource(*requestSource);

        // we will only get token from secure control channel
        if (requestSource == NProto::SOURCE_SECURE_CONTROL_CHANNEL) {
            internal.SetAuthToken(GetAuthToken(Context->client_metadata()));
        }
    }

    bool TryParseSourceFd(const TStringBuf& peer, ui32& fd)
    {
        static const TString PeerFdPrefix = "fd:";
        if (!peer.StartsWith(PeerFdPrefix)) {
            return false;
        }

        auto peerFd = peer.SubString(PeerFdPrefix.length(), peer.length());
        return TryFromString(peerFd, fd);
    }

    void ExecuteSocketEndpointRequest()
    {
        ui32 fd = 0;
        auto peer = Context->peer();
        bool result = TryParseSourceFd(peer, fd);

        if (!result) {
            TResponse response;
            auto& error = *response.MutableError();
            error.SetCode(E_FAIL);
            error.SetMessage(TStringBuilder() <<
                "Failed to parse request source fd: " << peer);
            Response = MakeFuture(std::move(response));
            return;
        }

        auto sessionService = AppCtx.SessionStorage->GetSessionService(fd);

        if (sessionService == nullptr) {
            TErrorResponse response(E_GRPC_UNAVAILABLE, TStringBuilder()
                << "Endpoint has been stopped (fd = " << fd << ").");
            Response = MakeFuture<TResponse>(std::move(response));
            return;
        }

        SourceFd = fd;
        Response = TMethod::Execute(
            *sessionService,
            CallContext,
            std::move(Request));
    }

    bool EndpointIsStopped() const
    {
        if (SourceFd == InvalidFd) {
            return false;
        }

        auto sessionService = AppCtx.SessionStorage->GetSessionService(SourceFd);
        return sessionService == nullptr;
    }

    void ExecuteRequest()
    {
        TString message;
        if (IsControlRequest(MetricRequest.RequestType)) {
            message = TStringBuilder() << *Request;
        }

        AppCtx.ServerStats->RequestStarted(
            AppCtx.Log,
            MetricRequest,
            *CallContext,
            message);

        try {
            ValidateRequest();

            const auto& internal = Request->GetHeaders().GetInternal();
            auto requestSource = internal.GetRequestSource();

            if (requestSource == NProto::SOURCE_FD_DATA_CHANNEL) {
                ExecuteSocketEndpointRequest();
            }  else {
                Response = TMethod::Execute(
                    *AppCtx.Service,
                    CallContext,
                    std::move(Request));
            }
        } catch (const TServiceError& e) {
            Response = MakeFuture(GetErrorResponse(e));
        } catch (...) {
            Response = MakeFuture(GetErrorResponse(CurrentExceptionMessage()));
        }

        auto* tag = AcquireCompletionTag();
        Response.Subscribe(
            [=] (const auto& response) {
                Y_UNUSED(response);

                if (AtomicCas(&RequestState, ExecutionCompleted, ExecutingRequest)) {
                    // will be processed on executor thread
                    EnqueueCompletion(ExecCtx.CompletionQueue.get(), tag);
                    return;
                }

                if (ReleaseCompletionTag()) {
                    // request completed and could be safely destroyed
                    delete this;
                }
            });
    }

    void SendResponse(const TResponse& response)
    {
        if (response.HasError()) {
            Error = response.GetError();
        }

        OnRequestCompletion(response, DataHolder);

        AppCtx.ServerStats->ResponseSent(MetricRequest, *CallContext);

        Writer.Finish(response, grpc::Status::OK, AcquireCompletionTag());
    }

    void SendError(const grpc::Status& status)
    {
        Error.SetCode(MAKE_GRPC_ERROR(status.error_code()));
        Error.SetMessage(ToString(status.error_message()));

        AppCtx.ServerStats->ResponseSent(MetricRequest, *CallContext);

        Writer.FinishWithError(status, AcquireCompletionTag());
    }

    void CompleteRequest()
    {
        AppCtx.ServerStats->RequestCompleted(
            AppCtx.Log,
            MetricRequest,
            *CallContext,
            Error);
    }

    static TStringBuf GetMetadata(
        const std::multimap<grpc::string_ref, grpc::string_ref>& metadata,
        const grpc::string_ref& key)
    {
        auto it = metadata.find(key);
        if (it != metadata.end()) {
            return { it->second.data(), it->second.size() };
        }
        return {};
    }

    static TResponse GetErrorResponse(const TServiceError& e)
    {
        TResponse response;

        auto& error = *response.MutableError();
        error.SetCode(e.GetCode());
        error.SetMessage(e.what());

        return response;
    }

    static TResponse GetErrorResponse(TString message)
    {
        TResponse response;

        auto& error = *response.MutableError();
        error.SetCode(E_FAIL);
        error.SetMessage(std::move(message));

        return response;
    }


    template <typename T>
    void OnRequestPreparation(const TRequest& request, T& data)
    {
        Y_UNUSED(request);
        Y_UNUSED(data);
    }

    template <>
    void OnRequestPreparation(
        const TRequest& request,
        TRequestDataHolder<TMountVolumeMethod>& data)
    {
        data.ClientId = GetClientId(request);
        data.InstanceId = request.GetInstanceId();
    }

    template <>
    void OnRequestPreparation(
        const TRequest& request,
        TRequestDataHolder<TUnmountVolumeMethod>& data)
    {
        data.ClientId = GetClientId(request);
        data.DiskId = request.GetDiskId();
    }

    template <typename T>
    void OnRequestCompletion(const TResponse& response, T& data)
    {
        Y_UNUSED(response);
        Y_UNUSED(data);
    }

    template <>
    void OnRequestCompletion(
        const TResponse& response,
        TRequestDataHolder<TMountVolumeMethod>& data)
    {
        if (!HasError(response) && response.HasVolume()) {
            AppCtx.ServerStats->MountVolume(
                response.GetVolume(),
                data.ClientId,
                data.InstanceId);
        }
    }

    template <>
    void OnRequestCompletion(
        const TResponse& response,
        TRequestDataHolder<TUnmountVolumeMethod>& data)
    {
        if (!HasError(response)) {
            AppCtx.ServerStats->UnmountVolume(data.DiskId, data.ClientId);
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class TServer final
    : public TAppContext
    , public IServer
{
    class TExecutor;

private:
    TGrpcInitializer GrpcInitializer;

    NProto::TBlockStoreService::AsyncService ControlService;
    NProto::TBlockStoreDataService::AsyncService DataService;

    TVector<std::unique_ptr<TExecutor>> Executors;

    std::unique_ptr<TEndpointPoller> EndpointPoller;

public:
    TServer(
        TServerAppConfigPtr config,
        ILoggingServicePtr logging,
        IServerStatsPtr serverStats,
        IBlockStorePtr service);

    ~TServer() override;

    void Start() override;
    void Stop() override;

    IClientAcceptorPtr GetClientAcceptor() override;

    TVector<TIncompleteRequest> GetIncompleteRequests() override;

private:
    template <typename TMethod, typename TService>
    void StartRequest(TService& service);
    void StartRequests();

    grpc::SslServerCredentialsOptions CreateSslOptions();

    void StartListenUnixSocket(const TString& unixSocketPath, ui32 backlog);
    void StopListenUnixSocket();
};

////////////////////////////////////////////////////////////////////////////////

class TServer::TExecutor final
    : public ISimpleThread
    , public TExecutorContext
{
private:
    TString Name;
    TExecutorCounters::TExecutorScope ExecutorScope;

public:
    TExecutor(
            TAppContext& appCtx,
            TString name,
            std::unique_ptr<grpc::ServerCompletionQueue> completionQueue)
        : Name(std::move(name))
        , ExecutorScope(appCtx.ServerStats->StartExecutor())
    {
        CompletionQueue = std::move(completionQueue);
    }

    void Shutdown()
    {
        if (CompletionQueue) {
            CompletionQueue->Shutdown();
        }

        Join();

        CompletionQueue.reset();
    }

private:
    void* ThreadProc() override
    {
        ::NCloud::SetCurrentThreadName(Name);

        auto tagName = TStringBuilder() << "BLOCKSTORE_THREAD_" << Name;
        NProfiling::TMemoryTagScope tagScope(tagName.data());

        void* tag;
        bool ok;
        while (WaitRequest(&tag, &ok)) {
            ProcessRequest(tag, ok);
        }

        return nullptr;
    }

    bool WaitRequest(void** tag, bool* ok)
    {
        auto activity = ExecutorScope.StartWait();

        return CompletionQueue->Next(tag, ok);
    }

    void ProcessRequest(void* tag, bool ok)
    {
        auto activity = ExecutorScope.StartExecute();

        TRequestHandlerPtr handler(static_cast<TRequestHandlerBase*>(tag));
        handler->Process(ok);

        if (!handler->ReleaseCompletionTag()) {
            handler.release();   // ownership transferred to CompletionQueue
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

TServer::TServer(
    TServerAppConfigPtr config,
    ILoggingServicePtr logging,
    IServerStatsPtr serverStats,
    IBlockStorePtr service)
{
    Config = std::move(config);
    Log = logging->CreateLog("BLOCKSTORE_SERVER");
    Logging = std::move(logging);
    ServerStats = std::move(serverStats);
    Service = std::move(service);
    SessionStorage = std::make_shared<TSessionStorage>(*this);
}

TServer::~TServer()
{
    Stop();
}

void TServer::Start()
{
    grpc::ServerBuilder builder;
    builder.RegisterService(&ControlService);
    builder.RegisterService(&DataService);

    ui32 maxMessageSize = Config->GetMaxMessageSize();
    if (maxMessageSize) {
        builder.SetMaxSendMessageSize(maxMessageSize);
        builder.SetMaxReceiveMessageSize(maxMessageSize);
    }

    ui32 memoryQuotaBytes = Config->GetMemoryQuotaBytes();
    if (memoryQuotaBytes) {
        grpc::ResourceQuota quota("memory_bound");
        quota.Resize(memoryQuotaBytes);

        builder.SetResourceQuota(quota);
    }

    if (Config->GetKeepAliveEnabled()) {
        builder.SetOption(std::make_unique<TKeepAliveOption>(
            Config->GetKeepAliveIdleTimeout(),
            Config->GetKeepAliveProbeTimeout(),
            Config->GetKeepAliveProbesCount()));
    }

    if (auto arg = Config->GetGrpcKeepAliveTime()) {
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, arg);
    }

    if (auto arg = Config->GetGrpcKeepAliveTimeout()) {
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, arg);
    }

    if (auto arg = Config->GetGrpcKeepAlivePermitWithoutCalls()) {
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, arg);
    }

    if (auto arg = Config->GetGrpcHttp2MinRecvPingIntervalWithoutData()) {
        builder.AddChannelArgument(GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS, arg);
    }

    if (auto arg = Config->GetGrpcHttp2MinSentPingIntervalWithoutData()) {
        builder.AddChannelArgument(GRPC_ARG_HTTP2_MIN_SENT_PING_INTERVAL_WITHOUT_DATA_MS, arg);
    }

    if (auto port = Config->GetPort()) {
        auto address = Join(":", Config->GetHost(), port);
        STORAGE_INFO("Listen on (insecure control) " << address);

        auto credentials = CreateInsecureServerCredentials();
        credentials->SetAuthMetadataProcessor(
            std::make_shared<TAuthMetadataProcessor>(
                NProto::SOURCE_INSECURE_CONTROL_CHANNEL));

        builder.AddListeningPort(
            address,
            std::move(credentials));
    }

    if (auto port = Config->GetSecurePort()) {
        auto host = Config->GetSecureHost() ? Config->GetSecureHost() : Config->GetHost();
        auto address = Join(":", host, port);
        STORAGE_INFO("Listen on (secure control) " << address);

        auto sslOptions = CreateSslOptions();
        auto credentials = grpc::SslServerCredentials(sslOptions);
        credentials->SetAuthMetadataProcessor(
            std::make_shared<TAuthMetadataProcessor>(
                NProto::SOURCE_SECURE_CONTROL_CHANNEL));

        builder.AddListeningPort(
            address,
            std::move(credentials));
    }

    if (auto port = Config->GetDataPort()) {
        auto address = Join(":", Config->GetDataHost(), port);
        STORAGE_INFO("Listen on (data) " << address);

        auto credentials = CreateInsecureServerCredentials();
        credentials->SetAuthMetadataProcessor(
            std::make_shared<TAuthMetadataProcessor>(
                NProto::SOURCE_TCP_DATA_CHANNEL));

        builder.AddListeningPort(
            address,
            std::move(credentials));
    }

    ui32 threadsCount = Config->GetThreadsCount();
    for (size_t i = 1; i <= threadsCount; ++i) {
        auto executor = std::make_unique<TExecutor>(
            *this,
            TStringBuilder() << "SRV" << i,
            builder.AddCompletionQueue());
        executor->Start();
        Executors.push_back(std::move(executor));
    }

    Server = builder.BuildAndStart();
    if (!Server) {
        ythrow TServiceError(E_FAIL)
            << "could not start gRPC server";
    }

    auto unixSocketPath = Config->GetUnixSocketPath();
    if (unixSocketPath) {
        ui32 backlog = Config->GetUnixSocketBacklog();
        StartListenUnixSocket(unixSocketPath, backlog);
    }

    StartRequests();
}

grpc::SslServerCredentialsOptions TServer::CreateSslOptions()
{
    grpc::SslServerCredentialsOptions sslOptions;
    sslOptions.client_certificate_request = GRPC_SSL_REQUEST_CLIENT_CERTIFICATE_AND_VERIFY;

    if (const auto& rootCertsFile = Config->GetRootCertsFile()) {
        sslOptions.pem_root_certs = ReadFile(rootCertsFile);
    }

    if (Config->GetCerts().empty()) {
        // TODO: Remove, when old CertFile, CertPrivateKeyFile options are gone.
        grpc::SslServerCredentialsOptions::PemKeyCertPair keyCert;

        Y_ENSURE(Config->GetCertFile(), "Empty CertFile");
        keyCert.cert_chain = ReadFile(Config->GetCertFile());

        Y_ENSURE(Config->GetCertPrivateKeyFile(), "Empty CertPrivateKeyFile");
        keyCert.private_key = ReadFile(Config->GetCertPrivateKeyFile());

        sslOptions.pem_key_cert_pairs.push_back(keyCert);
    }

    for (const auto& cert: Config->GetCerts()) {
        grpc::SslServerCredentialsOptions::PemKeyCertPair keyCert;

        Y_ENSURE(cert.CertFile, "Empty CertFile");
        keyCert.cert_chain = ReadFile(cert.CertFile);

        Y_ENSURE(cert.CertPrivateKeyFile, "Empty CertPrivateKeyFile");
        keyCert.private_key = ReadFile(cert.CertPrivateKeyFile);

        sslOptions.pem_key_cert_pairs.push_back(keyCert);
    }

    return sslOptions;
}

void TServer::StartListenUnixSocket(
    const TString& unixSocketPath,
    ui32 backlog)
{
    STORAGE_INFO("Listen on (data) " << unixSocketPath.Quote());

    EndpointPoller = std::make_unique<TEndpointPoller>(SessionStorage);
    EndpointPoller->Start();

    auto error = EndpointPoller->StartListenEndpoint(
        unixSocketPath,
        backlog,
        true,   // multiClient
        Service);

    if (HasError(error)) {
        ythrow TServiceError(error.GetCode()) << error.GetMessage();
    }
}

void TServer::StopListenUnixSocket()
{
    if (EndpointPoller) {
        EndpointPoller->Stop();
        EndpointPoller.reset();
    }
}

void TServer::Stop()
{
    if (AtomicSwap(&ShouldStop, 1) == 1) {
        return;
    }

    STORAGE_INFO("Shutting down");

    StopListenUnixSocket();

    for (auto& executor: Executors) {
        executor->RequestsInFlight.CancelAll();
    }

    auto deadline = Config->GetShutdownTimeout().ToDeadLine();

    if (Server) {
        Server->Shutdown(deadline);
    }

    for (;;) {
        size_t requestsCount = 0;
        for (auto& executor: Executors) {
            requestsCount += executor->RequestsInFlight.GetCount();
        }

        if (!requestsCount) {
            break;
        }

        if (deadline <= TInstant::Now()) {
            STORAGE_WARN("Some requests are still active on shutdown: " << requestsCount);
            break;
        }

        Sleep(TDuration::MilliSeconds(100));
    }

    for (auto& executor: Executors) {
        executor->Shutdown();
    }

    Executors.clear();
}

IClientAcceptorPtr TServer::GetClientAcceptor()
{
    return SessionStorage;
}

template <typename TMethod, typename TService>
void TServer::StartRequest(TService& service)
{
    ui32 preparedRequestsCount = Config->GetPreparedRequestsCount();
    for (auto& executor: Executors) {
        for (size_t i = 0; i < preparedRequestsCount; ++i) {
            TRequestHandler<TService, TMethod>::Start(*this, *executor, service);
        }
    }
}

void TServer::StartRequests()
{
#define BLOCKSTORE_START_REQUEST(name, service, ...)                           \
    StartRequest<T##name##Method>(service);                                    \
// BLOCKSTORE_START_REQUEST

    BLOCKSTORE_GRPC_SERVICE(BLOCKSTORE_START_REQUEST, ControlService)
    BLOCKSTORE_GRPC_DATA_SERVICE(BLOCKSTORE_START_REQUEST, DataService)

#undef BLOCKSTORE_START_REQUEST
}

TVector<TIncompleteRequest> TServer::GetIncompleteRequests()
{
    TVector<TIncompleteRequest> requestsInFlight;

    for (auto& executor: Executors) {
        auto requests = executor->RequestsInFlight.GetRequests();
        requestsInFlight.insert(
            requestsInFlight.end(),
            requests.begin(),
            requests.end());
    }

    return requestsInFlight;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IServerPtr CreateServer(
    TServerAppConfigPtr config,
    ILoggingServicePtr logging,
    IServerStatsPtr serverStats,
    IBlockStorePtr service)
{
    return std::make_shared<TServer>(
        std::move(config),
        std::move(logging),
        std::move(serverStats),
        std::move(service));
}

}   // namespace NCloud::NBlockStore::NServer
