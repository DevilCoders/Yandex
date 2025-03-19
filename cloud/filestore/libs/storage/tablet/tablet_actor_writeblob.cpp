#include "tablet_actor.h"

#include "helpers.h"

#include <cloud/filestore/libs/diagnostics/critical_events.h>

#include <ydb/core/base/blobstorage.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/stream/str.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TWriteBlobRequest
{
    TActorId Proxy;
    TLogoBlobID BlobId;
    std::unique_ptr<TEvBlobStorage::TEvPut> Request;

    TWriteBlobRequest(
            const TActorId& proxy,
            const TLogoBlobID& blobId,
            std::unique_ptr<TEvBlobStorage::TEvPut> request)
        : Proxy(proxy)
        , BlobId(blobId)
        , Request(std::move(request))
    {}
};

////////////////////////////////////////////////////////////////////////////////

TString DumpBlobIds(const TVector<TWriteBlobRequest>& requests)
{
    TStringStream out;

    out << requests[0].BlobId;
    for (size_t i = 1; i < requests.size(); ++i) {
        out << ", " << requests[i].BlobId;
    }

    return out.Str();
}

////////////////////////////////////////////////////////////////////////////////

class TWriteBlobActor final
    : public TActorBootstrapped<TWriteBlobActor>
{
private:
    const ui64 TabletId;
    const TActorId Tablet;
    const TRequestInfoPtr RequestInfo;

    TVector<TWriteBlobRequest> Requests;

    using TWriteResult =
        TEvIndexTabletPrivate::TWriteBlobCompleted::TWriteRequestResult;
    TVector<TWriteResult> WriteResults;

public:
    TWriteBlobActor(
        ui64 tabletId,
        const TActorId& tablet,
        TRequestInfoPtr requestInfo,
        TVector<TWriteBlobRequest> requests);

    void Bootstrap(const TActorContext& ctx);

private:
    STFUNC(StateWork);

    void SendRequests(const TActorContext& ctx);
    void HandlePutResult(
        const TEvBlobStorage::TEvPutResult::TPtr& ev,
        const TActorContext& ctx);

    void HandlePoisonPill(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx);

    void ReplyError(
        const TActorContext& ctx,
        const TEvBlobStorage::TEvPutResult& response,
        const TString reason);

    void ReplyAndDie(
        const TActorContext& ctx,
        const NProto::TError& error = {});
};

////////////////////////////////////////////////////////////////////////////////

TWriteBlobActor::TWriteBlobActor(
        ui64 tabletId,
        const TActorId& tablet,
        TRequestInfoPtr requestInfo,
        TVector<TWriteBlobRequest> requests)
    : TabletId(tabletId)
    , Tablet(tablet)
    , RequestInfo(std::move(requestInfo))
    , Requests(std::move(requests))
{
    ActivityType = TFileStoreActivities::TABLET_WORKER;
}

void TWriteBlobActor::Bootstrap(const TActorContext& ctx)
{
    FILESTORE_TRACK(
        RequestReceived_TabletWorker,
        RequestInfo->CallContext,
        "WriteBlob");

    SendRequests(ctx);
    Become(&TThis::StateWork);
}

void TWriteBlobActor::SendRequests(const TActorContext& ctx)
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

void TWriteBlobActor::HandlePutResult(
    const TEvBlobStorage::TEvPutResult::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    RequestInfo->CallContext->LWOrbit.Join(msg->Orbit);

    if (msg->Status != NKikimrProto::OK) {
        ReplyError(ctx, *msg, msg->ErrorReason);
        return;
    }

    WriteResults.push_back({
        msg->Id,
        msg->StatusFlags,
        msg->ApproximateFreeSpaceShare,
    });

    Y_VERIFY(WriteResults.size() <= Requests.size());
    if (WriteResults.size() == Requests.size()) {
        ReplyAndDie(ctx);
    }
}

void TWriteBlobActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);
    ReplyAndDie(ctx, MakeError(E_REJECTED, "request cancelled"));
}

void TWriteBlobActor::ReplyError(
    const TActorContext& ctx,
    const TEvBlobStorage::TEvPutResult& response,
    const TString reason)
{
      LOG_ERROR(ctx, TFileStoreComponents::TABLET,
          "[%lu] TEvBlobStorage::TEvPut failed: %s\n%s",
          TabletId,
          reason.c_str(),
          response.Print(false).data());

      auto error = MakeError(E_REJECTED, "TEvBlobStorage::TEvPut failed: " + reason);
      ReplyAndDie(ctx, error);
}

void TWriteBlobActor::ReplyAndDie(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    {
        // notify tablet
        auto response = std::make_unique<TEvIndexTabletPrivate::TEvWriteBlobCompleted>(error);
        response->Results = std::move(WriteResults);
        NCloud::Send(ctx, Tablet, std::move(response));
    }

    FILESTORE_TRACK(
        ResponseSent_TabletWorker,
        RequestInfo->CallContext,
        "WriteBlob");

    if (RequestInfo->Sender != Tablet) {
        // reply to caller
        auto response = std::make_unique<TEvIndexTabletPrivate::TEvWriteBlobResponse>(error);
        NCloud::Reply(ctx, *RequestInfo, std::move(response));
    }

    Die(ctx);
}

STFUNC(TWriteBlobActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        HFunc(TEvBlobStorage::TEvPutResult, HandlePutResult);

        default:
            HandleUnexpectedEvent(ctx, ev, TFileStoreComponents::TABLET_WORKER);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleWriteBlob(
    const TEvIndexTabletPrivate::TEvWriteBlobRequest::TPtr& ev,
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
        requestInfo->CallContext,
        "WriteBlob");

    TVector<TWriteBlobRequest> requests(Reserve(msg->Blobs.size()));
    for (auto& blob: msg->Blobs) {
        if (blob.BlobContent.size() >= MaxBlobStorageBlobSize) {
            TStringStream ss;
            ss << blob.BlobId;

            LOG_ERROR(ctx, TFileStoreComponents::TABLET,
                "[%lu] Too large blob [%s] %lu",
                TabletID(),
                ss.Str().c_str(),
                blob.BlobContent.size());

            ReportTabletBSFailure();
            Suicide(ctx);
            return;
        }

        auto blobId = MakeBlobId(TabletID(), blob.BlobId);

        auto proxy = Info()->BSProxyIDForChannel(
            blob.BlobId.Channel(),
            blob.BlobId.Generation());

        auto request = std::make_unique<TEvBlobStorage::TEvPut>(
            blobId,
            std::move(blob.BlobContent),
            blob.Deadline,
            blob.Async
                ? NKikimrBlobStorage::AsyncBlob
                : NKikimrBlobStorage::UserData);

        if (!msg->CallContext->LWOrbit.Fork(request->Orbit)) {
            FILESTORE_TRACK(
                ForkFailed,
                msg->CallContext,
                "TEvBlobStorage::TEvPut");
        }

        requests.emplace_back(proxy, blobId, std::move(request));
    }

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] WriteBlob started (%s)",
        TabletID(),
        DumpBlobIds(requests).c_str());

    auto actor = std::make_unique<TWriteBlobActor>(
        TabletID(),
        ctx.SelfID,
        std::move(requestInfo),
        std::move(requests));

    auto actorId = NCloud::Register(ctx, std::move(actor));
    WorkerActors.insert(actorId);
}

void TIndexTabletActor::HandleWriteBlobCompleted(
    const TEvIndexTabletPrivate::TEvWriteBlobCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] WriteBlob completed (%s)",
        TabletID(),
        FormatError(msg->GetError()).c_str());

    WorkerActors.erase(ev->Sender);

    const auto validFlag = NKikimrBlobStorage::EStatusFlags::StatusIsValid;
    for (const auto& result: msg->Results) {
        if (result.StorageStatusFlags.Check(validFlag)) {
            if (result.StorageStatusFlags.Check(NKikimrBlobStorage::StatusDiskSpaceLightYellowMove)) {
                RegisterChannelToMove(result.BlobId.Channel());
            }
            if (result.StorageStatusFlags.Check(NKikimrBlobStorage::StatusDiskSpaceYellowStop)) {
                RegisterUnwritableChannel(result.BlobId.Channel());
            }

            ReassignDataChannelsIfNeeded(ctx);
        }
    }

    if (FAILED(msg->GetStatus())) {
        LOG_WARN(ctx, TFileStoreComponents::TABLET,
            "[%lu] Stop tablet because of WriteBlob error: %s",
            TabletID(),
            FormatError(msg->GetError()).data());

        ReportTabletBSFailure();
        Suicide(ctx);
        return;
    }
}

}   // namespace NCloud::NFileStore::NStorage
