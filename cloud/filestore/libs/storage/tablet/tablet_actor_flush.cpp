#include "tablet_actor.h"

#include <cloud/filestore/libs/diagnostics/profile_log.h>
#include <cloud/filestore/libs/storage/tablet/model/blob_builder.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TFlushActor final
    : public TActorBootstrapped<TFlushActor>
{
private:
    const TActorId Tablet;
    const TString FileSystemId;
    const TRequestInfoPtr RequestInfo;
    const ui64 CommitId;
    const ui32 BlockSize;
    const IProfileLogPtr ProfileLog;
    /*const*/ TVector<TMixedBlob> Blobs;

    NProto::TProfileLogRequestInfo ProfileLogRequest;

public:
    TFlushActor(
        const TActorId& tablet,
        TInstant flushStartTs,
        TString fileSystemId,
        TRequestInfoPtr requestInfo,
        ui64 commitId,
        ui32 blockSize,
        IProfileLogPtr profileLog,
        TVector<TMixedBlob> blobs);

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

TFlushActor::TFlushActor(
        const TActorId& tablet,
        TInstant flushStartTs,
        TString fileSystemId,
        TRequestInfoPtr requestInfo,
        ui64 commitId,
        ui32 blockSize,
        IProfileLogPtr profileLog,
        TVector<TMixedBlob> blobs)
    : Tablet(tablet)
    , FileSystemId(std::move(fileSystemId))
    , RequestInfo(std::move(requestInfo))
    , CommitId(commitId)
    , BlockSize(blockSize)
    , ProfileLog(std::move(profileLog))
    , Blobs(std::move(blobs))
{
    ActivityType = TFileStoreActivities::TABLET_WORKER;

    ProfileLogRequest.SetTimestampMcs(flushStartTs.MicroSeconds());
}

void TFlushActor::Bootstrap(const TActorContext& ctx)
{
    FILESTORE_TRACK(
        RequestReceived_TabletWorker,
        RequestInfo->CallContext,
        "Flush");

    InitProfileLogByteRanges(BlockSize, Blobs, ProfileLogRequest);

    WriteBlob(ctx);
    Become(&TThis::StateWork);
}

void TFlushActor::WriteBlob(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvIndexTabletPrivate::TEvWriteBlobRequest>(
        RequestInfo->CallContext
    );

    for (auto& blob: Blobs) {
        request->Blobs.emplace_back(blob.BlobId, std::move(blob.BlobContent));
    }

    NCloud::Send(ctx, Tablet, std::move(request));
}

void TFlushActor::HandleWriteBlobResponse(
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

void TFlushActor::AddBlob(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvIndexTabletPrivate::TEvAddBlobRequest>(
        RequestInfo->CallContext
    );
    request->Mode = EAddBlobMode::Flush;

    for (auto& blob: Blobs) {
        request->MixedBlobs.emplace_back(blob.BlobId, std::move(blob.Blocks));
    }

    NCloud::Send(ctx, Tablet, std::move(request));
}

void TFlushActor::HandleAddBlobResponse(
    const TEvIndexTabletPrivate::TEvAddBlobResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    ReplyAndDie(ctx, msg->GetError());
}

void TFlushActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);
    ReplyAndDie(ctx, MakeError(E_REJECTED, "request cancelled"));
}

void TFlushActor::ReplyAndDie(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    // log request
    ProfileLogRequest.SetDurationMcs(
        ctx.Now().MicroSeconds() - ProfileLogRequest.GetTimestampMcs());
    ProfileLogRequest.SetRequestType(static_cast<ui32>(
        EFileStoreSystemRequest::Flush));

    ProfileLog->Write({FileSystemId, std::move(ProfileLogRequest)});

    {
        // notify tablet
        auto response = std::make_unique<TEvIndexTabletPrivate::TEvFlushCompleted>(error);
        response->CommitId = CommitId;
        NCloud::Send(ctx, Tablet, std::move(response));
    }

    FILESTORE_TRACK(
        ResponseSent_TabletWorker,
        RequestInfo->CallContext,
        "Flush");

    if (RequestInfo->Sender != Tablet) {
        // reply to caller
        auto response = std::make_unique<TEvIndexTabletPrivate::TEvFlushResponse>(error);
        NCloud::Reply(ctx, *RequestInfo, std::move(response));
    }

    Die(ctx);
}

STFUNC(TFlushActor::StateWork)
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

void TIndexTabletActor::EnqueueFlushIfNeeded(const TActorContext& ctx)
{
    auto freshBlocksDataSize = GetFreshBlocksCount() * GetBlockSize();
    if (freshBlocksDataSize < Config->GetFlushThreshold()) {
        return;
    }

    if (FlushState.Enqueue()) {
        ctx.Send(SelfId(), new TEvIndexTabletPrivate::TEvFlushRequest());
    }
}

void TIndexTabletActor::HandleFlush(
    const TEvIndexTabletPrivate::TEvFlushRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto flushStartTs = ctx.Now();

    auto* msg = ev->Get();

    FILESTORE_TRACK(
        RequestReceived_Tablet,
        msg->CallContext,
        "Flush");

    auto replyError = [] (
        const TActorContext& ctx,
        auto& ev,
        const NProto::TError& error)
    {
        FILESTORE_TRACK(
            ResponseSent_Tablet,
            ev.Get()->CallContext,
            "Flush");

        if (ev.Sender != ctx.SelfID) {
            // reply to caller
            auto response = std::make_unique<TEvIndexTabletPrivate::TEvFlushResponse>(error);
            NCloud::Reply(ctx, ev, std::move(response));
        }
    };

    if (!FlushState.Start()) {
        replyError(ctx, *ev, MakeError(S_ALREADY, "flush is in progress"));
        return;
    }

    TMixedBlobBuilder builder(
        GetRangeIdHasher(),
        GetBlockSize(),
        CalculateMaxBlocksInBlob(Config->GetMaxBlobSize(), GetBlockSize()));

    FindFreshBlocks(builder);

    auto blobs = builder.Finish();
    if (!blobs) {
        replyError(ctx, *ev, MakeError(S_FALSE, "nothing to flush"));
        FlushState.Complete();

        return;
    }

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] Flush started (%u blocks in %u blobs)",
        TabletID(),
        builder.GetBlocksCount(),
        builder.GetBlobsCount());

    ui64 commitId = GenerateCommitId();

    ui32 blobIndex = 0;
    for (auto& blob: blobs) {
        const auto ok = GenerateBlobId(
            commitId,
            blob.BlobContent.size(),
            blobIndex++,
            &blob.BlobId);

        if (!ok) {
            ReassignDataChannelsIfNeeded(ctx);

            replyError(
                ctx,
                *ev,
                MakeError(E_FS_OUT_OF_SPACE, "failed to generate blobId"));

            FlushState.Complete();

            return;
        }
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    AcquireCollectBarrier(commitId);

    auto actor = std::make_unique<TFlushActor>(
        ctx.SelfID,
        flushStartTs,
        GetFileSystemId(),
        std::move(requestInfo),
        commitId,
        GetBlockSize(),
        ProfileLog,
        std::move(blobs));

    auto actorId = NCloud::Register(ctx, std::move(actor));
    WorkerActors.insert(actorId);
}

void TIndexTabletActor::HandleFlushCompleted(
    const TEvIndexTabletPrivate::TEvFlushCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] Flush completed (%s)",
        TabletID(),
        FormatError(msg->GetError()).c_str());

    ReleaseCollectBarrier(msg->CommitId);
    FlushState.Complete();

    WorkerActors.erase(ev->Sender);
    EnqueueFlushIfNeeded(ctx);
    EnqueueBlobIndexOpIfNeeded(ctx);
}

}   // namespace NCloud::NFileStore::NStorage
