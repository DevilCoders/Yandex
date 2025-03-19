#include "server.h"

#include "config.h"
#include "probes.h"

#include <cloud/filestore/public/api/grpc/service.grpc.pb.h>

#include <cloud/filestore/libs/diagnostics/profile_log.h>
#include <cloud/filestore/libs/diagnostics/request_stats.h>

#include <cloud/filestore/libs/service/context.h>
#include <cloud/filestore/libs/service/endpoint.h>
#include <cloud/filestore/libs/service/filestore.h>
#include <cloud/filestore/libs/service/request.h>
#include <cloud/filestore/libs/service/request_helpers.h>

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/thread.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/grpc/completion.h>
#include <cloud/storage/core/libs/grpc/credentials.h>
#include <cloud/storage/core/libs/grpc/initializer.h>
#include <cloud/storage/core/libs/grpc/keepalive.h>
#include <cloud/storage/core/libs/grpc/time.h>

#include <library/cpp/actors/prof/tag.h>
#include <library/cpp/threading/atomic/bool.h>

#include <contrib/libs/grpc/include/grpcpp/completion_queue.h>
#include <contrib/libs/grpc/include/grpcpp/resource_quota.h>
#include <contrib/libs/grpc/include/grpcpp/security/auth_metadata_processor.h>
#include <contrib/libs/grpc/include/grpcpp/security/server_credentials.h>
#include <contrib/libs/grpc/include/grpcpp/server.h>
#include <contrib/libs/grpc/include/grpcpp/server_builder.h>
#include <contrib/libs/grpc/include/grpcpp/server_context.h>
#include <contrib/libs/grpc/include/grpcpp/server_posix.h>
#include <contrib/libs/grpc/include/grpcpp/support/status.h>

#include <util/generic/deque.h>
#include <util/generic/hash_set.h>
#include <util/string/builder.h>
#include <util/string/join.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/thread.h>

namespace NCloud::NFileStore::NServer {

using namespace NThreading;

LWTRACE_USING(FILESTORE_SERVER_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

NProto::TError MakeGrpcError(const grpc::Status& status)
{
    NProto::TError error;
    if (!status.ok()) {
        error.SetCode(MAKE_GRPC_ERROR(status.error_code()));
        error.SetMessage(TString(status.error_message()));
    }
    return error;
}

grpc::Status MakeGrpcStatus(const NProto::TError& error)
{
    if (HasError(error)) {
        return { grpc::UNAVAILABLE, error.GetMessage() };
    }
    return grpc::Status::OK;
}

////////////////////////////////////////////////////////////////////////////////

#define FILESTORE_DECLARE_METHOD(name, proto, method, ...)                     \
    struct T##name##Method                                                     \
    {                                                                          \
        static constexpr auto RequestName = TStringBuf(#method);              \
        static constexpr auto RequestType = EFileStoreRequest::method;         \
                                                                               \
        using TRequest = NProto::T##proto##Request;                            \
        using TResponse = NProto::T##proto##Response;                          \
                                                                               \
        template <typename T, typename ...TArgs>                               \
        static auto Prepare(T& service, TArgs&& ...args)                       \
        {                                                                      \
            return service.Request##method(std::forward<TArgs>(args)...);      \
        }                                                                      \
                                                                               \
        template <typename T, typename ...TArgs>                               \
        static auto Execute(T& service, TArgs&& ...args)                       \
        {                                                                      \
            return service.method(std::forward<TArgs>(args)...);               \
        }                                                                      \
    };                                                                         \
// FILESTORE_DECLARE_METHOD

#define FILESTORE_DECLARE_METHOD_FS(name, ...) \
    FILESTORE_DECLARE_METHOD(name##Fs, name, name, __VA_ARGS__)

#define FILESTORE_DECLARE_METHOD_VHOST(name, ...) \
    FILESTORE_DECLARE_METHOD(name##Vhost, name, name, __VA_ARGS__)

#define FILESTORE_DECLARE_METHOD_STREAM(name, ...) \
    FILESTORE_DECLARE_METHOD(name##Stream, name, name##Stream, __VA_ARGS__)

FILESTORE_SERVICE(FILESTORE_DECLARE_METHOD_FS)
FILESTORE_ENDPOINT_SERVICE(FILESTORE_DECLARE_METHOD_VHOST)
FILESTORE_DECLARE_METHOD_STREAM(GetSessionEvents)

#undef FILESTORE_DECLARE_METHOD
#undef FILESTORE_DECLARE_METHOD_FS
#undef FILESTORE_DECLARE_METHOD_VHOST
#undef FILESTORE_DECLARE_METHOD_STREAM

////////////////////////////////////////////////////////////////////////////////

class TRequestHandlerBase
{
private:
    TAtomic RefCount = 0;

protected:
    TServerCallContextPtr CallContext;
    NAtomic::TBool Started = false;

public:
    virtual ~TRequestHandlerBase() = default;

    virtual void Process(bool ok) = 0;
    virtual void Cancel() = 0;

    TMaybe<TIncompleteRequest> ToIncompleteRequest(ui64 nowCycles)
    {
        if (!Started) {
            return Nothing();
        }

        ui64 startedCycles = CallContext->GetStartedCycles();
        if (startedCycles > nowCycles) {
            return Nothing();
        }

        return TIncompleteRequest{
            NProto::EStorageMediaKind::STORAGE_MEDIA_DEFAULT, // TODO cache media kind on prepare
            static_cast<ui32>(CallContext->RequestType),
            CyclesToDurationSafe(nowCycles - startedCycles)
        };
    }

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
    : public IIncompleteRequestProvider
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

    TVector<TIncompleteRequest> GetIncompleteRequests() override
    {
        auto now = GetCycleCount();
        with_lock (RequestsLock) {
            TVector<TIncompleteRequest> requests;
            requests.reserve(Requests.size());
            for (auto* r: Requests) {
                if (auto incompleteRequest = r->ToIncompleteRequest(now)) {
                    requests.push_back(std::move(*incompleteRequest));
                }
            }

            return requests;
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TAppContext
{
    TLog Log;
    TAtomic ShouldStop = 0;
    IServerStatsPtr Stats;
    IProfileLogPtr ProfileLog;
};

struct TFileStoreContext : TAppContext
{
    NProto::TFileStoreService::AsyncService Service;
    IFileStoreServicePtr ServiceImpl;
};

struct TEndpointManagerContext : TAppContext
{
    NProto::TEndpointManagerService::AsyncService Service;
    IEndpointManagerPtr ServiceImpl;
};

struct TExecutorContext
{
    std::unique_ptr<grpc::ServerCompletionQueue> CompletionQueue;
    TRequestsInFlight RequestsInFlight;
};

////////////////////////////////////////////////////////////////////////////////

template <typename TAppContext, typename TMethod>
class TRequestHandler final
    : public TRequestHandlerBase
{
    using TRequest = typename TMethod::TRequest;
    using TResponse = typename TMethod::TResponse;

private:
    TAppContext& AppCtx;
    TExecutorContext& ExecCtx;

    std::unique_ptr<grpc::ServerContext> Context = std::make_unique<grpc::ServerContext>();
    grpc::ServerAsyncResponseWriter<TResponse> Writer;

    std::shared_ptr<TRequest> Request = std::make_shared<TRequest>();
    ui64 RequestId = 0;
    TFuture<TResponse> Response;

    enum {
        WaitingForRequest = 0,
        ExecutingRequest = 1,
        ExecutionCompleted = 2,
        ExecutionCancelled = 3,
        SendingResponse = 4,
        RequestCompleted = 5,
    };
    TAtomic RequestState = WaitingForRequest;

    using TBlockRanges =
        google::protobuf::RepeatedPtrField<NProto::TProfileLogBlockRange>;
    TBlockRanges ProfileLogBlockRanges;
    NProto::TProfileLogNodeInfo ProfileLogNodeInfo;
    ui32 ProfileLogErrorCode = S_OK;

public:
    TRequestHandler(TAppContext& appCtx, TExecutorContext& execCtx)
        : AppCtx(appCtx)
        , ExecCtx(execCtx)
        , Writer(Context.get())
    {}

    static void Start(TAppContext& appCtx, TExecutorContext& execCtx)
    {
        using THandler = TRequestHandler<TAppContext, TMethod>;
        auto handler = std::make_unique<THandler>(appCtx, execCtx);

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
            Start(AppCtx, ExecCtx);
        }

        if (!ok) {
            AtomicSet(RequestState, RequestCompleted);
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
                            SendResponse(response);
                        } catch (const TServiceError& e) {
                            SendResponse(ErrorResponse<TResponse>(
                                e.GetCode(), TString(e.GetMessage())));
                        } catch (...) {
                            SendResponse(ErrorResponse<TResponse>(
                                E_FAIL, CurrentExceptionMessage()));
                        }

                        // request is in progress now
                        return;
                    }
                    break;

                case ExecutionCancelled:
                    if (AtomicCas(&RequestState, SendingResponse, ExecutionCancelled)) {
                        // cancel inflight requests due to server shutting down
                        SendResponse(grpc::Status(
                            grpc::StatusCode::UNAVAILABLE,
                            "Server shutting down"));

                        // request is in progress now
                        return;
                    }
                    break;

                case SendingResponse:
                    if (AtomicCas(&RequestState, RequestCompleted, SendingResponse)) {
                    }
                    break;

                case RequestCompleted:
                    CompleteRequest();

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
            AppCtx.Service,
            Context.get(),
            Request.get(),
            &Writer,
            ExecCtx.CompletionQueue.get(),
            ExecCtx.CompletionQueue.get(),
            AcquireCompletionTag());
    }

    template <typename T>
    void InitProfileLogNodeInfo(const T&)
    {
    }

    void InitProfileLogNodeInfo(const NProto::TCreateNodeRequest& request)
    {
        ProfileLogNodeInfo.SetNewParentNodeId(request.GetNodeId());
        ProfileLogNodeInfo.SetNewNodeName(request.GetName());
    }

    void InitProfileLogNodeInfo(const NProto::TUnlinkNodeRequest& request)
    {
        ProfileLogNodeInfo.SetParentNodeId(request.GetNodeId());
        ProfileLogNodeInfo.SetNodeName(request.GetName());
    }

    void InitProfileLogNodeInfo(const NProto::TRenameNodeRequest& request)
    {
        ProfileLogNodeInfo.SetParentNodeId(request.GetNodeId());
        ProfileLogNodeInfo.SetNodeName(request.GetName());
        ProfileLogNodeInfo.SetNewParentNodeId(request.GetNewParentId());
        ProfileLogNodeInfo.SetNewNodeName(request.GetNewName());
    }

    void InitProfileLogNodeInfo(const NProto::TCreateHandleRequest& request)
    {
        ProfileLogNodeInfo.SetParentNodeId(request.GetNodeId());
        ProfileLogNodeInfo.SetNodeName(request.GetName());
    }

    template <typename T>
    void FinalizeProfileLogNodeInfo(const T&)
    {
    }

    void FinalizeProfileLogNodeInfo(const NProto::TCreateNodeResponse& response)
    {
        ProfileLogNodeInfo.SetNodeId(response.GetNode().GetId());
    }

    void FinalizeProfileLogNodeInfo(const NProto::TCreateHandleResponse& response)
    {
        ProfileLogNodeInfo.SetNodeId(response.GetNodeAttr().GetId());
    }

    template <typename T>
    void InitProfileLogBlockRanges(const T& request)
    {
        if (auto nodeId = GetNodeId(request)) {
            auto* range = ProfileLogBlockRanges.Add();
            range->SetNodeId(nodeId);
        }
    }

    void InitProfileLogBlockRanges(const NProto::TWriteDataRequest& request)
    {
        auto* range = ProfileLogBlockRanges.Add();
        range->SetNodeId(request.GetNodeId());
        range->SetOffset(request.GetOffset());
        range->SetBytes(request.GetBuffer().Size());
    }

    void InitProfileLogBlockRanges(const NProto::TReadDataRequest& request)
    {
        auto* range = ProfileLogBlockRanges.Add();
        range->SetNodeId(request.GetNodeId());
        range->SetOffset(request.GetOffset());
        range->SetBytes(request.GetLength());
    }

    void InitProfileLogBlockRanges(const NProto::TAddClusterNodeRequest&)
    {
    }

    void InitProfileLogBlockRanges(const NProto::TRemoveClusterNodeRequest&)
    {
    }

    void InitProfileLogBlockRanges(const NProto::TAddClusterClientsRequest&)
    {
    }

    void InitProfileLogBlockRanges(const NProto::TRemoveClusterClientsRequest&)
    {
    }

    void InitProfileLogBlockRanges(const NProto::TListClusterClientsRequest&)
    {
    }

    void InitProfileLogBlockRanges(const NProto::TUpdateClusterRequest&)
    {
    }

    void PrepareRequestContext()
    {
        auto& headers = *Request->MutableHeaders();

        auto now = TInstant::Now();
        auto timestamp = TInstant::MicroSeconds(headers.GetTimestamp());
        if (!timestamp || timestamp > now || now - timestamp > TDuration::Seconds(1)) {
            // fix request timestamp
            timestamp = now;
            headers.SetTimestamp(timestamp.MicroSeconds());
        }

        RequestId = headers.GetRequestId();
        if (!RequestId) {
            RequestId = CreateRequestId();
            headers.SetRequestId(RequestId);
        }

        CallContext = MakeIntrusive<TServerCallContext>(RequestId, GetFileSystemId(*Request));
        CallContext->RequestSize = CalculateRequestSize(*Request);
        CallContext->RequestType = TMethod::RequestType;
    }

    void ExecuteRequest()
    {
        InitProfileLogBlockRanges(*Request);
        InitProfileLogNodeInfo(*Request);

        auto& Log = AppCtx.Log;

        STORAGE_TRACE(TMethod::RequestName
            << " #" << RequestId
            << " execute request: " << DumpMessage(*Request));

        FILESTORE_TRACK(
            ExecuteRequest,
            CallContext,
            TString(TMethod::RequestName),
            NProto::STORAGE_MEDIA_SSD,  // TODO NBS-2954
            CallContext->FileSystemId);

        AppCtx.Stats->RequestStarted(Log, *CallContext);
        Started = true;

        try {
            Response = TMethod::Execute(
                *AppCtx.ServiceImpl,
                CallContext,
                std::move(Request));
        } catch (const TServiceError& e) {
            Response = MakeFuture(
                ErrorResponse<TResponse>(e.GetCode(), TString(e.GetMessage())));
        } catch (...) {
            Response = MakeFuture(
                ErrorResponse<TResponse>(E_FAIL, CurrentExceptionMessage()));
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
        auto& Log = AppCtx.Log;

        if (HasError(response)) {
            CallContext->Error = response.GetError();
        }

        STORAGE_TRACE(TMethod::RequestName
            << " #" << RequestId
            << " send response: " << DumpMessage(response));

        FILESTORE_TRACK(
            SendResponse,
            CallContext,
            TString(TMethod::RequestName));

        AppCtx.Stats->ResponseSent(*CallContext);

        FinalizeProfileLogNodeInfo(response);

        Writer.Finish(response, grpc::Status::OK, AcquireCompletionTag());
    }

    void SendResponse(const grpc::Status& status)
    {
        auto& Log = AppCtx.Log;

        CallContext->Error = MakeGrpcError(status);
        STORAGE_TRACE(TMethod::RequestName
            << " #" << RequestId
            << " send response: " << FormatError(CallContext->Error));

        FILESTORE_TRACK(
            SendResponse,
            CallContext,
            TString(TMethod::RequestName));

        AppCtx.Stats->ResponseSent(*CallContext);

        ProfileLogErrorCode = CallContext->Error.GetCode();

        Writer.FinishWithError(status, AcquireCompletionTag());
    }

    void CompleteRequest()
    {
        auto& Log = AppCtx.Log;

        STORAGE_TRACE(TMethod::RequestName
            << " #" << RequestId
            << " request completed");

        if (CallContext) {
            FILESTORE_TRACK(
                RequestCompleted,
                CallContext,
                TString(TMethod::RequestName));

            AppCtx.Stats->RequestCompleted(Log, *CallContext);

            IProfileLog::TRecord record;
            record.FileSystemId = CallContext->FileSystemId;
            record.Request.SetTimestampMcs(
                CallContext->RequestStartedTs.MicroSeconds());
            record.Request.SetDurationMcs(
                (Now() - CallContext->RequestStartedTs).MicroSeconds());
            record.Request.SetRequestType(
                static_cast<ui32>(CallContext->RequestType));
            *record.Request.MutableRanges() = std::move(ProfileLogBlockRanges);
            *record.Request.MutableNodeInfo() = std::move(ProfileLogNodeInfo);
            record.Request.SetErrorCode(ProfileLogErrorCode);

            AppCtx.ProfileLog->Write(std::move(record));
        } else {
            // request processing was aborted
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename TAppContext, typename TMethod>
class TStreamRequestHandler final
    : public TRequestHandlerBase
{
    using TRequest = typename TMethod::TRequest;
    using TResponse = typename TMethod::TResponse;

    class TResponseHandler final
        : public IResponseHandler<TResponse>
    {
    private:
        TStreamRequestHandler* Self;

    public:
        TResponseHandler(TStreamRequestHandler* self)
            : Self(self)
        {}

        void HandleResponse(const TResponse& response) override
        {
            Self->HandleResponse(response);
        }

        void HandleCompletion(const NProto::TError& error) override
        {
            Self->HandleCompletion(error);

            if (Self->ReleaseCompletionTag()) {
                // request completed and could be safely destroyed
                delete Self;
            }
        }
    };

private:
    TAppContext& AppCtx;
    TExecutorContext& ExecCtx;

    std::unique_ptr<grpc::ServerContext> Context = std::make_unique<grpc::ServerContext>();
    grpc::ServerAsyncWriter<TResponse> Writer;

    std::shared_ptr<TRequest> Request = std::make_shared<TRequest>();

    enum {
        WaitingForRequest = 0,
        ExecutingRequest = 1,
        ExecutionCompleted = 2,
        ExecutionCancelled = 3,
        SendingResponse = 4,
        SendingCompletion = 5,
        RequestCompleted = 6,
    };
    TAtomic RequestState = WaitingForRequest;

    TDeque<TResponse> ResponseQueue;
    TMaybe<grpc::Status> CompletionStatus;
    TAdaptiveLock ResponseLock;

    ui64 RequestId = 0;

public:
    TStreamRequestHandler(TAppContext& appCtx, TExecutorContext& execCtx)
        : AppCtx(appCtx)
        , ExecCtx(execCtx)
        , Writer(Context.get())
    {}

    static void Start(TAppContext& appCtx, TExecutorContext& execCtx)
    {
        using THandler = TStreamRequestHandler<TAppContext, TMethod>;
        auto handler = std::make_unique<THandler>(appCtx, execCtx);

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
            Start(AppCtx, ExecCtx);
        }

        if (!ok) {
            AtomicSet(RequestState, RequestCompleted);
        }

        for (;;) {
            switch (AtomicGet(RequestState)) {
                case WaitingForRequest:
                    if (AtomicCas(&RequestState, ExecutingRequest, WaitingForRequest)) {
                        PrepareRequestContext();
                        ExecuteRequest();

                        // request is in progress now
                        return;
                    }
                    break;

                case ExecutionCompleted:
                    with_lock (ResponseLock) {
                        if (ResponseQueue) {
                            if (AtomicCas(&RequestState, SendingResponse, ExecutionCompleted)) {
                                SendResponse(ResponseQueue.front());
                                ResponseQueue.pop_front();

                                // request is in progress now
                                return;
                            }
                        } else {
                            if (AtomicCas(&RequestState, SendingCompletion, ExecutionCompleted)) {
                                Y_VERIFY(CompletionStatus);
                                SendCompletion(*CompletionStatus);

                                // request is in progress now
                                return;
                            }
                        }
                    }
                    break;

                case ExecutionCancelled:
                    if (AtomicCas(&RequestState, SendingCompletion, ExecutionCancelled)) {
                        // cancel inflight requests due to server shutting down
                        SendCompletion(grpc::Status(
                            grpc::StatusCode::UNAVAILABLE,
                            "Server shutting down"));

                        // request is in progress now
                        return;
                    }
                    break;

                case SendingResponse:
                    with_lock (ResponseLock) {
                        if (ResponseQueue) {
                            SendResponse(ResponseQueue.front());
                            ResponseQueue.pop_front();

                            // request is in progress now
                            return;
                        } else if (CompletionStatus) {
                            if (AtomicCas(&RequestState, SendingCompletion, SendingResponse)) {
                                SendCompletion(*CompletionStatus);

                                // request is in progress now
                                return;
                            }
                        } else {
                            if (AtomicCas(&RequestState, ExecutingRequest, SendingResponse)) {
                                // request is in progress now
                                return;
                            }
                        }
                    }
                    break;

                case SendingCompletion:
                    if (AtomicCas(&RequestState, RequestCompleted, SendingCompletion)) {
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
            AppCtx.Service,
            Context.get(),
            Request.get(),
            &Writer,
            ExecCtx.CompletionQueue.get(),
            ExecCtx.CompletionQueue.get(),
            AcquireCompletionTag());
    }

    void PrepareRequestContext()
    {
        CallContext = MakeIntrusive<TServerCallContext>(RequestId, GetFileSystemId(*Request));
        CallContext->RequestType = TMethod::RequestType;
        // TODO fill context
    }

    void ExecuteRequest()
    {
        AcquireCompletionTag();
        // TODO report stats

        try {
            TMethod::Execute(
                *AppCtx.ServiceImpl,
                CallContext,
                std::move(Request),
                std::make_shared<TResponseHandler>(this));

            // request is in progress now
            return;

        } catch (const TServiceError& e) {
            HandleCompletion(MakeError(e.GetCode(), TString(e.GetMessage())));
        } catch (...) {
            HandleCompletion(MakeError(E_FAIL, CurrentExceptionMessage()));
        }

        bool finalRelease = ReleaseCompletionTag();
        Y_VERIFY(!finalRelease);
    }

    void HandleResponse(const TResponse& response)
    {
        with_lock (ResponseLock) {
            Y_VERIFY(!CompletionStatus);
            ResponseQueue.push_back(response);
        }

        if (AtomicCas(&RequestState, ExecutionCompleted, ExecutingRequest)) {
            // will be processed on executor thread
            EnqueueCompletion(ExecCtx.CompletionQueue.get(), AcquireCompletionTag());
        }
    }

    void HandleCompletion(const NProto::TError& error)
    {
        with_lock (ResponseLock) {
            Y_VERIFY(!CompletionStatus);
            CompletionStatus = MakeGrpcStatus(error);
        }

        if (AtomicCas(&RequestState, ExecutionCompleted, ExecutingRequest)) {
            // will be processed on executor thread
            EnqueueCompletion(ExecCtx.CompletionQueue.get(), AcquireCompletionTag());
        }
    }

    void SendResponse(const TResponse& response)
    {
        auto& Log = AppCtx.Log;

        STORAGE_TRACE(TMethod::RequestName
            << " send response: " << DumpMessage(response));

        Writer.Write(response, AcquireCompletionTag());
    }

    void SendCompletion(const grpc::Status& status)
    {
        auto& Log = AppCtx.Log;

        STORAGE_TRACE(TMethod::RequestName
            << " send completion: " << FormatError(MakeGrpcError(status)));

        Writer.Finish(status, AcquireCompletionTag());
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename T>
using TFileStoreHandler = TRequestHandler<TFileStoreContext, T>;

template <typename T>
using TFileStoreStreamHandler = TStreamRequestHandler<TFileStoreContext, T>;

void StartRequests(TFileStoreContext& appCtx, TExecutorContext& execCtx)
{
#define FILESTORE_START_REQUEST(name, ...)                                     \
    TFileStoreHandler<T##name##Fs##Method>::Start(appCtx, execCtx);            \
// FILESTORE_START_REQUEST

    FILESTORE_SERVICE(FILESTORE_START_REQUEST)

#undef FILESTORE_START_REQUEST

    TFileStoreStreamHandler<TGetSessionEventsStreamMethod>::Start(appCtx, execCtx);
}

////////////////////////////////////////////////////////////////////////////////

template <typename T>
using TEndpointManagerHandler = TRequestHandler<TEndpointManagerContext, T>;

void StartRequests(TEndpointManagerContext& appCtx, TExecutorContext& execCtx)
{
#define FILESTORE_START_REQUEST(name, ...)                                     \
    TEndpointManagerHandler<T##name##Vhost##Method>::Start(appCtx, execCtx);   \
// FILESTORE_START_REQUEST

    FILESTORE_ENDPOINT_SERVICE(FILESTORE_START_REQUEST)

#undef FILESTORE_START_REQUEST
}

////////////////////////////////////////////////////////////////////////////////

class TExecutor final
    : public ISimpleThread
    , public TExecutorContext
{
private:
    TString Name;

public:
    TExecutor(
            TString name,
            std::unique_ptr<grpc::ServerCompletionQueue> completionQueue)
        : Name(std::move(name))
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

        auto tagName = TStringBuilder() << "NFS_THREAD_" << Name;
        NProfiling::TMemoryTagScope tagScope(tagName.c_str());

        void* tag;
        bool ok;
        while (WaitRequest(&tag, &ok)) {
            ProcessRequest(tag, ok);
        }

        return nullptr;
    }

    bool WaitRequest(void** tag, bool* ok)
    {
        return CompletionQueue->Next(tag, ok);
    }

    void ProcessRequest(void* tag, bool ok)
    {
        TRequestHandlerPtr handler(static_cast<TRequestHandlerBase*>(tag));
        handler->Process(ok);

        if (!handler->ReleaseCompletionTag()) {
            handler.release();   // ownership transferred to CompletionQueue
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename TAppContext>
class TServer final
    : public IServer
{
private:
    const TGrpcInitializer GrpcInitializer;
    const TServerConfigPtr Config;
    const ILoggingServicePtr Logging;

    TAppContext AppCtx;
    std::unique_ptr<grpc::Server> Server;
    TAdaptiveLock ExecutorsLock;
    TVector<std::unique_ptr<TExecutor>> Executors;

public:
    template <typename T>
    TServer(
            TServerConfigPtr config,
            ILoggingServicePtr logging,
            IServerStatsPtr serverStats,
            IProfileLogPtr profileLog,
            T service)
        : Config(std::move(config))
        , Logging(std::move(logging))
    {
        AppCtx.ServiceImpl = std::move(service);
        AppCtx.Stats = std::move(serverStats);
        AppCtx.ProfileLog = std::move(profileLog);
    }

    ~TServer() override
    {
        Stop();
    }

    void Start() override
    {
        auto& Log = AppCtx.Log;
        Log = Logging->CreateLog("NFS_SERVER");

        grpc::ServerBuilder builder;
        builder.RegisterService(&AppCtx.Service);

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

        if (auto port = Config->GetPort()) {
            auto address = Join(":", Config->GetHost(), port);
            builder.AddListeningPort(
                address,
                grpc::InsecureServerCredentials());

            STORAGE_INFO("Start listening on insecure " << address);
        }

        with_lock (ExecutorsLock) {
            ui32 threadsCount = Config->GetThreadsCount();
            for (size_t i = 1; i <= threadsCount; ++i) {
                auto executor = std::make_unique<TExecutor>(
                    TStringBuilder() << "SRV" << i,
                    builder.AddCompletionQueue());
                executor->Start();
                Executors.push_back(std::move(executor));
            }
        }

        Server = builder.BuildAndStart();
        if (!Server) {
            ythrow TServiceError(E_FAIL)
                << "could not start gRPC server";
        }

        with_lock (ExecutorsLock) {
            for (auto& executor: Executors) {
                for (ui32 i = 0; i < Config->GetPreparedRequestsCount(); ++i) {
                    StartRequests(AppCtx, *executor);
                }
            }
        }
    }

    void Stop() override
    {
        auto& Log = AppCtx.Log;

        if (AtomicSwap(&AppCtx.ShouldStop, 1) == 1) {
            return;
        }

        STORAGE_INFO("Shutting down");

        with_lock (ExecutorsLock) {
            for (auto& executor: Executors) {
                executor->RequestsInFlight.CancelAll();
            }
        }

        auto deadline = Config->GetShutdownTimeout().ToDeadLine();
        if (Server) {
            Server->Shutdown(deadline);
        }

        for (;;) {
            size_t requestsCount = 0;
            with_lock (ExecutorsLock) {
                for (auto& executor: Executors) {
                    requestsCount += executor->RequestsInFlight.GetCount();
                }
            }

            if (!requestsCount) {
                break;
            }

            if (deadline <= TInstant::Now()) {
                STORAGE_WARN("Some requests are still active on shutdown: "
                    << requestsCount);
                break;
            }

            Sleep(TDuration::MilliSeconds(100));
        }

        with_lock (ExecutorsLock) {
            for (auto& executor: Executors) {
                executor->Shutdown();
            }

            Executors.clear();
        }
    }

    TVector<TIncompleteRequest> GetIncompleteRequests() override
    {
        TGuard g{ExecutorsLock};
        TVector<TIncompleteRequest> requests;
        for (auto& executor: Executors) {
            auto executorRequests = executor->RequestsInFlight.GetIncompleteRequests();
            requests.insert(
                requests.end(),
                executorRequests.begin(),
                executorRequests.end());
        }
        return requests;
    }
};

using TFileStoreServer = TServer<TFileStoreContext>;
using TEndpointManagerServer = TServer<TEndpointManagerContext>;

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IServerPtr CreateServer(
    TServerConfigPtr config,
    ILoggingServicePtr logging,
    IServerStatsPtr serverStats,
    IProfileLogPtr profileLog,
    IFileStoreServicePtr service)
{
    return std::make_shared<TFileStoreServer>(
        std::move(config),
        std::move(logging),
        std::move(serverStats),
        std::move(profileLog),
        std::move(service));
}

////////////////////////////////////////////////////////////////////////////////

IServerPtr CreateServer(
    TServerConfigPtr config,
    ILoggingServicePtr logging,
    IServerStatsPtr serverStats,
    IProfileLogPtr profileLog,
    IEndpointManagerPtr service)
{
    return std::make_shared<TEndpointManagerServer>(
        std::move(config),
        std::move(logging),
        std::move(serverStats),
        std::move(profileLog),
        std::move(service));
}

}   // namespace NCloud::NFileStore::NServer
