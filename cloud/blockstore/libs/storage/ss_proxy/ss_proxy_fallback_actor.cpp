#include "ss_proxy_fallback_actor.h"

#include "path_description_cache.h"

#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/volume_label.h>
#include <cloud/blockstore/libs/storage/ss_proxy/ss_proxy_events_private.h>

#include <ydb/core/base/appdata.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

template <class TResponse>
class TReadPathDescriptionCacheActor final
    : public TActorBootstrapped<TReadPathDescriptionCacheActor<TResponse>>
{
private:
    using TSelf = TReadPathDescriptionCacheActor<TResponse>;
    using TReadCacheRequest =
        TEvSSProxyPrivate::TEvReadPathDescriptionCacheRequest;
    using TReadCacheResponse =
        TEvSSProxyPrivate::TEvReadPathDescriptionCacheResponse;

    const TRequestInfoPtr RequestInfo;
    const TActorId PathDescriptionCache;
    const TVector<TString> Paths;
    size_t PathIndex = 0;

public:
    TReadPathDescriptionCacheActor(
        TRequestInfoPtr requestInfo,
        TActorId pathDescriptionCache,
        TVector<TString> paths);

    void Bootstrap(const TActorContext& ctx);

private:
    void ReadCache(const TActorContext& ctx);
    void HandleReadCacheResponse(
        const TReadCacheResponse::TPtr& ev,
        const TActorContext& ctx);

    void ReplyAndDie(
        const TActorContext& ctx,
        std::unique_ptr<TResponse> response);

private:
    STFUNC(StateWork);
};

template <class TResponse>
TReadPathDescriptionCacheActor<TResponse>::TReadPathDescriptionCacheActor(
        TRequestInfoPtr requestInfo,
        TActorId pathDescriptionCache,
        TVector<TString> paths)
    : RequestInfo(std::move(requestInfo))
    , PathDescriptionCache(std::move(pathDescriptionCache))
    , Paths(std::move(paths))
{
    TSelf::ActivityType = TBlockStoreActivities::SS_PROXY;
}

template <class TResponse>
void TReadPathDescriptionCacheActor<TResponse>::Bootstrap(
    const TActorContext& ctx)
{
    TSelf::Become(&TSelf::StateWork);
    ReadCache(ctx);
}

template <class TResponse>
void TReadPathDescriptionCacheActor<TResponse>::ReadCache(
    const TActorContext& ctx)
{
    auto& path = Paths[PathIndex];
    auto request = std::make_unique<TReadCacheRequest>(std::move(path));
    NCloud::Send(ctx, PathDescriptionCache, std::move(request));

    ++PathIndex;
}

template <class TResponse>
void TReadPathDescriptionCacheActor<TResponse>::HandleReadCacheResponse(
    const TReadCacheResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    std::unique_ptr<TResponse> response;
    if (HasError(msg->Error)) {
        if (msg->Error.GetCode() == E_NOT_FOUND && PathIndex < Paths.size()) {
            ReadCache(ctx);
            return;
        }

        auto error = std::move(msg->Error);
        if (error.GetCode() == E_NOT_FOUND) {
            // should not return fatal error to client
            error = MakeError(
                E_REJECTED,
                "E_NOT_FOUND from PathDescriptionCache converted to E_REJECTED"
            );
        }

        response = std::make_unique<TResponse>(std::move(error));
    } else {
        response = std::make_unique<TResponse>(
            std::move(msg->Path), std::move(msg->PathDescription));
    }

    ReplyAndDie(ctx, std::move(response));
}

template <class TResponse>
void TReadPathDescriptionCacheActor<TResponse>::ReplyAndDie(
    const TActorContext& ctx,
    std::unique_ptr<TResponse> response)
{
    NCloud::Reply(ctx, *RequestInfo, std::move(response));
    TSelf::Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

template <class TResponse>
STFUNC(TReadPathDescriptionCacheActor<TResponse>::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TReadCacheResponse, HandleReadCacheResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SS_PROXY);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TSSProxyFallbackActor::TSSProxyFallbackActor(
        TStorageConfigPtr config,
        IFileIOServicePtr fileIO)
    : Config(std::move(config))
    , FileIOService(std::move(fileIO))
{
    ActivityType = TBlockStoreActivities::SS_PROXY;
}

void TSSProxyFallbackActor::Bootstrap(const TActorContext& ctx)
{
    TThis::Become(&TThis::StateWork);

    const auto& filepath = Config->GetPathDescriptionCacheFilePath();
    if (filepath && FileIOService) {
        auto cache = std::make_unique<TPathDescriptionCache>(
            filepath, FileIOService, false /* syncEnabled */);
        PathDescriptionCache = ctx.Register(
            cache.release(), TMailboxType::HTSwap, AppData()->IOPoolId);
    }
}

////////////////////////////////////////////////////////////////////////////////

bool TSSProxyFallbackActor::HandleRequests(STFUNC_SIG)
{
    switch (ev->GetTypeRewrite()) {
        BLOCKSTORE_SS_PROXY_REQUESTS(BLOCKSTORE_HANDLE_REQUEST, TEvSSProxy)

        default:
            return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TSSProxyFallbackActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        default:
            if (!HandleRequests(ev, ctx)) {
                HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SS_PROXY);
            }
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////

void TSSProxyFallbackActor::HandleCreateVolume(
    const TEvSSProxy::TEvCreateVolumeRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto error = MakeError(E_NOT_IMPLEMENTED);
    auto response =
        std::make_unique<TEvSSProxy::TEvCreateVolumeResponse>(error);
    NCloud::ReplyNoTrace(ctx, *ev, std::move(response));
}

void TSSProxyFallbackActor::HandleDescribeScheme(
    const TEvSSProxy::TEvDescribeSchemeRequest::TPtr& ev,
    const TActorContext& ctx)
{
    using TResponse = TEvSSProxy::TEvDescribeSchemeResponse;

    if (!PathDescriptionCache) {
        // should not return fatal error to client
        auto error = MakeError(E_REJECTED, "PathDescriptionCache is not set");
        auto response = std::make_unique<TResponse>(error);
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    const auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    NCloud::Register<TReadPathDescriptionCacheActor<TResponse>>(
        ctx,
        std::move(requestInfo),
        PathDescriptionCache,
        TVector<TString>{std::move(msg->Path)});
}

void TSSProxyFallbackActor::HandleDescribeVolume(
    const TEvSSProxy::TEvDescribeVolumeRequest::TPtr& ev,
    const TActorContext& ctx)
{
    using TResponse = TEvSSProxy::TEvDescribeVolumeResponse;

    if (!PathDescriptionCache) {
        // should not return fatal error to client
        auto error = MakeError(E_REJECTED, "PathDescriptionCache is not set");
        auto response = std::make_unique<TResponse>(error);
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    const auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    TString dir = TStringBuilder() << Config->GetSchemeShardDir() << '/';
    TString path = TStringBuilder() << dir << DiskIdToPath(msg->DiskId);
    // path for volumes with old layout
    TString fallbackPath =
        TStringBuilder() << dir << DiskIdToPathDeprecated(msg->DiskId);

    NCloud::Register<TReadPathDescriptionCacheActor<TResponse>>(
        ctx,
        std::move(requestInfo),
        PathDescriptionCache,
        TVector<TString>{std::move(path), std::move(fallbackPath)});
}

void TSSProxyFallbackActor::HandleModifyScheme(
    const TEvSSProxy::TEvModifySchemeRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto error = MakeError(E_NOT_IMPLEMENTED);
    auto response =
        std::make_unique<TEvSSProxy::TEvModifySchemeResponse>(error);
    NCloud::ReplyNoTrace(ctx, *ev, std::move(response));
}

void TSSProxyFallbackActor::HandleModifyVolume(
    const TEvSSProxy::TEvModifyVolumeRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto error = MakeError(E_NOT_IMPLEMENTED);
    auto response =
        std::make_unique<TEvSSProxy::TEvModifyVolumeResponse>(error);
    NCloud::ReplyNoTrace(ctx, *ev, std::move(response));
}

void TSSProxyFallbackActor::HandleWaitSchemeTx(
    const TEvSSProxy::TEvWaitSchemeTxRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto error = MakeError(E_NOT_IMPLEMENTED);
    auto response =
        std::make_unique<TEvSSProxy::TEvWaitSchemeTxResponse>(error);
    NCloud::ReplyNoTrace(ctx, *ev, std::move(response));
}

void TSSProxyFallbackActor::HandleSyncPathDescriptionCache(
    const TEvSSProxy::TEvSyncPathDescriptionCacheRequest::TPtr& ev,
    const TActorContext& ctx)
{
    using TResponse = TEvSSProxy::TEvSyncPathDescriptionCacheResponse;

    auto error = MakeError(E_NOT_IMPLEMENTED);
    auto response = std::make_unique<TResponse>(std::move(error));
    NCloud::ReplyNoTrace(ctx, *ev, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage
