#include "part2_actor.h"

#include <cloud/blockstore/libs/diagnostics/critical_events.h>
#include <cloud/blockstore/libs/kikimr/helpers.h>

#include <cloud/storage/core/libs/common/alloc.h>

#include <ydb/core/base/blobstorage.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>
#include <library/cpp/actors/core/hfunc.h>

namespace NCloud::NBlockStore::NStorage::NPartition2 {

using namespace NActors;

using namespace NKikimr;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

class TReadBlobActor final
    : public TActorBootstrapped<TReadBlobActor>
{
    using TRequest = TEvPartitionPrivate::TEvReadBlobRequest;
    using TResponse = TEvPartitionPrivate::TEvReadBlobResponse;

private:
    const TRequestInfoPtr RequestInfo;

    const TActorId Tablet;
    const ui64 TabletId;
    const ui32 BlockSize;
    const EStorageAccessMode StorageAccessMode;
    const std::unique_ptr<TRequest> Request;

    TInstant RequestSent;
    TInstant ResponseReceived;

public:
    TReadBlobActor(
        TRequestInfoPtr requestInfo,
        const TActorId& tablet,
        ui64 tabletId,
        ui32 blockSize,
        const EStorageAccessMode storageAccessMode,
        std::unique_ptr<TRequest> request);

    void Bootstrap(const TActorContext& ctx);

private:
    void SendGetRequest(const TActorContext& ctx);

    void NotifyCompleted(const TActorContext& ctx, const NProto::TError& error);

    void ReplyAndDie(
        const TActorContext& ctx,
        std::unique_ptr<TResponse> response);

    void ReplyError(
        const TActorContext& ctx,
        const TEvBlobStorage::TEvGetResult& response,
        const TString& description);

private:
    STFUNC(StateWork);

    void HandleGetResult(
        const TEvBlobStorage::TEvGetResult::TPtr& ev,
        const TActorContext& ctx);

    void HandlePoisonPill(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TReadBlobActor::TReadBlobActor(
        TRequestInfoPtr requestInfo,
        const TActorId& tablet,
        ui64 tabletId,
        ui32 blockSize,
        const EStorageAccessMode storageAccessMode,
        std::unique_ptr<TRequest> request)
    : RequestInfo(std::move(requestInfo))
    , Tablet(tablet)
    , TabletId(tabletId)
    , BlockSize(blockSize)
    , StorageAccessMode(storageAccessMode)
    , Request(std::move(request))
{
    ActivityType = TBlockStoreActivities::PARTITION_WORKER;
}

void TReadBlobActor::Bootstrap(const TActorContext& ctx)
{
    TRequestScope timer(*RequestInfo);

    Become(&TThis::StateWork);

    LWTRACK(
        RequestReceived_PartitionWorker,
        RequestInfo->CallContext->LWOrbit,
        "ReadBlob",
        RequestInfo->CallContext->RequestId);

    SendGetRequest(ctx);
}

void TReadBlobActor::SendGetRequest(const TActorContext& ctx)
{
    using TEvGetQuery = TEvBlobStorage::TEvGet::TQuery;

    size_t blocksCount = Request->BlobOffsets.size();

    TArrayHolder<TEvGetQuery> queries(new TEvGetQuery[blocksCount]);
    size_t queriesCount = 0;

    for (size_t i = 0; i < blocksCount; ++i) {
        if (i && Request->BlobOffsets[i] == Request->BlobOffsets[i-1] + 1) {
            // extend range
            queries[queriesCount-1].Size += BlockSize;
        } else {
            queries[queriesCount++].Set(
                Request->BlobId,
                Request->BlobOffsets[i] * BlockSize,
                BlockSize);
        }
    }

    auto request = std::make_unique<TEvBlobStorage::TEvGet>(
        queries,
        queriesCount,
        Request->Deadline,
        Request->Async
            ? NKikimrBlobStorage::AsyncRead
            : NKikimrBlobStorage::FastRead);

    auto traceId = RequestInfo->TraceId.Clone();
    BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

    request->Orbit = std::move(RequestInfo->CallContext->LWOrbit);

    RequestSent = ctx.Now();

    SendToBSProxy(
        ctx,
        Request->Proxy,
        request.release(),
        0,  // cookie
        std::move(traceId));
}

void TReadBlobActor::NotifyCompleted(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    auto request = std::make_unique<TEvPartitionPrivate::TEvReadBlobCompleted>(
        error);
    request->BlobId = Request->BlobId;
    request->BytesCount = Request->BlobOffsets.size() * BlockSize;
    request->RequestTime = ResponseReceived - RequestSent;
    request->GroupId = Request->GroupId;

    NCloud::Send(ctx, Tablet, std::move(request));
}

void TReadBlobActor::ReplyAndDie(
    const TActorContext& ctx,
    std::unique_ptr<TResponse> response)
{
    NotifyCompleted(ctx, response->GetError());

    BLOCKSTORE_TRACE_SENT(ctx, &RequestInfo->TraceId, this, response);

    if (ResponseReceived) {
        LWTRACK(
            ResponseSent_Partition,
            RequestInfo->CallContext->LWOrbit,
            "ReadBlob",
            RequestInfo->CallContext->RequestId);
    }

    NCloud::Reply(ctx, *RequestInfo, std::move(response));
    Die(ctx);
}

void TReadBlobActor::ReplyError(
    const TActorContext& ctx,
    const TEvBlobStorage::TEvGetResult& response,
    const TString& description)
{
    LOG_ERROR(ctx, TBlockStoreComponents::PARTITION,
        "[%lu] TEvBlobStorage::TEvGet failed: %s\n%s",
        TabletId,
        description.data(),
        response.Print(false).data());

    auto error = MakeError(E_REJECTED, "TEvBlobStorage::TEvGet failed: " + description);
    ReplyAndDie(ctx, std::make_unique<TResponse>(error));
}

////////////////////////////////////////////////////////////////////////////////

void TReadBlobActor::HandleGetResult(
    const TEvBlobStorage::TEvGetResult::TPtr& ev,
    const TActorContext& ctx)
{
    ResponseReceived = ctx.Now();

    auto* msg = ev->Get();

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, msg, &ev->TraceId);

    RequestInfo->CallContext->LWOrbit = std::move(msg->Orbit);

    if (msg->Status != NKikimrProto::OK) {
        ReplyError(ctx, *msg, msg->ErrorReason);
        return;
    }

    auto blobId = Request->BlobId;
    size_t blocksCount = Request->BlobOffsets.size();

    if (auto guard = Request->Sglist.Acquire()) {
        const auto& sglist = guard.Get();
        size_t sglistIndex = 0;

        for (size_t i = 0; i < msg->ResponseSz; ++i) {
            auto& response = msg->Responses[i];

            if (response.Status != NKikimrProto::OK) {
                if (ShouldRepair(response.Status)
                        && StorageAccessMode == EStorageAccessMode::Repair)
                {
                    LOG_WARN(ctx, TBlockStoreComponents::PARTITION,
                        "[%lu] Repairing TEvBlobStorage::TEvGet %s error (%s)",
                        TabletId,
                        NKikimrProto::EReplyStatus_Name(response.Status).data(),
                        msg->Print(false).data());

                    const auto marker = GetBrokenDataMarker();
                    auto& block = sglist[sglistIndex];
                    Y_VERIFY(block.Data());
                    memcpy(
                        const_cast<char*>(block.Data()),
                        marker.Data(),
                        Min(block.Size(), marker.Size())
                    );
                    ++sglistIndex;

                    while (sglistIndex < sglist.size()) {
                        const auto offset = Request->BlobOffsets[sglistIndex];
                        const auto prevOffset = Request->BlobOffsets[sglistIndex - 1];
                        if (offset != prevOffset + 1) {
                            break;
                        }

                        auto& block = sglist[sglistIndex];
                        Y_VERIFY(block.Data());
                        memcpy(
                            const_cast<char*>(block.Data()),
                            marker.Data(),
                            Min(block.Size(), marker.Size())
                        );

                        ++sglistIndex;
                    }

                    continue;
                } else {
                    ReplyError(ctx, *msg, "read error");
                    return;
                }
            }

            if (response.Id != blobId ||
                response.Buffer.empty() ||
                response.Buffer.size() % BlockSize != 0)
            {
                ReplyError(ctx, *msg, "invalid response received");
                return;
            }

            char* head = response.Buffer.begin();
            for (size_t offset = 0; offset < response.Buffer.size(); offset += BlockSize) {
                if (sglistIndex >= sglist.size()) {
                    ReplyError(ctx, *msg, "response is out of range");
                    return;
                }

                Y_VERIFY(sglist[sglistIndex].Size() == BlockSize);
                void* to = const_cast<char*>(sglist[sglistIndex].Data());
                memcpy(to, head + offset, BlockSize);
                ++sglistIndex;
            }
        }

        if (sglistIndex != blocksCount) {
            ReplyError(ctx, *msg, "invalid response received");
            return;
        }
    } else {
        auto error =
            MakeError(E_REJECTED, "TReadBlobActor::HandleGetResult failed");
        ReplyAndDie(ctx, std::make_unique<TResponse>(error));
        return;
    }

    auto response = std::make_unique<TResponse>();
    response->ExecCycles = RequestInfo->GetExecCycles();
    ReplyAndDie(ctx, std::move(response));
}

void TReadBlobActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    auto response = std::make_unique<TEvPartitionPrivate::TEvReadBlobResponse>(
        MakeError(E_REJECTED, "Tablet is dead"));

    ReplyAndDie(ctx, std::move(response));
}

STFUNC(TReadBlobActor::StateWork)
{
    TRequestScope timer(*RequestInfo);

    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        HFunc(TEvBlobStorage::TEvGetResult, HandleGetResult);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::PARTITION_WORKER);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TPartitionActor::HandleReadBlob(
    const TEvPartitionPrivate::TEvReadBlobRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto msg = ev->Release();

    auto requestInfo = CreateRequestInfo<TEvPartitionPrivate::TReadBlobMethod>(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    TRequestScope timer(*requestInfo);

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    LWTRACK(
        RequestReceived_Partition,
        requestInfo->CallContext->LWOrbit,
        "ReadBlob",
        requestInfo->CallContext->RequestId);

    const auto blob = msg->BlobId;

    auto readBlobActor = std::make_unique<TReadBlobActor>(
        requestInfo,
        SelfId(),
        TabletID(),
        State->GetBlockSize(),
        StorageAccessMode,
        std::unique_ptr<TEvPartitionPrivate::TEvReadBlobRequest>(msg.Release()));

    if (blob.TabletID() != TabletID()) {
        // Treat this situation as we were reading from base disk.
        // TODO: verify that |blobTabletId| corresponds to base disk partition
        // tablet.
        auto actorId = NCloud::Register(ctx, std::move(readBlobActor));
        Actors.insert(actorId);
        return;
    }

    ui32 channel = blob.Channel();
    State->EnqueueIORequest(channel, std::move(readBlobActor));
    ProcessIOQueue(ctx, channel);
}

void TPartitionActor::HandleReadBlobCompleted(
    const TEvPartitionPrivate::TEvReadBlobCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    Actors.erase(ev->Sender);

    const auto& blobTabletId = msg->BlobId.TabletID();

    if (FAILED(msg->GetStatus())) {
        if (blobTabletId != TabletID()) {
            // Treat this situation as we were reading from base disk.
            // TODO: verify that |blobTabletId| corresponds to base disk
            // partition tablet.
            LOG_DEBUG(
                ctx,
                TBlockStoreComponents::PARTITION,
                "[%lu] Failed to read blob from base disk, blob tablet: %lu error: %s",
                TabletID(),
                blobTabletId,
                FormatError(msg->GetError()).data());
            return;
        }

        if (State->IncrementReadBlobErrorCount()
                >= Config->GetMaxReadBlobErrorsBeforeSuicide())
        {
            LOG_WARN(ctx, TBlockStoreComponents::PARTITION,
                "[%lu] Stop tablet because of too many ReadBlob errors: %s",
                TabletID(),
                FormatError(msg->GetError()).data());

            ReportTabletBSFailure();
            Suicide(ctx);
        } else {
            LOG_WARN(ctx, TBlockStoreComponents::PARTITION,
                "[%lu] ReadBlob error happened: %s",
                TabletID(),
                FormatError(msg->GetError()).data());
        }
        return;
    }

    const ui32 channel = msg->BlobId.Channel();
    const ui32 group = msg->GroupId;
    const bool isOverlayDisk = blobTabletId != TabletID();
    Y_VERIFY(group != Max<ui32>());

    UpdateReadThroughput(ctx, channel, group, msg->BytesCount, isOverlayDisk);
    UpdateNetworkStats(ctx, msg->BytesCount);
    UpdateExecutorStats(ctx);

    if (blobTabletId != TabletID()) {
        // Treat this situation as we were reading from base disk.
        // TODO: verify that |blobTabletId| corresponds to base disk partition
        // tablet.
        return;
    }

    PartCounters->RequestCounters.ReadBlob.AddRequest(msg->RequestTime.MicroSeconds(), msg->BytesCount);

    State->CompleteIORequest(channel);

    ProcessIOQueue(ctx, channel);
}

}   // namespace NCloud::NBlockStore::NStorage::NPartition2
