#include "tablet_actor.h"

#include "helpers.h"

#include <ydb/core/base/blobstorage.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/stream/str.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TReadBlockRange
{
    ui32 BlockOffset = 0;
    ui32 Count = 0;
};

////////////////////////////////////////////////////////////////////////////////

struct TReadBlobRequest
{
    TActorId Proxy;
    TLogoBlobID BlobId;
    TVector<TReadBlockRange> Ranges;
    std::unique_ptr<TEvBlobStorage::TEvGet> Request;

    TReadBlobRequest(
            const TActorId& proxy,
            const TLogoBlobID& blobId,
            TVector<TReadBlockRange> ranges,
            std::unique_ptr<TEvBlobStorage::TEvGet> request)
        : Proxy(proxy)
        , BlobId(blobId)
        , Ranges(std::move(ranges))
        , Request(std::move(request))
    {}
};

////////////////////////////////////////////////////////////////////////////////

TString DumpBlobIds(const TVector<TReadBlobRequest>& requests)
{
    TStringStream out;

    out << requests[0].BlobId;
    for (size_t i = 1; i < requests.size(); ++i) {
        out << ", " << requests[i].BlobId;
    }

    return out.Str();
}

////////////////////////////////////////////////////////////////////////////////

class TReadBlobActor final
    : public TActorBootstrapped<TReadBlobActor>
{
private:
    const ui64 TabletId;
    const TActorId Tablet;
    const TRequestInfoPtr RequestInfo;
    const IBlockBufferPtr Buffer;
    const ui32 BlockSize;

    TVector<TReadBlobRequest> Requests;
    size_t RequestsCompleted = 0;

public:
    TReadBlobActor(
        ui64 tabletId,
        const TActorId& tablet,
        TRequestInfoPtr requestInfo,
        IBlockBufferPtr buffer,
        ui32 blockSize,
        TVector<TReadBlobRequest> requests);

    void Bootstrap(const TActorContext& ctx);

private:
    STFUNC(StateWork);

    void SendRequests(const TActorContext& ctx);
    void HandleGetResult(
        const TEvBlobStorage::TEvGetResult::TPtr& ev,
        const TActorContext& ctx);

    void HandlePoisonPill(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx);

    void ReplyAndDie(
        const TActorContext& ctx,
        const NProto::TError& error = {});

    void ReplyError(
        const TActorContext& ctx,
        const TEvBlobStorage::TEvGetResult& response,
        const TString& reason);
};

////////////////////////////////////////////////////////////////////////////////

TReadBlobActor::TReadBlobActor(
        ui64 tabletId,
        const TActorId& tablet,
        TRequestInfoPtr requestInfo,
        IBlockBufferPtr buffer,
        ui32 blockSize,
        TVector<TReadBlobRequest> requests)
    : TabletId(tabletId)
    , Tablet(tablet)
    , RequestInfo(std::move(requestInfo))
    , Buffer(std::move(buffer))
    , BlockSize(blockSize)
    , Requests(std::move(requests))
{
    ActivityType = TFileStoreActivities::TABLET_WORKER;
}

void TReadBlobActor::Bootstrap(const TActorContext& ctx)
{
    FILESTORE_TRACK(
        RequestReceived_TabletWorker,
        RequestInfo->CallContext,
        "ReadBlob");

    SendRequests(ctx);
    Become(&TThis::StateWork);
}

void TReadBlobActor::SendRequests(const TActorContext& ctx)
{
    size_t requestIndex = 0;
    for (auto& request: Requests) {
        SendToBSProxy(
            ctx,
            request.Proxy,
            request.Request.release(),
            requestIndex++);
    }
}

void TReadBlobActor::HandleGetResult(
    const TEvBlobStorage::TEvGetResult::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    if (msg->Status != NKikimrProto::OK) {
        ReplyError(ctx, *msg, msg->ErrorReason);
        return;
    }

    RequestInfo->CallContext->LWOrbit.Join(msg->Orbit);

    size_t requestIndex = ev->Cookie;
    Y_VERIFY(requestIndex < Requests.size());

    const auto& request = Requests[requestIndex];

    auto rangeIt = request.Ranges.begin();
    Y_VERIFY(rangeIt != request.Ranges.end());

    for (size_t i = 0; i < msg->ResponseSz; ++i) {
        auto& response = msg->Responses[i];

        if (response.Status != NKikimrProto::OK) {
            ReplyError(ctx, *msg, "read error");
            return;
        }

        if (response.Id != request.BlobId ||
            response.Buffer.empty() ||
            response.Buffer.size() % BlockSize != 0)
        {
            ReplyError(ctx, *msg, "invalid response received");
            return;
        }

        ui32 blocksCount = response.Buffer.size() / BlockSize;
        ui32 rangeOffset = 0;

        const char* ptr = response.Buffer.begin();

        for (size_t j = 0; j < blocksCount; ++j) {
            size_t inRange = j - rangeOffset;
            if (inRange >= rangeIt->Count) {
                ++rangeIt;
                rangeOffset = j;
                inRange = 0;
            }

            Buffer->SetBlock(
                rangeIt->BlockOffset + inRange,
                TStringBuf(ptr, BlockSize));
            ptr += BlockSize;
        }

        Y_VERIFY(blocksCount - rangeOffset == rangeIt->Count);
        ++rangeIt;
    }

    Y_VERIFY(rangeIt == request.Ranges.end());

    Y_VERIFY(RequestsCompleted < Requests.size());
    if (++RequestsCompleted == Requests.size()) {
        ReplyAndDie(ctx);
    }
}

void TReadBlobActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);
    ReplyAndDie(ctx, MakeError(E_REJECTED, "request cancelled"));
}

void TReadBlobActor::ReplyError(
        const TActorContext& ctx,
        const TEvBlobStorage::TEvGetResult& response,
        const TString& message)
{
    LOG_ERROR(ctx, TFileStoreComponents::TABLET,
        "[%lu] TEvBlobStorage::TEvGet failed: %s\n%s",
        TabletId,
        message.data(),
        response.Print(false).data());

    auto error = MakeError(E_REJECTED, "TEvBlobStorage::TEvGet failed: " + message);
    ReplyAndDie(ctx, error);
}

void TReadBlobActor::ReplyAndDie(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    {
        // notify tablet
        auto response = std::make_unique<TEvIndexTabletPrivate::TEvReadBlobCompleted>(error);
        NCloud::Send(ctx, Tablet, std::move(response));
    }

    FILESTORE_TRACK(
        ResponseSent_TabletWorker,
        RequestInfo->CallContext,
        "ReadBlob");

    if (RequestInfo->Sender != Tablet) {
        // reply to caller
        auto response = std::make_unique<TEvIndexTabletPrivate::TEvReadBlobResponse>(error);
        NCloud::Reply(ctx, *RequestInfo, std::move(response));
    }

    Die(ctx);
}

STFUNC(TReadBlobActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        HFunc(TEvBlobStorage::TEvGetResult, HandleGetResult);

        default:
            HandleUnexpectedEvent(ctx, ev, TFileStoreComponents::TABLET_WORKER);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleReadBlob(
    const TEvIndexTabletPrivate::TEvReadBlobRequest::TPtr& ev,
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
        "ReadBlob");

    ui32 blockSize = GetBlockSize();

    TVector<TReadBlobRequest> requests(Reserve(msg->Blobs.size()));
    for (auto& blob: msg->Blobs) {
        auto blobId = MakeBlobId(TabletID(), blob.BlobId);

        auto proxy = Info()->BSProxyIDForChannel(
            blob.BlobId.Channel(),
            blob.BlobId.Generation());

        ui32 blocksCount = blob.Blocks.size();

        using TEvGetQuery = TEvBlobStorage::TEvGet::TQuery;

        TArrayHolder<TEvGetQuery> queries(new TEvGetQuery[blocksCount]);
        size_t queriesCount = 0;

        TVector<TReadBlockRange> ranges;
        ranges.reserve(blocksCount);

        for (size_t i = 0; i < blocksCount; ++i) {
            const auto& curBlock = blob.Blocks[i];
            if (i && curBlock.BlobOffset == blob.Blocks[i - 1].BlobOffset + 1) {
                const auto& prevBlock = blob.Blocks[i - 1];

                // extend range
                queries[queriesCount - 1].Size += blockSize;
                if (curBlock.BlockOffset == prevBlock.BlockOffset + 1) {
                    ++ranges.back().Count;
                } else {
                    ranges.push_back({
                        curBlock.BlockOffset,
                        1
                    });
                }
            } else {
                queries[queriesCount++].Set(
                    blobId,
                    blob.Blocks[i].BlobOffset * blockSize,
                    blockSize);

                ranges.push_back({
                    blob.Blocks[i].BlockOffset,
                    1
                });
            }
        }

        auto request = std::make_unique<TEvBlobStorage::TEvGet>(
            queries,
            queriesCount,
            blob.Deadline,
            blob.Async
                ? NKikimrBlobStorage::AsyncRead
                : NKikimrBlobStorage::FastRead);

        if (!msg->CallContext->LWOrbit.Fork(request->Orbit)) {
            FILESTORE_TRACK(
                ForkFailed,
                msg->CallContext,
                "TEvBlobStorage::TEvGet");
        }

        requests.emplace_back(
            proxy,
            blobId,
            std::move(ranges),
            std::move(request));
    }

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] ReadBlob started (%s)",
        TabletID(),
        DumpBlobIds(requests).c_str());

    auto actor = std::make_unique<TReadBlobActor>(
        TabletID(),
        ctx.SelfID,
        std::move(requestInfo),
        std::move(msg->Buffer),
        blockSize,
        std::move(requests));

    auto actorId = NCloud::Register(ctx, std::move(actor));
    WorkerActors.insert(actorId);
}

void TIndexTabletActor::HandleReadBlobCompleted(
    const TEvIndexTabletPrivate::TEvReadBlobCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] ReadBlob completed (%s)",
        TabletID(),
        FormatError(msg->GetError()).c_str());

    WorkerActors.erase(ev->Sender);
}

}   // namespace NCloud::NFileStore::NStorage
