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

class TReadBlockVisitor final
    : public IFreshBlockVisitor
    , public IMixedBlockVisitor
    , public IFreshBytesVisitor
{
private:
    const ui32 BlockSize;
    TBlockWithBytes& Block;
    ui64 BlockMinCommitId = 0;
    bool ApplyingByteLayer = false;

public:
    TReadBlockVisitor(ui32 blockSize, TBlockWithBytes& block)
        : BlockSize(blockSize)
        , Block(block)
    {
    }

    void Accept(const TBlock& block, TStringBuf blockData) override
    {
        Y_VERIFY(!ApplyingByteLayer);

        if (block.MinCommitId > BlockMinCommitId) {
            BlockMinCommitId = block.MinCommitId;
            TOwningFreshBlock ofb;
            static_cast<TBlock&>(ofb) = block;
            ofb.BlockData = blockData;
            Block.Block = std::move(ofb);
        }
    }

    void Accept(
        const TBlock& block,
        const TPartialBlobId& blobId,
        ui32 blobOffset) override
    {
        Y_VERIFY(!ApplyingByteLayer);

        if (block.MinCommitId > BlockMinCommitId) {
            BlockMinCommitId = block.MinCommitId;
            TBlockDataRef ref;
            static_cast<TBlock&>(ref) = block;
            ref.BlobId = blobId;
            ref.BlobOffset = blobOffset;
            Block.Block = std::move(ref);
        }
    }

    void Accept(const TBytes& bytes, TStringBuf data) override
    {
        ApplyingByteLayer = true;

        if (bytes.MinCommitId > BlockMinCommitId) {
            const auto bytesOffset =
                static_cast<ui64>(Block.BlockIndex) * BlockSize;
            Block.BytesMinCommitId = bytes.MinCommitId;
            Block.BlockBytes.Intervals.push_back({
                IntegerCast<ui32>(bytes.Offset - bytesOffset),
                TString(data)
            });
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class TFlushBytesActor final
    : public TActorBootstrapped<TFlushBytesActor>
{
private:
    const TActorId Tablet;
    const TString FileSystemId;
    const TRequestInfoPtr RequestInfo;
    const ui64 CollectCommitId;
    const ui32 BlockSize;
    const ui64 ChunkId;
    const IProfileLogPtr ProfileLog;
    NProto::TProfileLogRequestInfo ProfileLogRequest;
    TVector<TMixedBlobMeta> SrcBlobs;
    TVector<TMixedBlobMeta> SrcBlobsToRead;
    TVector<TVector<ui32>> SrcBlobOffsets;
    const TVector<TFlushBytesBlob> DstBlobs;

    THashMap<TPartialBlobId, IBlockBufferPtr, TPartialBlobIdHash> Buffers;
    using TBlobOffsetMap =
        THashMap<TBlockLocationInBlob, ui32, TBlockLocationInBlobHash>;
    TBlobOffsetMap SrcBlobOffset2DstBlobOffset;

    size_t RequestsInFlight = 0;

public:
    TFlushBytesActor(
        const TActorId& tablet,
        TString fileSystemId,
        TRequestInfoPtr requestInfo,
        ui64 collectCommitId,
        ui32 blockSize,
        ui64 chunkId,
        IProfileLogPtr profileLog,
        NProto::TProfileLogRequestInfo profileLogRequest,
        TVector<TMixedBlobMeta> srcBlobs,
        TVector<TMixedBlobMeta> srcBlobsToRead,
        TVector<TVector<ui32>> srcBlobOffsets,
        TVector<TFlushBytesBlob> dstBlobs);

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

TFlushBytesActor::TFlushBytesActor(
        const TActorId& tablet,
        TString fileSystemId,
        TRequestInfoPtr requestInfo,
        ui64 collectCommitId,
        ui32 blockSize,
        ui64 chunkId,
        IProfileLogPtr profileLog,
        NProto::TProfileLogRequestInfo profileLogRequest,
        TVector<TMixedBlobMeta> srcBlobs,
        TVector<TMixedBlobMeta> srcBlobsToRead,
        TVector<TVector<ui32>> srcBlobOffsets,
        TVector<TFlushBytesBlob> dstBlobs)
    : Tablet(tablet)
    , FileSystemId(std::move(fileSystemId))
    , RequestInfo(std::move(requestInfo))
    , CollectCommitId(collectCommitId)
    , BlockSize(blockSize)
    , ChunkId(chunkId)
    , ProfileLog(std::move(profileLog))
    , ProfileLogRequest(std::move(profileLogRequest))
    , SrcBlobs(std::move(srcBlobs))
    , SrcBlobsToRead(std::move(srcBlobsToRead))
    , SrcBlobOffsets(std::move(srcBlobOffsets))
    , DstBlobs(std::move(dstBlobs))
{
    ActivityType = TFileStoreActivities::TABLET_WORKER;
}

void TFlushBytesActor::Bootstrap(const TActorContext& ctx)
{
    FILESTORE_TRACK(
        RequestReceived_TabletWorker,
        RequestInfo->CallContext,
        "FlushBytes");

    Become(&TThis::StateWork);

    ReadBlob(ctx);

    if (!RequestsInFlight) {
        WriteBlob(ctx);
    }
}

void TFlushBytesActor::ReadBlob(const TActorContext& ctx)
{
    for (ui32 i = 0; i < SrcBlobsToRead.size(); ++i) {
        auto& blobToRead = SrcBlobsToRead[i];
        auto& blobOffsets = SrcBlobOffsets[i];
        TVector<TReadBlob::TBlock> blocks(Reserve(blobToRead.Blocks.size()));

        ui32 blockOffset = 0;

        for (ui32 j = 0; j < blobToRead.Blocks.size(); ++j) {
            const auto& block = blobToRead.Blocks[j];
            if (block.MinCommitId < block.MaxCommitId) {
                TBlockLocationInBlob key{blobToRead.BlobId, blobOffsets[j]};
                SrcBlobOffset2DstBlobOffset[key] = blockOffset;

                blocks.emplace_back(blobOffsets[j], blockOffset++);
            }
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
            request->Blobs.emplace_back(blobToRead.BlobId, std::move(blocks));

            Buffers[blobToRead.BlobId] = request->Buffer;

            NCloud::Send(ctx, Tablet, std::move(request));
            ++RequestsInFlight;
        }
    }
}

void TFlushBytesActor::HandleReadBlobResponse(
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

void TFlushBytesActor::WriteBlob(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvIndexTabletPrivate::TEvWriteBlobRequest>(
        RequestInfo->CallContext
    );

    for (const auto& blob: DstBlobs) {
        TString blobContent(Reserve(BlockSize * blob.Blocks.size()));

        for (const auto& block: blob.Blocks) {
            blobContent.append(BlockSize, 0);
            TStringBuf blockContent =
                TStringBuf(blobContent).substr(blobContent.Size() - BlockSize);
            if (auto* ref = std::get_if<TBlockDataRef>(&block.Block)) {
                TBlockLocationInBlob key{ref->BlobId, ref->BlobOffset};

                auto& buffer = Buffers[ref->BlobId];
                memcpy(
                    const_cast<char*>(blockContent.Data()),
                    buffer->GetBlock(SrcBlobOffset2DstBlobOffset[key]).Data(),
                    BlockSize
                );
            } else if (auto* fresh = std::get_if<TOwningFreshBlock>(&block.Block)) {
                memcpy(
                    const_cast<char*>(blockContent.Data()),
                    fresh->BlockData.Data(),
                    BlockSize
                );
            }

            for (const auto& interval: block.BlockBytes.Intervals) {
                memcpy(
                    const_cast<char*>(blockContent.Data()) + interval.OffsetInBlock,
                    interval.Data.Data(),
                    interval.Data.Size()
                );
            }
        }

        request->Blobs.emplace_back(blob.BlobId, std::move(blobContent));
    }

    NCloud::Send(ctx, Tablet, std::move(request));
}

void TFlushBytesActor::HandleWriteBlobResponse(
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

void TFlushBytesActor::AddBlob(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvIndexTabletPrivate::TEvAddBlobRequest>(
        RequestInfo->CallContext
    );
    request->Mode = EAddBlobMode::FlushBytes;
    request->SrcBlobs = std::move(SrcBlobs);

    THashMap<TBlockLocation, ui64, TBlockLocationHash> blockIndex2BytesCommitId;

    for (const auto& blob: DstBlobs) {
        TVector<TBlock> blocks(Reserve(blob.Blocks.size()));
        for (const auto& block: blob.Blocks) {
            ui64 maxCommitId = InvalidCommitId;

            if (auto* ref = std::get_if<TBlockDataRef>(&block.Block)) {
                maxCommitId = ref->MaxCommitId;
            } else if (auto* fresh = std::get_if<TOwningFreshBlock>(&block.Block)) {
                request->SrcBlocks.push_back(*fresh);
                maxCommitId = fresh->MaxCommitId;
            }

            blocks.emplace_back(
                block.NodeId,
                block.BlockIndex,
                block.BytesMinCommitId,
                maxCommitId
            );

            TBlockLocation key{block.NodeId, block.BlockIndex};
            auto& commitId = blockIndex2BytesCommitId[key];
            Y_VERIFY(commitId == 0);

            commitId = block.BytesMinCommitId;
        }

        request->MixedBlobs.emplace_back(blob.BlobId, std::move(blocks));
    }

    for (auto& srcBlob: request->SrcBlobs) {
        for (auto& block: srcBlob.Blocks) {
            TBlockLocation key{block.NodeId, block.BlockIndex};
            if (auto p = blockIndex2BytesCommitId.FindPtr(key)) {
                block.MaxCommitId = *p;
            }
        }
    }

    for (auto& block: request->SrcBlocks) {
        TBlockLocation key{block.NodeId, block.BlockIndex};
        if (auto p = blockIndex2BytesCommitId.FindPtr(key)) {
            block.MaxCommitId = *p;
        }
    }

    NCloud::Send(ctx, Tablet, std::move(request));
}

void TFlushBytesActor::HandleAddBlobResponse(
    const TEvIndexTabletPrivate::TEvAddBlobResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    ReplyAndDie(ctx, msg->GetError());
}

void TFlushBytesActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);
    ReplyAndDie(ctx, MakeError(E_REJECTED, "request cancelled"));
}

void TFlushBytesActor::ReplyAndDie(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    // log request
    ProfileLogRequest.SetDurationMcs(
        ctx.Now().MicroSeconds() - ProfileLogRequest.GetTimestampMcs());

    ProfileLog->Write({FileSystemId, std::move(ProfileLogRequest)});

    {
        // notify tablet
        auto response =
            std::make_unique<TEvIndexTabletPrivate::TEvFlushBytesCompleted>(error);
        response->CollectCommitId = CollectCommitId;
        response->ChunkId = ChunkId;
        response->CallContext = RequestInfo->CallContext;

        NCloud::Send(ctx, Tablet, std::move(response));
    }

    FILESTORE_TRACK(
        ResponseSent_TabletWorker,
        RequestInfo->CallContext,
        "FlushBytes");

    if (RequestInfo->Sender != Tablet) {
        // reply to caller
        auto response =
            std::make_unique<TEvIndexTabletPrivate::TEvFlushBytesResponse>(error);
        NCloud::Reply(ctx, *RequestInfo, std::move(response));
    }

    Die(ctx);
}

STFUNC(TFlushBytesActor::StateWork)
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

void TIndexTabletActor::HandleFlushBytes(
    const TEvIndexTabletPrivate::TEvFlushBytesRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_TRACK(
        RequestReceived_Tablet,
        msg->CallContext,
        "FlushBytes");

    auto replyError = [] (
        const TActorContext& ctx,
        auto& ev,
        const NProto::TError& error)
    {
        FILESTORE_TRACK(
            ResponseSent_Tablet,
            ev.Get()->CallContext,
            "FlushBytes");

        if (ev.Sender != ctx.SelfID) {
            // reply to caller
            auto response =
                std::make_unique<TEvIndexTabletPrivate::TEvFlushBytesResponse>(error);
            NCloud::Reply(ctx, ev, std::move(response));
        }
    };

    if (!BlobIndexOpState.Start()) {
        if (FlushState.Start()) {
            FlushState.Complete();
        }

        replyError(
            ctx,
            *ev,
            MakeError(S_FALSE, "cleanup/compaction is in progress")
        );

        return;
    }

    if (!FlushState.Start()) {
        BlobIndexOpState.Complete();

        replyError(
            ctx,
            *ev,
            MakeError(S_FALSE, "flush is in progress")
        );

        return;
    }

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] FlushBytes started",
        TabletID());

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    TVector<TBytes> bytes;
    auto cleanupInfo = StartFlushBytes(&bytes);

    if (bytes.empty()) {
        FlushState.Complete();
        BlobIndexOpState.Complete();

        replyError(
            ctx,
            *ev,
            MakeError(S_ALREADY, "no bytes to flush")
        );

        return;
    }

    ExecuteTx<TFlushBytes>(
        ctx,
        std::move(requestInfo),
        cleanupInfo.ClosingCommitId,
        cleanupInfo.ChunkId,
        std::move(bytes)
    );
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_FlushBytes(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TFlushBytes& args)
{
    Y_UNUSED(ctx);

    TIndexTabletDatabase db(tx.DB);

    args.ProfileLogRequest.SetTimestampMcs(ctx.Now().MicroSeconds());

    bool ready = true;

    for (auto& b: args.Bytes) {
        ready &= LoadMixedBlocks(
            db,
            GetMixedRangeIndex(b.NodeId, b.Offset / GetBlockSize()));
    }

    return ready;
}

void TIndexTabletActor::ExecuteTx_FlushBytes(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TFlushBytes& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);
}

void TIndexTabletActor::CompleteTx_FlushBytes(
    const TActorContext& ctx,
    TTxIndexTablet::TFlushBytes& args)
{
    auto replyError = [] (
        const TActorContext& ctx,
        TRequestInfo& requestInfo,
        const NProto::TError& error)
    {
        FILESTORE_TRACK(
            ResponseSent_Tablet,
            requestInfo.CallContext,
            "FlushBytes");

        if (requestInfo.Sender != ctx.SelfID) {
            // reply to caller
            auto response =
                std::make_unique<TEvIndexTabletPrivate::TEvFlushBytesResponse>(error);
            NCloud::Reply(ctx, requestInfo, std::move(response));
        }
    };

    args.CollectCommitId = Max<ui64>();

    for (const auto& bytes: args.Bytes) {
        args.CollectCommitId = Min(args.CollectCommitId, bytes.MinCommitId);
    }

    THashMap<TBlockLocation, TBlockWithBytes, TBlockLocationHash> blockMap;

    struct TSrcBlobInfo
    {
        TMixedBlobMeta SrcBlob;
        TMixedBlobMeta SrcBlobToRead;
        TVector<ui32> BlobOffsets;
    };
    THashMap<TPartialBlobId, TSrcBlobInfo, TPartialBlobIdHash> srcBlobMap;

    for (const auto& bytes: args.Bytes) {
        {
            auto* range = args.ProfileLogRequest.AddRanges();
            range->SetNodeId(bytes.NodeId);
            range->SetOffset(bytes.Offset);
            range->SetBytes(bytes.Length);
        }

        TByteRange byteRange(bytes.Offset, bytes.Length, GetBlockSize());
        Y_VERIFY(byteRange.BlockCount() == 1);
        ui32 blockIndex = byteRange.FirstBlock();
        TBlockLocation key{bytes.NodeId, blockIndex};

        if (blockMap.contains(key)) {
            // we have already processed this block and found all actual data
            continue;
        }

        TBlockWithBytes blockWithBytes;
        blockWithBytes.NodeId = bytes.NodeId;
        blockWithBytes.BlockIndex = blockIndex;

        TReadBlockVisitor visitor(GetBlockSize(), blockWithBytes);

        FindFreshBlocks(
            visitor,
            bytes.NodeId,
            args.ReadCommitId,
            blockIndex,
            1);

        FindMixedBlocks(
            visitor,
            bytes.NodeId,
            args.ReadCommitId,
            blockIndex,
            1);

        FindFreshBytes(
            visitor,
            bytes.NodeId,
            args.ReadCommitId,
            TByteRange::BlockRange(blockIndex, GetBlockSize()));

        if (blockWithBytes.BlockBytes.Intervals.empty()) {
            // this interval has been overwritten by a full block, skipping
            continue;
        }

        if (auto* ref = std::get_if<TBlockDataRef>(&blockWithBytes.Block)) {
            // the base block at blockIndex belongs to a blob - we need to read
            // its data from blobstorage and update this blob's blocklist upon
            // TxAddBlob

            auto& srcBlobInfo = srcBlobMap[ref->BlobId];
            if (!srcBlobInfo.SrcBlob.BlobId) {
                const auto rangeId = GetMixedRangeIndex(bytes.NodeId, blockIndex);
                srcBlobInfo.SrcBlob = FindBlob(rangeId, ref->BlobId);
                srcBlobInfo.SrcBlobToRead.BlobId = ref->BlobId;
            }
            srcBlobInfo.SrcBlobToRead.Blocks.push_back(
                static_cast<const TBlock&>(*ref)
            );
            srcBlobInfo.BlobOffsets.push_back(ref->BlobOffset);
        }

        // XXX not adding srcBlobs whose blocks were overwritten by fresh
        // blocks - relying on a future Flush here

        blockMap[key] = std::move(blockWithBytes);
    }

    if (blockMap.empty()) {
        // all bytes have been overwritten by full blocks, can trim straight away
        ExecuteTx<TTrimBytes>(
            ctx,
            args.RequestInfo,
            args.ChunkId);

        replyError(ctx, *args.RequestInfo, {});

        return;
    }

    TVector<TMixedBlobMeta> srcBlobs;
    TVector<TMixedBlobMeta> srcBlobsToRead;
    TVector<TVector<ui32>> srcBlobOffsets;
    for (auto& x: srcBlobMap) {
        srcBlobs.push_back(std::move(x.second.SrcBlob));
        srcBlobsToRead.push_back(std::move(x.second.SrcBlobToRead));
        srcBlobOffsets.push_back(std::move(x.second.BlobOffsets));
    }

    TVector<TBlockWithBytes> blocks;
    for (auto& x: blockMap) {
        blocks.push_back(std::move(x.second));
    }
    Sort(
        blocks.begin(),
        blocks.end(),
        TBlockWithBytesCompare());

    TFlushBytesBlobBuilder builder(
        GetRangeIdHasher(),
        CalculateMaxBlocksInBlob(Config->GetMaxBlobSize(), GetBlockSize()));

    for (auto& block: blocks) {
        builder.Accept(std::move(block));
    }

    auto dstBlobs = builder.Finish();
    Y_VERIFY(dstBlobs);

    auto commitId = GenerateCommitId();

    ui32 blobIndex = 0;
    for (auto& blob: dstBlobs) {
        const auto ok = GenerateBlobId(
            commitId,
            blob.Blocks.size() * GetBlockSize(),
            blobIndex++,
            &blob.BlobId);

        if (!ok) {
            ReassignDataChannelsIfNeeded(ctx);

            replyError(
                ctx,
                *args.RequestInfo,
                MakeError(E_FS_OUT_OF_SPACE, "failed to generate blobId"));

            BlobIndexOpState.Complete();
            FlushState.Complete();

            return;
        }
    }

    AcquireCollectBarrier(args.CollectCommitId);

    auto actor = std::make_unique<TFlushBytesActor>(
        ctx.SelfID,
        GetFileSystemId(),
        args.RequestInfo,
        args.CollectCommitId,
        GetBlockSize(),
        args.ChunkId,
        ProfileLog,
        std::move(args.ProfileLogRequest),
        std::move(srcBlobs),
        std::move(srcBlobsToRead),
        std::move(srcBlobOffsets),
        std::move(dstBlobs));

    auto actorId = NCloud::Register(ctx, std::move(actor));
    WorkerActors.insert(actorId);
}

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleFlushBytesCompleted(
    const TEvIndexTabletPrivate::TEvFlushBytesCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    // RECEIVED FROM TFlushBytesActor!
    const auto* msg = ev->Get();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] FlushBytes completed (%s)",
        TabletID(),
        FormatError(msg->GetError()).c_str());

    ReleaseCollectBarrier(msg->CollectCommitId);
    WorkerActors.erase(ev->Sender);

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    FILESTORE_TRACK(
        RequestReceived_Tablet,
        msg->CallContext,
        "TrimBytes");

    ExecuteTx<TTrimBytes>(ctx, std::move(requestInfo), msg->ChunkId);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_TrimBytes(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TTrimBytes& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    args.ProfileLogRequest.SetTimestampMcs(ctx.Now().MicroSeconds());

    return true;
}

void TIndexTabletActor::ExecuteTx_TrimBytes(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TTrimBytes& args)
{
    Y_UNUSED(ctx);

    TIndexTabletDatabase db(tx.DB);

    FinishFlushBytes(db, args.ChunkId, args.ProfileLogRequest);
}

void TIndexTabletActor::CompleteTx_TrimBytes(
    const TActorContext& ctx,
    TTxIndexTablet::TTrimBytes& args)
{
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] TrimBytes completed (%lu)",
        TabletID(),
        args.ChunkId);

    FILESTORE_TRACK(
        ResponseSent_Tablet,
        args.RequestInfo->CallContext,
        "TrimBytes");

    args.ProfileLogRequest.SetDurationMcs(
        ctx.Now().MicroSeconds() - args.ProfileLogRequest.GetTimestampMcs());

    ProfileLog->Write({GetFileSystemId(), std::move(args.ProfileLogRequest)});

    BlobIndexOpState.Complete();
    FlushState.Complete();

    EnqueueBlobIndexOpIfNeeded(ctx);
    EnqueueCollectGarbageIfNeeded(ctx);
}

}   // namespace NCloud::NFileStore::NStorage
