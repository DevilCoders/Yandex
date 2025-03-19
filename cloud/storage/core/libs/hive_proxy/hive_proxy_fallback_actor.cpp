#include "hive_proxy_fallback_actor.h"

#include "tablet_boot_info_cache.h"

#include <ydb/core/base/appdata.h>

#include <library/cpp/protobuf/util/pb_io.h>

namespace NCloud::NStorage {

using namespace NActors;

using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TRequestInfo
{
    NActors::TActorId Sender;
    ui64 Cookie = 0;

    TRequestInfo(NActors::TActorId sender, ui64 cookie)
        : Sender(sender)
        , Cookie(cookie)
    {}
};

////////////////////////////////////////////////////////////////////////////////

class TReadTabletBootInfoCacheActor final
    : public TActorBootstrapped<TReadTabletBootInfoCacheActor>
{
private:
    using TRequest =
        TEvHiveProxyPrivate::TEvReadTabletBootInfoCacheRequest;
    using TResponse =
        TEvHiveProxyPrivate::TEvReadTabletBootInfoCacheResponse;

    using TReply = std::function<
        void(const TActorContext&, std::unique_ptr<TResponse>)
    >;

    int LogComponent;
    TActorId TabletBootInfoCache;
    ui64 TabletId;
    TReply Reply;

public:
    TReadTabletBootInfoCacheActor(
            int logComponent,
            TActorId tabletBootInfoCache,
            ui64 tabletId,
            TReply reply)
        : LogComponent(logComponent)
        , TabletBootInfoCache(std::move(tabletBootInfoCache))
        , TabletId(tabletId)
        , Reply(std::move(reply))
    {
        ActivityType = TStorageActivities::HIVE_PROXY;
    }

    void Bootstrap(const TActorContext& ctx)
    {
        TThis::Become(&TThis::StateWork);
        Request(ctx);
    }

private:
    void Request(const TActorContext& ctx)
    {
        auto request = std::make_unique<TRequest>(TabletId);
        NCloud::Send(ctx, TabletBootInfoCache, std::move(request));
    }

    void HandleResponse(
        const TResponse::TPtr& ev,
        const TActorContext& ctx)
    {
        const auto* msg = ev->Get();

        std::unique_ptr<TResponse> response;
        if (HasError(msg->Error)) {
            auto error = std::move(msg->Error);
            if (error.GetCode() == E_NOT_FOUND) {
                // should not return fatal error to client
                error = MakeError(
                    E_REJECTED,
                    "E_NOT_FOUND from TabletBootInfoCache converted to E_REJECTED"
                );
            }

            response = std::make_unique<TResponse>(std::move(error));
        } else {
            response = std::make_unique<TResponse>(
                std::move(msg->StorageInfo), msg->SuggestedGeneration);
        }

        Reply(ctx, std::move(response));
        TThis::Die(ctx);
    }

private:
    STFUNC(StateWork)
    {
        switch (ev->GetTypeRewrite()) {
            HFunc(TResponse, HandleResponse);

            default:
                HandleUnexpectedEvent(ctx, ev, LogComponent);
                break;
        }
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

THiveProxyFallbackActor::THiveProxyFallbackActor(
        THiveProxyConfig config,
        IFileIOServicePtr fileIO)
    : Config(std::move(config))
    , FileIOService(std::move(fileIO))
{
    ActivityType = TStorageActivities::HIVE_PROXY;
}

void THiveProxyFallbackActor::Bootstrap(const TActorContext& ctx)
{
    TThis::Become(&TThis::StateWork);

    if (Config.TabletBootInfoCacheFilePath && FileIOService) {
        auto cache = std::make_unique<TTabletBootInfoCache>(
            Config.LogComponent,
            Config.TabletBootInfoCacheFilePath,
            FileIOService,
            false /* syncEnabled */
        );
        TabletBootInfoCache = ctx.Register(
            cache.release(), TMailboxType::HTSwap, AppData()->IOPoolId);
    }
}

////////////////////////////////////////////////////////////////////////////////

bool THiveProxyFallbackActor::HandleRequests(STFUNC_SIG)
{
    switch (ev->GetTypeRewrite()) {
        STORAGE_HIVE_PROXY_REQUESTS(STORAGE_HANDLE_REQUEST, TEvHiveProxy)

        default:
            return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(THiveProxyFallbackActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        default:
            if (!HandleRequests(ev, ctx)) {
                HandleUnexpectedEvent(ctx, ev, Config.LogComponent);
            }
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////

void THiveProxyFallbackActor::HandleLockTablet(
    const TEvHiveProxy::TEvLockTabletRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto response = std::make_unique<TEvHiveProxy::TEvLockTabletResponse>();
    NCloud::Reply(ctx, *ev, std::move(response));
}

void THiveProxyFallbackActor::HandleUnlockTablet(
    const TEvHiveProxy::TEvUnlockTabletRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto response = std::make_unique<TEvHiveProxy::TEvUnlockTabletResponse>();
    NCloud::Reply(ctx, *ev, std::move(response));
}

void THiveProxyFallbackActor::HandleGetStorageInfo(
    const TEvHiveProxy::TEvGetStorageInfoRequest::TPtr& ev,
    const TActorContext& ctx)
{
    using TResponse = TEvHiveProxy::TEvGetStorageInfoResponse;

    if (!TabletBootInfoCache) {
        // should not return fatal error to client
        auto error = MakeError(E_REJECTED, "TabletBootInfoCache is not set");
        auto response = std::make_unique<TResponse>(error);
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    const auto* msg = ev->Get();

    auto requestInfo = TRequestInfo(ev->Sender, ev->Cookie);
    auto reply = [=](const auto& ctx, auto r) {
        if (HasError(r->Error)) {
            NCloud::ReplyNoTrace(
                ctx,
                requestInfo,
                std::make_unique<TResponse>(r->Error)
            );
            return;
        }

        auto response = std::make_unique<TResponse>(std::move(r->StorageInfo));
        NCloud::ReplyNoTrace(ctx, requestInfo, std::move(response));
    };

    NCloud::Register<TReadTabletBootInfoCacheActor>(
        ctx,
        Config.LogComponent,
        TabletBootInfoCache,
        msg->TabletId,
        std::move(reply));
}

void THiveProxyFallbackActor::HandleBootExternal(
    const TEvHiveProxy::TEvBootExternalRequest::TPtr& ev,
    const TActorContext& ctx)
{
    using TResponse = TEvHiveProxy::TEvBootExternalResponse;

    if (!TabletBootInfoCache) {
        // should not return fatal error to client
        auto error = MakeError(E_REJECTED, "TabletBootInfoCache is not set");
        auto response = std::make_unique<TResponse>(error);
        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    const auto* msg = ev->Get();

    auto requestInfo = TRequestInfo(ev->Sender, ev->Cookie);
    auto reply = [=](const auto& ctx, auto r) {
        if (HasError(r->Error)) {
            NCloud::ReplyNoTrace(
                ctx,
                requestInfo,
                std::make_unique<TResponse>(r->Error)
            );
            return;
        }


        auto response = std::make_unique<TResponse>(
            std::move(r->StorageInfo),
            r->SuggestedGeneration,
            TEvHiveProxy::TEvBootExternalResponse::EBootMode::MASTER,
            0  // SlaveId
        );
        NCloud::ReplyNoTrace(ctx, requestInfo, std::move(response));
    };

    NCloud::Register<TReadTabletBootInfoCacheActor>(
        ctx,
        Config.LogComponent,
        TabletBootInfoCache,
        msg->TabletId,
        std::move(reply));
}

void THiveProxyFallbackActor::HandleReassignTablet(
    const TEvHiveProxy::TEvReassignTabletRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto error = MakeError(E_NOT_IMPLEMENTED);
    auto response =
        std::make_unique<TEvHiveProxy::TEvReassignTabletResponse>(error);
    NCloud::Reply(ctx, *ev, std::move(response));
}

void THiveProxyFallbackActor::HandleCreateTablet(
    const TEvHiveProxy::TEvCreateTabletRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto error = MakeError(E_NOT_IMPLEMENTED);
    auto response =
        std::make_unique<TEvHiveProxy::TEvCreateTabletResponse>(error);
    NCloud::Reply(ctx, *ev, std::move(response));
}

void THiveProxyFallbackActor::HandleLookupTablet(
    const TEvHiveProxy::TEvLookupTabletRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto error = MakeError(E_NOT_IMPLEMENTED);
    auto response =
        std::make_unique<TEvHiveProxy::TEvLookupTabletResponse>(error);
    NCloud::Reply(ctx, *ev, std::move(response));
}

void THiveProxyFallbackActor::HandleDrainNode(
    const TEvHiveProxy::TEvDrainNodeRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto error = MakeError(E_NOT_IMPLEMENTED);
    auto response =
        std::make_unique<TEvHiveProxy::TEvDrainNodeResponse>(error);
    NCloud::Reply(ctx, *ev, std::move(response));
}

void THiveProxyFallbackActor::HandleSyncTabletBootInfoCache(
    const TEvHiveProxy::TEvSyncTabletBootInfoCacheRequest::TPtr& ev,
    const TActorContext& ctx)
{
    if (TabletBootInfoCache) {
        ctx.Send(ev->Forward(TabletBootInfoCache));
    } else {
        auto response =
            std::make_unique<TEvHiveProxy::TEvSyncTabletBootInfoCacheResponse>(
                MakeError(S_FALSE));
        NCloud::Reply(ctx, *ev, std::move(response));
    }
}

}   // namespace NCloud::NStorage
