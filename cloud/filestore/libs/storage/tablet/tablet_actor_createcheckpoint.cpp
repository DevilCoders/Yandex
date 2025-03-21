#include "tablet_actor.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleCreateCheckpoint(
    const TEvService::TEvCreateCheckpointRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    const auto& checkpointId = msg->Record.GetCheckpointId();
    const auto nodeId = msg->Record.GetNodeId();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] CreateCheckpoint started (checkpointId: %s, nodeId: %lu)",
        TabletID(),
        checkpointId.Quote().c_str(),
        nodeId);

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TCreateCheckpoint>(
        ctx,
        std::move(requestInfo),
        checkpointId,
        nodeId);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_CreateCheckpoint(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TCreateCheckpoint& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TIndexTabletActor::ExecuteTx_CreateCheckpoint(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TCreateCheckpoint& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_ERROR(CreateCheckpoint, args);

    TIndexTabletDatabase db(tx.DB);

    args.CommitId = GenerateCommitId();

    auto* checkpoint = CreateCheckpoint(
        db,
        args.CheckpointId,
        args.NodeId,
        args.CommitId);
    Y_VERIFY(checkpoint);
}

void TIndexTabletActor::CompleteTx_CreateCheckpoint(
    const TActorContext& ctx,
    TTxIndexTablet::TCreateCheckpoint& args)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] CreateCheckpoint completed (%s)",
        TabletID(),
        FormatError(args.Error).c_str());

    auto response = std::make_unique<TEvService::TEvCreateCheckpointResponse>(args.Error);
    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
