#include "client.h"

#include "config.h"
#include "probes.h"

#include <cloud/filestore/public/api/grpc/service.grpc.pb.h>

#include <cloud/filestore/libs/service/context.h>
#include <cloud/filestore/libs/service/endpoint.h>
#include <cloud/filestore/libs/service/filestore.h>
#include <cloud/filestore/libs/service/request.h>

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/thread.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/grpc/completion.h>
#include <cloud/storage/core/libs/grpc/time.h>

#include <library/cpp/actors/prof/tag.h>

#include <contrib/libs/grpc/include/grpcpp/channel.h>
#include <contrib/libs/grpc/include/grpcpp/client_context.h>
#include <contrib/libs/grpc/include/grpcpp/completion_queue.h>
#include <contrib/libs/grpc/include/grpcpp/create_channel.h>
#include <contrib/libs/grpc/include/grpcpp/security/credentials.h>
#include <contrib/libs/grpc/include/grpcpp/support/status.h>

#include <util/generic/hash_set.h>
#include <util/random/random.h>
#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/system/spinlock.h>
#include <util/system/thread.h>

namespace NCloud::NFileStore::NClient {

using namespace NThreading;

LWTRACE_USING(FILESTORE_CLIENT_PROVIDER);

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

////////////////////////////////////////////////////////////////////////////////

#define FILESTORE_DECLARE_METHOD(name, proto, method, ...)                     \
    struct T##name##Method                                                     \
    {                                                                          \
        static constexpr auto RequestName = TStringBuf(#method);              \
                                                                               \
        using TRequest = NProto::T##proto##Request;                            \
        using TResponse = NProto::T##proto##Response;                          \
                                                                               \
        template <typename T, typename ...TArgs>                               \
        static auto Execute(T& service, TArgs&& ...args)                       \
        {                                                                      \
            return service.Async##method(std::forward<TArgs>(args)...);        \
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

public:
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

struct TAppContext
{
    const TClientConfigPtr Config;

    TLog Log;
    TAtomic ShouldStop = 0;

    TAppContext(TClientConfigPtr config)
        : Config(std::move(config))
    {}
};

struct TFileStoreContext : TAppContext
{
    std::shared_ptr<NProto::TFileStoreService::Stub> Service;

    TFileStoreContext(TClientConfigPtr config)
        : TAppContext(std::move(config))
    {}
};

struct TEndpointManagerContext : TAppContext
{
    std::shared_ptr<NProto::TEndpointManagerService::Stub> Service;

    TEndpointManagerContext(TClientConfigPtr config)
        : TAppContext(std::move(config))
    {}
};

struct TExecutorContext
{
    std::unique_ptr<grpc::CompletionQueue> CompletionQueue;
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
    TExecutorContext& ExecutorCtx;

    grpc::ClientContext Context;
    std::unique_ptr<grpc::ClientAsyncResponseReader<TResponse>> Reader;

    TCallContextPtr CallContext;
    std::shared_ptr<TRequest> Request;
    ui64 RequestId = 0;
    TPromise<TResponse> Promise;

    TResponse Response;
    grpc::Status Status;

    enum {
        WaitingForRequest = 0,
        SendingRequest = 1,
        RequestCompleted = 2,
    };
    TAtomic RequestState = WaitingForRequest;

public:
    TRequestHandler(
            TAppContext& appCtx,
            TExecutorContext& executorCtx,
            TCallContextPtr callContext,
            std::shared_ptr<TRequest> request,
            const TPromise<TResponse>& promise)
        : AppCtx(appCtx)
        , ExecutorCtx(executorCtx)
        , CallContext(std::move(callContext))
        , Request(std::move(request))
        , Promise(promise)
    {}

    static void Start(
        TAppContext& appCtx,
        TExecutorContext& executorCtx,
        TCallContextPtr callContext,
        std::shared_ptr<TRequest> request,
        TPromise<TResponse>& promise)
    {
        auto handler = std::make_unique<TRequestHandler<TAppContext, TMethod>>(
            appCtx,
            executorCtx,
            std::move(callContext),
            std::move(request),
            promise);

        executorCtx.RequestsInFlight.Register(handler.get());

        if (EnqueueCompletion(executorCtx.CompletionQueue.get(), handler->AcquireCompletionTag())) {
            handler.release();   // ownership transferred to CompletionQueue
            return;
        }

        executorCtx.RequestsInFlight.Unregister(handler.get());

        handler->Status = grpc::Status::CANCELLED;
        handler->HandleResponse();
    }

    void Process(bool ok) override
    {
        Y_UNUSED(ok);

        if (AtomicGet(AppCtx.ShouldStop)) {
            AtomicSet(RequestState, RequestCompleted);

            Response.Clear();
            Status = grpc::Status::CANCELLED;
        }

        for (;;) {
            switch (AtomicGet(RequestState)) {
                case WaitingForRequest:
                    if (AtomicCas(&RequestState, SendingRequest, WaitingForRequest)) {
                        PrepareRequestContext();
                        SendRequest();

                        // request is in progress now
                        return;
                    }
                    break;

                case SendingRequest:
                    if (AtomicCas(&RequestState, RequestCompleted, SendingRequest)) {
                    }
                    break;

                case RequestCompleted:
                    HandleResponse();
                    CompleteRequest();

                    // request completed and could be safely destroyed
                    ExecutorCtx.RequestsInFlight.Unregister(this);
                    return;
            }
        }
    }

    void Cancel() override
    {
        Context.TryCancel();
    }

private:
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

        auto requestTimeout = TDuration::MilliSeconds(headers.GetRequestTimeout());
        if (!requestTimeout) {
            requestTimeout = AppCtx.Config->GetRequestTimeout();
            headers.SetRequestTimeout(requestTimeout.MilliSeconds());
        }

        RequestId = CallContext->RequestId;
        if (!RequestId) {
            RequestId = CreateRequestId();
            headers.SetRequestId(RequestId);
        }

        Context.set_deadline(now + requestTimeout);
    }

    void SendRequest()
    {
        auto& Log = AppCtx.Log;

        STORAGE_TRACE(TMethod::RequestName
            << " #" << RequestId
            << " send request: " << DumpMessage(*Request));

        FILESTORE_TRACK(
            SendRequest,
            CallContext,
            TString(TMethod::RequestName));

        Reader = TMethod::Execute(
            *AppCtx.Service,
            &Context,
            *Request,
            ExecutorCtx.CompletionQueue.get());

        // no more need Request; try to free memory
        Request.reset();

        Reader->Finish(&Response, &Status, AcquireCompletionTag());
    }

    void HandleResponse()
    {
        auto& Log = AppCtx.Log;

        if (!Status.ok()) {
            *Response.MutableError() = MakeGrpcError(Status);
        }

        STORAGE_TRACE(TMethod::RequestName
            << " #" << RequestId
            << " response received: " << DumpMessage(Response));

        FILESTORE_TRACK(
            ResponseReceived,
            CallContext,
            TString(TMethod::RequestName));

        try {
            Promise.SetValue(std::move(Response));
        } catch (...) {
            STORAGE_ERROR(TMethod::RequestName
                << " #" << RequestId
                << " exception in callback: "
                << CurrentExceptionMessage());
        }
    }

    void CompleteRequest()
    {
        auto& Log = AppCtx.Log;

        STORAGE_TRACE(TMethod::RequestName
            << " #" << RequestId
            << " request completed");

        FILESTORE_TRACK(
            RequestCompleted,
            CallContext,
            TString(TMethod::RequestName));
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename TAppContext, typename TMethod>
class TStreamRequestHandler final
    : public TRequestHandlerBase
{
    using TRequest = typename TMethod::TRequest;
    using TResponse = typename TMethod::TResponse;

private:
    TAppContext& AppCtx;
    TExecutorContext& ExecutorCtx;

    grpc::ClientContext Context;
    std::unique_ptr<grpc::ClientAsyncReader<TResponse>> Reader;

    TCallContextPtr CallContext;
    std::shared_ptr<TRequest> Request;
    IResponseHandlerPtr<TResponse> ResponseHandler;

    TResponse Response;
    grpc::Status Status;

    enum {
        WaitingForRequest = 0,
        SendingRequest = 1,
        WaitingForResponse = 2,
        WaitingForCompletion = 3,
        RequestCompleted = 4,
    };
    TAtomic RequestState = WaitingForRequest;

public:
    TStreamRequestHandler(
            TAppContext& appCtx,
            TExecutorContext& executorCtx,
            TCallContextPtr callContext,
            std::shared_ptr<TRequest> request,
            IResponseHandlerPtr<TResponse> responseHandler)
        : AppCtx(appCtx)
        , ExecutorCtx(executorCtx)
        , CallContext(std::move(callContext))
        , Request(std::move(request))
        , ResponseHandler(std::move(responseHandler))
    {}

    static void Start(
        TAppContext& appCtx,
        TExecutorContext& executorCtx,
        TCallContextPtr callContext,
        std::shared_ptr<TRequest> request,
        IResponseHandlerPtr<TResponse> responseHandler)
    {
        auto handler = std::make_unique<TStreamRequestHandler<TAppContext, TMethod>>(
            appCtx,
            executorCtx,
            std::move(callContext),
            std::move(request),
            std::move(responseHandler));

        executorCtx.RequestsInFlight.Register(handler.get());

        if (EnqueueCompletion(executorCtx.CompletionQueue.get(), handler->AcquireCompletionTag())) {
            handler.release();   // ownership transferred to CompletionQueue
            return;
        }

        executorCtx.RequestsInFlight.Unregister(handler.get());

        handler->Status = grpc::Status::CANCELLED;
        handler->HandleCompletion();
    }

    void Process(bool ok) override
    {
        if (AtomicGet(AppCtx.ShouldStop)) {
            AtomicSet(RequestState, RequestCompleted);

            Response.Clear();
            Status = grpc::Status::CANCELLED;
        }

        for (;;) {
            switch (AtomicGet(RequestState)) {
                case WaitingForRequest:
                    if (AtomicCas(&RequestState, SendingRequest, WaitingForRequest)) {
                        SendRequest();

                        // request is in progress now
                        return;
                    }
                    break;

                case SendingRequest:
                    if (AtomicCas(&RequestState, WaitingForResponse, SendingRequest)) {
                        ReadResponse();

                        // request is in progress now
                        return;
                    }
                    break;

                case WaitingForResponse:
                    if (ok) {
                        HandleResponse();
                        ReadResponse();

                        // request is in progress now
                        return;
                    }
                    if (AtomicCas(&RequestState, WaitingForCompletion, WaitingForResponse)) {
                        ReadCompletion();

                        // request is in progress now
                        return;
                    }
                    break;

                case WaitingForCompletion:
                    if (AtomicCas(&RequestState, RequestCompleted, WaitingForCompletion)) {
                    }
                    break;

                case RequestCompleted:
                    HandleCompletion();

                    // request completed and could be safely destroyed
                    ExecutorCtx.RequestsInFlight.Unregister(this);
                    return;
            }
        }
    }

    void Cancel() override
    {
        Context.TryCancel();
    }

private:
    void SendRequest()
    {
        auto& Log = AppCtx.Log;

        STORAGE_TRACE(TMethod::RequestName
            << " send request: " << DumpMessage(*Request));

        Reader = TMethod::Execute(
            *AppCtx.Service,
            &Context,
            *Request,
            ExecutorCtx.CompletionQueue.get(),
            AcquireCompletionTag());

        // no more need Request; try to free memory
        Request.reset();
    }

    void ReadResponse()
    {
        auto& Log = AppCtx.Log;

        STORAGE_TRACE(TMethod::RequestName << " read response");

        Reader->Read(&Response, AcquireCompletionTag());
    }

    void ReadCompletion()
    {
        auto& Log = AppCtx.Log;

        STORAGE_TRACE(TMethod::RequestName << " read completion");

        Reader->Finish(&Status, AcquireCompletionTag());
    }

    void HandleResponse()
    {
        auto& Log = AppCtx.Log;

        STORAGE_TRACE(TMethod::RequestName
            << " response received: " << DumpMessage(Response));

        try {
            ResponseHandler->HandleResponse(Response);
        } catch (...) {
            STORAGE_ERROR(TMethod::RequestName
                << " exception in callback: " << CurrentExceptionMessage());
        }
    }

    void HandleCompletion()
    {
        auto& Log = AppCtx.Log;

        auto error = MakeGrpcError(Status);
        STORAGE_TRACE(TMethod::RequestName
            << " completion received: " << FormatError(error));

        try {
            ResponseHandler->HandleCompletion(error);
        } catch (...) {
            STORAGE_ERROR(TMethod::RequestName
                << " exception in callback: " << CurrentExceptionMessage());
        }
    }
};

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
            std::unique_ptr<grpc::CompletionQueue> completionQueue)
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

template <typename TAppContext, typename TClient>
class TClientBase
    : public TClient
{
protected:
    const ILoggingServicePtr Logging;

    TAppContext AppCtx;
    TVector<std::unique_ptr<TExecutor>> Executors;

public:
    TClientBase(TClientConfigPtr config, ILoggingServicePtr logging)
        : Logging(std::move(logging))
        , AppCtx(std::move(config))
    {}

    ~TClientBase()
    {
        Stop();
    }

    void Start() override
    {
        AppCtx.Log = Logging->CreateLog("NFS_CLIENT");

        StartClient();
    }

    void Stop() override
    {
        auto& Log = AppCtx.Log;

        if (AtomicSwap(&AppCtx.ShouldStop, 1) == 1) {
            return;
        }

        STORAGE_INFO("Shutting down");

        for (auto& executor: Executors) {
            executor->RequestsInFlight.CancelAll();
        }

        for (auto& executor: Executors) {
            executor->Shutdown();
        }

        AppCtx.Service.reset();
    }

    template <typename TMethod>
    TFuture<typename TMethod::TResponse> StartRequest(
        TCallContextPtr callContext,
        std::shared_ptr<typename TMethod::TRequest> request)
    {
        auto promise = NewPromise<typename TMethod::TResponse>();

        TRequestHandler<TAppContext, TMethod>::Start(
            AppCtx,
            PickExecutor(),
            std::move(callContext),
            std::move(request),
            promise);

        return promise.GetFuture();
    }

    template <typename TMethod>
    void StartStreamRequest(
        TCallContextPtr callContext,
        std::shared_ptr<typename TMethod::TRequest> request,
        IResponseHandlerPtr<typename TMethod::TResponse> responseHandler)
    {
        TStreamRequestHandler<TAppContext, TMethod>::Start(
            AppCtx,
            PickExecutor(),
            std::move(callContext),
            std::move(request),
            std::move(responseHandler));
    }

protected:
    virtual void InitService(std::shared_ptr<::grpc::Channel> channel) = 0;

    void StartClient()
    {
        auto& Log = AppCtx.Log;

        auto address = ::Join(":", AppCtx.Config->GetHost(), AppCtx.Config->GetPort());
        STORAGE_INFO("Connect to " << address);

        auto channel = CreateCustomChannel(
            std::move(address),
            grpc::InsecureChannelCredentials(),
            grpc::ChannelArguments());

        InitService(std::move(channel));

        ui32 threadsCount = AppCtx.Config->GetThreadsCount();
        for (size_t i = 1; i <= threadsCount; ++i) {
            auto executor = std::make_unique<TExecutor>(
                TStringBuilder() << "CLI" << i,
                std::make_unique<grpc::CompletionQueue>());

            executor->Start();
            Executors.push_back(std::move(executor));
        }
    }

    TExecutor& PickExecutor()
    {
        size_t index = 0;
        if (Executors.size() > 1) {
            // pick random executor
            index = RandomNumber(Executors.size());
        }
        return *Executors[index];
    }
};

////////////////////////////////////////////////////////////////////////////////

class TFileStoreClient final
    : public TClientBase<TFileStoreContext, IFileStoreService>
{
public:
    using TClientBase::TClientBase;

    void InitService(std::shared_ptr<::grpc::Channel> channel) override
    {
        AppCtx.Service = NProto::TFileStoreService::NewStub(std::move(channel));
    }

#define FILESTORE_IMPLEMENT_METHOD(name, ...)                                  \
    TFuture<NProto::T##name##Response> name(                                   \
        TCallContextPtr callContext,                                           \
        std::shared_ptr<NProto::T##name##Request> request) override            \
    {                                                                          \
        return StartRequest<T##name##Fs##Method>(                              \
            std::move(callContext),                                            \
            std::move(request));                                               \
    }                                                                          \
// FILESTORE_IMPLEMENT_METHOD

    FILESTORE_SERVICE(FILESTORE_IMPLEMENT_METHOD)

#undef FILESTORE_IMPLEMENT_METHOD

    void GetSessionEventsStream(
        TCallContextPtr callContext,
        std::shared_ptr<NProto::TGetSessionEventsRequest> request,
        IResponseHandlerPtr<NProto::TGetSessionEventsResponse> responseHandler) override
    {
        StartStreamRequest<TGetSessionEventsStreamMethod>(
            std::move(callContext),
            std::move(request),
            std::move(responseHandler));
    }
};

////////////////////////////////////////////////////////////////////////////////

class TEndpointManagerClient final
    : public TClientBase<TEndpointManagerContext, IEndpointManager>
{
public:
    using TClientBase::TClientBase;

    TVector<TIncompleteRequest> GetIncompleteRequests() override
    {
        return {};
    }

    void InitService(std::shared_ptr<::grpc::Channel> channel) override
    {
        AppCtx.Service = NProto::TEndpointManagerService::NewStub(std::move(channel));
    }

#define FILESTORE_IMPLEMENT_METHOD(name, ...)                                  \
    TFuture<NProto::T##name##Response> name(                                   \
        TCallContextPtr callContext,                                           \
        std::shared_ptr<NProto::T##name##Request> request) override            \
    {                                                                          \
        return StartRequest<T##name##Vhost##Method>(                           \
            std::move(callContext),                                            \
            std::move(request));                                               \
    }                                                                          \
// FILESTORE_IMPLEMENT_METHOD

    FILESTORE_ENDPOINT_SERVICE(FILESTORE_IMPLEMENT_METHOD)

#undef FILESTORE_IMPLEMENT_METHOD
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IFileStoreServicePtr CreateFileStoreClient(
    TClientConfigPtr config,
    ILoggingServicePtr logging)
{
    return std::make_shared<TFileStoreClient>(
        std::move(config),
        std::move(logging));
}

IEndpointManagerPtr CreateEndpointManagerClient(
    TClientConfigPtr config,
    ILoggingServicePtr logging)
{
    return std::make_shared<TEndpointManagerClient>(
        std::move(config),
        std::move(logging));
}

}   // namespace NCloud::NFileStore::NClient
