#include "tablet_actor.h"

#include <cloud/filestore/libs/storage/tablet/model/group_by.h>

#include <util/generic/set.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TAddBlobsExecutor
{
private:
    TIndexTabletState& State;
    THashMap<ui32, TCompactionStats> RangeId2CompactionStats;

public:
    TAddBlobsExecutor(TIndexTabletState& state)
        : State(state)
    {
    }

public:
    void Execute(
        const TActorContext& ctx,
        TTransactionContext& tx,
        TTxIndexTablet::TAddBlob& args)
    {
        Y_UNUSED(ctx);

        TIndexTabletDatabase db(tx.DB);

        switch (args.Mode) {
            case EAddBlobMode::Write:
                Execute_AddBlob_Write(db, args);
                break;

            case EAddBlobMode::Flush:
                Execute_AddBlob_Flush(db, args);
                break;

            case EAddBlobMode::FlushBytes:
                Execute_AddBlob_FlushBytes(db, args);
                break;

            case EAddBlobMode::Compaction:
                Execute_AddBlob_Compaction(db, args);
                break;
        }

        UpdateCompactionMap(db);
    }

private:
    void Execute_AddBlob_Write(
        TIndexTabletDatabase& db,
        TTxIndexTablet::TAddBlob& args)
    {
        Y_VERIFY(!args.SrcBlobs);

        // Flush/Compaction just transfers blocks from one place to another,
        // but Write is different: we need to generate MinCommitId
        // and mark overwritten blocks now
        args.CommitId = State.GenerateCommitId();

        for (auto& blob: args.MergedBlobs) {
            auto& block = blob.Block;

            if (!args.Nodes.contains(block.NodeId)) {
                // already deleted
                continue;
            }

            Y_VERIFY(block.MinCommitId == InvalidCommitId
                && block.MaxCommitId == InvalidCommitId);
            block.MinCommitId = args.CommitId;

            State.MarkFreshBlocksDeleted(
                db,
                block.NodeId,
                args.CommitId,
                block.BlockIndex,
                blob.BlocksCount);

            State.MarkMixedBlocksDeleted(
                db,
                block.NodeId,
                args.CommitId,
                block.BlockIndex,
                blob.BlocksCount);

            State.WriteMixedBlocks(
                db,
                blob.BlobId,
                blob.Block,
                blob.BlocksCount);
        }

        TVector<bool> isMixedBlobWritten(args.MixedBlobs.size());
        for (ui32 i = 0; i < args.MixedBlobs.size(); ++i) {
            auto& blob = args.MixedBlobs[i];

            for (auto& block: blob.Blocks) {
                Y_VERIFY(block.MinCommitId == InvalidCommitId
                    && block.MaxCommitId == InvalidCommitId);
                block.MinCommitId = args.CommitId;
            }

            GroupBy(
                MakeArrayRef(blob.Blocks),
                [] (const auto& l, const auto& r) {
                    return r.NodeId == l.NodeId
                        && r.BlockIndex == l.BlockIndex + 1;
                },
                [&] (TArrayRef<const TBlock> group) {
                    State.MarkFreshBlocksDeleted(
                        db,
                        group[0].NodeId,
                        args.CommitId,
                        group[0].BlockIndex,
                        group.size());

                    State.MarkMixedBlocksDeleted(
                        db,
                        group[0].NodeId,
                        args.CommitId,
                        group[0].BlockIndex,
                        group.size());
                });

            isMixedBlobWritten[i] =
                State.WriteMixedBlocks(db, blob.BlobId, blob.Blocks);
        }

        for (auto [id, maxOffset]: args.WriteRanges) {
            auto it = args.Nodes.find(id);
            Y_VERIFY(it != args.Nodes.end());

            if (it->Attrs.GetSize() < maxOffset) {
                auto attrs = CopyAttrs(it->Attrs, E_CM_CMTIME);
                attrs.SetSize(maxOffset);

                State.UpdateNode(
                    db,
                    id,
                    it->MinCommitId,
                    args.CommitId,
                    attrs,
                    it->Attrs);
            }
        }

        // compaction map update should be done after deleted blocks are marked
        for (const auto& blob: args.MergedBlobs) {
            auto& block = blob.Block;

            if (!args.Nodes.contains(block.NodeId)) {
                // already deleted
                continue;
            }

            // TODO rewrite this in a more efficient way
            THashSet<ui32> rangeIds;
            for (ui32 i = 0; i < blob.BlocksCount; ++i) {
                const auto rangeId =
                    State.GetMixedRangeIndex(block.NodeId, block.BlockIndex + i);
                rangeIds.insert(rangeId);
            }

            for (const auto rangeId: rangeIds) {
                auto& stats = AccessCompactionStats(rangeId);
                stats.BlobsCount += 1;
            }
        }

        for (ui32 i = 0; i < args.MixedBlobs.size(); ++i) {
            const auto& blob = args.MixedBlobs[i];
            const auto rangeId = State.GetMixedRangeIndex(blob.Blocks);
            auto& stats = AccessCompactionStats(rangeId);

            if (isMixedBlobWritten[i]) {
                stats.BlobsCount += 1;
            }
        }
    }

    void Execute_AddBlob_Flush(
        TIndexTabletDatabase& db,
        TTxIndexTablet::TAddBlob& args)
    {
        Y_VERIFY(!args.SrcBlobs);
        Y_VERIFY(!args.MergedBlobs);

        for (auto& blob: args.MixedBlobs) {
            for (auto& block: blob.Blocks) {
                Y_VERIFY(block.MinCommitId != InvalidCommitId);

                if (block.MaxCommitId == InvalidCommitId) {
                    // block could be overwritten while we were flushing
                    auto freshBlock = State.FindFreshBlock(
                        block.NodeId,
                        block.MinCommitId,
                        block.BlockIndex);

                    Y_VERIFY(freshBlock);
                    block.MaxCommitId = freshBlock->MaxCommitId;
                }
            }

            State.DeleteFreshBlocks(db, blob.Blocks);

            const auto rangeId = State.GetMixedRangeIndex(blob.Blocks);
            auto& stats = AccessCompactionStats(rangeId);
            if (State.WriteMixedBlocks(db, blob.BlobId, blob.Blocks)) {
                stats.BlobsCount += 1;
            }
        }
    }

    void Execute_AddBlob_FlushBytes(
        TIndexTabletDatabase& db,
        TTxIndexTablet::TAddBlob& args)
    {
        Y_VERIFY(!args.MergedBlobs);

        for (auto& blob: args.SrcBlobs) {
            State.UpdateBlockLists(db, blob);
        }

        for (auto& block: args.SrcBlocks) {
            Y_VERIFY(block.MaxCommitId != InvalidCommitId);

            State.MarkFreshBlocksDeleted(
                db,
                block.NodeId,
                block.MaxCommitId,
                block.BlockIndex,
                1
            );
        }

        for (auto& blob: args.MixedBlobs) {
            const auto rangeId = State.GetMixedRangeIndex(blob.Blocks);
            auto& stats = AccessCompactionStats(rangeId);
            if (State.WriteMixedBlocks(db, blob.BlobId, blob.Blocks)) {
                stats.BlobsCount += 1;
            }
        }
    }

    void Execute_AddBlob_Compaction(
        TIndexTabletDatabase& db,
        TTxIndexTablet::TAddBlob& args)
    {
        Y_VERIFY(!args.MergedBlobs);

        for (const auto& blob: args.SrcBlobs) {
            const auto rangeId = State.GetMixedRangeIndex(blob.Blocks);
            auto& stats = AccessCompactionStats(rangeId);
            // Zeroing compaction counter for the overwritten blobs.
            stats.BlobsCount = 0;
            State.DeleteMixedBlocks(db, blob.BlobId, blob.Blocks);
        }

        for (auto& blob: args.MixedBlobs) {
            const auto rangeId = State.GetMixedRangeIndex(blob.Blocks);
            auto& stats = AccessCompactionStats(rangeId);
            // Setting compaction counter to 1 for the new blobs.
            // The set of ranges for the new blobs can sometimes be a strict
            // subset of the set of ranges for the overwritten blobs.
            // see NBS-2556
            if (State.WriteMixedBlocks(db, blob.BlobId, blob.Blocks)) {
                stats.BlobsCount = 1;
            }
        }
    }

    void UpdateCompactionMap(TIndexTabletDatabase& db)
    {
        for (const auto& x: RangeId2CompactionStats) {
            db.WriteCompactionMap(
                x.first,
                x.second.BlobsCount,
                x.second.DeletionsCount
            );
            State.UpdateCompactionMap(
                x.first,
                x.second.BlobsCount,
                x.second.DeletionsCount
            );
        }
    }

    TCompactionStats& AccessCompactionStats(ui32 rangeId)
    {
        auto& stats = RangeId2CompactionStats[rangeId];
        if (!stats.BlobsCount) {
            stats = State.GetCompactionStats(rangeId);
        }
        return stats;
    }
};

}

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleAddBlob(
    const TEvIndexTabletPrivate::TEvAddBlobRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    FILESTORE_TRACK(
        RequestReceived_Tablet,
        msg->CallContext,
        "AddBlob");

    ExecuteTx<TAddBlob>(
        ctx,
        std::move(requestInfo),
        msg->Mode,
        std::move(msg->SrcBlobs),
        std::move(msg->SrcBlocks),
        std::move(msg->MixedBlobs),
        std::move(msg->MergedBlobs),
        std::move(msg->WriteRanges));
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_AddBlob(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TAddBlob& args)
{
    Y_UNUSED(ctx);

    TIndexTabletDatabase db(tx.DB);

    args.CommitId = GetCurrentCommitId();

    TSet<ui32> rangeIds;
    for (const auto& blob: args.MergedBlobs) {
        rangeIds.insert(GetMixedRangeIndex(
            blob.Block.NodeId,
            blob.Block.BlockIndex,
            blob.BlocksCount));
    }
    for (const auto& blob: args.MixedBlobs) {
        rangeIds.insert(GetMixedRangeIndex(blob.Blocks));
    }

    bool ready = true;
    for (auto [id, _]: args.WriteRanges) {
        TMaybe<TIndexTabletDatabase::TNode> node;
        if (!ReadNode(db, id, args.CommitId, node)) {
            ready = false;
            continue;
        }

        if (!node) {
            // already deleted
            continue;
        }

        auto [it, inserted]= args.Nodes.insert(*node);
        Y_VERIFY(inserted);
    }

    for (ui32 rangeId: rangeIds) {
        if (!LoadMixedBlocks(db, rangeId)) {
            ready = false;
        }
    }

    return ready;
}

void TIndexTabletActor::ExecuteTx_AddBlob(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TAddBlob& args)
{
    TAddBlobsExecutor executor(*this);

    executor.Execute(ctx, tx, args);
}

void TIndexTabletActor::CompleteTx_AddBlob(
    const TActorContext& ctx,
    TTxIndexTablet::TAddBlob& args)
{
    FILESTORE_TRACK(
        ResponseSent_Tablet,
        args.RequestInfo->CallContext,
        "AddBlob");

    auto response = std::make_unique<TEvIndexTabletPrivate::TEvAddBlobResponse>();
    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));

    EnqueueCollectGarbageIfNeeded(ctx);
}

}   // namespace NCloud::NFileStore::NStorage
