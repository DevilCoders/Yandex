#include "volume_actor.h"

#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/storage/api/service.h>
#include <cloud/blockstore/libs/storage/core/disk_counters.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>

// TODO: invalid reference
#include <cloud/blockstore/libs/storage/service/service_events_private.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;
using namespace NPartition;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

void FillCheckpoints(
    TVector<TString> checkpoints,
    NProto::TStatVolumeResponse& response)
{
    for (auto& cp: checkpoints) {
        *response.MutableCheckpoints()->Add() = std::move(cp);
    }
}

////////////////////////////////////////////////////////////////////////////////

#define MERGE_FIELD(name)                                                      \
    target.Set##name(target.Get##name() + source.Get##name());                 \
// MERGE_FIELD

#define MERGE_FIELD_MAX(name)                                                  \
    target.Set##name(Max(target.Get##name(), source.Get##name()));             \
// MERGE_FIELD_MAX

void MergeIOCounters(const NProto::TIOCounters& source, NProto::TIOCounters& target)
{
    MERGE_FIELD(RequestsCount);
    MERGE_FIELD(BlocksCount);
    MERGE_FIELD(ExecTime);
    MERGE_FIELD(WaitTime);
    MERGE_FIELD(BatchCount);
}

void Merge(const NProto::TVolumeStats& source, NProto::TVolumeStats& target)
{
    MergeIOCounters(
        source.GetUserReadCounters(),
        *target.MutableUserReadCounters()
    );
    MergeIOCounters(
        source.GetUserWriteCounters(),
        *target.MutableUserWriteCounters()
    );
    MergeIOCounters(
        source.GetSysReadCounters(),
        *target.MutableSysReadCounters()
    );
    MergeIOCounters(
        source.GetSysWriteCounters(),
        *target.MutableSysWriteCounters()
    );

    MERGE_FIELD(MixedBlobsCount);
    MERGE_FIELD(MergedBlobsCount);

    MERGE_FIELD(FreshBlocksCount);
    MERGE_FIELD(MixedBlocksCount);
    MERGE_FIELD(MergedBlocksCount);
    MERGE_FIELD(UsedBlocksCount);
    MERGE_FIELD(LogicalUsedBlocksCount);
    MERGE_FIELD(GarbageBlocksCount);

    MERGE_FIELD(NonEmptyRangeCount);
    MERGE_FIELD(CheckpointBlocksCount);

    MERGE_FIELD_MAX(CompactionGarbageScore);

    MERGE_FIELD(GarbageQueueSize);

    MERGE_FIELD_MAX(CleanupDelay);
    MERGE_FIELD_MAX(CompactionDelay);
}

////////////////////////////////////////////////////////////////////////////////

class TStatPartitionActor final
    : public TActorBootstrapped<TStatPartitionActor>
{
private:
    const TRequestInfoPtr RequestInfo;
    const TVector<TActorId> PartActorIds;
    NProto::TStatVolumeResponse Record;
    TVector<TString> Checkpoints;
    ui32 Responses = 0;

public:
    TStatPartitionActor(
        TRequestInfoPtr requestInfo,
        TVector<TActorId> partActorIds,
        NProto::TStatVolumeResponse record,
        TVector<TString> checkpoints);

    void Bootstrap(const TActorContext& ctx);

private:
    STFUNC(StateWork);

    void HandleStatPartitionResponse(
        const TEvPartition::TEvStatPartitionResponse::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TStatPartitionActor::TStatPartitionActor(
        TRequestInfoPtr requestInfo,
        TVector<TActorId> partActorIds,
        NProto::TStatVolumeResponse record,
        TVector<TString> checkpoints)
    : RequestInfo(std::move(requestInfo))
    , PartActorIds(std::move(partActorIds))
    , Record(std::move(record))
    , Checkpoints(std::move(checkpoints))
{
    ActivityType = TBlockStoreActivities::VOLUME;
}

void TStatPartitionActor::Bootstrap(const TActorContext& ctx)
{
    for (const auto& partActorId: PartActorIds) {
        // TODO: handle undelivery
        NCloud::Send(
            ctx,
            partActorId,
            std::make_unique<TEvPartition::TEvStatPartitionRequest>()
        );
    }

    Become(&TThis::StateWork);
}

////////////////////////////////////////////////////////////////////////////////

void TStatPartitionActor::HandleStatPartitionResponse(
    const TEvPartition::TEvStatPartitionResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    if (FAILED(msg->GetStatus())) {
        Record.MutableError()->CopyFrom(msg->GetError());
    } else {
        auto& target = *Record.MutableStats();
        const auto& source = msg->Record.GetStats();
        Merge(source, target);
    }

    if (++Responses == PartActorIds.size() || FAILED(msg->GetStatus())) {
        auto response = std::make_unique<TEvService::TEvStatVolumeResponse>(
            Record
        );
        FillCheckpoints(std::move(Checkpoints), response->Record);

        LWTRACK(
            ResponseSent_Volume,
            RequestInfo->CallContext->LWOrbit,
            "StatVolume",
            RequestInfo->CallContext->RequestId);

        BLOCKSTORE_TRACE_SENT(ctx, &RequestInfo->TraceId, this, response);
        NCloud::Reply(ctx, *RequestInfo, std::move(response));

        Die(ctx);
    }
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TStatPartitionActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvPartition::TEvStatPartitionResponse, HandleStatPartitionResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::VOLUME);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::HandleStatVolume(
    const TEvService::TEvStatVolumeRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_VOLUME_COUNTER(StatVolume);

    auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo<TEvService::TStatVolumeMethod>(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        ev->TraceId.Clone());

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    LWTRACK(
        RequestReceived_Volume,
        requestInfo->CallContext->LWOrbit,
        "StatVolume",
        requestInfo->CallContext->RequestId);

    if (State->GetPartitionsState() != TPartitionInfo::READY) {
        StartPartitionsIfNeeded(ctx);

        if (!State->Ready()) {
            LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
                "[%lu] StatVolume request delayed until volume and partitions are ready",
                TabletID());

            PendingRequests.emplace_back(NActors::IEventHandlePtr(ev.Release()), requestInfo);
            return;
        }
    }

    NProto::TStatVolumeResponse record;
    auto* volume = record.MutableVolume();
    record.SetMountSeqNumber(
        State->GetMountSeqNumber());
    VolumeConfigToVolume(State->GetMeta().GetVolumeConfig(), *volume);

    auto* stats = record.MutableStats();
    stats->SetWriteAndZeroRequestsInFlight(WriteAndZeroRequestsInFlight.Size());
    stats->SetBoostBudget(
        State->GetThrottlingPolicy().GetCurrentBoostBudget().MilliSeconds());
    if (State->GetUsedBlocks()) {
        stats->SetVolumeUsedBlocksCount(State->GetUsedBlocks()->Count());
    }

    TVector<TString> checkpoints = State->GetActiveCheckpoints().Get();

    if (State->GetPartitions()) {
        TVector<TActorId> partActorIds;
        for (const auto& partition: State->GetPartitions()) {
            partActorIds.push_back(partition.Owner);
        }

        NCloud::Register<TStatPartitionActor>(
            ctx,
            std::move(requestInfo),
            std::move(partActorIds),
            std::move(record),
            std::move(checkpoints));
    } else {
        auto response = std::make_unique<TEvService::TEvStatVolumeResponse>();
        response->Record.CopyFrom(record);
        auto* volume = response->Record.MutableVolume();
        State->FillDeviceInfo(*volume);
        FillCheckpoints(std::move(checkpoints),response->Record);

        BLOCKSTORE_TRACE_SENT(ctx, &requestInfo->TraceId, this, response);

        LWTRACK(
            ResponseSent_Volume,
            requestInfo->CallContext->LWOrbit,
            "StatVolume",
            requestInfo->CallContext->RequestId);

        NCloud::Reply(ctx, *requestInfo, std::move(response));
    }
}

}   // namespace NCloud::NBlockStore::NStorage
