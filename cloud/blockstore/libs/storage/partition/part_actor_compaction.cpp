#include "part_actor.h"

#include <cloud/blockstore/libs/common/block_buffer.h>
#include <cloud/blockstore/libs/diagnostics/block_digest.h>
#include <cloud/blockstore/libs/diagnostics/profile_log.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

#include <cloud/storage/core/libs/common/alloc.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/generic/algorithm.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NCloud::NBlockStore::NStorage::NPartition {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TRangeCompactionInfo
{
    const TBlockRange32 BlockRange;
    const TPartialBlobId DataBlobId;
    const TBlockMask DataBlobSkipMask;
    const TPartialBlobId ZeroBlobId;
    const TBlockMask ZeroBlobSkipMask;
    const ui32 BlobsSkippedByCompaction;
    const ui32 BlocksSkippedByCompaction;

    TGuardedBuffer<TBlockBuffer> BlobContent;
    TVector<ui32> ZeroBlocks;
    TAffectedBlobs AffectedBlobs;
    TAffectedBlocks AffectedBlocks;

    TRangeCompactionInfo(
            TBlockRange32 blockRange,
            TPartialBlobId dataBlobId,
            TBlockMask dataBlobSkipMask,
            TPartialBlobId zeroBlobId,
            TBlockMask zeroBlobSkipMask,
            ui32 blobsSkippedByCompaction,
            ui32 blocksSkippedByCompaction,
            TBlockBuffer blobContent,
            TVector<ui32> zeroBlocks,
            TAffectedBlobs affectedBlobs,
            TAffectedBlocks affectedBlocks)
        : BlockRange(blockRange)
        , DataBlobId(dataBlobId)
        , DataBlobSkipMask(dataBlobSkipMask)
        , ZeroBlobId(zeroBlobId)
        , ZeroBlobSkipMask(zeroBlobSkipMask)
        , BlobsSkippedByCompaction(blobsSkippedByCompaction)
        , BlocksSkippedByCompaction(blocksSkippedByCompaction)
        , BlobContent(std::move(blobContent))
        , ZeroBlocks(std::move(zeroBlocks))
        , AffectedBlobs(std::move(affectedBlobs))
        , AffectedBlocks(std::move(affectedBlocks))
    {}
};

class TCompactionActor final
    : public TActorBootstrapped<TCompactionActor>
{
public:
    struct TRequest
    {
        TPartialBlobId BlobId;
        TActorId Proxy;
        ui16 BlobOffset;
        ui32 BlockIndex;
        size_t IndexInBlobContent;
        ui32 GroupId;
        ui32 RangeCompactionIndex;

        TRequest(const TPartialBlobId& blobId,
                 const TActorId& proxy,
                 ui16 blobOffset,
                 ui32 blockIndex,
                 size_t indexInBlobContent,
                 ui32 groupId,
                 ui32 rangeCompactionIndex)
            : BlobId(blobId)
            , Proxy(proxy)
            , BlobOffset(blobOffset)
            , BlockIndex(blockIndex)
            , IndexInBlobContent(indexInBlobContent)
            , GroupId(groupId)
            , RangeCompactionIndex(rangeCompactionIndex)
        {}
    };

    struct TBatchRequest
    {
        TPartialBlobId BlobId;
        TActorId Proxy;
        TVector<ui16> BlobOffsets;
        TVector<TRequest*> Requests;
        TRangeCompactionInfo* RangeCompactionInfo = nullptr;
        ui32 GroupId = 0;

        TBatchRequest() = default;

        TBatchRequest(const TPartialBlobId& blobId,
                      const TActorId& proxy,
                      TVector<ui16> blobOffsets,
                      TVector<TRequest*> requests,
                      TRangeCompactionInfo* rangeCompactionInfo,
                      ui32 groupId)
            : BlobId(blobId)
            , Proxy(proxy)
            , BlobOffsets(std::move(blobOffsets))
            , Requests(std::move(requests))
            , RangeCompactionInfo(rangeCompactionInfo)
            , GroupId(groupId)
        {}
    };

private:
    const TRequestInfoPtr RequestInfo;

    const ui64 TabletId;
    const TActorId Tablet;
    const ui32 BlockSize;
    const ui32 MaxBlocksInBlob;
    const ui32 MaxAffectedBlocksPerCompaction;
    const IBlockDigestGeneratorPtr BlockDigestGenerator;

    const ui64 CommitId;

    TVector<TRangeCompactionInfo> RangeCompactionInfos;
    TVector<TRequest> Requests;

    TVector<IProfileLog::TBlockInfo> AffectedBlockInfos;

    TVector<TBatchRequest> BatchRequests;
    size_t BatchRequestsCompleted = 0;
    size_t WriteBlobRequestsCompleted = 0;

    ui64 ReadExecCycles = 0;
    ui64 ReadWaitCycles = 0;

    TVector<TCallContextPtr> ForkedReadCallContexts;
    TVector<TCallContextPtr> ForkedWriteCallContexts;
    bool SafeToUseOrbit = true;

public:
    TCompactionActor(
        TRequestInfoPtr requestInfo,
        ui64 tabletId,
        const TActorId& tablet,
        ui32 blockSize,
        ui32 maxBlocksInBlob,
        ui32 maxAffectedBlocksPerCompaction,
        IBlockDigestGeneratorPtr blockDigestGenerator,
        ui64 commitId,
        TVector<TRangeCompactionInfo> rangeCompactionInfos,
        TVector<TRequest> requests);

    void Bootstrap(const TActorContext& ctx);

private:
    void InitBlockDigests();

    void ReadBlocks(const TActorContext& ctx);
    void WriteBlobs(const TActorContext& ctx);
    void AddBlobs(const TActorContext& ctx);

    void NotifyCompleted(const TActorContext& ctx, const NProto::TError& error);
    bool HandleError(const TActorContext& ctx, const NProto::TError& error);

    void ReplyAndDie(
        const TActorContext& ctx,
        std::unique_ptr<TEvPartitionPrivate::TEvCompactionResponse> response);

private:
    STFUNC(StateWork);

    void HandleReadBlobResponse(
        const TEvPartitionPrivate::TEvReadBlobResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleWriteBlobResponse(
        const TEvPartitionPrivate::TEvWriteBlobResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleAddBlobsResponse(
        const TEvPartitionPrivate::TEvAddBlobsResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandlePoisonPill(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TCompactionActor::TCompactionActor(
        TRequestInfoPtr requestInfo,
        ui64 tabletId,
        const TActorId& tablet,
        ui32 blockSize,
        ui32 maxBlocksInBlob,
        ui32 maxAffectedBlocksPerCompaction,
        IBlockDigestGeneratorPtr blockDigestGenerator,
        ui64 commitId,
        TVector<TRangeCompactionInfo> rangeCompactionInfos,
        TVector<TRequest> requests)
    : RequestInfo(std::move(requestInfo))
    , TabletId(tabletId)
    , Tablet(tablet)
    , BlockSize(blockSize)
    , MaxBlocksInBlob(maxBlocksInBlob)
    , MaxAffectedBlocksPerCompaction(maxAffectedBlocksPerCompaction)
    , BlockDigestGenerator(std::move(blockDigestGenerator))
    , CommitId(commitId)
    , RangeCompactionInfos(std::move(rangeCompactionInfos))
    , Requests(std::move(requests))
{
    ActivityType = TBlockStoreActivities::PARTITION_WORKER;
}

void TCompactionActor::Bootstrap(const TActorContext& ctx)
{
    TRequestScope timer(*RequestInfo);

    Become(&TThis::StateWork);

    LWTRACK(
        RequestReceived_PartitionWorker,
        RequestInfo->CallContext->LWOrbit,
        "Compaction",
        RequestInfo->CallContext->RequestId);

    if (Requests) {
        ReadBlocks(ctx);

        if (BatchRequestsCompleted == Requests.size()) {
            // all blocks are in Fresh index there is nothing to read
            WriteBlobs(ctx);
        }
    } else {
        InitBlockDigests();

        // for zeroed range we only add deletion marker to the index
        AddBlobs(ctx);
    }
}

void TCompactionActor::InitBlockDigests()
{
    for (auto& rc: RangeCompactionInfos) {
        const auto& sgList = rc.BlobContent.Get().GetBlocks();

        if (rc.DataBlobId) {
            Y_VERIFY(sgList.size() == rc.BlockRange.Size() - rc.DataBlobSkipMask.Count());

            ui32 skipped = 0;
            for (const auto blockIndex: xrange(rc.BlockRange)) {
                if (rc.DataBlobSkipMask.Get(blockIndex - rc.BlockRange.Start)) {
                    ++skipped;
                    continue;
                }

                const auto digest = BlockDigestGenerator->ComputeDigest(
                    blockIndex,
                    sgList[blockIndex - rc.BlockRange.Start - skipped]
                );

                if (digest.Defined()) {
                    AffectedBlockInfos.push_back({blockIndex, *digest});
                }
            }
        }

        if (rc.ZeroBlobId) {
            ui32 skipped = 0;
            for (const auto blockIndex: xrange(rc.BlockRange)) {
                if (rc.ZeroBlobSkipMask.Get(blockIndex - rc.BlockRange.Start)) {
                    ++skipped;
                    continue;
                }

                const auto digest = BlockDigestGenerator->ComputeDigest(
                    blockIndex,
                    TBlockDataRef::CreateZeroBlock(BlockSize)
                );

                if (digest.Defined()) {
                    AffectedBlockInfos.push_back({blockIndex, *digest});
                }
            }
        }
    }
}

void TCompactionActor::ReadBlocks(const TActorContext& ctx)
{
    TVector<TRequest*> requests(Reserve(Requests.size()));
    for (auto& r: Requests) {
        requests.push_back(&r);
    }

    Sort(requests, [] (const TRequest* l, const TRequest* r) {
        return l->BlobId < r->BlobId
            || l->BlobId == r->BlobId && l->BlobOffset < r->BlobOffset;
    });

    TBatchRequest current;
    for (auto* r: requests) {
        if (IsDeletionMarker(r->BlobId)) {
            ++BatchRequestsCompleted;
            continue;
        }

        if (current.BlobId != r->BlobId) {
            if (current.BlobOffsets) {
                BatchRequests.emplace_back(
                    current.BlobId,
                    current.Proxy,
                    std::move(current.BlobOffsets),
                    std::move(current.Requests),
                    current.RangeCompactionInfo,
                    current.GroupId);
            }
            current.GroupId = r->GroupId;
            current.BlobId = r->BlobId;
            current.Proxy = r->Proxy;
            current.RangeCompactionInfo =
                &RangeCompactionInfos[r->RangeCompactionIndex];
        }

        current.BlobOffsets.push_back(r->BlobOffset);
        current.Requests.push_back(r);
    }

    if (current.BlobOffsets) {
        BatchRequests.emplace_back(
            current.BlobId,
            current.Proxy,
            std::move(current.BlobOffsets),
            std::move(current.Requests),
            current.RangeCompactionInfo,
            current.GroupId);
    }

    ui32 batchIndex = 0;
    for (auto& batch: BatchRequests) {
        auto& blobContent = batch.RangeCompactionInfo->BlobContent;
        const auto& srcSglist = blobContent.Get().GetBlocks();

        TSgList subset(Reserve(batch.Requests.size()));
        for (const auto* r: batch.Requests) {
            subset.push_back(srcSglist[r->IndexInBlobContent]);
        }

        auto subSgList = blobContent.CreateGuardedSgList(std::move(subset));

        auto request = std::make_unique<TEvPartitionPrivate::TEvReadBlobRequest>(
            MakeBlobId(TabletId, batch.BlobId),
            batch.Proxy,
            std::move(batch.BlobOffsets),
            std::move(subSgList),
            batch.GroupId,
            true);  // async

        if (!RequestInfo->CallContext->LWOrbit.Fork(request->CallContext->LWOrbit)) {
            LWTRACK(
                ForkFailed,
                RequestInfo->CallContext->LWOrbit,
                "TEvPartitionPrivate::TEvReadBlobRequest",
                RequestInfo->CallContext->RequestId);
        }

        ForkedReadCallContexts.emplace_back(request->CallContext);

        auto traceId = RequestInfo->TraceId.Clone();
        BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

        NCloud::Send(
            ctx,
            Tablet,
            std::move(request),
            batchIndex++,
            std::move(traceId));
    }
}

void TCompactionActor::WriteBlobs(const TActorContext& ctx)
{
    InitBlockDigests();

    for (auto& rc: RangeCompactionInfos) {
        if (!rc.DataBlobId) {
            ++WriteBlobRequestsCompleted;
            continue;
        }

        auto request = std::make_unique<TEvPartitionPrivate::TEvWriteBlobRequest>(
            rc.DataBlobId,
            rc.BlobContent.GetGuardedSgList(),
            true);  // async

        if (!RequestInfo->CallContext->LWOrbit.Fork(request->CallContext->LWOrbit)) {
            LWTRACK(
                ForkFailed,
                RequestInfo->CallContext->LWOrbit,
                "TEvPartitionPrivate::TEvWriteBlobRequest",
                RequestInfo->CallContext->RequestId);
        }

        ForkedWriteCallContexts.emplace_back(request->CallContext);

        auto traceId = RequestInfo->TraceId.Clone();
        BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

        NCloud::Send(
            ctx,
            Tablet,
            std::move(request),
            0,  // cookie
            std::move(traceId));
    }

    SafeToUseOrbit = false;
}

void TCompactionActor::AddBlobs(const TActorContext& ctx)
{
    TVector<TAddMergedBlob> mergedBlobs;
    TVector<TMergedBlobCompactionInfo> blobCompactionInfos;
    TAffectedBlobs affectedBlobs;
    TAffectedBlocks affectedBlocks;

    auto addBlob = [&] (
        const TPartialBlobId& blobId,
        TBlockRange32 range,
        TBlockMask skipMask,
        ui32 blobsSkipped,
        ui32 blocksSkipped)
    {
        while (skipMask.Get(range.End - range.Start)) {
            Y_VERIFY(range.End > range.Start);
            // modifying skipMask is crucial since otherwise there would be
            // 2 blobs with the same key in merged index (the key is
            // commitId + blockRange.End)
            skipMask.Reset(range.End - range.Start);
            --range.End;
        }

        mergedBlobs.emplace_back(
            blobId,
            range,
            TBlockMask{},   // holeMask
            skipMask);

        blobCompactionInfos.push_back({blobsSkipped, blocksSkipped});
    };

    for (auto& rc: RangeCompactionInfos) {
        if (rc.DataBlobId) {
            addBlob(
                rc.DataBlobId,
                rc.BlockRange,
                rc.DataBlobSkipMask,
                rc.BlobsSkippedByCompaction,
                rc.BlocksSkippedByCompaction);
        }

        if (rc.ZeroBlobId) {
            ui32 blobsSkipped = 0;
            ui32 blocksSkipped = 0;

            if (!rc.DataBlobId) {
                blobsSkipped = rc.BlobsSkippedByCompaction;
                blocksSkipped = rc.BlocksSkippedByCompaction;
            }

            addBlob(
                rc.ZeroBlobId,
                rc.BlockRange,
                rc.ZeroBlobSkipMask,
                blobsSkipped,
                blocksSkipped);
        }

        if (rc.DataBlobId && rc.ZeroBlobId) {
            // if both blobs are present, none of them should contain all range
            // blocks
            Y_VERIFY(rc.DataBlobSkipMask.Count());
            Y_VERIFY(rc.ZeroBlobSkipMask.Count());
        }

        for (auto it = rc.AffectedBlobs.begin(); it != rc.AffectedBlobs.end(); ) {
            auto& blockMask = it->second.BlockMask.GetRef();

            // could already be full
            if (IsBlockMaskFull(blockMask, MaxBlocksInBlob)) {
                ++it;

                continue;
            }

            // mask overwritten blocks
            for (ui16 blobOffset: it->second.Offsets) {
                blockMask.Set(blobOffset);
            }

            auto& affectedBlob = affectedBlobs[it->first];
            Y_VERIFY(affectedBlob.Offsets.empty());
            Y_VERIFY(affectedBlob.BlockMask.Empty());
            Y_VERIFY(affectedBlob.AffectedBlockIndices.empty());
            affectedBlob = std::move(it->second);

            ++it;
        }

        Sort(rc.AffectedBlocks, [] (const auto& l, const auto& r) {
            // sort by (BlockIndex ASC, CommitId DESC)
            return (l.BlockIndex < r.BlockIndex)
                || (l.BlockIndex == r.BlockIndex && l.CommitId > r.CommitId);
        });

        if (rc.AffectedBlocks.size() > MaxAffectedBlocksPerCompaction) {
            // KIKIMR-6286: preventing heavy transactions
            LOG_WARN(ctx, TBlockStoreComponents::PARTITION,
                "[%lu] Cropping AffectedBlocks: %lu -> %lu, range: %s",
                TabletId,
                rc.AffectedBlocks.size(),
                MaxAffectedBlocksPerCompaction,
                DescribeRange(rc.BlockRange).c_str());

            rc.AffectedBlocks.crop(MaxAffectedBlocksPerCompaction);
        }

        affectedBlocks.insert(
            affectedBlocks.end(),
            rc.AffectedBlocks.begin(),
            rc.AffectedBlocks.end());
    }

    auto request = std::make_unique<TEvPartitionPrivate::TEvAddBlobsRequest>(
        RequestInfo->CallContext,
        CommitId,
        TVector<TAddMixedBlob>(),
        std::move(mergedBlobs),
        TVector<TAddFreshBlob>(),
        ADD_COMPACTION_RESULT,
        std::move(affectedBlobs),
        std::move(affectedBlocks),
        std::move(blobCompactionInfos));

    auto traceId = RequestInfo->TraceId.Clone();
    BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

    SafeToUseOrbit = false;

    NCloud::Send(
        ctx,
        Tablet,
        std::move(request),
        0,  // cookie
        std::move(traceId));
}

void TCompactionActor::NotifyCompleted(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    auto request = std::make_unique<TEvPartitionPrivate::TEvCompactionCompleted>(error);

    request->ExecCycles = RequestInfo->GetExecCycles();
    request->TotalCycles = RequestInfo->GetTotalCycles();

    request->CommitId = CommitId;

    {
        auto execTime = CyclesToDurationSafe(ReadExecCycles);
        auto waitTime = CyclesToDurationSafe(ReadWaitCycles);

        auto& counters = *request->Stats.MutableSysReadCounters();
        counters.SetRequestsCount(1);
        counters.SetBlocksCount(Requests.size());
        counters.SetExecTime(execTime.MicroSeconds());
        counters.SetWaitTime(waitTime.MicroSeconds());
    }

    {
        auto execTime = CyclesToDurationSafe(
            RequestInfo->GetExecCycles() - ReadExecCycles);
        auto waitTime = CyclesToDurationSafe(
            RequestInfo->GetWaitCycles() - ReadExecCycles - ReadWaitCycles);

        ui64 blocksCount = 0;
        for (auto& rc: RangeCompactionInfos) {
            blocksCount += rc.DataBlobId.BlobSize() / BlockSize;
        }
        auto& counters = *request->Stats.MutableSysWriteCounters();
        counters.SetRequestsCount(1);
        counters.SetBlocksCount(blocksCount);
        counters.SetExecTime(execTime.MicroSeconds());
        counters.SetWaitTime(waitTime.MicroSeconds());
    }

    for (const auto& rc: RangeCompactionInfos) {
        request->AffectedRanges.push_back(ConvertRangeSafe(rc.BlockRange));
    }
    request->AffectedBlockInfos = std::move(AffectedBlockInfos);

    NCloud::Send(ctx, Tablet, std::move(request));
}

bool TCompactionActor::HandleError(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    if (FAILED(error.GetCode())) {
        ReplyAndDie(
            ctx,
            std::make_unique<TEvPartitionPrivate::TEvCompactionResponse>(
                error
            )
        );
        return true;
    }
    return false;
}

void TCompactionActor::ReplyAndDie(
    const TActorContext& ctx,
    std::unique_ptr<TEvPartitionPrivate::TEvCompactionResponse> response)
{
    NotifyCompleted(ctx, response->GetError());

    BLOCKSTORE_TRACE_SENT(ctx, &RequestInfo->TraceId, this, response);

    if (SafeToUseOrbit) {
        LWTRACK(
            ResponseSent_Partition,
            RequestInfo->CallContext->LWOrbit,
            "Compaction",
            RequestInfo->CallContext->RequestId);
    }

    NCloud::Reply(ctx, *RequestInfo, std::move(response));
    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TCompactionActor::HandleReadBlobResponse(
    const TEvPartitionPrivate::TEvReadBlobResponse::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    RequestInfo->AddExecCycles(msg->ExecCycles);

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, msg, &ev->TraceId);

    if (HandleError(ctx, msg->GetError())) {
        return;
    }

    ui32 batchIndex = ev->Cookie;

    Y_VERIFY(batchIndex < BatchRequests.size());
    auto& batch = BatchRequests[batchIndex];

    BatchRequestsCompleted += batch.Requests.size();
    Y_VERIFY(BatchRequestsCompleted <= Requests.size());
    if (BatchRequestsCompleted < Requests.size()) {
        return;
    }

    ReadExecCycles = RequestInfo->GetExecCycles();
    ReadWaitCycles = RequestInfo->GetWaitCycles();

    for (auto context: ForkedReadCallContexts) {
        RequestInfo->CallContext->LWOrbit.Join(context->LWOrbit);
    }

    WriteBlobs(ctx);
}

void TCompactionActor::HandleWriteBlobResponse(
    const TEvPartitionPrivate::TEvWriteBlobResponse::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    RequestInfo->AddExecCycles(msg->ExecCycles);

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, msg, &ev->TraceId);

    if (HandleError(ctx, msg->GetError())) {
        return;
    }

    ++WriteBlobRequestsCompleted;
    Y_VERIFY(WriteBlobRequestsCompleted <= RangeCompactionInfos.size());
    if (WriteBlobRequestsCompleted < RangeCompactionInfos.size()) {
        return;
    }

    SafeToUseOrbit = true;

    for (auto context: ForkedWriteCallContexts) {
        RequestInfo->CallContext->LWOrbit.Join(context->LWOrbit);
    }

    AddBlobs(ctx);
}

void TCompactionActor::HandleAddBlobsResponse(
    const TEvPartitionPrivate::TEvAddBlobsResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, msg, &ev->TraceId);

    SafeToUseOrbit = true;

    if (HandleError(ctx, msg->GetError())) {
        return;
    }

    ReplyAndDie(
        ctx,
        std::make_unique<TEvPartitionPrivate::TEvCompactionResponse>()
    );
}

void TCompactionActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    auto response = std::make_unique<TEvPartitionPrivate::TEvCompactionResponse>(
        MakeError(E_REJECTED, "Tablet is dead"));

    ReplyAndDie(ctx, std::move(response));
}

STFUNC(TCompactionActor::StateWork)
{
    TRequestScope timer(*RequestInfo);

    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);
        HFunc(TEvPartitionPrivate::TEvReadBlobResponse, HandleReadBlobResponse);
        HFunc(TEvPartitionPrivate::TEvWriteBlobResponse, HandleWriteBlobResponse);
        HFunc(TEvPartitionPrivate::TEvAddBlobsResponse, HandleAddBlobsResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::PARTITION_WORKER);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////

class TCompactionBlockVisitor final
    : public IFreshBlocksIndexVisitor
    , public IBlocksIndexVisitor
{
private:
    TTxPartition::TRangeCompaction& Args;
    const ui64 MaxCommitId;

public:
    TCompactionBlockVisitor(
            TTxPartition::TRangeCompaction& args,
            ui64 maxCommitId)
        : Args(args)
        , MaxCommitId(maxCommitId)
    {}

    bool Visit(const TFreshBlock& block) override
    {
        Args.MarkBlock(
            block.Meta.BlockIndex,
            block.Meta.CommitId,
            block.Content);
        return true;
    }

    bool KeepTrackOfAffectedBlocks = false;

    bool Visit(
        ui32 blockIndex,
        ui64 commitId,
        const TPartialBlobId& blobId,
        ui16 blobOffset) override
    {
        if (commitId > MaxCommitId) {
            return true;
        }

        Args.MarkBlock(
            blockIndex,
            commitId,
            blobId,
            blobOffset,
            KeepTrackOfAffectedBlocks);
        return true;
    }
};

////////////////////////////////////////////////////////////////////////////////

ui32 GarbagePercentage(ui64 used, ui64 total)
{
    const double p = (total - used) * 100. / Max(used, 1UL);
    const double MAX_P = 1'000;
    return Min(p, MAX_P);
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TPartitionActor::EnqueueCompactionIfNeeded(const TActorContext& ctx)
{
    if (State->GetCompactionState().Status != EOperationStatus::Idle) {
        // already enqueued
        return;
    }

    if (!State->IsCompactionAllowed()) {
        // not allowed for now
        return;
    }

    // TODO: move this logic to TPartitionState to simplify unit testing
    const auto& cm = State->GetCompactionMap();
    auto topRange = cm.GetTop();
    auto topByGarbageBlockCount = cm.GetTopByGarbageBlockCount();
    TEvPartitionPrivate::ECompactionMode mode =
        TEvPartitionPrivate::RangeCompaction;
    bool throttlingAllowed = true;

    auto& scoreHistory = State->GetCompactionScoreHistory();
    const auto now = ctx.Now();
    if (scoreHistory.LastTs() + Config->GetMaxCompactionDelay() <= now) {
        scoreHistory.Register({
            now,
            {
                topRange.Stat.Score,
                topByGarbageBlockCount.Stat.GarbageBlockCount(),
            },
        });
    }

    const auto blockCount = State->GetMixedBlocksCount()
        + State->GetMergedBlocksCount();
    const auto diskGarbage =
        GarbagePercentage(State->GetUsedBlocksCount(), blockCount);

    const bool diskGarbageBelowThreshold =
        diskGarbage < Config->GetCompactionGarbageThreshold();

    if (topRange.Stat.Score <= 0) {
        if (!Config->GetV1GarbageCompactionEnabled()) {
            // nothing to compact
            return;
        }

        if (!State->GetCheckpoints().IsEmpty()) {
            // should not compact, see NBS-1042
            return;
        }

        // ranges containing 0 used blocks could have a nonzero BlockCount value
        // in the corresponding compaction range before r7082716
        const auto isZeroedRange = topByGarbageBlockCount.Stat.BlockCount
            && !topByGarbageBlockCount.Stat.UsedBlockCount;

        if (topByGarbageBlockCount.Stat.Compacted
                || topByGarbageBlockCount.Stat.BlobCount < 2
                && !isZeroedRange)
        {
            // nothing to compact
            return;
        }

        const auto rangeGarbage = GarbagePercentage(
            topByGarbageBlockCount.Stat.UsedBlockCount,
            topByGarbageBlockCount.Stat.BlockCount
        );

        if (rangeGarbage < Config->GetCompactionRangeGarbageThreshold()) {
            // not enough garbage in this range
            if (diskGarbageBelowThreshold) {
                // and not enough garbage on the whole disk, no need to compact
                return;
            }

            if (rangeGarbage < Config->GetCompactionGarbageThreshold()) {
                // really not enough garbage in this range, see NBS-1045
                return;
            }
        }

        mode = TEvPartitionPrivate::GarbageCompaction;
    } else if (topRange.Stat.Score >= Config->GetCompactionScoreLimitForThrottling()) {
        throttlingAllowed = false;
    }

    State->GetCompactionState().SetStatus(EOperationStatus::Enqueued);

    auto request = std::make_unique<TEvPartitionPrivate::TEvCompactionRequest>(
        MakeIntrusive<TCallContext>(CreateRequestId()),
        mode
    );

    if (mode == TEvPartitionPrivate::GarbageCompaction
            || !diskGarbageBelowThreshold)
    {
        request->ForceFullCompaction = true;
    }

    auto traceId = NWilson::TTraceId::NewTraceId();
    BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

    if (throttlingAllowed && Config->GetMaxCompactionDelay()) {
        auto execTime = State->GetCompactionExecTimeForLastSecond(ctx.Now());
        auto delay = Config->GetMinCompactionDelay();
        if (Config->GetMaxCompactionExecTimePerSecond()) {
            auto throttlingFactor = double(execTime.GetValue())
                / Config->GetMaxCompactionExecTimePerSecond().GetValue();
            const auto throttleDelay = (TDuration::Seconds(1) - execTime) * throttlingFactor;

            delay = Max(delay, throttleDelay);
        }

        delay = Min(delay, Config->GetMaxCompactionDelay());
        State->SetCompactionDelay(delay);
    } else {
        State->SetCompactionDelay({});
    }

    if (State->GetCompactionDelay()) {
        ctx.Schedule(State->GetCompactionDelay(), request.release());
    } else {
        NCloud::Send(
            ctx,
            SelfId(),
            std::move(request),
            0,  // cookie
            std::move(traceId));
    }
}

void TPartitionActor::HandleCompaction(
    const TEvPartitionPrivate::TEvCompactionRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo<TEvPartitionPrivate::TCompactionMethod>(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    TRequestScope timer(*requestInfo);

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    LWTRACK(
        BackgroundTaskStarted_Partition,
        requestInfo->CallContext->LWOrbit,
        "Compaction",
        static_cast<ui32>(PartitionConfig.GetStorageMediaKind()),
        requestInfo->CallContext->RequestId,
        PartitionConfig.GetDiskId());

    auto replyError = [=] (
        const TActorContext& ctx,
        TRequestInfo& requestInfo,
        ui32 errorCode,
        TString errorReason)
    {
        auto response = std::make_unique<TEvPartitionPrivate::TEvCompactionResponse>(
            MakeError(errorCode, std::move(errorReason)));

        BLOCKSTORE_TRACE_SENT(ctx, &requestInfo.TraceId, this, response);

        LWTRACK(
            ResponseSent_Partition,
            requestInfo.CallContext->LWOrbit,
            "Compaction",
            requestInfo.CallContext->RequestId);

        NCloud::Reply(ctx, requestInfo, std::move(response));
    };

    if (State->GetCompactionState().Status == EOperationStatus::Started) {
        replyError(ctx, *requestInfo, E_TRY_AGAIN, "compaction already started");
        return;
    }

    if (!State->IsCompactionAllowed()) {
        State->GetCompactionState().SetStatus(EOperationStatus::Idle);

        replyError(ctx, *requestInfo, E_BS_OUT_OF_SPACE, "all channels readonly");
        return;
    }

    TVector<TCompactionCounter> tops;

    const auto& cm = State->GetCompactionMap();

    if (msg->BlockIndex.Defined()) {
        const auto startIndex = cm.GetRangeStart(*msg->BlockIndex);
        tops.push_back({startIndex, cm.Get(startIndex)});
        State->OnNewCompactionRange();
    } else if (msg->Mode == TEvPartitionPrivate::GarbageCompaction) {
        const auto& top = State->GetCompactionMap().GetTopByGarbageBlockCount();
        tops.push_back({top.BlockIndex, top.Stat});
    } else {
        ui32 rangeCount = 1;

        const bool batchCompactionEnabledForCloud =
            Config->IsBatchCompactionFeatureEnabled(
                PartitionConfig.GetCloudId(),
                PartitionConfig.GetFolderId());
        const bool batchCompactionEnabled =
            Config->GetBatchCompactionEnabled() || batchCompactionEnabledForCloud;

        if (batchCompactionEnabled) {
            rangeCount = Config->GetCompactionRangeCountPerRun();
        }

        tops = State->GetCompactionMap().GetTopsFromGroups(rangeCount);
    }

    if (tops.empty() || !tops.front().Stat.BlobCount) {
        State->GetCompactionState().SetStatus(EOperationStatus::Idle);

        replyError(ctx, *requestInfo, S_ALREADY, "nothing to compact");
        return;
    }

    ui64 commitId = State->GenerateCommitId();
    if (commitId == InvalidCommitId) {
        requestInfo->CancelRequest(ctx);
        RebootPartitionOnCommitIdOverflow(ctx, "Compaction");
        return;
    }

    TVector<std::pair<ui32, TBlockRange32>> ranges(Reserve(tops.size()));
    for (const auto& x: tops) {
        const ui32 rangeIdx = cm.GetRangeIndex(x.BlockIndex);

        const TBlockRange32 blockRange(
            x.BlockIndex,
            Min(
                State->GetBlocksCount(),
                static_cast<ui64>(x.BlockIndex) + cm.GetRangeSize()
            ) - 1
        );

        LOG_DEBUG(ctx, TBlockStoreComponents::PARTITION,
            "[%lu] Start compaction @%lu (range: %s, blobs: %u, blocks: %u"
            ", reads: %u, blobsread: %u, blocksread: %u, score: %f)",
            TabletID(),
            commitId,
            DescribeRange(blockRange).c_str(),
            x.Stat.BlobCount,
            x.Stat.BlockCount,
            x.Stat.ReadRequestCount,
            x.Stat.ReadRequestBlobCount,
            x.Stat.ReadRequestBlockCount,
            x.Stat.Score);

        ranges.emplace_back(rangeIdx, blockRange);
    }

    State->GetCompactionState().SetStatus(EOperationStatus::Started);

    State->GetCommitQueue().AcquireBarrier(commitId);
    State->GetCleanupQueue().AcquireBarrier(commitId);
    State->GetGarbageQueue().AcquireBarrier(commitId);

    AddTransaction(*requestInfo);

    auto tx = CreateTx<TCompaction>(
        requestInfo,
        commitId,
        msg->ForceFullCompaction,
        std::move(ranges));

    ui64 minCommitId = State->GetCommitQueue().GetMinCommitId();
    Y_VERIFY(minCommitId <= commitId);

    if (minCommitId == commitId) {
        // start execution
        ExecuteTx(ctx, std::move(tx));
    } else {
        // delay execution until all previous commits completed
        State->GetCommitQueue().Enqueue(std::move(tx), commitId);
    }
}

void TPartitionActor::ProcessCommitQueue(const TActorContext& ctx)
{
    ui64 minCommitId = State->GetCommitQueue().GetMinCommitId();

    while (!State->GetCommitQueue().Empty()) {
        ui64 commitId = State->GetCommitQueue().Peek();
        Y_VERIFY(minCommitId <= commitId);

        if (minCommitId == commitId) {
            // start execution
            ExecuteTx(ctx, State->GetCommitQueue().Dequeue());
        } else {
            // delay execution until all previous commits completed
            break;
        }
    }

    // TODO: too many different queues exist
    // Since create checkpoint operation waits for the last commit to complete
    // here we force checkpoints queue to try to proceed to the next
    // create checkpoint request
    ProcessCheckpointQueue(ctx);
}

void TPartitionActor::HandleCompactionCompleted(
    const TEvPartitionPrivate::TEvCompactionCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    ui64 commitId = msg->CommitId;
    LOG_DEBUG(ctx, TBlockStoreComponents::PARTITION,
        "[%lu] Complete compaction @%lu",
        TabletID(),
        commitId);

    UpdateStats(msg->Stats);

    State->GetCommitQueue().ReleaseBarrier(commitId);
    State->GetCleanupQueue().ReleaseBarrier(commitId);
    State->GetGarbageQueue().ReleaseBarrier(commitId);

    State->GetCompactionState().SetStatus(EOperationStatus::Idle);

    Actors.erase(ev->Sender);

    const auto d = CyclesToDurationSafe(msg->TotalCycles);
    ui32 blocks = msg->Stats.GetSysReadCounters().GetBlocksCount()
        + msg->Stats.GetSysWriteCounters().GetBlocksCount();
    PartCounters->RequestCounters.Compaction.AddRequest(
        d.MicroSeconds(),
        blocks * State->GetBlockSize());
    State->SetLastCompactionExecTime(d, ctx.Now());

    const auto ts = ctx.Now() - d;

    {
        IProfileLog::TSysReadWriteRequest request;
        request.RequestType = ESysRequestType::Compaction;
        request.Duration = d;
        request.Ranges = std::move(msg->AffectedRanges);

        IProfileLog::TRecord record;
        record.DiskId = State->GetConfig().GetDiskId();
        record.Ts = ts;
        record.Request = std::move(request);

        ProfileLog->Write(std::move(record));
    }

    if (msg->AffectedBlockInfos) {
        IProfileLog::TSysReadWriteRequestBlockInfos request;
        request.RequestType = ESysRequestType::Compaction;
        request.BlockInfos = std::move(msg->AffectedBlockInfos);
        request.CommitId = commitId;

        IProfileLog::TRecord record;
        record.DiskId = State->GetConfig().GetDiskId();
        record.Ts = ts;
        record.Request = std::move(request);

        ProfileLog->Write(std::move(record));
    }

    EnqueueCompactionIfNeeded(ctx);
    EnqueueCleanupIfNeeded(ctx);
    ProcessCommitQueue(ctx);
}

namespace {

////////////////////////////////////////////////////////////////////////////////

void PrepareRangeCompaction(
    const TStorageConfig& config,
    const TString& cloudId,
    const TString& folderId,
    const ui64 commitId,
    const bool forceFullCompaction,
    const TActorContext& ctx,
    const ui64 tabletId,
    THashSet<TPartialBlobId, TPartialBlobIdHash>& affectedBlobIds,
    bool& ready,
    TPartitionDatabase& db,
    TPartitionState& state,
    TTxPartition::TRangeCompaction& args)
{
    const bool incrementalCompactionEnabledForCloud =
        config.IsIncrementalCompactionFeatureEnabled(cloudId, folderId);
    const bool incrementalCompactionEnabled =
        config.GetIncrementalCompactionEnabled()
        || incrementalCompactionEnabledForCloud;

    TCompactionBlockVisitor visitor(args, commitId);
    state.FindFreshBlocks(visitor, args.BlockRange, commitId);
    visitor.KeepTrackOfAffectedBlocks = true;
    ready &= state.FindMixedBlocksForCompaction(
        db,
        visitor,
        args.RangeIdx);
    visitor.KeepTrackOfAffectedBlocks = false;
    ready &= db.FindMergedBlocks(
        visitor,
        args.BlockRange,
        true,   // precharge
        state.GetMaxBlocksInBlob(),
        commitId);

    if (ready) {
        for (const auto& x: args.AffectedBlobs) {
            if (affectedBlobIds.contains(x.first)) {
                args.Discarded = true;
                return;
            }
        }

        for (const auto& x: args.AffectedBlobs) {
            affectedBlobIds.insert(x.first);
        }
    }

    if (ready
            && incrementalCompactionEnabled
            && !forceFullCompaction)
    {
        THashMap<TPartialBlobId, ui32, TPartialBlobIdHash> liveBlocks;
        for (const auto& m: args.BlockMarks) {
            if (m.CommitId && m.BlobId) {
                ++liveBlocks[m.BlobId];
            }
        }

        TVector<TPartialBlobId> blobIds;
        blobIds.reserve(liveBlocks.size());
        for (const auto& x: liveBlocks) {
            blobIds.push_back(x.first);
        }

        Sort(
            blobIds.begin(),
            blobIds.end(),
            [&] (const TPartialBlobId& l, const TPartialBlobId& r) {
                return liveBlocks[l] < liveBlocks[r];
            }
        );

        auto it = blobIds.begin();
        args.BlobsSkipped = blobIds.size();
        ui32 blocks = 0;

        while (it != blobIds.end()) {
            const auto bytes = blocks * state.GetBlockSize();
            const auto blobCountOk = args.BlobsSkipped
                <= config.GetMaxSkippedBlobsDuringCompaction();
            const auto byteCountOk =
                bytes >= config.GetTargetCompactionBytesPerOp();

            if (blobCountOk && byteCountOk) {
                break;
            }

            blocks += liveBlocks[*it];
            --args.BlobsSkipped;
            ++it;
        }

        // liveBlocks will contain only skipped blobs after this
        for (auto it2 = blobIds.begin(); it2 != it; ++it2) {
            liveBlocks.erase(*it2);
        }

        while (it != blobIds.end()) {
            args.BlocksSkipped += liveBlocks[*it];
            ++it;
        }

        LOG_DEBUG(ctx, TBlockStoreComponents::PARTITION,
            "[%lu] Dropping last %u blobs, %u blocks"
            ", remaining blobs: %u, blocks: %u",
            tabletId,
            args.BlobsSkipped,
            args.BlocksSkipped,
            liveBlocks.size(),
            blocks);

        THashSet<ui32> skippedBlockIndices;

        for (const auto& x: liveBlocks) {
            auto ab = args.AffectedBlobs.find(x.first);
            Y_VERIFY(ab != args.AffectedBlobs.end());
            for (const auto blockIndex: ab->second.AffectedBlockIndices) {
                // we can actually add extra indices to skippedBlockIndices,
                // but it does not cause data corruption - the important thing
                // is to ensure that all skipped indices are added, not that
                // all non-skipped are preserved
                skippedBlockIndices.insert(blockIndex);
            }
            args.AffectedBlobs.erase(ab);
        }

        if (liveBlocks.size()) {
            TAffectedBlocks affectedBlocks;
            for (const auto& b: args.AffectedBlocks) {
                if (!skippedBlockIndices.contains(b.BlockIndex)) {
                    affectedBlocks.push_back(b);
                }
            }
            args.AffectedBlocks = std::move(affectedBlocks);

            for (auto& m: args.BlockMarks) {
                if (liveBlocks.contains(m.BlobId)) {
                    m = {};
                }
            }
        }
    }

    for (auto& kv: args.AffectedBlobs) {
        if (db.ReadBlockMask(kv.first, kv.second.BlockMask)) {
            Y_VERIFY(kv.second.BlockMask.Defined(),
                "Could not read block mask for blob: %s",
                ToString(MakeBlobId(tabletId, kv.first)).data());
        } else {
            ready = false;
        }
    }
}

void CompleteRangeCompaction(
    const ui64 commitId,
    TTabletStorageInfo& tabletStorageInfo,
    TPartitionState& state,
    TTxPartition::TRangeCompaction& args,
    TVector<TCompactionActor::TRequest>& requests,
    TVector<TRangeCompactionInfo>& result)
{
    const EChannelPermissions compactionPermissions =
        EChannelPermission::SystemWritesAllowed;
    const auto initialRequestsSize = requests.size();

    // at first we count number of data blocks
    size_t dataBlocksCount = 0, zeroBlocksCount = 0;
    for (const auto& mark: args.BlockMarks) {
        if (mark.CommitId) {
            // there could be fresh block OR merged/mixed block
            Y_VERIFY(!(mark.BlockContent && !IsDeletionMarker(mark.BlobId)));
            if (mark.BlockContent || !IsDeletionMarker(mark.BlobId)) {
                ++dataBlocksCount;
            } else {
                ++zeroBlocksCount;
            }
        }
    }

    // determine the results kind
    TPartialBlobId dataBlobId, zeroBlobId;
    TBlockMask dataBlobSkipMask, zeroBlobSkipMask;

    if (dataBlocksCount) {
        ui32 skipped = 0;
        for (const auto& mark: args.BlockMarks) {
            if (!mark.BlockContent && IsDeletionMarker(mark.BlobId)) {
                ++skipped;
            }
        }

        dataBlobId = state.GenerateBlobId(
            EChannelDataKind::Merged,
            compactionPermissions,
            commitId,
            (args.BlockRange.Size() - skipped) * state.GetBlockSize(),
            result.size());
    }

    if (zeroBlocksCount) {
        // for zeroed region we will write blob without any data
        // XXX same commitId used for 2 blobs: data blob and zero blob
        // we differentiate between them by storing the last block index in
        // MergedBlocksIndex::RangeEnd not for the last block of the processed
        // compaction range but for the last actual block that's referenced by
        // the corresponding blob
        zeroBlobId = state.GenerateBlobId(
            EChannelDataKind::Merged,
            compactionPermissions,
            commitId,
            0,
            result.size());
    }

    // now build the blob content for all blocks to be written
    TBlockBuffer blobContent(TProfilingAllocator::Instance());
    TVector<ui32> zeroBlocks;

    ui32 blockIndex = args.BlockRange.Start;
    for (auto& mark: args.BlockMarks) {
        if (mark.CommitId) {
            if (mark.BlockContent) {
                Y_VERIFY(IsDeletionMarker(mark.BlobId));
                requests.emplace_back(
                    mark.BlobId,
                    TActorId(),
                    mark.BlobOffset,
                    blockIndex,
                    blobContent.GetBlocksCount(),
                    0,
                    result.size());

                // fresh block will be written
                blobContent.AddBlock({
                    mark.BlockContent.Data(),
                    mark.BlockContent.Size()
                });

                if (zeroBlobId) {
                    zeroBlobSkipMask.Set(blockIndex - args.BlockRange.Start);
                }
            } else if (!IsDeletionMarker(mark.BlobId)) {
                const auto proxy = tabletStorageInfo.BSProxyIDForChannel(
                    mark.BlobId.Channel(),
                    mark.BlobId.Generation());

                requests.emplace_back(
                    mark.BlobId,
                    proxy,
                    mark.BlobOffset,
                    blockIndex,
                    blobContent.GetBlocksCount(),
                    tabletStorageInfo.GroupFor(
                        mark.BlobId.Channel(),
                        mark.BlobId.Generation()),
                    result.size());

                // we will read this block later
                blobContent.AddBlock(state.GetBlockSize(), char(0));

                if (zeroBlobId) {
                    zeroBlobSkipMask.Set(blockIndex - args.BlockRange.Start);
                }
            } else {
                dataBlobSkipMask.Set(blockIndex - args.BlockRange.Start);
                zeroBlocks.push_back(blockIndex);
            }
        } else {
            if (dataBlobId) {
                dataBlobSkipMask.Set(blockIndex - args.BlockRange.Start);
            }
            if (zeroBlobId) {
                zeroBlobSkipMask.Set(blockIndex - args.BlockRange.Start);
            }
        }

        ++blockIndex;
    }

    result.emplace_back(
        args.BlockRange,
        dataBlobId,
        dataBlobSkipMask,
        zeroBlobId,
        zeroBlobSkipMask,
        args.BlobsSkipped,
        args.BlocksSkipped,
        std::move(blobContent),
        std::move(zeroBlocks),
        std::move(args.AffectedBlobs),
        std::move(args.AffectedBlocks));

    if (!dataBlobId && !zeroBlobId) {
        const auto rangeDescr = DescribeRange(args.BlockRange);
        Y_FAIL("No blocks in compacted range: %s", rangeDescr.c_str());
    }
    Y_VERIFY(requests.size() - initialRequestsSize == dataBlocksCount);
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

bool TPartitionActor::PrepareCompaction(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxPartition::TCompaction& args)
{
    TRequestScope timer(*args.RequestInfo);
    TPartitionDatabase db(tx.DB);

    bool ready = true;

    THashSet<TPartialBlobId, TPartialBlobIdHash> affectedBlobIds;

    for (auto& rangeCompaction: args.RangeCompactions) {
        PrepareRangeCompaction(
            *Config,
            PartitionConfig.GetCloudId(),
            PartitionConfig.GetFolderId(),
            args.CommitId,
            args.ForceFullCompaction,
            ctx,
            TabletID(),
            affectedBlobIds,
            ready,
            db,
            *State,
            rangeCompaction);
    }

    return ready;
}

void TPartitionActor::ExecuteCompaction(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxPartition::TCompaction& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);
}

void TPartitionActor::CompleteCompaction(
    const TActorContext& ctx,
    TTxPartition::TCompaction& args)
{
    TRequestScope timer(*args.RequestInfo);

    RemoveTransaction(*args.RequestInfo);

    for (auto& rangeCompaction: args.RangeCompactions) {
        if (!rangeCompaction.Discarded) {
            State->RaiseRangeTemperature(rangeCompaction.RangeIdx);
        }
    }

    TVector<TRangeCompactionInfo> rangeCompactionInfos;
    TVector<TCompactionActor::TRequest> requests;

    for (auto& rangeCompaction: args.RangeCompactions) {
        if (rangeCompaction.Discarded) {
            continue;
        }

        CompleteRangeCompaction(
            args.CommitId,
            *Info(),
            *State,
            rangeCompaction,
            requests,
            rangeCompactionInfos);
    }

    auto actor = NCloud::Register<TCompactionActor>(
        ctx,
        args.RequestInfo,
        TabletID(),
        SelfId(),
        State->GetBlockSize(),
        State->GetMaxBlocksInBlob(),
        Config->GetMaxAffectedBlocksPerCompaction(),
        BlockDigestGenerator,
        args.CommitId,
        std::move(rangeCompactionInfos),
        std::move(requests));

    Actors.insert(actor);
}

}   // namespace NCloud::NBlockStore::NStorage::NPartition
