#include "part_actor.h"

#include <cloud/blockstore/libs/common/iovector.h>
#include <cloud/blockstore/libs/diagnostics/block_digest.h>
#include <cloud/blockstore/libs/diagnostics/critical_events.h>
#include <cloud/blockstore/libs/diagnostics/profile_log.h>
#include <cloud/blockstore/libs/storage/core/block_handler.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/probes.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>
#include <cloud/blockstore/libs/storage/core/write_buffer_request.h>

#include <cloud/storage/core/libs/api/hive_proxy.h>
#include <cloud/storage/core/libs/common/alloc.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/generic/vector.h>
#include <util/string/builder.h>

namespace NCloud::NBlockStore::NStorage::NPartition {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

using namespace NCloud::NStorage;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

IWriteBlocksHandlerPtr CreateWriteHandler(
    const TBlockRange64& writeRange,
    std::unique_ptr<TEvService::TEvWriteBlocksRequest> request,
    ui32 blockSize)
{
    return CreateWriteBlocksHandler(
        writeRange,
        std::move(request),
        blockSize);
}

IWriteBlocksHandlerPtr CreateWriteHandler(
    const TBlockRange64& writeRange,
    std::unique_ptr<TEvService::TEvWriteBlocksLocalRequest> request,
    ui32 blockSize)
{
    Y_UNUSED(blockSize);
    return CreateWriteBlocksHandler(
        writeRange,
        std::move(request));
}

TGuardedSgList GetSglist(const NProto::TWriteBlocksRequest& request)
{
    return TGuardedSgList(GetSgList(request));
}

const TGuardedSgList& GetSglist(const NProto::TWriteBlocksLocalRequest& request)
{
    return request.Sglist;
}

template <typename ...T>
IEventBasePtr CreateWriteBlocksResponse(bool replyLocal, T&& ...args)
{
    if (replyLocal) {
        return std::make_unique<TEvService::TEvWriteBlocksLocalResponse>(
            std::forward<T>(args)...);
    } else {
        return std::make_unique<TEvService::TEvWriteBlocksResponse>(
            std::forward<T>(args)...);
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TPartitionActor::HandleWriteBlocks(
    const TEvService::TEvWriteBlocksRequest::TPtr& ev,
    const TActorContext& ctx)
{
    HandleWriteBlocksRequest<TEvService::TWriteBlocksMethod>(
        ev, ctx, false);
}

void TPartitionActor::HandleWriteBlocksLocal(
    const TEvService::TEvWriteBlocksLocalRequest::TPtr& ev,
    const TActorContext& ctx)
{
    HandleWriteBlocksRequest<TEvService::TWriteBlocksLocalMethod>(
        ev, ctx, true);
}

template <typename TMethod>
void TPartitionActor::HandleWriteBlocksRequest(
    const typename TMethod::TRequest::TPtr& ev,
    const TActorContext& ctx,
    bool replyLocal)
{
    auto msg = ev->Release();

    auto requestInfo = CreateRequestInfo<TMethod>(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    TRequestScope timer(*requestInfo);

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    LWTRACK(
        RequestReceived_Partition,
        requestInfo->CallContext->LWOrbit,
        "WriteBlocks",
        requestInfo->CallContext->RequestId);

    auto sglist = GetSglist(msg->Record);

    ui32 blocksCount = 0;

    if (auto guard = sglist.Acquire()) {
        for (const auto& buffer: guard.Get()) {
            if (!buffer.Size() || buffer.Size() % State->GetBlockSize() != 0) {
                auto response = std::make_unique<typename TMethod::TResponse>(
                    MakeError(E_ARGUMENT, TStringBuilder()
                        << "invalid buffer length: " << buffer.Size()));

                BLOCKSTORE_TRACE_SENT(ctx, &requestInfo->TraceId, this, response);

                LWTRACK(
                    ResponseSent_Partition,
                    requestInfo->CallContext->LWOrbit,
                    "WriteBlocks",
                    requestInfo->CallContext->RequestId);

                NCloud::Reply(ctx, *requestInfo, std::move(response));
                return;
            }

            blocksCount += buffer.Size() / State->GetBlockSize();
        }
    } else {
        auto response = std::make_unique<typename TMethod::TResponse>(
            MakeError(E_REJECTED, "failed to acquire input buffer"));

        BLOCKSTORE_TRACE_SENT(ctx, &requestInfo->TraceId, this, response);

        LWTRACK(
            ResponseSent_Partition,
            requestInfo->CallContext->LWOrbit,
            "WriteBlocks",
            requestInfo->CallContext->RequestId);

        NCloud::Reply(ctx, *requestInfo, std::move(response));
        return;
    }

    TBlockRange64 writeRange;

    auto ok = InitReadWriteBlockRange(
        msg->Record.GetStartIndex(),
        blocksCount,
        &writeRange
    );

    if (!ok) {
        auto response = std::make_unique<typename TMethod::TResponse>(
            MakeError(E_ARGUMENT, TStringBuilder()
                << "invalid block range ["
                << "index: " << msg->Record.GetStartIndex()
                << ", count: " << blocksCount
                << "]"));

        BLOCKSTORE_TRACE_SENT(ctx, &requestInfo->TraceId, this, response);

        LWTRACK(
            ResponseSent_Partition,
            requestInfo->CallContext->LWOrbit,
            "WriteBlocks",
            requestInfo->CallContext->RequestId);

        NCloud::Reply(ctx, *requestInfo, std::move(response));
        return;
    }

    auto writeHandler = CreateWriteHandler(
        writeRange,
        std::unique_ptr<typename TMethod::TRequest>(msg.Release()),
        State->GetBlockSize());

    WriteBlocks(
        ctx,
        requestInfo,
        ConvertRangeSafe(writeRange),
        std::move(writeHandler),
        replyLocal
    );
}

void TPartitionActor::WriteBlocks(
    const TActorContext& ctx,
    TRequestInfoPtr requestInfo,
    const TBlockRange32& writeRange,
    IWriteBlocksHandlerPtr writeHandler,
    bool replyLocal)
{
    auto replyError = [=] (const TActorContext& ctx, NProto::TError error) {
        LOG_DEBUG(ctx, TBlockStoreComponents::PARTITION,
            "[%lu] WriteBlocks error: %s",
            TabletID(),
            error.GetMessage().c_str());

        auto response = CreateWriteBlocksResponse(replyLocal, std::move(error));

        BLOCKSTORE_TRACE_SENT(ctx, &requestInfo->TraceId, this, response);

        LWTRACK(
            ResponseSent_Partition,
            requestInfo->CallContext->LWOrbit,
            "WriteBlocks",
            requestInfo->CallContext->RequestId);

        NCloud::Reply(ctx, *requestInfo, std::move(response));
    };

    if (!State->IsWriteAllowed(EChannelPermission::UserWritesAllowed)) {
        replyError(ctx, MakeError(E_BS_OUT_OF_SPACE, "insufficient disk space"));

        ReassignChannelsIfNeeded(ctx);

        return;
    }

    // RejectProbability is broken in our case (KIKIMR-10194)
    // if (Executor()->GetRejectProbability() >= 0.95) {
    //     replyError(ctx, MakeError(E_REJECTED, "rejected by tablet executor"));
    //
    //     return;
    // }

    ++WriteAndZeroRequestsInProgress;

    TRequestInBuffer<TWriteBufferRequestData> requestInBuffer{
        writeRange.Size(),
        {
            std::move(requestInfo),
            writeRange,
            std::move(writeHandler),
            replyLocal
        }
    };

    const auto requestSize = writeRange.Size() * State->GetBlockSize();
    const auto writeBlobThreshold =
        GetWriteBlobThreshold(*Config, PartitionConfig.GetStorageMediaKind());

    if (requestSize < writeBlobThreshold) {
        if (Config->GetWriteRequestBatchingEnabled()) {
            // we will try to batch small writes and, if batching fails,
            // we will accumulate these writes in FreshBlocks table
            EnqueueProcessWriteQueueIfNeeded(ctx);

            LOG_TRACE(ctx, TBlockStoreComponents::PARTITION,
                "[%lu] Enqueueing fresh blocks (range: %s)",
                TabletID(),
                DescribeRange(writeRange).data()
            );
            State->GetWriteBuffer().Put(std::move(requestInBuffer));
        } else {
            WriteFreshBlocks(ctx, std::move(requestInBuffer));
        }
    } else {
        // large writes could skip FreshBlocks table completely
        WriteMergedBlocks(ctx, std::move(requestInBuffer));
    }
}

void TPartitionActor::HandleWriteBlocksCompleted(
    const TEvPartitionPrivate::TEvWriteBlocksCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    ui64 commitId = msg->CommitId;
    LOG_TRACE(ctx, TBlockStoreComponents::PARTITION,
        "[%lu] Complete write blocks @%lu",
        TabletID(),
        commitId);

    UpdateStats(msg->Stats);

    ui64 blocksCount = msg->Stats.GetUserWriteCounters().GetBlocksCount();
    ui64 requestBytes = blocksCount * State->GetBlockSize();

    UpdateNetworkStat(ctx.Now(), requestBytes);
    UpdateCPUUsageStat(CyclesToDurationSafe(msg->ExecCycles).MicroSeconds());
    UpdateExecutorStats(ctx);

    auto time = CyclesToDurationSafe(msg->TotalCycles).MicroSeconds();
    const auto requestCount =
        msg->Stats.GetUserWriteCounters().GetRequestsCount();
    PartCounters->RequestCounters.WriteBlocks.AddRequest(
        time,
        requestBytes,
        requestCount
    );

    if (msg->AffectedBlockInfos) {
        IProfileLog::TReadWriteRequestBlockInfos request;
        request.RequestType = EBlockStoreRequest::WriteBlocks;
        request.BlockInfos = std::move(msg->AffectedBlockInfos);
        request.CommitId = commitId;

        IProfileLog::TRecord record;
        record.DiskId = State->GetConfig().GetDiskId();
        record.Ts = ctx.Now();
        record.Request = std::move(request);

        ProfileLog->Write(std::move(record));
    }

    State->GetCommitQueue().ReleaseBarrier(commitId);
    if (msg->CollectGarbageBarrierAcquired) {
        State->GetGarbageQueue().ReleaseBarrier(commitId);
    }

    Actors.erase(ev->Sender);

    if (Executor()->GetStats().IsAnyChannelYellowMove) {
        ScheduleYellowStateUpdate(ctx);
    }

    Y_VERIFY_DEBUG(WriteAndZeroRequestsInProgress >= requestCount);
    WriteAndZeroRequestsInProgress -= requestCount;

    ProcessDrainRequests(ctx);
    ProcessCommitQueue(ctx);
    EnqueueFlushIfNeeded(ctx);
}

}   // namespace NCloud::NBlockStore::NStorage::NPartition
