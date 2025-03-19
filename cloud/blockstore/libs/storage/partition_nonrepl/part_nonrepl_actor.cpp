#include "part_nonrepl_actor.h"

#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/forward_helpers.h>
#include <cloud/blockstore/libs/storage/core/probes.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>
#include <cloud/blockstore/libs/storage/core/unimplemented.h>

#include <ydb/core/base/appdata.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

////////////////////////////////////////////////////////////////////////////////

TNonreplicatedPartitionActor::TNonreplicatedPartitionActor(
        TStorageConfigPtr config,
        TNonreplicatedPartitionConfigPtr partConfig,
        TActorId statActorId)
    : Config(std::move(config))
    , PartConfig(std::move(partConfig))
    , StatActorId(statActorId)
    , DeviceStats(PartConfig->GetDevices().size())
    , PartCounters(CreatePartitionDiskCounters(EPublishingPolicy::NonRepl))
{
    ActivityType = TBlockStoreActivities::PARTITION;
}

TNonreplicatedPartitionActor::~TNonreplicatedPartitionActor()
{
}

void TNonreplicatedPartitionActor::Bootstrap(const TActorContext& ctx)
{
    Become(&TThis::StateWork);
    ScheduleCountersUpdate(ctx);
    ctx.Schedule(Config->GetNonReplicatedMinRequestTimeout(), new TEvents::TEvWakeup());
}

bool TNonreplicatedPartitionActor::CheckReadWriteBlockRange(const TBlockRange64& range) const
{
    return range.End >= range.Start && PartConfig->GetBlockCount() > range.End;
}

void TNonreplicatedPartitionActor::ScheduleCountersUpdate(const TActorContext& ctx)
{
    if (!UpdateCountersScheduled) {
        ctx.Schedule(UpdateCountersInterval,
            new TEvNonreplPartitionPrivate::TEvUpdateCounters());
        UpdateCountersScheduled = true;
    }
}

bool TNonreplicatedPartitionActor::IsInflightLimitReached() const
{
    return RequestsInProgress.size() >= Config->GetNonReplicatedInflightLimit();
}

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
bool TNonreplicatedPartitionActor::InitRequests(
    const typename TMethod::TRequest& msg,
    const NActors::TActorContext& ctx,
    TRequestInfo& requestInfo,
    const TBlockRange64& blockRange,
    TVector<TDeviceRequest>* deviceRequests,
    TRequest* request)
{
    auto reply = [=] (
        const TActorContext& ctx,
        TRequestInfo& requestInfo,
        NProto::TError error)
    {
        auto response = std::make_unique<typename TMethod::TResponse>(
            std::move(error));

        BLOCKSTORE_TRACE_SENT(ctx, &requestInfo.TraceId, this, response);

        LWTRACK(
            ResponseSent_Partition,
            requestInfo.CallContext->LWOrbit,
            TMethod::Name,
            requestInfo.CallContext->RequestId);

        NCloud::Reply(ctx, requestInfo, std::move(response));
    };

    if (IsInflightLimitReached()) {
        reply(ctx, requestInfo, MakeError(E_REJECTED, "Inflight limit reached"));
        return false;
    }

    if (!CheckReadWriteBlockRange(blockRange)) {
        reply(ctx, requestInfo, MakeError(E_ARGUMENT, TStringBuilder()
            << "invalid block range ["
            << "index: " << blockRange.Start
            << ", count: " << blockRange.Size()
            << "]"));
        return false;
    }

    if (!msg.Record.GetHeaders().GetIsBackgroundRequest()
            && RequiresReadWriteAccess<TMethod>())
    {
        if (PartConfig->IsReadOnly()) {
            reply(ctx, requestInfo, PartConfig->MakeIOError("disk in error state"));
            return false;
        }

        if (HasBrokenDevice) {
            reply(ctx, requestInfo, PartConfig->MakeIOError("disk has broken device"));
            return false;
        }
    } else if (RequiresCheckpointSupport(msg.Record)) {
        reply(ctx, requestInfo, MakeError(E_ARGUMENT, TStringBuilder()
            << "checkpoints not supported"));
        return false;
    }

    *deviceRequests = PartConfig->ToDeviceRequests(blockRange);

    if (deviceRequests->empty()) {
        // block range contains only dummy devices
        reply(ctx, requestInfo, NProto::TError());
        return false;
    }

    request->Ts = ctx.Now();
    request->Timeout = Config->GetNonReplicatedMinRequestTimeout();
    for (const auto& dr: *deviceRequests) {
        const auto& deviceStat = DeviceStats[dr.DeviceIdx];
        if (deviceStat.TimedOutStateDuration
                > Config->GetMaxTimedOutDeviceStateDuration())
        {
            HasBrokenDevice = true;

            reply(ctx, requestInfo, PartConfig->MakeIOError(TStringBuilder()
                << "broken device requested: "
                << dr.Device.GetDeviceUUID()));
            return false;
        }

        if (dr.Device.GetNodeId() == 0) {
            HasBrokenDevice = true;

            reply(ctx, requestInfo, PartConfig->MakeIOError(TStringBuilder()
                << "unavailable device requested: "
                << dr.Device.GetDeviceUUID()));
            return false;
        }

        request->DeviceIndices.push_back(dr.DeviceIdx);
        if (deviceStat.CurrentTimeout > request->Timeout) {
            request->Timeout = deviceStat.CurrentTimeout;
        }
    }

    return true;
}

template bool TNonreplicatedPartitionActor::InitRequests<TEvService::TWriteBlocksMethod>(
    const TEvService::TWriteBlocksMethod::TRequest& msg,
    const TActorContext& ctx,
    TRequestInfo& requestInfo,
    const TBlockRange64& blockRange,
    TVector<TDeviceRequest>* deviceRequests,
    TRequest* request);

template bool TNonreplicatedPartitionActor::InitRequests<TEvService::TWriteBlocksLocalMethod>(
    const TEvService::TWriteBlocksLocalMethod::TRequest& msg,
    const TActorContext& ctx,
    TRequestInfo& requestInfo,
    const TBlockRange64& blockRange,
    TVector<TDeviceRequest>* deviceRequests,
    TRequest* request);

template bool TNonreplicatedPartitionActor::InitRequests<TEvService::TZeroBlocksMethod>(
    const TEvService::TZeroBlocksMethod::TRequest& msg,
    const TActorContext& ctx,
    TRequestInfo& requestInfo,
    const TBlockRange64& blockRange,
    TVector<TDeviceRequest>* deviceRequests,
    TRequest* request);

template bool TNonreplicatedPartitionActor::InitRequests<TEvService::TReadBlocksMethod>(
    const TEvService::TReadBlocksMethod::TRequest& msg,
    const TActorContext& ctx,
    TRequestInfo& requestInfo,
    const TBlockRange64& blockRange,
    TVector<TDeviceRequest>* deviceRequests,
    TRequest* request);

template bool TNonreplicatedPartitionActor::InitRequests<TEvService::TReadBlocksLocalMethod>(
    const TEvService::TReadBlocksLocalMethod::TRequest& msg,
    const TActorContext& ctx,
    TRequestInfo& requestInfo,
    const TBlockRange64& blockRange,
    TVector<TDeviceRequest>* deviceRequests,
    TRequest* request);

////////////////////////////////////////////////////////////////////////////////

void TNonreplicatedPartitionActor::HandleUpdateCounters(
    const TEvNonreplPartitionPrivate::TEvUpdateCounters::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    UpdateCountersScheduled = false;

    SendStats(ctx);
    ScheduleCountersUpdate(ctx);
}

void TNonreplicatedPartitionActor::HandleWakeup(
    const TEvents::TEvWakeup::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    const auto now = ctx.Now();

    for (const auto& [id, request]: RequestsInProgress) {
        if (now - request.Ts < request.Timeout) {
            continue;
        }

        NCloud::Send<TEvents::TEvWakeup>(ctx, id);

        for (const auto i: request.DeviceIndices) {
            auto& deviceStat = DeviceStats[i];
            const auto timeoutWeight = Min(
                request.Timeout,
                now - deviceStat.LastTimeoutTs
            );
            deviceStat.TimedOutStateDuration += timeoutWeight;
            deviceStat.LastTimeoutTs = now;
            if (!deviceStat.CurrentTimeout.GetValue()) {
                deviceStat.CurrentTimeout =
                    Config->GetNonReplicatedMinRequestTimeout();
            }
            deviceStat.CurrentTimeout = Min(
                Config->GetNonReplicatedMaxRequestTimeout(),
                deviceStat.CurrentTimeout + timeoutWeight
            );
        }
    }

    ctx.Schedule(Config->GetNonReplicatedMinRequestTimeout(), new TEvents::TEvWakeup());
}

void TNonreplicatedPartitionActor::ReplyAndDie(const NActors::TActorContext& ctx)
{
    NCloud::Reply(ctx, *Poisoner, std::make_unique<TEvents::TEvPoisonTaken>());
    Die(ctx);
}

void TNonreplicatedPartitionActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Become(&TThis::StateZombie);

    Poisoner = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        MakeIntrusive<TCallContext>(),
        std::move(ev->TraceId));

    if (!RequestsInProgress.empty()) {
        return;
    }

    ReplyAndDie(ctx);
}

bool TNonreplicatedPartitionActor::HandleRequests(STFUNC_SIG)
{
    Y_UNUSED(ctx);
    switch (ev->GetTypeRewrite()) {
        // TODO

        default:
            return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_HANDLE_UNIMPLEMENTED_REQUEST(name, ns)                      \
    void TNonreplicatedPartitionActor::Handle##name(                           \
        const ns::TEv##name##Request::TPtr& ev,                                \
        const TActorContext& ctx)                                              \
    {                                                                          \
        RejectUnimplementedRequest<ns::T##name##Method>(ev, ctx);              \
    }                                                                          \
                                                                               \
// BLOCKSTORE_HANDLE_UNIMPLEMENTED_REQUEST

BLOCKSTORE_HANDLE_UNIMPLEMENTED_REQUEST(DescribeBlocks,           TEvVolume);
BLOCKSTORE_HANDLE_UNIMPLEMENTED_REQUEST(CompactRange,             TEvVolume);
BLOCKSTORE_HANDLE_UNIMPLEMENTED_REQUEST(GetCompactionStatus,      TEvVolume);
BLOCKSTORE_HANDLE_UNIMPLEMENTED_REQUEST(RebuildMetadata,          TEvVolume);
BLOCKSTORE_HANDLE_UNIMPLEMENTED_REQUEST(GetRebuildMetadataStatus, TEvVolume);

////////////////////////////////////////////////////////////////////////////////

/*
STFUNC(TNonreplicatedPartitionActor::StateInit)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvWakeup, HandleWakeup);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::PARTITION);
            break;
    }
}
*/

STFUNC(TNonreplicatedPartitionActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvNonreplPartitionPrivate::TEvUpdateCounters, HandleUpdateCounters);
        HFunc(TEvents::TEvWakeup, HandleWakeup);

        HFunc(TEvService::TEvReadBlocksRequest, HandleReadBlocks);
        HFunc(TEvService::TEvWriteBlocksRequest, HandleWriteBlocks);
        HFunc(TEvService::TEvZeroBlocksRequest, HandleZeroBlocks);

        HFunc(TEvService::TEvReadBlocksLocalRequest, HandleReadBlocksLocal);
        HFunc(TEvService::TEvWriteBlocksLocalRequest, HandleWriteBlocksLocal);

        HFunc(TEvNonreplPartitionPrivate::TEvReadBlocksCompleted, HandleReadBlocksCompleted);
        HFunc(TEvNonreplPartitionPrivate::TEvWriteBlocksCompleted, HandleWriteBlocksCompleted);
        HFunc(TEvNonreplPartitionPrivate::TEvZeroBlocksCompleted, HandleZeroBlocksCompleted);

        HFunc(TEvVolume::TEvDescribeBlocksRequest, HandleDescribeBlocks);
        HFunc(TEvVolume::TEvGetCompactionStatusRequest, HandleGetCompactionStatus);
        HFunc(TEvVolume::TEvCompactRangeRequest, HandleCompactRange);
        HFunc(TEvVolume::TEvRebuildMetadataRequest, HandleRebuildMetadata);
        HFunc(TEvVolume::TEvGetRebuildMetadataStatusRequest, HandleGetRebuildMetadataStatus);

        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        IgnoreFunc(TEvVolume::TEvRWClientIdChanged);

        default:
            if (!HandleRequests(ev, ctx)) {
                HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::PARTITION);
            }
            break;
    }
}

STFUNC(TNonreplicatedPartitionActor::StateZombie)
{
    switch (ev->GetTypeRewrite()) {
        IgnoreFunc(TEvNonreplPartitionPrivate::TEvUpdateCounters);

        HFunc(TEvents::TEvWakeup, HandleWakeup);

        HFunc(TEvService::TEvReadBlocksRequest, RejectReadBlocks);
        HFunc(TEvService::TEvWriteBlocksRequest, RejectWriteBlocks);
        HFunc(TEvService::TEvZeroBlocksRequest, RejectZeroBlocks);

        HFunc(TEvService::TEvReadBlocksLocalRequest, RejectReadBlocksLocal);
        HFunc(TEvService::TEvWriteBlocksLocalRequest, RejectWriteBlocksLocal);

        HFunc(TEvNonreplPartitionPrivate::TEvReadBlocksCompleted, HandleReadBlocksCompleted);
        HFunc(TEvNonreplPartitionPrivate::TEvWriteBlocksCompleted, HandleWriteBlocksCompleted);
        HFunc(TEvNonreplPartitionPrivate::TEvZeroBlocksCompleted, HandleZeroBlocksCompleted);

        HFunc(TEvVolume::TEvDescribeBlocksRequest, RejectDescribeBlocks);
        HFunc(TEvVolume::TEvGetCompactionStatusRequest, RejectGetCompactionStatus);
        HFunc(TEvVolume::TEvCompactRangeRequest, RejectCompactRange);
        HFunc(TEvVolume::TEvRebuildMetadataRequest, RejectRebuildMetadata);
        HFunc(TEvVolume::TEvGetRebuildMetadataStatusRequest, RejectGetRebuildMetadataStatus);

        IgnoreFunc(TEvents::TEvPoisonPill);
        IgnoreFunc(TEvVolume::TEvRWClientIdChanged);

        default:
            if (!HandleRequests(ev, ctx)) {
                HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::PARTITION);
            }
            break;
    }
}

}   // namespace NCloud::NBlockStore::NStorage
