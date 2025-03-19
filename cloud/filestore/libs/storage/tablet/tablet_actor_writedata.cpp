#include "tablet_actor.h"

#include "helpers.h"

#include <cloud/filestore/libs/storage/tablet/model/blob_builder.h>
#include <cloud/filestore/libs/storage/tablet/model/split_range.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TWriteDataActor final
    : public TActorBootstrapped<TWriteDataActor>
{
private:
    const TActorId Tablet;
    const TRequestInfoPtr RequestInfo;
    const ui64 CommitId;
    /*const*/ TVector<TMergedBlob> Blobs;
    const TWriteRange WriteRange;

public:
    TWriteDataActor(
        const TActorId& tablet,
        TRequestInfoPtr requestInfo,
        ui64 commitId,
        TVector<TMergedBlob> blobs,
        TWriteRange writeRange);

    void Bootstrap(const TActorContext& ctx);

private:
    STFUNC(StateWork);

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

TWriteDataActor::TWriteDataActor(
        const TActorId& tablet,
        TRequestInfoPtr requestInfo,
        ui64 commitId,
        TVector<TMergedBlob> blobs,
        TWriteRange writeRange)
    : Tablet(tablet)
    , RequestInfo(std::move(requestInfo))
    , CommitId(commitId)
    , Blobs(std::move(blobs))
    , WriteRange(writeRange)
{
    ActivityType = TFileStoreActivities::TABLET_WORKER;
}

void TWriteDataActor::Bootstrap(const TActorContext& ctx)
{
    FILESTORE_TRACK(
        RequestReceived_TabletWorker,
        RequestInfo->CallContext,
        "WriteData");

    WriteBlob(ctx);
    Become(&TThis::StateWork);
}

void TWriteDataActor::WriteBlob(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvIndexTabletPrivate::TEvWriteBlobRequest>(
        RequestInfo->CallContext
    );

    for (auto& blob: Blobs) {
        request->Blobs.emplace_back(blob.BlobId, std::move(blob.BlobContent));
    }

    NCloud::Send(ctx, Tablet, std::move(request));
}

void TWriteDataActor::HandleWriteBlobResponse(
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

void TWriteDataActor::AddBlob(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvIndexTabletPrivate::TEvAddBlobRequest>(
        RequestInfo->CallContext
    );
    request->Mode = EAddBlobMode::Write;
    request->WriteRanges.push_back(WriteRange);

    for (const auto& blob: Blobs) {
        request->MergedBlobs.emplace_back(
            blob.BlobId,
            blob.Block,
            blob.BlocksCount);
    }

    NCloud::Send(ctx, Tablet, std::move(request));
}

void TWriteDataActor::HandleAddBlobResponse(
    const TEvIndexTabletPrivate::TEvAddBlobResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    ReplyAndDie(ctx, msg->GetError());
}

void TWriteDataActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);
    ReplyAndDie(ctx, MakeError(E_REJECTED, "request cancelled"));
}

void TWriteDataActor::ReplyAndDie(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    {
        // notify tablet
        auto response = std::make_unique<TEvIndexTabletPrivate::TEvWriteDataCompleted>(error);
        response->CommitId = CommitId;
        NCloud::Send(ctx, Tablet, std::move(response));
    }

    FILESTORE_TRACK(
        ResponseSent_TabletWorker,
        RequestInfo->CallContext,
        "WriteData");

    if (RequestInfo->Sender != Tablet) {
        // reply to caller
        auto response = std::make_unique<TEvService::TEvWriteDataResponse>(error);
        NCloud::Reply(ctx, *RequestInfo, std::move(response));
    }

    Die(ctx);
}

STFUNC(TWriteDataActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        HFunc(TEvIndexTabletPrivate::TEvWriteBlobResponse, HandleWriteBlobResponse);
        HFunc(TEvIndexTabletPrivate::TEvAddBlobResponse, HandleAddBlobResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TFileStoreComponents::TABLET_WORKER);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleWriteData(
    const TEvService::TEvWriteDataRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    FILESTORE_VALIDATE_EVENT_SESSION(WriteData, msg->Record);

    const auto& sessionId = GetSessionId(msg->Record);
    ui64 handle = msg->Record.GetHandle();
    auto& buffer = *msg->Record.MutableBuffer();
    TByteRange byteRange(
        msg->Record.GetOffset(),
        buffer.size(),
        GetBlockSize()
    );

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%s] WriteData @%lu %s",
        TabletID(),
        sessionId.Quote().c_str(),
        handle,
        byteRange.Describe().c_str());

    FILESTORE_TRACK(
        RequestReceived_Tablet,
        msg->CallContext,
        "WriteData");

    auto replyError = [&] (NProto::TError error) {
        FILESTORE_TRACK(
            ResponseSent_Tablet,
            msg->CallContext,
            "WriteData");

        using TResponse = TEvService::TEvWriteDataResponse;
        auto response = std::make_unique<TResponse>(std::move(error));
        NCloud::Reply(ctx, *ev, std::move(response));
    };

    auto error = ValidateDataRequest(byteRange);
    if (FAILED(error.GetCode())) {
        replyError(std::move(error));
        return;
    }

    if (!IsWriteAllowed(BuildBackpressureThresholds())) {
        replyError(MakeError(E_REJECTED, "rejected due to backpressure"));
        return;
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    auto blockBuffer = CreateBlockBuffer(byteRange, std::move(buffer));

    if (Config->GetWriteBatchEnabled()) {
        auto request = std::make_unique<TWriteRequest>(
            std::move(requestInfo),
            msg->Record,
            byteRange,
            std::move(blockBuffer));

        EnqueueWriteBatch(ctx, std::move(request));
        return;
    }

    ExecuteTx<TWriteData>(
        ctx,
        std::move(requestInfo),
        Config->GetWriteBlobThreshold(),
        msg->Record,
        byteRange,
        std::move(blockBuffer));
}

void TIndexTabletActor::HandleWriteDataCompleted(
    const TEvIndexTabletPrivate::TEvWriteDataCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    if (FAILED(msg->GetError().GetCode())) {
        LOG_ERROR(ctx, TFileStoreComponents::TABLET,
            "[%lu] WriteData failed (%s)",
            TabletID(),
            FormatError(msg->GetError()).c_str());
    } else {
        LOG_TRACE(ctx, TFileStoreComponents::TABLET,
            "[%lu] WriteData completed (%s)",
            TabletID(),
            FormatError(msg->GetError()).c_str());
    }

    ReleaseCollectBarrier(msg->CommitId);

    WorkerActors.erase(ev->Sender);
    EnqueueBlobIndexOpIfNeeded(ctx);
}

////////////////////////////////////////////////////////////////////////////////

bool TIndexTabletActor::PrepareTx_WriteData(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TWriteData& args)
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

    if (!HasFlag(handle->GetFlags(), NProto::TCreateHandleRequest::E_WRITE)) {
        args.Error = ErrorInvalidHandle(args.Handle);
        return true;
    }

    args.NodeId = handle->GetNodeId();
    args.CommitId = GetCurrentCommitId();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] [%lu] WriteNodeData @%lu %s",
        TabletID(),
        args.NodeId,
        args.CommitId,
        args.ByteRange.Describe().c_str());

    TIndexTabletDatabase db(tx.DB);

    bool ready = true;
    if (!ReadNode(db, args.NodeId, args.CommitId, args.Node)) {
        ready = false;
    }

    TSet<ui32> rangeIds;
    SplitRange(
        args.ByteRange.FirstBlock(),
        args.ByteRange.BlockCount(),
        BlockGroupSize,
        [&] (ui32 blockOffset, ui32 blocksCount) {
            rangeIds.insert(GetMixedRangeIndex(
                args.NodeId,
                IntegerCast<ui32>(args.ByteRange.FirstBlock() + blockOffset),
                blocksCount));
        });

    for (ui32 rangeId: rangeIds) {
        if (!LoadMixedBlocks(db, rangeId)) {
            ready = false;
        }
    }

    if (!ready) {
        return false;
    }

    // TODO: access check
    Y_VERIFY(args.Node);
    if (!HasSpaceLeft(args.Node->Attrs, args.ByteRange.End())) {
        args.Error = ErrorNoSpaceLeft();
        return true;
    }

    return true;
}

void TIndexTabletActor::ExecuteTx_WriteData(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxIndexTablet::TWriteData& args)
{
    Y_UNUSED(ctx);

    FILESTORE_VALIDATE_TX_ERROR(WriteData, args);

    if (args.ShouldWriteBlob()) {
        return;
    }

    TIndexTabletDatabase db(tx.DB);

    args.CommitId = GenerateCommitId();

    // XXX mark head and tail?

    MarkFreshBlocksDeleted(
        db,
        args.NodeId,
        args.CommitId,
        args.ByteRange.FirstAlignedBlock(),
        args.ByteRange.AlignedBlockCount());

    SplitRange(
        args.ByteRange.FirstAlignedBlock(),
        args.ByteRange.AlignedBlockCount(),
        BlockGroupSize,
        [&] (ui32 blockOffset, ui32 blocksCount) {
            MarkMixedBlocksDeleted(
                db,
                args.NodeId,
                args.CommitId,
                args.ByteRange.FirstAlignedBlock() + blockOffset,
                blocksCount);
        });

    for (ui64 b = args.ByteRange.FirstAlignedBlock();
            b < args.ByteRange.FirstAlignedBlock() + args.ByteRange.AlignedBlockCount();
            ++b)
    {
        WriteFreshBlock(
            db,
            args.NodeId,
            args.CommitId,
            b,
            args.Buffer->GetBlock(b - args.ByteRange.FirstAlignedBlock()));
    }

    if (args.ByteRange.UnalignedHeadLength()) {
        WriteFreshBytes(
            db,
            args.NodeId,
            args.CommitId,
            args.ByteRange.Offset,
            args.Buffer->GetUnalignedHead()
        );
    }

    if (args.ByteRange.UnalignedTailLength()) {
        WriteFreshBytes(
            db,
            args.NodeId,
            args.CommitId,
            args.ByteRange.UnalignedTailOffset(),
            args.Buffer->GetUnalignedTail()
        );
    }

    auto attrs = CopyAttrs(args.Node->Attrs, E_CM_MTIME);
    if (args.ByteRange.End() > args.Node->Attrs.GetSize()) {
        attrs.SetSize(args.ByteRange.End());
    }

    UpdateNode(
        db,
        args.NodeId,
        args.Node->MinCommitId,
        args.CommitId,
        attrs,
        args.Node->Attrs);
}

void TIndexTabletActor::CompleteTx_WriteData(
    const TActorContext& ctx,
    TTxIndexTablet::TWriteData& args)
{
    auto reply = [] (
        const TActorContext& ctx,
        TTxIndexTablet::TWriteData& args)
    {
        FILESTORE_TRACK(
            ResponseSent_Tablet,
            args.RequestInfo->CallContext,
            "WriteData");

        auto response =
            std::make_unique<TEvService::TEvWriteDataResponse>(args.Error);
        NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
    };

    if (FAILED(args.Error.GetCode())) {
        LOG_ERROR(ctx, TFileStoreComponents::TABLET,
            "[%lu] WriteData failed (%s)",
            TabletID(),
            FormatError(args.Error).c_str());

        reply(ctx, args);
        return;
    }

    if (!args.ShouldWriteBlob()) {
        LOG_TRACE(ctx, TFileStoreComponents::TABLET,
            "[%lu] WriteData completed (fresh)",
            TabletID());

        FILESTORE_TRACK(
            ResponseSent_Tablet,
            args.RequestInfo->CallContext,
            "WriteData");

        auto response = std::make_unique<TEvService::TEvWriteDataResponse>();
        NCloud::Reply(ctx, *args.RequestInfo, std::move(response));

        EnqueueFlushIfNeeded(ctx);
        EnqueueBlobIndexOpIfNeeded(ctx);
        return;
    }

    TMergedBlobBuilder builder(GetBlockSize());

    SplitRange(
        args.ByteRange.FirstAlignedBlock(),
        args.ByteRange.AlignedBlockCount(),
        BlockGroupSize,
        [&] (ui32 blockOffset, ui32 blocksCount) {
            TBlock block {
                args.NodeId,
                IntegerCast<ui32>(
                    args.ByteRange.FirstAlignedBlock() + blockOffset
                ),
                // correct CommitId will be assigned later in AddBlobs
                InvalidCommitId,
                InvalidCommitId
            };

            builder.Accept(block, blocksCount, blockOffset, *args.Buffer);
        });

    auto blobs = builder.Finish();
    Y_VERIFY(blobs);

    args.CommitId = GenerateCommitId();

    ui32 blobIndex = 0;
    for (auto& blob: blobs) {
        const auto ok = GenerateBlobId(
            args.CommitId,
            blob.BlobContent.size(),
            blobIndex++,
            &blob.BlobId);

        if (!ok) {
            ReassignDataChannelsIfNeeded(ctx);

            args.Error = MakeError(E_FS_OUT_OF_SPACE, "failed to generate blobId");
            reply(ctx, args);

            return;
        }
    }

    AcquireCollectBarrier(args.CommitId);

    auto actor = std::make_unique<TWriteDataActor>(
        ctx.SelfID,
        args.RequestInfo,
        args.CommitId,
        std::move(blobs),
        TWriteRange{args.NodeId, args.ByteRange.End()});

    auto actorId = NCloud::Register(ctx, std::move(actor));
    WorkerActors.insert(actorId);
}

}   // namespace NCloud::NFileStore::NStorage
