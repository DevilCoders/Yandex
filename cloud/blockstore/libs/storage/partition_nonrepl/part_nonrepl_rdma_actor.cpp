#include "part_nonrepl_rdma_actor.h"

#include <cloud/blockstore/libs/diagnostics/critical_events.h>
#include <cloud/blockstore/libs/diagnostics/public.h>
#include <cloud/blockstore/libs/rdma/protobuf.h>
#include <cloud/blockstore/libs/service_local/rdma_protocol.h>
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

struct TRdmaHandler: NRdma::IClientHandler
{
    void HandleResponse(
        NRdma::TClientRequestPtr req,
        ui32 status,
        size_t responseBytes) override
    {
        TRdmaContext* rdmaContext = static_cast<TRdmaContext*>(req->Context);
        rdmaContext->RequestHandler->HandleResponse(
            std::move(req),
            status,
            responseBytes);
    }
};

////////////////////////////////////////////////////////////////////////////////

TNonreplicatedPartitionRdmaActor::TNonreplicatedPartitionRdmaActor(
        TStorageConfigPtr config,
        TNonreplicatedPartitionConfigPtr partConfig,
        NRdma::IClientPtr rdmaClient,
        TActorId statActorId)
    : Config(std::move(config))
    , PartConfig(std::move(partConfig))
    , RdmaClient(std::move(rdmaClient))
    , StatActorId(statActorId)
    , PartCounters(CreatePartitionDiskCounters(EPublishingPolicy::NonRepl))
{
    ActivityType = TBlockStoreActivities::PARTITION;
}

TNonreplicatedPartitionRdmaActor::~TNonreplicatedPartitionRdmaActor()
{
}

void TNonreplicatedPartitionRdmaActor::Bootstrap(const TActorContext& ctx)
{
    Handler = std::make_shared<TRdmaHandler>();

    for (const auto& d: PartConfig->GetDevices()) {
        auto& ep = AgentId2EndpointFuture[d.GetAgentId()];
        if (ep.Initialized()) {
            continue;
        }

        ep = RdmaClient->StartEndpoint(
            d.GetAgentId(),
            Config->GetRdmaTargetPort(),
            Handler);
    }

    Become(&TThis::StateWork);
    ScheduleCountersUpdate(ctx);
    ctx.Schedule(
        Config->GetNonReplicatedMinRequestTimeout(),
        new TEvents::TEvWakeup());
}

bool TNonreplicatedPartitionRdmaActor::CheckReadWriteBlockRange(
    const TBlockRange64& range) const
{
    return range.End >= range.Start && PartConfig->GetBlockCount() > range.End;
}

void TNonreplicatedPartitionRdmaActor::ScheduleCountersUpdate(
    const TActorContext& ctx)
{
    if (!UpdateCountersScheduled) {
        ctx.Schedule(UpdateCountersInterval,
            new TEvNonreplPartitionPrivate::TEvUpdateCounters());
        UpdateCountersScheduled = true;
    }
}

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
bool TNonreplicatedPartitionRdmaActor::InitRequests(
    const NActors::TActorContext& ctx,
    TRequestInfo& requestInfo,
    const TBlockRange64& blockRange,
    TVector<TDeviceRequest>* deviceRequests)
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

    if (!CheckReadWriteBlockRange(blockRange)) {
        reply(ctx, requestInfo, MakeError(E_ARGUMENT, TStringBuilder()
            << "invalid block range ["
            << "index: " << blockRange.Start
            << ", count: " << blockRange.Size()
            << "]"));
        return false;
    }

    *deviceRequests = PartConfig->ToDeviceRequests(blockRange);

    if (deviceRequests->empty()) {
        // block range contains only dummy devices
        reply(ctx, requestInfo, NProto::TError());
        return false;
    }

    for (auto& r: *deviceRequests) {
        auto& ep = AgentId2Endpoint[r.Device.GetAgentId()];
        if (!ep) {
            auto* f = AgentId2EndpointFuture.FindPtr(r.Device.GetAgentId());
            if (!f) {
                Y_VERIFY_DEBUG(0);

                reply(ctx, requestInfo, MakeError(
                    E_INVALID_STATE,
                    TStringBuilder() << "endpoint not found for agent: "
                    << r.Device.GetAgentId()));
                return false;
            }

            if (f->HasException()) {
                SendRdmaUnavailableIfNeeded(ctx);

                reply(ctx, requestInfo, MakeError(
                    E_REJECTED,
                    TStringBuilder() << "endpoint init failed for agent: "
                    << r.Device.GetAgentId()));
                return false;
            }

            if (!f->HasValue()) {
                reply(ctx, requestInfo, MakeError(
                    E_REJECTED,
                    TStringBuilder() << "endpoint not initialized for agent: "
                    << r.Device.GetAgentId()));
                return false;
            }

            ep = f->GetValue();
        }
    }

    return true;
}

template bool TNonreplicatedPartitionRdmaActor::InitRequests<TEvService::TWriteBlocksMethod>(
    const TActorContext& ctx,
    TRequestInfo& requestInfo,
    const TBlockRange64& blockRange,
    TVector<TDeviceRequest>* deviceRequests);

template bool TNonreplicatedPartitionRdmaActor::InitRequests<TEvService::TWriteBlocksLocalMethod>(
    const TActorContext& ctx,
    TRequestInfo& requestInfo,
    const TBlockRange64& blockRange,
    TVector<TDeviceRequest>* deviceRequests);

template bool TNonreplicatedPartitionRdmaActor::InitRequests<TEvService::TZeroBlocksMethod>(
    const TActorContext& ctx,
    TRequestInfo& requestInfo,
    const TBlockRange64& blockRange,
    TVector<TDeviceRequest>* deviceRequests);

template bool TNonreplicatedPartitionRdmaActor::InitRequests<TEvService::TReadBlocksMethod>(
    const TActorContext& ctx,
    TRequestInfo& requestInfo,
    const TBlockRange64& blockRange,
    TVector<TDeviceRequest>* deviceRequests);

template bool TNonreplicatedPartitionRdmaActor::InitRequests<TEvService::TReadBlocksLocalMethod>(
    const TActorContext& ctx,
    TRequestInfo& requestInfo,
    const TBlockRange64& blockRange,
    TVector<TDeviceRequest>* deviceRequests);

////////////////////////////////////////////////////////////////////////////////

NProto::TError TNonreplicatedPartitionRdmaActor::SendReadRequests(
    const NActors::TActorContext& ctx,
    TCallContextPtr callContext,
    const NProto::THeaders& headers,
    const TString& sessionId,
    NRdma::IClientHandler* handler,
    const TVector<TDeviceRequest>& deviceRequests)
{
    auto* serializer = TBlockStoreProtocol::Serializer();

    struct TDeviceRequestInfo
    {
        NRdma::IClientEndpointPtr Endpoint;
        NRdma::TClientRequestPtr ClientRequest;
        std::unique_ptr<TDeviceReadRequestContext> DeviceRequestContext;
    };

    TVector<TDeviceRequestInfo> requests;

    ui64 startBlockIndexOffset = 0;
    for (auto& r: deviceRequests) {
        auto& ep = AgentId2Endpoint[r.Device.GetAgentId()];
        Y_VERIFY(ep);
        auto dr = std::make_unique<TDeviceReadRequestContext>();
        dr->Endpoint = ep;
        dr->RequestHandler = handler;

        ui64 sz = r.DeviceBlockRange.Size() * PartConfig->GetBlockSize();
        dr->StartIndexOffset = startBlockIndexOffset;
        startBlockIndexOffset += r.DeviceBlockRange.Size();

        NProto::TReadDeviceBlocksRequest deviceRequest;
        deviceRequest.MutableHeaders()->CopyFrom(headers);
        deviceRequest.SetDeviceUUID(r.Device.GetDeviceUUID());
        deviceRequest.SetStartIndex(r.DeviceBlockRange.Start);
        deviceRequest.SetBlockSize(PartConfig->GetBlockSize());
        deviceRequest.SetBlocksCount(r.DeviceBlockRange.Size());
        deviceRequest.SetSessionId(sessionId);

        auto [req, err] = ep->AllocateRequest(
            &*dr,
            serializer->MessageByteSize(deviceRequest, 0),
            4_KB + sz);

        if (HasError(err)) {
            LOG_ERROR(ctx, TBlockStoreComponents::PARTITION,
                "Failed to allocate rdma memory for ReadDeviceBlocksRequest"
                ", error: %s",
                FormatError(err).c_str());

            for (auto& request: requests) {
                request.Endpoint->FreeRequest(std::move(request.ClientRequest));
            }

            return err;
        }

        serializer->Serialize(
            req->RequestBuffer,
            TBlockStoreProtocol::ReadDeviceBlocksRequest,
            deviceRequest,
            TContIOVector(nullptr, 0));

        requests.push_back({ep, std::move(req), std::move(dr)});
    }

    for (auto& request: requests) {
        request.Endpoint->SendRequest(
            std::move(request.ClientRequest),
            callContext);
        request.DeviceRequestContext.release();
    }

    return {};
}

////////////////////////////////////////////////////////////////////////////////

void TNonreplicatedPartitionRdmaActor::SendRdmaUnavailableIfNeeded(
    const TActorContext& ctx)
{
    if (SentRdmaUnavailableNotification) {
        return;
    }

    NCloud::Send(
        ctx,
        PartConfig->GetParentActorId(),
        std::make_unique<TEvVolume::TEvRdmaUnavailable>()
    );

    ReportRdmaError();

    SentRdmaUnavailableNotification = true;
}

////////////////////////////////////////////////////////////////////////////////

void TNonreplicatedPartitionRdmaActor::HandleReadBlocksCompleted(
    const TEvNonreplPartitionPrivate::TEvReadBlocksCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    LOG_TRACE(ctx, TBlockStoreComponents::PARTITION,
        "[%s] Complete read blocks", SelfId().ToString().c_str());

    const auto requestBytes = msg->Stats.GetUserReadCounters().GetBlocksCount()
        * PartConfig->GetBlockSize();
    const auto time = CyclesToDurationSafe(msg->TotalCycles).MicroSeconds();
    PartCounters->RequestCounters.ReadBlocks.AddRequest(time, requestBytes);

    RequestsInProgress.erase(ev->Cookie);

    if (RequestsInProgress.empty() && Poisoner) {
        ReplyAndDie(ctx);
    }
}

void TNonreplicatedPartitionRdmaActor::HandleWriteBlocksCompleted(
    const TEvNonreplPartitionPrivate::TEvWriteBlocksCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    LOG_TRACE(ctx, TBlockStoreComponents::PARTITION,
        "[%s] Complete write blocks", SelfId().ToString().c_str());

    const auto requestBytes = msg->Stats.GetUserWriteCounters().GetBlocksCount()
        * PartConfig->GetBlockSize();
    const auto time = CyclesToDurationSafe(msg->TotalCycles).MicroSeconds();
    PartCounters->RequestCounters.WriteBlocks.AddRequest(time, requestBytes);

    RequestsInProgress.erase(ev->Cookie);

    if (RequestsInProgress.empty() && Poisoner) {
        ReplyAndDie(ctx);
    }
}

void TNonreplicatedPartitionRdmaActor::HandleZeroBlocksCompleted(
    const TEvNonreplPartitionPrivate::TEvZeroBlocksCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    LOG_TRACE(ctx, TBlockStoreComponents::PARTITION,
        "[%s] Complete zero blocks", SelfId().ToString().c_str());

    const auto requestBytes = msg->Stats.GetUserWriteCounters().GetBlocksCount()
        * PartConfig->GetBlockSize();
    const auto time = CyclesToDurationSafe(msg->TotalCycles).MicroSeconds();
    PartCounters->RequestCounters.ZeroBlocks.AddRequest(time, requestBytes);

    RequestsInProgress.erase(ev->Cookie);

    if (RequestsInProgress.empty() && Poisoner) {
        ReplyAndDie(ctx);
    }
}

////////////////////////////////////////////////////////////////////////////////

void TNonreplicatedPartitionRdmaActor::HandleUpdateCounters(
    const TEvNonreplPartitionPrivate::TEvUpdateCounters::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    UpdateCountersScheduled = false;

    SendStats(ctx);
    ScheduleCountersUpdate(ctx);
}

void TNonreplicatedPartitionRdmaActor::HandleWakeup(
    const TEvents::TEvWakeup::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    // TODO timeout logic?

    ctx.Schedule(
        Config->GetNonReplicatedMinRequestTimeout(),
        new TEvents::TEvWakeup());
}

void TNonreplicatedPartitionRdmaActor::ReplyAndDie(const NActors::TActorContext& ctx)
{
    NCloud::Reply(ctx, *Poisoner, std::make_unique<TEvents::TEvPoisonTaken>());
    Die(ctx);
}

void TNonreplicatedPartitionRdmaActor::HandlePoisonPill(
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

bool TNonreplicatedPartitionRdmaActor::HandleRequests(STFUNC_SIG)
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
    void TNonreplicatedPartitionRdmaActor::Handle##name(                       \
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

STFUNC(TNonreplicatedPartitionRdmaActor::StateWork)
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

STFUNC(TNonreplicatedPartitionRdmaActor::StateZombie)
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
