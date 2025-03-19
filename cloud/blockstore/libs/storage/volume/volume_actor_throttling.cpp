#include "volume_actor.h"

#include <cloud/blockstore/libs/common/proto_helpers.h>

#include <cloud/blockstore/libs/service/request_helpers.h>

#include <cloud/blockstore/libs/storage/api/partition.h>
#include <cloud/blockstore/libs/storage/core/forward_helpers.h>
#include <cloud/blockstore/libs/storage/core/probes.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;
using namespace NKikimr;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr TDuration MinPostponeQueueFlushInterval = TDuration::MilliSeconds(1);

////////////////////////////////////////////////////////////////////////////////

template <typename T>
TVolumeThrottlingPolicy::TRequestInfo BuildThrottlingRequestInfo(
    ui32, const T&, const ui32)
{
    return {};
}

TVolumeThrottlingPolicy::TRequestInfo BuildThrottlingRequestInfo(
    const ui32 blockSize,
    const TEvService::TEvReadBlocksRequest& request,
    const ui32 policyVersion)
{
    return {
        IntegerCast<ui32>(CalculateBytesCount(request.Record, blockSize)),
        TVolumeThrottlingPolicy::EOpType::Read,
        policyVersion,
    };
}

TVolumeThrottlingPolicy::TRequestInfo BuildThrottlingRequestInfo(
    const ui32 blockSize,
    const TEvService::TEvWriteBlocksRequest& request,
    const ui32 policyVersion)
{
    return {
        IntegerCast<ui32>(CalculateBytesCount(request.Record, blockSize)),
        TVolumeThrottlingPolicy::EOpType::Write,
        policyVersion,
    };
}

TVolumeThrottlingPolicy::TRequestInfo BuildThrottlingRequestInfo(
    const ui32 blockSize,
    const TEvService::TEvZeroBlocksRequest& request,
    const ui32 policyVersion)
{
    return {
        IntegerCast<ui32>(CalculateBytesCount(request.Record, blockSize)),
        TVolumeThrottlingPolicy::EOpType::Zero,
        policyVersion,
    };
}

TVolumeThrottlingPolicy::TRequestInfo BuildThrottlingRequestInfo(
    const ui32 blockSize,
    const TEvService::TEvReadBlocksLocalRequest& request,
    const ui32 policyVersion)
{
    return {
        blockSize * request.Record.GetBlocksCount(),
        TVolumeThrottlingPolicy::EOpType::Read,
        policyVersion,
    };
}

TVolumeThrottlingPolicy::TRequestInfo BuildThrottlingRequestInfo(
    const ui32 blockSize,
    const TEvService::TEvWriteBlocksLocalRequest& request,
    const ui32 policyVersion)
{
    return {
        blockSize * request.Record.BlocksCount,
        TVolumeThrottlingPolicy::EOpType::Write,
        policyVersion,
    };
}

TVolumeThrottlingPolicy::TRequestInfo BuildThrottlingRequestInfo(
    const ui32 blockSize,
    const TEvVolume::TEvDescribeBlocksRequest& request,
    const ui32 policyVersion)
{
    return {
        blockSize * request.Record.GetBlocksCountToRead(),
        TVolumeThrottlingPolicy::EOpType::Describe,
        policyVersion,
    };
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::UpdateDelayCounter(
    TVolumeThrottlingPolicy::EOpType opType,
    TDuration time)
{
    if (!VolumeSelfCounters) {
        return;
    }
    switch (opType) {
        case TVolumeThrottlingPolicy::EOpType::Read:
            VolumeSelfCounters->RequestCounters.ReadBlocks.Increment(time.MicroSeconds());
            return;
        case TVolumeThrottlingPolicy::EOpType::Write:
            VolumeSelfCounters->RequestCounters.WriteBlocks.Increment(time.MicroSeconds());
            return;
        case TVolumeThrottlingPolicy::EOpType::Zero:
            VolumeSelfCounters->RequestCounters.ZeroBlocks.Increment(time.MicroSeconds());
            return;
        case TVolumeThrottlingPolicy::EOpType::Describe:
            VolumeSelfCounters->RequestCounters.DescribeBlocks.Increment(time.MicroSeconds());
            return;
        case TVolumeThrottlingPolicy::EOpType::Last:
        default:
            Y_VERIFY_DEBUG(0);
    }
}

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::HandleBackpressureReport(
    const NPartition::TEvPartition::TEvBackpressureReport::TPtr& ev,
    const TActorContext& ctx)
{
    ui32 index = State->GetPartitions().size();
    if (!State->FindPartitionStatInfoByOwner(ev->Sender, index)) {
        LOG_WARN(ctx, TBlockStoreComponents::VOLUME,
            "Partition %s for disk %s backpressure report not found",
            ToString(ev->Sender).c_str(),
            State->GetDiskId().Quote().c_str());
    }

    auto& policy = State->AccessThrottlingPolicy();
    policy.OnBackpressureReport(ctx.Now(), *ev->Get(), index);
}

void TVolumeActor::Postpone(
    const TActorContext& ctx,
    TVolumeThrottlingPolicy::TRequestInfo requestInfo,
    TCallContextPtr callContext,
    IEventHandlePtr ev)
{
    Y_UNUSED(ctx);

    if (PostponedQueueFlushInProgress) {
        Y_VERIFY_DEBUG(!PostponedRequests.front().Event);
        auto& pr = PostponedRequests.front();
        pr.Event = std::move(ev);
        pr.Info = requestInfo;
    } else {
        auto cycles = GetCycleCount();
        auto now = ctx.Now();
        AtomicSet(callContext->PostponeTsCycles, cycles);
        AtomicSet(callContext->PostponeTs, now.MicroSeconds());
        PostponedRequests.push_back({
            requestInfo,
            callContext,
            std::move(ev)
        });
   }

    VolumeSelfCounters->Cumulative.ThrottlerPostponedRequests.Increment(1);
}

void TVolumeActor::HandleWakeup(
    const TEvents::TEvWakeup::TPtr&,
    const TActorContext& ctx)
{
    Y_VERIFY_DEBUG(PostponedQueueFlushScheduled);
    PostponedQueueFlushScheduled = false;
    PostponedQueueFlushInProgress = true;

    while (PostponedRequests.size()) {
        auto& x = PostponedRequests.front();
        State->AccessThrottlingPolicy().OnPostponedEvent(ctx.Now(), x.Info);
        TAutoPtr<IEventHandle> ev = x.Event.release();
        Receive(ev, ctx);

        if (PostponedQueueFlushScheduled) {
            Y_VERIFY(x.Event);
            break;
        } else {
            auto start = AtomicGet(x.CallContext->PostponeTsCycles);
            auto delay = CyclesToDurationSafe(GetCycleCount() - start);
            x.CallContext->AddTime(EProcessingStage::Postponed, delay);
            UpdateDelayCounter(x.Info.OpType, delay);
        }

        PostponedRequests.pop_front();
    }

    PostponedQueueFlushInProgress = false;
}

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
NProto::TError TVolumeActor::Throttle(
    const TActorContext& ctx,
    const typename TMethod::TRequest::TPtr& ev,
    bool throttlingDisabled)
{
    static const auto ok = MakeError(S_OK);
    static const auto err =
        MakeError(E_REJECTED, "Throttled");  // TODO: E_BS_THROTTLED;

    if (!RequiresThrottling<TMethod>()
            || throttlingDisabled
            || !GetThrottlingEnabled(*Config, State->GetConfig()))
    {
        return ok;
    }

    auto* msg = ev->Get();

    auto& tp = State->AccessThrottlingPolicy();
    const auto requestInfo = BuildThrottlingRequestInfo(
        State->GetConfig().GetBlockSize(),
        *msg,
        tp.GetVersion()
    );

    if (requestInfo.OpType == TVolumeThrottlingPolicy::EOpType::Describe &&
        requestInfo.ByteCount == 0)
    {
        // DescribeBlocks with zero weight should not be affected by
        // throttling limits.
        return ok;
    }

    bool rejected = false;
    if (PostponedRequests && !PostponedQueueFlushInProgress) {
        Y_VERIFY_DEBUG(PostponedQueueFlushScheduled);

        if (tp.TryPostpone(ctx.Now(), requestInfo)) {
            LWTRACK(
                RequestPostponed_Volume,
                msg->CallContext->LWOrbit,
                TMethod::Name,
                msg->CallContext->RequestId);

            LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
                "[%lu] Added %s request to the postponed queue, queue size: %lu",
                TabletID(),
                TMethod::Name,
                PostponedRequests.size());

            Postpone(
                ctx,
                requestInfo,
                msg->CallContext,
                IEventHandlePtr(ev.Release())
            );

            return ok;
        }

        rejected = true;
    } else {
        TAtomicBase postponeTs =
            AtomicGet(msg->CallContext->PostponeTs);
        TDuration queueTime;
        if (postponeTs) {
            queueTime = ctx.Now() - TInstant::MicroSeconds(postponeTs);
        }
        const auto delay = tp.SuggestDelay(ctx.Now(), queueTime, requestInfo);

        if (delay.Defined()) {
            if (delay->GetValue()) {
                LWTRACK(
                    RequestPostponed_Volume,
                    msg->CallContext->LWOrbit,
                    TMethod::Name,
                    msg->CallContext->RequestId);

                LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
                    "[%lu] Postponed %s request by %lu us",
                    TabletID(),
                    TMethod::Name,
                    delay->MicroSeconds());

                Postpone(
                    ctx,
                    requestInfo,
                    msg->CallContext,
                    IEventHandlePtr(ev.Release())
                );

                Y_VERIFY_DEBUG(!PostponedQueueFlushScheduled);
                PostponedQueueFlushScheduled = true;

                ctx.Schedule(
                    Max(*delay, MinPostponeQueueFlushInterval),
                    new TEvents::TEvWakeup()
                );

                return ok;
            } else if (PostponedQueueFlushInProgress) {
                LWTRACK(
                    RequestAdvanced_Volume,
                    msg->CallContext->LWOrbit,
                    TMethod::Name,
                    msg->CallContext->RequestId);

                LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
                    "[%lu] Advanced %s request",
                    TabletID(),
                    TMethod::Name);
            }
        } else {
            rejected = true;
        }
    }

    if (rejected) {
        VolumeSelfCounters->Cumulative.ThrottlerRejectedRequests.Increment(1);

        return err;
    }

    if (!PostponedQueueFlushInProgress) {
        // throttling caused no delay for this request
        UpdateDelayCounter(requestInfo.OpType, TDuration::MilliSeconds(0));
    }

    return ok;
}

////////////////////////////////////////////////////////////////////////////////

#define GENERATE_IMPL(name, ns)                                                \
template NProto::TError TVolumeActor::Throttle<                                \
    ns::T##name##Method>(                                                      \
        const TActorContext& ctx,                                              \
        const ns::TEv##name##Request::TPtr& ev,                                \
        bool throttlingDisabled);                                              \
// GENERATE_IMPL

GENERATE_IMPL(ReadBlocks,         TEvService)
GENERATE_IMPL(WriteBlocks,        TEvService)
GENERATE_IMPL(ZeroBlocks,         TEvService)
GENERATE_IMPL(CreateCheckpoint,   TEvService)
GENERATE_IMPL(DeleteCheckpoint,   TEvService)
GENERATE_IMPL(GetChangedBlocks,   TEvService)
GENERATE_IMPL(ReadBlocksLocal,    TEvService)
GENERATE_IMPL(WriteBlocksLocal,   TEvService)

GENERATE_IMPL(DescribeBlocks,           TEvVolume)
GENERATE_IMPL(GetUsedBlocks,            TEvVolume)
GENERATE_IMPL(GetPartitionInfo,         TEvVolume)
GENERATE_IMPL(CompactRange,             TEvVolume)
GENERATE_IMPL(GetCompactionStatus,      TEvVolume)
GENERATE_IMPL(DeleteCheckpointData,     TEvVolume)
GENERATE_IMPL(RebuildMetadata,          TEvVolume)
GENERATE_IMPL(GetRebuildMetadataStatus, TEvVolume)

}   // namespace NCloud::NBlockStore::NStorage
