#include "tablet_actor.h"

#include <cloud/filestore/libs/storage/tablet/model/split_range.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/generic/algorithm.h>
#include <util/generic/cast.h>
#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

bool ShouldReadBlobs(const TVector<TBlockDataRef>& blocks)
{
    for (const auto& block: blocks) {
        if (block.BlobId) {
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////

void CopyFileData(
    const TByteRange origin,
    const TByteRange aligned,
    const ui64 fileSize,
    TStringBuf content,
    TString* out)
{
    auto end = Min(fileSize, origin.End());
    if (end < aligned.End()) {
        ui64 delta = Min(aligned.End() - end, content.size());
        content.Chop(delta);
    }

    Y_VERIFY_DEBUG(origin.Offset >= aligned.Offset);
    content.Skip(origin.Offset - aligned.Offset);

    out->assign(content.data(), content.size());
}

////////////////////////////////////////////////////////////////////////////////

void ApplyBytes(
    const TByteRange& byteRange,
    TVector<TBlockBytes> bytes,
    IBlockBuffer& buffer)
{
    Y_VERIFY(byteRange.IsAligned());
    for (ui32 i = 0; i < byteRange.AlignedBlockCount(); ++i) {
        auto target = buffer.GetBlock(i);
        for (auto& interval: bytes[i].Intervals) {
            memcpy(
                const_cast<char*>(target.Data()) + interval.OffsetInBlock,
                interval.Data.data(),
                interval.Data.size()
            );
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

class TReadDataVisitor final
    : public IFreshBlockVisitor
    , public IMixedBlockVisitor
    , public IFreshBytesVisitor
{
private:
    TTxIndexTablet::TReadData& Args;
    bool ApplyingByteLayer = false;

public:
    TReadDataVisitor(TTxIndexTablet::TReadData& args)
        : Args(args)
    {
        Y_VERIFY(Args.AlignedByteRange.IsAligned());
    }

    void Accept(const TBlock& block, TStringBuf blockData) override
    {
        Y_VERIFY(!ApplyingByteLayer);

        ui32 blockOffset = block.BlockIndex - Args.AlignedByteRange.FirstBlock();
        Y_VERIFY(blockOffset < Args.AlignedByteRange.BlockCount());

        auto& prev = Args.Blocks[blockOffset];
        if (Update(prev, block, {}, 0)) {
            Args.Buffer->SetBlock(blockOffset, blockData);
        }
    }

    void Accept(
        const TBlock& block,
        const TPartialBlobId& blobId,
        ui32 blobOffset) override
    {
        Y_VERIFY(!ApplyingByteLayer);
        Y_VERIFY(blobId);

        ui32 blockOffset = block.BlockIndex - Args.AlignedByteRange.FirstBlock();
        Y_VERIFY(blockOffset < Args.AlignedByteRange.BlockCount());

        auto& prev = Args.Blocks[blockOffset];
        if (Update(prev, block, blobId, blobOffset)) {
            Args.Buffer->ClearBlock(blockOffset);
        }
    }

    void Accept(const TBytes& bytes, TStringBuf data) override
    {
        ApplyingByteLayer = true;

        const auto firstBlockOffset =
            Args.AlignedByteRange.FirstBlock() * Args.AlignedByteRange.BlockSize;
        ui64 i = 0;

        while (i < bytes.Length) {
            auto offset = bytes.Offset + i;
            auto relOffset = offset - firstBlockOffset;
            auto blockIndex = relOffset / Args.AlignedByteRange.BlockSize;
            auto offsetInBlock = relOffset - blockIndex * Args.AlignedByteRange.BlockSize;
            // FreshBytes should be organized in such a way that newer commits
            // for the same bytes will be visited later than older commits, so
            // tracking individual byte commit ids is not needed
            auto& prev = Args.Blocks[blockIndex];
            auto next = Min<ui32>(
                bytes.Length,
                (blockIndex + 1) * Args.AlignedByteRange.BlockSize
            );
            if (prev.MinCommitId < bytes.MinCommitId) {
                Args.Bytes[blockIndex].Intervals.push_back({
                    IntegerCast<ui32>(offsetInBlock),
                    TString(data.Data() + i, next - i)
                });
            }

            i = next;
        }
    }

private:
    bool Update(
        TBlockDataRef& prev,
        const TBlock& block,
        const TPartialBlobId& blobId,
        ui32 blobOffset)
    {
        if (prev.MinCommitId < block.MinCommitId) {
            memcpy(&prev, &block, sizeof(TBlock));
            prev.BlobId = blobId;
            prev.BlobOffset = blobOffset;
            return true;
        }
        return false;
    }
};

////////////////////////////////////////////////////////////////////////////////

class TReadDataActor final
    : public TActorBootstrapped<TReadDataActor>
{
private:
    const TActorId Tablet;
    const TRequestInfoPtr RequestInfo;
    const ui64 CommitId;
    const TByteRange OriginByteRange;
    const TByteRange AlignedByteRange;
    const ui64 TotalSize;
    const TVector<TBlockDataRef> Blocks;
    TVector<TBlockBytes> Bytes;
    const IBlockBufferPtr Buffer;

public:
    TReadDataActor(
        const TActorId& tablet,
        TRequestInfoPtr requestInfo,
        ui64 commitId,
        TByteRange originByteRange,
        TByteRange alignedByteRange,
        ui64 totalSize,
        TVector<TBlockDataRef> blocks,
        TVector<TBlockBytes> bytes,
        IBlockBufferPtr buffer);

    void Bootstrap(const TActorContext& ctx);

private:
    STFUNC(StateWork);

    void ReadBlob(const TActorContext& ctx);
    void HandleReadBlobResponse(
        const TEvIndexTabletPrivate::TEvReadBlobResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandlePoisonPill(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx);

    void ReplyAndDie(
        const TActorContext& ctx,
        const NProto::TError& error = {});
};

////////////////////////////////////////////////////////////////////////////////

TReadDataActor::TReadDataActor(
        const TActorId& tablet,
        TRequestInfoPtr requestInfo,
        ui64 commitId,
        TByteRange originByteRange,
        TByteRange alignedByteRange,
        ui64 totalSize,
        TVector<TBlockDataRef> blocks,
        TVector<TBlockBytes> bytes,
        IBlockBufferPtr buffer)
    : Tablet(tablet)
    , RequestInfo(std::move(requestInfo))
    , CommitId(commitId)
    , OriginByteRange(originByteRange)
    , AlignedByteRange(alignedByteRange)
    , TotalSize(totalSize)
    , Blocks(std::move(blocks))
    , Bytes(std::move(bytes))
    , Buffer(std::move(buffer))
{
    ActivityType = TFileStoreActivities::TABLET_WORKER;
    Y_VERIFY_DEBUG(AlignedByteRange.IsAligned());
}

void TReadDataActor::Bootstrap(const TActorContext& ctx)
{
    FILESTORE_TRACK(
        RequestReceived_TabletWorker,
        RequestInfo->CallContext,
        "ReadData");

    ReadBlob(ctx);
    Become(&TThis::StateWork);
}

void TReadDataActor::ReadBlob(const TActorContext& ctx)
{
    using TBlocksByBlob = THashMap<
        TPartialBlobId,
        TVector<TReadBlob::TBlock>,
        TPartialBlobIdHash
    >;

    TBlocksByBlob blocksByBlob;

    ui32 blockOffset = 0;
    for (const auto& block: Blocks) {
        ++blockOffset;

        if (!block.BlobId) {
            continue;
        }

        blocksByBlob[block.BlobId].emplace_back(block.BlobOffset, blockOffset - 1);
    }

    auto request = std::make_unique<TEvIndexTabletPrivate::TEvReadBlobRequest>(
        RequestInfo->CallContext);
    request->Buffer = Buffer;

    auto comparer = [] (const auto& l, const auto& r) {
        return l.BlobOffset < r.BlobOffset;
    };

    for (auto& [blobId, blocks]: blocksByBlob) {
        Sort(blocks, comparer);
        request->Blobs.emplace_back(blobId, std::move(blocks));
    }

    NCloud::Send(ctx, Tablet, std::move(request));
}

void TReadDataActor::HandleReadBlobResponse(
    const TEvIndexTabletPrivate::TEvReadBlobResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    ApplyBytes(AlignedByteRange, std::move(Bytes), *Buffer);
    ReplyAndDie(ctx, msg->GetError());
}

void TReadDataActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);
    ReplyAndDie(ctx, MakeError(E_REJECTED, "request cancelled"));
}

void TReadDataActor::ReplyAndDie(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    {
        // notify tablet
        auto response = std::make_unique<TEvIndexTabletPrivate::TEvReadDataCompleted>(error);
        response->CommitId = CommitId;
        NCloud::Send(ctx, Tablet, std::move(response));
    }

    FILESTORE_TRACK(
        ResponseSent_TabletWorker,
        RequestInfo->CallContext,
        "ReadData");

    if (RequestInfo->Sender != Tablet) {
        // reply to caller
        auto response = std::make_unique<TEvService::TEvReadDataResponse>(error);
        if (SUCCEEDED(error.GetCode())) {
            CopyFileData(
                OriginByteRange,
                AlignedByteRange,
                TotalSize,
                Buffer->GetContentRef(),
                response->Record.MutableBuffer());
        }

        NCloud::Reply(ctx, *RequestInfo, std::move(response));
    }

    Die(ctx);
}

STFUNC(TReadDataActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        HFunc(TEvIndexTabletPrivate::TEvReadBlobResponse, HandleReadBlobResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TFileStoreComponents::TABLET_WORKER);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleReadData(
    const TEvService::TEvReadDataRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_VALIDATE_EVENT_SESSION(ReadData, msg->Record);

    const auto& sessionId = GetSessionId(msg->Record);
    ui64 handle = msg->Record.GetHandle();

    const TByteRange byteRange(
        msg->Record.GetOffset(),
        msg->Record.GetLength(),
        GetBlockSize()
    );

    auto alignedByteRange = byteRange.AlignedSuperRange();
    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] ReadData @%lu %s -> %s",
        TabletID(),
        sessionId.Quote().c_str(),
        handle,
        byteRange.Describe().c_str(),
        alignedByteRange.Describe().c_str());

    FILESTORE_TRACK(
        RequestReceived_Tablet,
        msg->CallContext,
        "ReadData");

    auto error = ValidateDataRequest(byteRange);
    if (FAILED(error.GetCode())) {
        FILESTORE_TRACK(
            ResponseSent_Tablet,
            msg->CallContext,
            "ReadData");

        auto response = std::make_unique<TEvService::TEvReadDataResponse>(error);
        NCloud::Reply(ctx, *ev, std::move(response));

        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    auto blockBuffer = CreateBlockBuffer(alignedByteRange);

    ExecuteTx<TReadData>(
        ctx,
        std::move(requestInfo),
        msg->Record,
        byteRange,
        alignedByteRange,
        std::move(blockBuffer));
}

void TIndexTabletActor::HandleReadDataCompleted(
    const TEvIndexTabletPrivate::TEvReadDataCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    if (FAILED(msg->GetError().GetCode())) {
        LOG_ERROR(ctx, TFileStoreComponents::TABLET,
            "[%lu] ReadData failed (%s)",
            TabletID(),
            FormatError(msg->GetError()).c_str());
    } else {
        LOG_TRACE(ctx, TFileStoreComponents::TABLET,
            "[%lu] ReadData completed (%s)",
            TabletID(),
            FormatError(msg->GetError()).c_str());
    }

    ReleaseCollectBarrier(msg->CommitId);

    WorkerActors.erase(ev->Sender);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_ReadData(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TReadData& args)
{
    Y_UNUSED(ctx);

    auto* session = FindSession(args.ClientId, args.SessionId);
    if (!session) {
        args.Error = ErrorInvalidSession(args.ClientId, args.SessionId);
        return true;
    }

    auto* handle = FindHandle(args.Handle);
    if (!handle || handle->Session != session) {
        args.Error = ErrorInvalidHandle(args.Handle);
        return true;
    }

    if (!HasFlag(handle->GetFlags(), NProto::TCreateHandleRequest::E_READ)) {
        args.Error = ErrorInvalidHandle(args.Handle);
        return true;
    }

    args.NodeId = handle->GetNodeId();
    args.CommitId = handle->GetCommitId();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] ReadNodeData @%lu [%lu] %s",
        TabletID(),
        session->GetSessionId().Quote().c_str(),
        args.Handle,
        args.NodeId,
        args.AlignedByteRange.Describe().c_str());

    if (args.CommitId == InvalidCommitId) {
        args.CommitId = GetCurrentCommitId();
    }

    TIndexTabletDatabase db(tx.DB);

    bool ready = true;
    if (!ReadNode(db, args.NodeId, args.CommitId, args.Node)) {
        ready = false;
    } else {
        Y_VERIFY(args.Node);
        // TODO: access check
    }

    TSet<ui32> rangeIds;
    SplitRange(
        args.AlignedByteRange.FirstBlock(),
        args.AlignedByteRange.BlockCount(),
        BlockGroupSize,
        [&] (ui32 blockOffset, ui32 blocksCount) {
            rangeIds.insert(GetMixedRangeIndex(
                args.NodeId,
                IntegerCast<ui32>(args.AlignedByteRange.FirstBlock() + blockOffset),
                blocksCount));
        });

    for (ui32 rangeId: rangeIds) {
        if (!LoadMixedBlocks(db, rangeId)) {
            ready = false;
        }
    }

    return ready;
}

void TIndexTabletActor::ExecuteTx_ReadData(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TReadData& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);

    FILESTORE_VALIDATE_TX_ERROR(ReadData, args);

    TReadDataVisitor visitor(args);

    FindFreshBlocks(
        visitor,
        args.NodeId,
        args.CommitId,
        args.AlignedByteRange.FirstBlock(),
        args.AlignedByteRange.BlockCount());

    SplitRange(
        args.AlignedByteRange.FirstBlock(),
        args.AlignedByteRange.BlockCount(),
        BlockGroupSize,
        [&] (ui32 blockOffset, ui32 blocksCount) {
            FindMixedBlocks(
                visitor,
                args.NodeId,
                args.CommitId,
                IntegerCast<ui32>(args.AlignedByteRange.FirstBlock() + blockOffset),
                blocksCount);
        });

    FindFreshBytes(
        visitor,
        args.NodeId,
        args.CommitId,
        args.AlignedByteRange);
}

void TIndexTabletActor::CompleteTx_ReadData(
    const TActorContext& ctx,
    TTxIndexTablet::TReadData& args)
{
    if (FAILED(args.Error.GetCode())) {
        LOG_ERROR(ctx, TFileStoreComponents::TABLET,
            "[%lu] ReadData failed (%s)",
            TabletID(),
            FormatError(args.Error).c_str());

        FILESTORE_TRACK(
            ResponseSent_Tablet,
            args.RequestInfo->CallContext,
            "ReadData");

        auto response = std::make_unique<TEvService::TEvReadDataResponse>(args.Error);
        NCloud::Reply(ctx, *args.RequestInfo, std::move(response));

        return;
    }

    if (!ShouldReadBlobs(args.Blocks)) {
        LOG_TRACE(ctx, TFileStoreComponents::TABLET,
            "[%lu] ReadData completed (fresh)",
            TabletID());

        ApplyBytes(args.AlignedByteRange, std::move(args.Bytes), *args.Buffer);

        auto response = std::make_unique<TEvService::TEvReadDataResponse>();
        CopyFileData(
            args.OriginByteRange,
            args.AlignedByteRange,
            args.Node->Attrs.GetSize(),
            args.Buffer->GetContentRef(),
            response->Record.MutableBuffer());

        FILESTORE_TRACK(
            ResponseSent_Tablet,
            args.RequestInfo->CallContext,
            "ReadData");

        NCloud::Reply(ctx, *args.RequestInfo, std::move(response));

        return;
    }

    AcquireCollectBarrier(args.CommitId);

    auto actor = std::make_unique<TReadDataActor>(
        ctx.SelfID,
        args.RequestInfo,
        args.CommitId,
        args.OriginByteRange,
        args.AlignedByteRange,
        args.Node->Attrs.GetSize(),
        std::move(args.Blocks),
        std::move(args.Bytes),
        std::move(args.Buffer));

    auto actorId = NCloud::Register(ctx, std::move(actor));
    WorkerActors.insert(actorId);
}

}   // namespace NCloud::NFileStore::NStorage
