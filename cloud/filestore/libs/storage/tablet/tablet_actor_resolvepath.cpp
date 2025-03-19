#include "tablet_actor.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

NProto::TError ValidateRequest(const NProto::TResolvePathRequest& request)
{
    const auto& path = request.GetPath();
    if (path.empty()) {
        return ErrorInvalidArgument();
    } else if (path.size() > MaxPath) {
        return ErrorNameTooLong(path);
    }

    return {};
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleResolvePath(
    const TEvService::TEvResolvePathRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_VALIDATE_EVENT_SESSION(ResolvePath, msg->Record);

    const auto& sessionId = GetSessionId(msg->Record);
    const auto& path = msg->Record.GetPath();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] ResolvePath %s",
        TabletID(),
        sessionId.Quote().c_str(),
        path.Quote().c_str());

    auto error = ValidateRequest(msg->Record);
    if (FAILED(error.GetCode())) {
        auto response = std::make_unique<TEvService::TEvResolvePathResponse>(
            error);

        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TResolvePath>(
        ctx,
        std::move(requestInfo),
        msg->Record);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_ResolvePath(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TResolvePath& args)
{
    // TODO
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TIndexTabletActor::ExecuteTx_ResolvePath(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TResolvePath& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);
}

void TIndexTabletActor::CompleteTx_ResolvePath(
    const TActorContext& ctx,
    TTxIndexTablet::TResolvePath& args)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] ResolvePath completed (%s)",
        TabletID(),
        args.SessionId.Quote().c_str(),
        FormatError(args.Error).c_str());

    auto response = std::make_unique<TEvService::TEvResolvePathResponse>(args.Error);
    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
