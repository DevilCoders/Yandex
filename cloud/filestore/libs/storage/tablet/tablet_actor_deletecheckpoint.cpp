#include "tablet_actor.h"

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

constexpr size_t RemoveCheckpointNodesBatchSize = 100;
constexpr size_t RemoveCheckpointBlobsBatchSize = 100;

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleDeleteCheckpoint(
    const TEvIndexTabletPrivate::TEvDeleteCheckpointRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] DeleteCheckpoint started (checkpointId: %s, mode: %s)",
        TabletID(),
        msg->CheckpointId.Quote().c_str(),
        ToString(msg->Mode).c_str());

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TDeleteCheckpoint>(
        ctx,
        std::move(requestInfo),
        msg->CheckpointId,
        msg->Mode,
        GetCurrentCommitId());
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_DeleteCheckpoint(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TDeleteCheckpoint& args)
{
    Y_UNUSED(ctx);

    auto* checkpoint = FindCheckpoint(args.CheckpointId);
    if (!checkpoint) {
        args.Error = ErrorInvalidCheckpoint(args.CheckpointId);
        return true;
    }

    args.CommitId = checkpoint->GetCommitId();

    TIndexTabletDatabase db(tx.DB);

    bool ready = true;
    switch (args.Mode) {
        case EDeleteCheckpointMode::MarkCheckpointDeleted:
            // nothing to do
            break;

        case EDeleteCheckpointMode::RemoveCheckpointNodes: {
            ready = db.ReadCheckpointNodes(
                args.CommitId,
                args.NodeIds,
                RemoveCheckpointNodesBatchSize);

            if (ready) {
                for (ui64 nodeId: args.NodeIds) {
                    TMaybe<TIndexTabletDatabase::TNode> node;
                    if (!db.ReadNodeVer(nodeId, args.CommitId, node)) {
                        ready = false;
                    }

                    if (ready && node) {
                        args.Nodes.emplace_back(node.GetRef());
                    }

                    if (!db.ReadNodeAttrVers(nodeId, args.CommitId, args.NodeAttrs)) {
                        ready = false;
                    }

                    if (!db.ReadNodeRefVers(nodeId, args.CommitId, args.NodeRefs)) {
                        ready = false;
                    }
                }
            }
            break;
        }

        case EDeleteCheckpointMode::RemoveCheckpointBlobs: {
            ready = db.ReadCheckpointBlobs(
                args.CommitId,
                args.Blobs,
                RemoveCheckpointBlobsBatchSize);

            if (ready) {
                for (const auto& blob: args.Blobs) {
                    TMaybe<TIndexTabletDatabase::TMixedBlob> mixedBlob;
                    if (!db.ReadMixedBlocks(blob.RangeId, blob.BlobId, mixedBlob)) {
                        ready = false;
                    }

                    if (ready) {
                        Y_VERIFY(mixedBlob);
                        args.MixedBlobs.emplace_back(std::move(mixedBlob.GetRef()));
                    }
                }
            }
            break;
        }

        case EDeleteCheckpointMode::RemoveCheckpoint:
            // nothing to do
            break;
    }

    return ready;
}

void TIndexTabletActor::ExecuteTx_DeleteCheckpoint(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TDeleteCheckpoint& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_ERROR(DeleteCheckpoint, args);

    // needed to prevent the blobs updated during this tx from being deleted
    // before this tx completes
    AcquireCollectBarrier(args.CollectBarrier);

    auto* checkpoint = FindCheckpoint(args.CheckpointId);
    Y_VERIFY(checkpoint);

    TIndexTabletDatabase db(tx.DB);

    switch (args.Mode) {
        case EDeleteCheckpointMode::MarkCheckpointDeleted:
            MarkCheckpointDeleted(db, checkpoint);
            break;

        case EDeleteCheckpointMode::RemoveCheckpointNodes: {
            if (!args.NodeIds) {
                args.Error = MakeError(S_FALSE, "no more nodes");
                return;
            }

            for (const auto& node: args.Nodes) {
                RewriteNode(
                    db,
                    node.NodeId,
                    node.MinCommitId,
                    node.MaxCommitId,
                    node.Attrs);
            }

            for (const auto& attr: args.NodeAttrs) {
                RewriteNodeAttr(
                    db,
                    attr.NodeId,
                    attr.MinCommitId,
                    attr.MaxCommitId,
                    attr);
            }

            for (const auto& ref: args.NodeRefs) {
                RewriteNodeRef(
                    db,
                    ref.NodeId,
                    ref.MinCommitId,
                    ref.MaxCommitId,
                    ref.Name,
                    ref.ChildNodeId);
            }

            RemoveCheckpointNodes(db, checkpoint, args.NodeIds);
            break;
        }

        case EDeleteCheckpointMode::RemoveCheckpointBlobs: {
            if (!args.Blobs) {
                args.Error = MakeError(S_FALSE, "no more blobs");
                return;
            }

            Y_VERIFY(args.Blobs.size() == args.MixedBlobs.size());
            for (size_t i = 0; i < args.Blobs.size(); ++i) {
                Y_VERIFY(args.Blobs[i].BlobId == args.MixedBlobs[i].BlobId);

                const auto rangeId = args.Blobs[i].RangeId;

                TMixedBlobMeta blob {
                    args.MixedBlobs[i].BlobId,
                    args.MixedBlobs[i].BlockList.DecodeBlocks(),
                };

                TMixedBlobStats stats {
                    args.MixedBlobs[i].GarbageBlocks,
                    args.MixedBlobs[i].CheckpointBlocks,
                };

                RewriteMixedBlocks(db, rangeId, blob, stats);
                RemoveCheckpointBlob(db, checkpoint, rangeId, blob.BlobId);
            }
            break;
        }

        case EDeleteCheckpointMode::RemoveCheckpoint:
            RemoveCheckpoint(db, checkpoint);
            break;
    }
}

void TIndexTabletActor::CompleteTx_DeleteCheckpoint(
    const TActorContext& ctx,
    TTxIndexTablet::TDeleteCheckpoint& args)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] DeleteCheckpoint completed (%s)",
        TabletID(),
        FormatError(args.Error).c_str());

    ReleaseCollectBarrier(args.CollectBarrier);

    auto response = std::make_unique<TEvIndexTabletPrivate::TEvDeleteCheckpointResponse>(args.Error);
    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
