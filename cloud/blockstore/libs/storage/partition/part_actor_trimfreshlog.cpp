#include "part_actor.h"

#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/partition_common/actor_trimfreshlog.h>

namespace NCloud::NBlockStore::NStorage::NPartition {

using namespace NActors;

using namespace NKikimr;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

////////////////////////////////////////////////////////////////////////////////

void TPartitionActor::EnqueueTrimFreshLogIfNeeded(const TActorContext& ctx)
{
    if (State->GetTrimFreshLogState().Status != EOperationStatus::Idle) {
        // already enqueued
        return;
    }

    ui64 trimFreshLogToCommitId = Min(
        State->GetLastFlushToCommitId(),
        State->GetTrimFreshLogBarriers().GetMinCommitId() - 1);

    if (trimFreshLogToCommitId == State->GetLastTrimFreshLogToCommitId()) {
        // not ready
        return;
    }

    State->GetTrimFreshLogState().SetStatus(EOperationStatus::Enqueued);

    using TRequest = TEvPartitionCommonPrivate::TEvTrimFreshLogRequest;
    auto request = std::make_unique<TRequest>(
        MakeIntrusive<TCallContext>(CreateRequestId())
    );

    auto traceId = NWilson::TTraceId::NewTraceId();
    BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

    if (State->GetTrimFreshLogTimeout()) {
        LOG_DEBUG(ctx, TBlockStoreComponents::PARTITION,
            "[%lu] TrimFreshLog request scheduled: %lu, %s",
            TabletID(),
            request->CallContext->RequestId,
            State->GetTrimFreshLogTimeout().ToString().c_str());

        ctx.Schedule(State->GetTrimFreshLogTimeout(), request.release());
    } else {
        LOG_DEBUG(ctx, TBlockStoreComponents::PARTITION,
            "[%lu] TrimFreshLog request sent: %lu",
            TabletID(),
            request->CallContext->RequestId);

        NCloud::Send(
            ctx,
            SelfId(),
            std::move(request),
            0,  // cookie
            std::move(traceId));
    }
}

void TPartitionActor::HandleTrimFreshLog(
    const TEvPartitionCommonPrivate::TEvTrimFreshLogRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    using TMethod = TEvPartitionCommonPrivate::TTrimFreshLogMethod;
    auto requestInfo = CreateRequestInfo<TMethod>(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    TRequestScope timer(*requestInfo);

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    LWTRACK(
        BackgroundTaskStarted_Partition,
        requestInfo->CallContext->LWOrbit,
        "TrimFreshLog",
        static_cast<ui32>(PartitionConfig.GetStorageMediaKind()),
        requestInfo->CallContext->RequestId,
        PartitionConfig.GetDiskId());

    auto replyError = [=] (
        const TActorContext& ctx,
        TRequestInfo& requestInfo,
        ui32 errorCode,
        TString errorReason)
    {
        using TResponse = TEvPartitionCommonPrivate::TEvTrimFreshLogResponse;
        auto response = std::make_unique<TResponse>(
            MakeError(errorCode, std::move(errorReason))
        );

        BLOCKSTORE_TRACE_SENT(ctx, &requestInfo.TraceId, this, response);

        LWTRACK(
            ResponseSent_Partition,
            requestInfo.CallContext->LWOrbit,
            "TrimFreshLog",
            requestInfo.CallContext->RequestId);

        NCloud::Reply(ctx, requestInfo, std::move(response));
    };

    if (State->GetTrimFreshLogState().Status == EOperationStatus::Started) {
        replyError(ctx, *requestInfo, E_TRY_AGAIN, "trim already started");
        return;
    }

    ui64 trimFreshLogToCommitId = Min(
        State->GetLastFlushToCommitId(),
        State->GetTrimFreshLogBarriers().GetMinCommitId() - 1);

    auto collectCounter = State->NextCollectCounter();
    if (collectCounter == InvalidCollectCounter) {
        RebootPartitionOnCollectCounterOverflow(ctx, "TrimFreshLog");
        return;
    }

    LOG_DEBUG(ctx, TBlockStoreComponents::PARTITION,
        "[%lu] Start TrimFreshLog @%lu @%lu",
        TabletID(),
        trimFreshLogToCommitId,
        collectCounter);

    State->GetTrimFreshLogState().SetStatus(EOperationStatus::Started);

    TVector<ui32> freshChannels = State->GetChannelsByKind([](auto kind) {
        return kind == EChannelDataKind::Fresh;
    });

    auto actor = NCloud::Register<TTrimFreshLogActor>(
        ctx,
        requestInfo,
        SelfId(),
        Info(),
        trimFreshLogToCommitId,
        ParseCommitId(State->GetLastCommitId()).first,
        collectCounter,
        std::move(freshChannels));

    Actors.insert(actor);
}

void TPartitionActor::HandleTrimFreshLogCompleted(
    const TEvPartitionCommonPrivate::TEvTrimFreshLogCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    if (FAILED(msg->GetStatus())) {
        LOG_ERROR_S(ctx, TBlockStoreComponents::PARTITION,
            "[" << TabletID() << "]"
                << " TrimFreshLog failed: " << msg->GetStatus()
                << " reason: " << msg->GetError().GetMessage().Quote());

        State->RegisterTrimFreshLogError();
    } else {
        LOG_DEBUG(ctx, TBlockStoreComponents::PARTITION,
            "[%lu] TrimFreshLog completed",
            TabletID());

        State->RegisterTrimFreshLogSuccess();
        State->SetLastTrimFreshLogToCommitId(msg->CommitId);
        State->TrimFreshBlobs(msg->CommitId);
    }

    State->GetTrimFreshLogState().SetStatus(EOperationStatus::Idle);

    Actors.erase(ev->Sender);

    UpdateCPUUsageStat(CyclesToDurationSafe(msg->ExecCycles).MicroSeconds());
    UpdateExecutorStats(ctx);

    EnqueueTrimFreshLogIfNeeded(ctx);

    auto time = CyclesToDurationSafe(msg->TotalCycles).MicroSeconds();
    PartCounters->RequestCounters.TrimFreshLog.AddRequest(time);
}

}   // namespace NCloud::NBlockStore::NStorage::NPartition
