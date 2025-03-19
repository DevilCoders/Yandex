#include "tablet_actor.h"

#include <cloud/filestore/libs/diagnostics/profile_log.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleCleanup(
    const TEvIndexTabletPrivate::TEvCleanupRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_TRACK(
        RequestReceived_Tablet,
        msg->CallContext,
        "Cleanup");

    auto replyError = [] (
        const TActorContext& ctx,
        auto& ev,
        const NProto::TError& error)
    {
        FILESTORE_TRACK(
            ResponseSent_Tablet,
            ev.Get()->CallContext,
            "Cleanup");

        auto response = std::make_unique<TEvIndexTabletPrivate::TEvCleanupResponse>(error);
        NCloud::Reply(ctx, ev, std::move(response));
    };

    if (!BlobIndexOpState.Start()) {
        replyError(
            ctx,
            *ev,
            MakeError(S_ALREADY, "cleanup/compaction is in progress")
        );

        return;
    }

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] Cleanup started (range: #%u)",
        TabletID(),
        msg->RangeId);

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TCleanup>(
        ctx,
        std::move(requestInfo),
        msg->RangeId,
        GetCurrentCommitId());
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_Cleanup(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TCleanup& args)
{
    Y_UNUSED(ctx);

    TIndexTabletDatabase db(tx.DB);

    args.CommitId = GetCurrentCommitId();
    args.ProfileLogRequest.SetTimestampMcs(ctx.Now().MicroSeconds());

    return LoadMixedBlocks(db, args.RangeId);
}

void TIndexTabletActor::ExecuteTx_Cleanup(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TCleanup& args)
{
    Y_UNUSED(ctx);

    // needed to prevent the blobs updated during this tx from being deleted
    // before this tx completes
    AcquireCollectBarrier(args.CollectBarrier);

    TIndexTabletDatabase db(tx.DB);

    CleanupMixedBlockDeletions(db, args.RangeId, args.ProfileLogRequest);
}

void TIndexTabletActor::CompleteTx_Cleanup(
    const TActorContext& ctx,
    TTxIndexTablet::TCleanup& args)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] Cleanup completed",
        TabletID());

    BlobIndexOpState.Complete();
    ReleaseCollectBarrier(args.CollectBarrier);

    FILESTORE_TRACK(
        ResponseSent_Tablet,
        args.RequestInfo->CallContext,
        "Cleanup");

    args.ProfileLogRequest.SetDurationMcs(
        ctx.Now().MicroSeconds() - args.ProfileLogRequest.GetTimestampMcs());

    ProfileLog->Write({GetFileSystemId(), std::move(args.ProfileLogRequest)});

    if (args.RequestInfo->Sender != ctx.SelfID) {
        // reply to caller
        auto response = std::make_unique<TEvIndexTabletPrivate::TEvCleanupResponse>();
        NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
    }

    EnqueueBlobIndexOpIfNeeded(ctx);
    EnqueueCollectGarbageIfNeeded(ctx);
}

}   // namespace NCloud::NFileStore::NStorage
