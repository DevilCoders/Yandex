#include "tablet_proxy_actor.h"

#include <cloud/filestore/libs/storage/api/service.h>
#include <cloud/filestore/libs/storage/api/tablet.h>

#include <util/datetime/base.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr TDuration PipeInactivityTimeout = TDuration::Minutes(1);

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<NTabletPipe::IClientCache> CreateTabletPipeClientCache(
    const TStorageConfig& config)
{
    NTabletPipe::TClientConfig clientConfig;
    clientConfig.RetryPolicy = {
        .RetryLimitCount = config.GetPipeClientRetryCount(),
        .MinRetryTime = config.GetPipeClientMinRetryTime(),
        .MaxRetryTime = config.GetPipeClientMaxRetryTime()
    };

    return std::unique_ptr<NTabletPipe::IClientCache>(
        NTabletPipe::CreateUnboundedClientCache(clientConfig));
}

template <typename TArgs, ui32 EventId>
TString GetFileSystemId(const TRequestEvent<TArgs, EventId>& request)
{
    return request.FileSystemId;
}

template <typename TArgs, ui32 EventId>
TString GetFileSystemId(const TProtoRequestEvent<TArgs, EventId>& request)
{
    return request.Record.GetFileSystemId();
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TIndexTabletProxyActor::TIndexTabletProxyActor(TStorageConfigPtr config)
    : TActor(&TThis::StateWork)
    , Config(std::move(config))
    , ClientCache(CreateTabletPipeClientCache(*Config))
{
    ActivityType = TFileStoreActivities::TABLET_PROXY;
}

TIndexTabletProxyActor::TConnection& TIndexTabletProxyActor::CreateConnection(
    const TString& fileSystemId)
{
    if (TConnection* conn = Connections.FindPtr(fileSystemId)) {
        return *conn;
    }

    ui64 connectionId = ++ConnectionId;

    auto ins = Connections.emplace(fileSystemId, TConnection(connectionId, fileSystemId));
    Y_VERIFY(ins.second);

    ConnectionById[connectionId] = &ins.first->second;
    return ins.first->second;
}

void TIndexTabletProxyActor::StartConnection(
    const TActorContext& ctx,
    TConnection& conn,
    ui64 tabletId,
    const TString& path)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET_PROXY,
        "File store %s (path: %s) resolved: %lu",
        conn.FileSystemId.Quote().c_str(),
        path.Quote().c_str(),
        tabletId);

    ConnectionByTablet[tabletId] = &conn;

    conn.State = STARTED;
    conn.TabletId = tabletId;
    conn.Path = path;
    ++conn.Generation;

    ProcessPendingRequests(ctx, conn);
}

void TIndexTabletProxyActor::DestroyConnection(
    const TActorContext& ctx,
    TConnection& conn,
    const NProto::TError& error)
{
    conn.State = STOPPED;
    conn.Error = error;

    ProcessPendingRequests(ctx, conn);

    ConnectionById.erase(conn.Id);
    ConnectionByTablet.erase(conn.TabletId);
    Connections.erase(conn.FileSystemId);
}

void TIndexTabletProxyActor::OnConnectionError(
    const TActorContext& ctx,
    TConnection& conn,
    const NProto::TError& error)
{
    LOG_WARN(ctx, TFileStoreComponents::TABLET_PROXY,
        "Connection to file store %s (path: %s) failed: %s",
        conn.FileSystemId.Quote().c_str(),
        conn.Path.Quote().c_str(),
        FormatError(error).c_str());

    // will wait for tablet to recover
    conn.State = FAILED;

    CancelActiveRequests(ctx, conn);
    ProcessPendingRequests(ctx, conn);

}

void TIndexTabletProxyActor::ProcessPendingRequests(
    const TActorContext& ctx,
    TConnection& conn)
{
    auto requests = std::move(conn.Requests);

    for (auto& ev: requests) {
        TAutoPtr<IEventHandle> handle(ev.release());
        Receive(handle, ctx);
    }
}

void TIndexTabletProxyActor::CancelActiveRequests(
    const TActorContext& ctx,
    TConnection& conn)
{
    for (auto it = ActiveRequests.begin(); it != ActiveRequests.end(); ) {
        if (it->second.ConnectionId == conn.Id) {
            TAutoPtr<IEventHandle> handle(it->second.Request.release());
            Receive(handle, ctx);

            ActiveRequests.erase(it++);
        } else {
            ++it;
        }
    }

    conn.RequestsInflight = 0;
    conn.LastActivity = ctx.Now();
}

void TIndexTabletProxyActor::PostponeRequest(
    const TActorContext& ctx,
    TConnection& conn,
    IEventHandlePtr ev)
{
    Y_UNUSED(ctx);

    conn.Requests.emplace_back(std::move(ev));
}

template <typename TMethod>
void TIndexTabletProxyActor::ForwardRequest(
    const TActorContext& ctx,
    TConnection& conn,
    const typename TMethod::TRequest::TPtr& ev)
{
    auto clientId = ClientCache->Prepare(ctx, conn.TabletId);

    LOG_TRACE(ctx, TFileStoreComponents::TABLET_PROXY,
        "Forward request to file store: %lu (remote: %s)",
        conn.TabletId,
        ToString(clientId).c_str());

    ui64 requestId = ++RequestId;

    // TODO
    // SetRequestGeneration(conn.Generation, *ev->Get());

    auto event = std::make_unique<IEventHandle>(
        ev->Recipient,
        SelfId(),
        ev->ReleaseBase().Release(),
        0,          // flags
        requestId,  // cookie
        nullptr,    // forwardOnNondelivery
        ev->TraceId.Clone());

    event->Rewrite(TEvTabletPipe::EvSend, clientId);
    ctx.Send(event.release());

    ActiveRequests.emplace(
        requestId,
        TActiveRequest(conn.Id, IEventHandlePtr(ev.Release())));
    ++conn.RequestsInflight;
}

void TIndexTabletProxyActor::DescribeFileStore(
    const TActorContext& ctx,
    TConnection& conn)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET_PROXY,
        "Describe file store %s",
        conn.FileSystemId.Quote().c_str());

    NCloud::Send(
        ctx,
        MakeSSProxyServiceId(),
        std::make_unique<TEvSSProxy::TEvDescribeFileStoreRequest>(conn.FileSystemId),
        conn.Id);
}

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletProxyActor::ScheduleConnectionShutdown(
    const TActorContext& ctx,
    TConnection& conn)
{
    if (!conn.ActivityCheckScheduled &&
        conn.Requests.empty() &&
        !conn.RequestsInflight)
    {
        conn.ActivityCheckScheduled = true;
        ctx.Schedule(PipeInactivityTimeout, new TEvents::TEvWakeup(conn.Id));
    }
}

void TIndexTabletProxyActor::HandleWakeup(
    const TEvents::TEvWakeup::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    auto it = ConnectionById.find(msg->Tag);
    if (it == ConnectionById.end()) {
        // connection is already closed, nothing to do
        return;
    }

    TConnection* conn = it->second;
    conn->ActivityCheckScheduled = false;
    auto now = ctx.Now();

    if (conn->Requests.empty() &&
        !conn->RequestsInflight &&
        conn->LastActivity < now - PipeInactivityTimeout)
    {
        LOG_INFO(ctx, TFileStoreComponents::TABLET_PROXY,
            "Remove connection to tablet %lu for file system %s",
            conn->TabletId,
            conn->FileSystemId.Quote().c_str());
        ClientCache->Shutdown(ctx, conn->TabletId);
        ConnectionById.erase(conn->Id);
        ConnectionByTablet.erase(conn->TabletId);
        Connections.erase(conn->FileSystemId);
    } else {
        if (conn->LastActivity >= now - PipeInactivityTimeout) {
            auto timeEstimate = conn->LastActivity + PipeInactivityTimeout - now;
            ctx.Schedule(timeEstimate, new TEvents::TEvWakeup(conn->Id));
            conn->ActivityCheckScheduled = true;
        }
    }
}

void TIndexTabletProxyActor::HandleClientConnected(
    TEvTabletPipe::TEvClientConnected::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    auto it = ConnectionByTablet.find(msg->TabletId);
    if (it == ConnectionByTablet.end()) {
        return;
    }

    TConnection* conn = it->second;
    Y_VERIFY(conn);

    if (!ClientCache->OnConnect(ev)) {
        auto error = MakeKikimrError(msg->Status, "Could not connect");
        LOG_ERROR(ctx, TFileStoreComponents::TABLET_PROXY,
            "Cannot connect to tablet %lu: %s",
            msg->TabletId,
            FormatError(error).c_str());

        CancelActiveRequests(ctx, *conn);
        DestroyConnection(ctx, *conn, error);
        return;
    }

    if (conn->State == FAILED) {
        // Tablet recovered
        conn->State = STARTED;
        ++conn->Generation;
    }
}

void TIndexTabletProxyActor::HandleClientDestroyed(
    TEvTabletPipe::TEvClientDestroyed::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    auto it = ConnectionByTablet.find(msg->TabletId);
    if (it == ConnectionByTablet.end()) {
        return;
    }

    TConnection* conn = it->second;
    Y_VERIFY(conn);

    ClientCache->OnDisconnect(ev);

    auto error = MakeError(E_REJECTED, "connection broken");
    OnConnectionError(ctx, *conn, error);
}

void TIndexTabletProxyActor::HandleDescribeFileStoreResponse(
    const TEvSSProxy::TEvDescribeFileStoreResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    auto it = ConnectionById.find(ev->Cookie);
    if (it == ConnectionById.end()) {
        return;
    }

    TConnection* conn = it->second;
    Y_VERIFY(conn);

    const auto& error = msg->GetError();
    if (FAILED(error.GetCode())) {
        LOG_ERROR(ctx, TFileStoreComponents::TABLET_PROXY,
            "Could not resolve file store path %s: %s",
            msg->Path.Quote().c_str(),
            FormatError(error).c_str());

        DestroyConnection(ctx, *conn, error);
        return;
    }

    const auto& pathDescr = msg->PathDescription;
    const auto& fsDescr = pathDescr.GetFileStoreDescription();
    StartConnection(
        ctx,
        *conn,
        fsDescr.GetIndexTabletId(),
        msg->Path);
}

template <typename TMethod>
void TIndexTabletProxyActor::HandleRequest(
    const TActorContext& ctx,
    const typename TMethod::TRequest::TPtr& ev)
{
    // ActiveRequests contains IEventHandles without embedded messages,
    // only information about event type and sender. When we detect failure
    // in the pipe, we re-send all IEventHandle's from ActiveRequests
    // to ourself, enabling standard path for message processing.
    // So we need to handle the cases when message is re-sent or just came from
    // outside. We have to to check if message buffer is present
    // otherwise it is safe to use Get() to retrieve actual message.
    if (!ev->HasBuffer() && !ev->HasEvent()) {
        auto response = std::make_unique<typename TMethod::TResponse>(
            MakeError(E_REJECTED, "tablet is dead"));

        // FILESTORE_TRACE_SENT(ctx, &ev->TraceId, this, response);
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    const auto* msg = ev->Get();

    const TString& fileSystemId = GetFileSystemId(*msg);

    TConnection& conn = CreateConnection(fileSystemId);
    switch (conn.State) {
        case INITIAL:
        case FAILED:
            conn.State = RESOLVING;
            DescribeFileStore(ctx, conn);

            // pass-through

        case RESOLVING:
            PostponeRequest(ctx, conn, IEventHandlePtr(ev.Release()));
            break;

        case STARTED:
            ForwardRequest<TMethod>(ctx, conn, ev);
            break;

        case STOPPED: {
            auto response = std::make_unique<typename TMethod::TResponse>(
                conn.Error);

            // FILESTORE_TRACE_SENT(ctx, &ev->TraceId, this, response);
            NCloud::Reply(ctx, *ev, std::move(response));
            break;
        }
    }
}

void TIndexTabletProxyActor::HandleResponse(
    const TActorContext& ctx,
    TAutoPtr<IEventHandle>& ev)
{
    auto it = ActiveRequests.find(ev->Cookie);
    if (it == ActiveRequests.end()) {
        // ActiveRequests are cleared upon connection reset
        if (!LogLateMessage(ev, ctx)) {
            LogUnexpectedEvent(ctx, ev, TFileStoreComponents::TABLET_PROXY);
        }
        return;
    }

    // forward response to the caller
    TAutoPtr<IEventHandle> event;
    if (ev->HasEvent()) {
        event = new IEventHandle(
            it->second.Request->Sender,
            ev->Sender,
            ev->ReleaseBase().Release(),
            ev->Flags,
            it->second.Request->Cookie,
            nullptr,    // undeliveredRequestActor
            std::move(ev->TraceId));
    } else {
        event = new IEventHandle(
            ev->Type,
            ev->Flags,
            it->second.Request->Sender,
            ev->Sender,
            ev->ReleaseChainBuffer(),
            it->second.Request->Cookie,
            nullptr,    // undeliveredRequestActor
            std::move(ev->TraceId));
    }

    auto* conn = ConnectionById[it->second.ConnectionId];
    Y_VERIFY(conn);

    conn->LastActivity = ctx.Now();
    --conn->RequestsInflight;
    if (conn->Requests.empty() &&
        !conn->RequestsInflight) {
        ScheduleConnectionShutdown(ctx, *conn);
    }

    ctx.Send(event);
    ActiveRequests.erase(it);
}

void TIndexTabletProxyActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    for (const auto& conn: Connections) {
        ClientCache->Shutdown(ctx, conn.second.TabletId);
    }

    Die(ctx);
}

bool TIndexTabletProxyActor::HandleRequests(STFUNC_SIG)
{
#define FILESTORE_HANDLE_METHOD(name, ns)                                      \
    case ns::TEv##name##Request::EventType: {                                  \
        auto* x = reinterpret_cast<ns::TEv##name##Request::TPtr*>(&ev);        \
        HandleRequest<ns::T##name##Method>(ctx, *x);                           \
        break;                                                                 \
    }                                                                          \
    case ns::TEv##name##Response::EventType: {                                 \
        HandleResponse(ctx, ev);                                               \
        break;                                                                 \
    }                                                                          \
// FILESTORE_HANDLE_METHOD

    switch (ev->GetTypeRewrite()) {
        FILESTORE_SERVICE_REQUESTS_FWD(FILESTORE_HANDLE_METHOD, TEvService)
        FILESTORE_TABLET_REQUESTS(FILESTORE_HANDLE_METHOD, TEvIndexTablet)

        default:
            return false;
    }

    return true;

#undef FILESTORE_HANDLE_METHOD
}

bool TIndexTabletProxyActor::LogLateMessage(STFUNC_SIG)
{
#define FILESTORE_HANDLE_METHOD(name, ns)                                      \
    case ns::TEv##name##Request::EventType: {                                  \
        LOG_ERROR(ctx, TFileStoreComponents::TABLET_PROXY,                     \
            "Late request : (0x%08X) %s request",                              \
            ev->GetTypeRewrite(),                                              \
            #name);                                                            \
        break;                                                                 \
    }                                                                          \
    case ns::TEv##name##Response::EventType: {                                 \
        LOG_ERROR(ctx, TFileStoreComponents::TABLET_PROXY,                     \
            "Late response : (0x%08X) %s response",                            \
            ev->GetTypeRewrite(),                                              \
            #name);                                                            \
        break;                                                                 \
    }                                                                          \
// FILESTORE_HANDLE_METHOD

    switch (ev->GetTypeRewrite()) {
        FILESTORE_SERVICE_REQUESTS_FWD(FILESTORE_HANDLE_METHOD, TEvService)
        FILESTORE_TABLET_REQUESTS(FILESTORE_HANDLE_METHOD, TEvIndexTablet)

        default:
            return false;
    }

    return true;

#undef FILESTORE_HANDLE_METHOD
}

STFUNC(TIndexTabletProxyActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvWakeup, HandleWakeup);
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        HFunc(TEvTabletPipe::TEvClientConnected, HandleClientConnected);
        HFunc(TEvTabletPipe::TEvClientDestroyed, HandleClientDestroyed);

        HFunc(TEvSSProxy::TEvDescribeFileStoreResponse, HandleDescribeFileStoreResponse);

        default:
            if (!HandleRequests(ev, ctx)) {
                HandleUnexpectedEvent(ctx, ev, TFileStoreComponents::TABLET_PROXY);
            }
            break;
    }
}

}   // namespace NCloud::NFileStore::NStorage
