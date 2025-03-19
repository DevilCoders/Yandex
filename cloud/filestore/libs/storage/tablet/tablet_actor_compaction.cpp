#include "tablet_actor.h"

#include <cloud/filestore/libs/diagnostics/profile_log.h>
#include <cloud/filestore/libs/storage/tablet/model/blob_builder.h>
#include <cloud/filestore/libs/storage/tablet/model/block_buffer.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TCompactionActor final
    : public TActorBootstrapped<TCompactionActor>
{
private:
    const TActorId Tablet;
    const TString FileSystemId;
    const TRequestInfoPtr RequestInfo;
    const ui64 CommitId;
    const ui32 BlockSize;
    const IProfileLogPtr ProfileLog;

    TVector<TMixedBlobMeta> SrcBlobs;
    const TVector<TCompactionBlob> DstBlobs;

    THashMap<TPartialBlobId, IBlockBufferPtr, TPartialBlobIdHash> Buffers;

    size_t RequestsInFlight = 0;

    NProto::TProfileLogRequestInfo ProfileLogRequest;

public:
    TCompactionActor(
        const TActorId& tablet,
        TInstant compactionStartTs,
        TString fileSystemId,
        TRequestInfoPtr requestInfo,
        ui64 commitId,
        ui32 blockSize,
        IProfileLogPtr profileLog,
        TVector<TMixedBlobMeta> srcBlobs,
        TVector<TCompactionBlob> dstBlobs);

    void Bootstrap(const TActorContext& ctx);

private:
    STFUNC(StateWork);

    void ReadBlob(const TActorContext& ctx);
    void HandleReadBlobResponse(
        const TEvIndexTabletPrivate::TEvReadBlobResponse::TPtr& ev,
        const TActorContext& ctx);

    void WriteBlob(const TActorContext& ctx);
    void HandleWriteBlobResponse(
        const TEvIndexTabletPrivate::TEvWriteBlobResponse::TPtr& ev,
        const TActorContext& ctx);

    void AddBlob(const TActorContext& ctx);
    void HandleAddBlobResponse(
        const TEvIndexTabletPrivate::TEvAddBlobResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandlePoisonPill(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx);

    void ReplyAndDie(
        const TActorContext& ctx,
        const NProto::TError& error = {});
};

////////////////////////////////////////////////////////////////////////////////

TCompactionActor::TCompactionActor(
        const TActorId& tablet,
        TInstant compactionStartTs,
        TString fileSystemId,
        TRequestInfoPtr requestInfo,
        ui64 commitId,
        ui32 blockSize,
        IProfileLogPtr profileLog,
        TVector<TMixedBlobMeta> srcBlobs,
        TVector<TCompactionBlob> dstBlobs)
    : Tablet(tablet)
    , FileSystemId(std::move(fileSystemId))
    , RequestInfo(std::move(requestInfo))
    , CommitId(commitId)
    , BlockSize(blockSize)
    , ProfileLog(std::move(profileLog))
    , SrcBlobs(std::move(srcBlobs))
    , DstBlobs(std::move(dstBlobs))
{
    ActivityType = TFileStoreActivities::TABLET_WORKER;

    ProfileLogRequest.SetTimestampMcs(compactionStartTs.MicroSeconds());
}

void TCompactionActor::Bootstrap(const TActorContext& ctx)
{
    Become(&TThis::StateWork);

    FILESTORE_TRACK(
        RequestReceived_TabletWorker,
        RequestInfo->CallContext,
        "Compaction");

    InitProfileLogByteRanges(BlockSize, SrcBlobs, ProfileLogRequest);

    if (DstBlobs) {
        ReadBlob(ctx);

        if (!RequestsInFlight) {
            WriteBlob(ctx);
        }
    } else {
        AddBlob(ctx);
    }
}

void TCompactionActor::ReadBlob(const TActorContext& ctx)
{
    for (const auto& blob: SrcBlobs) {
        TVector<TReadBlob::TBlock> blocks(Reserve(blob.Blocks.size()));

        ui32 blobOffset = 0, blockOffset = 0;
        for (const auto& block: blob.Blocks) {
            if (block.MinCommitId < block.MaxCommitId) {
                blocks.emplace_back(blobOffset, blockOffset++);
            }
            ++blobOffset;
        }

        if (blocks) {
            auto request = std::make_unique<TEvIndexTabletPrivate::TEvReadBlobRequest>(
                RequestInfo->CallContext
            );
            request->Buffer = CreateBlockBuffer(TByteRange(
                0,
                blocks.size() * BlockSize,
                BlockSize
            ));
            request->Blobs.emplace_back(blob.BlobId, std::move(blocks));

            Buffers[blob.BlobId] = request->Buffer;

            NCloud::Send(ctx, Tablet, std::move(request));
            ++RequestsInFlight;
        }
    }
}

void TCompactionActor::HandleReadBlobResponse(
    const TEvIndexTabletPrivate::TEvReadBlobResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    if (FAILED(msg->GetStatus())) {
        ReplyAndDie(ctx, msg->GetError());
        return;
    }

    Y_VERIFY(RequestsInFlight);
    if (--RequestsInFlight == 0) {
        WriteBlob(ctx);
    }
}

void TCompactionActor::WriteBlob(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvIndexTabletPrivate::TEvWriteBlobRequest>(
        RequestInfo->CallContext
    );

    for (const auto& blob: DstBlobs) {
        TString blobContent(Reserve(BlockSize * blob.Blocks.size()));

        for (const auto& block: blob.Blocks) {
            auto& buffer = Buffers[block.BlobId];
            blobContent.append(buffer->GetBlock(block.BlobOffset));
        }

        request->Blobs.emplace_back(blob.BlobId, std::move(blobContent));
    }

    NCloud::Send(ctx, Tablet, std::move(request));
}

void TCompactionActor::HandleWriteBlobResponse(
    const TEvIndexTabletPrivate::TEvWriteBlobResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    if (FAILED(msg->GetStatus())) {
        ReplyAndDie(ctx, msg->GetError());
        return;
    }

    AddBlob(ctx);
}

void TCompactionActor::AddBlob(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvIndexTabletPrivate::TEvAddBlobRequest>(
        RequestInfo->CallContext
    );
    request->Mode = EAddBlobMode::Compaction;
    request->SrcBlobs = std::move(SrcBlobs);

    for (const auto& blob: DstBlobs) {
        TVector<TBlock> blocks(Reserve(blob.Blocks.size()));
        for (const auto& block: blob.Blocks) {
            blocks.emplace_back(block);
        }

        request->MixedBlobs.emplace_back(blob.BlobId, std::move(blocks));
    }

    NCloud::Send(ctx, Tablet, std::move(request));
}

void TCompactionActor::HandleAddBlobResponse(
    const TEvIndexTabletPrivate::TEvAddBlobResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    ReplyAndDie(ctx, msg->GetError());
}

void TCompactionActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);
    ReplyAndDie(ctx, MakeError(E_REJECTED, "request cancelled"));
}

void TCompactionActor::ReplyAndDie(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    // log request
    ProfileLogRequest.SetDurationMcs(
        ctx.Now().MicroSeconds() - ProfileLogRequest.GetTimestampMcs());
    ProfileLogRequest.SetRequestType(static_cast<ui32>(
        EFileStoreSystemRequest::Compaction));

    ProfileLog->Write({FileSystemId, std::move(ProfileLogRequest)});

    {
        // notify tablet
        auto response = std::make_unique<TEvIndexTabletPrivate::TEvCompactionCompleted>(error);
        response->CommitId = CommitId;
        NCloud::Send(ctx, Tablet, std::move(response));
    }

    FILESTORE_TRACK(
        ResponseSent_TabletWorker,
        RequestInfo->CallContext,
        "Compaction");

    if (RequestInfo->Sender != Tablet) {
        // reply to caller
        auto response = std::make_unique<TEvIndexTabletPrivate::TEvCompactionResponse>(error);
        NCloud::Reply(ctx, *RequestInfo, std::move(response));
    }

    Die(ctx);
}

STFUNC(TCompactionActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        HFunc(TEvIndexTabletPrivate::TEvReadBlobResponse, HandleReadBlobResponse);
        HFunc(TEvIndexTabletPrivate::TEvWriteBlobResponse, HandleWriteBlobResponse);
        HFunc(TEvIndexTabletPrivate::TEvAddBlobResponse, HandleAddBlobResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TFileStoreComponents::TABLET_WORKER);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::EnqueueBlobIndexOpIfNeeded(const TActorContext& ctx)
{
    auto [compactRangeId, compactionScore] = GetRangeToCompact();
    auto [cleanupRangeId, cleanupScore] = GetRangeToCleanup();

    if (BlobIndexOps.Empty()) {
        if (compactionScore >= Config->GetCompactionThreshold()) {
            BlobIndexOps.Push(EBlobIndexOp::Compaction);
        }

        if (cleanupScore >= Config->GetCleanupThreshold()) {
            BlobIndexOps.Push(EBlobIndexOp::Cleanup);
        }

        if (GetFreshBytesCount() >= Config->GetFlushBytesThreshold()) {
            BlobIndexOps.Push(EBlobIndexOp::FlushBytes);
        }
    }

    if (BlobIndexOps.Empty()) {
        return;
    }

    if (!BlobIndexOpState.Enqueue()) {
        return;
    }

    auto op = BlobIndexOps.Pop();

    switch (op) {
        case EBlobIndexOp::Compaction: {
            ctx.Send(
                SelfId(),
                new TEvIndexTabletPrivate::TEvCompactionRequest(compactRangeId)
            );
            break;
        }

        case EBlobIndexOp::Cleanup: {
            ctx.Send(
                SelfId(),
                new TEvIndexTabletPrivate::TEvCleanupRequest(cleanupRangeId)
            );
            break;
        }

        case EBlobIndexOp::FlushBytes: {
            // Flush blocked since FlushBytes op rewrites some fresh blocks as
            // blobs
            if (!FlushState.Enqueue()) {
                BlobIndexOpState.Complete();
                if (!BlobIndexOps.Empty()) {
                    EnqueueBlobIndexOpIfNeeded(ctx);
                }

                return;
            }

            ctx.Send(
                SelfId(),
                new TEvIndexTabletPrivate::TEvFlushBytesRequest()
            );
            break;
        }

        default: Y_VERIFY(0);
    }
}

void TIndexTabletActor::HandleCompaction(
    const TEvIndexTabletPrivate::TEvCompactionRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_TRACK(
        RequestReceived_Tablet,
        msg->CallContext,
        "Compaction");

    auto replyError = [] (
        const TActorContext& ctx,
        auto& ev,
        const NProto::TError& error)
    {
        FILESTORE_TRACK(
            ResponseSent_Tablet,
            ev.Get()->CallContext,
            "Compaction");

        auto response =
            std::make_unique<TEvIndexTabletPrivate::TEvCompactionResponse>(error);
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
        "[%lu] Compaction started (range: #%u)",
        TabletID(),
        msg->RangeId);

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ExecuteTx<TCompaction>(
        ctx,
        std::move(requestInfo),
        msg->RangeId);
}

void TIndexTabletActor::HandleCompactionCompleted(
    const TEvIndexTabletPrivate::TEvCompactionCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] Compaction completed (%s)",
        TabletID(),
        FormatError(msg->GetError()).c_str());

    ReleaseCollectBarrier(msg->CommitId);
    BlobIndexOpState.Complete();

    WorkerActors.erase(ev->Sender);
    EnqueueBlobIndexOpIfNeeded(ctx);
    EnqueueCollectGarbageIfNeeded(ctx);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_Compaction(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TCompaction& args)
{
    Y_UNUSED(ctx);

    TIndexTabletDatabase db(tx.DB);

    args.CommitId = GetCurrentCommitId();
    args.StartTs = ctx.Now();

    return LoadMixedBlocks(db, args.RangeId);
}

void TIndexTabletActor::ExecuteTx_Compaction(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TCompaction& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);
}

void TIndexTabletActor::CompleteTx_Compaction(
    const TActorContext& ctx,
    TTxIndexTablet::TCompaction& args)
{
    auto replyError = [] (
        const TActorContext& ctx,
        TTxIndexTablet::TCompaction& args,
        const NProto::TError& error)
    {
        FILESTORE_TRACK(
            ResponseSent_Tablet,
            args.RequestInfo->CallContext,
            "Compaction");

        if (args.RequestInfo->Sender != ctx.SelfID) {
            // reply to caller
            using TResponse = TEvIndexTabletPrivate::TEvCompactionResponse;
            auto response = std::make_unique<TResponse>(error);
            NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
        }
    };

    auto srcBlobs = GetBlobsForCompaction(args.RangeId);
    if (!srcBlobs) {
        LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
            "[%lu] Compaction completed (nothing to do)",
            TabletID());

        replyError(ctx, args, {});
        BlobIndexOpState.Complete();
        return;
    }

    TVector<TBlockDataRef> blocks(Reserve(srcBlobs.size() * MaxBlocksCount));

    for (const auto& blob: srcBlobs) {
        ui32 blockOffset = 0;   // offset in read buffer, not in blob!
        for (const auto& block: blob.Blocks) {
            if (block.MinCommitId < block.MaxCommitId) {
                blocks.emplace_back(
                    TBlockDataRef { block, blob.BlobId, blockOffset++ });
            }
        }
    }

    Sort(blocks, TBlockCompare());

    TCompactionBlobBuilder builder(
        CalculateMaxBlocksInBlob(Config->GetMaxBlobSize(), GetBlockSize()));

    for (const auto& block: blocks) {
        builder.Accept(block);
    }

    auto dstBlobs = builder.Finish();

    args.CommitId = GenerateCommitId();

    ui32 blobIndex = 0;
    for (auto& blob: dstBlobs) {
        const auto ok = GenerateBlobId(
            args.CommitId,
            blob.Blocks.size() * GetBlockSize(),
            blobIndex++,
            &blob.BlobId);

        if (!ok) {
            ReassignDataChannelsIfNeeded(ctx);

            replyError(
                ctx,
                args,
                MakeError(E_FS_OUT_OF_SPACE, "failed to generate blobId"));

            BlobIndexOpState.Complete();

            return;
        }
    }

    AcquireCollectBarrier(args.CommitId);

    auto actor = std::make_unique<TCompactionActor>(
        ctx.SelfID,
        args.StartTs,
        GetFileSystemId(),
        args.RequestInfo,
        args.CommitId,
        GetBlockSize(),
        ProfileLog,
        std::move(srcBlobs),
        std::move(dstBlobs));

    auto actorId = NCloud::Register(ctx, std::move(actor));
    WorkerActors.insert(actorId);
}

}   // namespace NCloud::NFileStore::NStorage
