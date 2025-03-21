#include "volume_actor.h"

#include "volume_database.h"

#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/storage/api/disk_registry.h>
#include <cloud/blockstore/libs/storage/api/disk_registry_proxy.h>
#include <cloud/blockstore/libs/storage/core/probes.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>
#include <cloud/blockstore/libs/storage/core/request_info.h>

#include <cloud/storage/core/libs/common/media.h>

#include <util/generic/guid.h>
#include <util/generic/scope.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::AcquireDisk(
    const TActorContext& ctx,
    TString clientId,
    NProto::EVolumeAccessMode accessMode,
    ui64 mountSeqNumber)
{
    Y_VERIFY(State);

    LOG_DEBUG_S(
        ctx,
        TBlockStoreComponents::VOLUME,
        "Acquiring disk " << State->GetDiskId()
    );

    auto request = std::make_unique<TEvDiskRegistry::TEvAcquireDiskRequest>();

    request->Record.SetDiskId(State->GetDiskId());
    request->Record.SetSessionId(std::move(clientId));
    request->Record.SetAccessMode(accessMode);
    request->Record.SetMountSeqNumber(mountSeqNumber);

    NCloud::Send(
        ctx,
        MakeDiskRegistryProxyServiceId(),
        std::move(request));
}

void TVolumeActor::ProcessNextAcquireReleaseDiskRequest(const TActorContext& ctx)
{
    if (AcquireReleaseDiskRequests) {
        auto& request = AcquireReleaseDiskRequests.front();

        if (request.IsAcquire) {
            AcquireDisk(
                ctx,
                request.ClientId,
                request.AccessMode,
                request.MountSeqNumber
            );
        } else {
            ReleaseDisk(ctx, request.ClientId);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::AcquireDiskIfNeeded(const TActorContext& ctx)
{
    if (!State->GetClients()) {
        return;
    }

    bool queueEmpty = AcquireReleaseDiskRequests.empty();

    for (const auto& x: State->GetClients()) {
        bool skip = false;
        for (const auto& clientRequest: PendingClientRequests) {
            if (x.first == clientRequest->GetClientId()) {
                // inflight client request will cause the right acquire/release
                // op anyway, so we should not interfere
                skip = true;
                break;
            }
        }

        if (skip) {
            continue;
        }

        TAcquireReleaseDiskRequest request(
            x.first,
            x.second.GetVolumeClientInfo().GetVolumeAccessMode(),
            x.second.GetVolumeClientInfo().GetMountSeqNumber(),
            nullptr
        );

        LOG_DEBUG_S(
            ctx,
            TBlockStoreComponents::VOLUME,
            "Reacquiring disk " << State->GetDiskId()
                << " for client " << request.ClientId
                << " with access mode " << static_cast<int>(request.AccessMode)
        );

        AcquireReleaseDiskRequests.push_back(std::move(request));
    }

    if (queueEmpty) {
        ProcessNextAcquireReleaseDiskRequest(ctx);
    }
}

void TVolumeActor::ScheduleAcquireDiskIfNeeded(const TActorContext& ctx)
{
    if (AcquireDiskScheduled) {
        return;
    }

    AcquireDiskScheduled = true;

    ctx.Schedule(
        Config->GetClientRemountPeriod(),
        new TEvVolumePrivate::TEvAcquireDiskIfNeeded()
    );
}

void TVolumeActor::HandleAcquireDiskIfNeeded(
    const TEvVolumePrivate::TEvAcquireDiskIfNeeded::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    AcquireDiskIfNeeded(ctx);

    AcquireDiskScheduled = false;
}

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::HandleReacquireDisk(
    const TEvVolume::TEvReacquireDisk::TPtr& ev,
    const NActors::TActorContext& ctx)
{
    Y_UNUSED(ev);

    AcquireDiskIfNeeded(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::HandleAcquireDiskResponse(
    const TEvDiskRegistry::TEvAcquireDiskResponse::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();
    // NOTE: record.GetDevices() contains only the devices located at available
    // agents
    auto& record = msg->Record;

    ScheduleAcquireDiskIfNeeded(ctx);

    if (AcquireReleaseDiskRequests.empty()) {
        LOG_DEBUG_S(
            ctx,
            TBlockStoreComponents::VOLUME,
            "Unexpected TEvAcquireDiskResponse for disk " << State->GetDiskId()
        );

        return;
    }

    auto& request = AcquireReleaseDiskRequests.front();
    auto& cr = request.ClientRequest;

    if (HasError(record.GetError())) {
        LOG_DEBUG_S(
            ctx,
            TBlockStoreComponents::VOLUME,
            "Can't acquire disk " << State->GetDiskId()
        );

        if (cr) {
            auto response = std::make_unique<TEvVolume::TEvAddClientResponse>(
                record.GetError());
            response->Record.MutableVolume()->SetDiskId(cr->DiskId);
            response->Record.SetClientId(cr->GetClientId());
            response->Record.SetTabletId(TabletID());

            BLOCKSTORE_TRACE_SENT(
                ctx,
                &cr->RequestInfo->TraceId,
                this,
                response
            );
            NCloud::Reply(ctx, *cr->RequestInfo, std::move(response));

            PendingClientRequests.pop_front();
            ProcessNextPendingClientRequest(ctx);
        }
    } else if (cr) {
        ExecuteTx<TAddClient>(
            ctx,
            cr->RequestInfo,
            cr->DiskId,
            cr->PipeServerActorId,
            cr->AddedClientInfo
        );
    }

    AcquireReleaseDiskRequests.pop_front();
    ProcessNextAcquireReleaseDiskRequest(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::HandleAddClient(
    const TEvVolume::TEvAddClientRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_VOLUME_COUNTER(AddClient);

    const auto* msg = ev->Get();
    const auto& diskId = msg->Record.GetDiskId();
    const auto& clientId = GetClientId(*msg);
    const auto accessMode = msg->Record.GetVolumeAccessMode();
    const auto mountMode = msg->Record.GetVolumeMountMode();
    const auto mountFlags = msg->Record.GetMountFlags();
    const auto mountSeqNumber = msg->Record.GetMountSeqNumber();
    const auto& host = msg->Record.GetHost();

    // If event was forwarded through pipe, its recipient and recipient rewrite
    // would be different
    TActorId pipeServerActorId;
    if (ev->Recipient != ev->GetRecipientRewrite()) {
        pipeServerActorId = ev->Recipient;
    }

    LOG_INFO(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Volume %s received add client %s request; pipe server %s, sender %s",
        TabletID(),
        diskId.Quote().c_str(),
        clientId.Quote().c_str(),
        ToString(pipeServerActorId).c_str(),
        ToString(ev->Sender).c_str());

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    NProto::TVolumeClientInfo clientInfo;
    clientInfo.SetClientId(clientId);
    clientInfo.SetVolumeAccessMode(accessMode);
    clientInfo.SetVolumeMountMode(mountMode);
    clientInfo.SetMountFlags(mountFlags);
    clientInfo.SetMountSeqNumber(mountSeqNumber);
    clientInfo.SetHost(host);

    auto request = std::make_shared<TClientRequest>(
        std::move(requestInfo),
        diskId,
        pipeServerActorId,
        std::move(clientInfo));
    PendingClientRequests.emplace_back(std::move(request));

    if (PendingClientRequests.size() == 1) {
        ProcessNextPendingClientRequest(ctx);
    } else {
        LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
            "[%lu] Postponing AddClientRequest[%s] for volume %s: another request in flight",
            TabletID(),
            clientId.Quote().data(),
            diskId.Quote().data());
    }
}

void TVolumeActor::ProcessNextPendingClientRequest(const TActorContext& ctx)
{
    Y_VERIFY(State);

    if (PendingClientRequests) {
        auto& request = PendingClientRequests.front();
        const auto mediaKind =
            State->GetMeta().GetConfig().GetStorageMediaKind();

        if (IsDiskRegistryMediaKind(mediaKind)
            && Config->GetAcquireNonReplicatedDevices())
        {
            if (request->RemovedClientId) {
                AcquireReleaseDiskRequests.emplace_back(
                    request->RemovedClientId,
                    request
                );
            } else {
                AcquireReleaseDiskRequests.emplace_back(
                    request->AddedClientInfo.GetClientId(),
                    request->AddedClientInfo.GetVolumeAccessMode(),
                    request->AddedClientInfo.GetMountSeqNumber(),
                    request
                );
            }

            if (AcquireReleaseDiskRequests.size() == 1) {
                ProcessNextAcquireReleaseDiskRequest(ctx);
            } else {
                LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
                    "[%lu] Postponing AcquireReleaseRequest[%s] for volume %s: another request in flight",
                    TabletID(),
                    AcquireReleaseDiskRequests.back().ClientId.Quote().data(),
                    State->GetDiskId().Quote().data());
            }

            return;
        }

        if (request->RemovedClientId) {
            ExecuteTx<TRemoveClient>(
                ctx,
                request->RequestInfo,
                request->DiskId,
                request->PipeServerActorId,
                request->RemovedClientId,
                request->IsMonRequest);
        } else {
            ExecuteTx<TAddClient>(
                ctx,
                request->RequestInfo,
                request->DiskId,
                request->PipeServerActorId,
                request->AddedClientInfo);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

bool TVolumeActor::PrepareAddClient(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxVolume::TAddClient& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TVolumeActor::ExecuteAddClient(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxVolume::TAddClient& args)
{
    Y_VERIFY(State);

    auto now = ctx.Now();
    TString prevWriter = State->GetReadWriteAccessClientId();

    auto res = State->AddClient(
        args.Info,
        args.PipeServerActorId,
        args.RequestInfo->Sender,
        now);
    args.Error = std::move(res.Error);

    TVolumeDatabase db(tx.DB);
    db.WriteHistory(
        State->LogAddClient(
            ctx.Now(),
            args.Info,
            args.Error,
            args.PipeServerActorId,
            args.RequestInfo->Sender));

    if (SUCCEEDED(args.Error.GetCode())) {
        if (prevWriter && prevWriter != State->GetReadWriteAccessClientId()) {
            const auto& clients = State->GetClients();
            auto it = clients.find(prevWriter);
            if (it != clients.end()) {
                LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
                    "[%lu] Replace %s writer with %s",
                    TabletID(),
                    prevWriter.Quote().data(),
                    State->GetReadWriteAccessClientId().Quote().data());
            }
        }

        for (const auto& clientId: res.RemovedClientIds) {
            db.RemoveClient(clientId);
            State->RemoveClient(clientId, TActorId());

            auto builder = TStringBuilder() << "Preempted by " << args.Info.GetClientId();
            db.WriteHistory(
                State->LogRemoveClient(ctx.Now(), clientId, builder, {}));
        }

        TVector<TString> staleClientIds;
        for (const auto& pair: State->GetClients()) {
            const auto& clientId = pair.first;
            const auto& clientInfo = pair.second;
            if (State->IsClientStale(clientInfo, now)) {
                staleClientIds.push_back(clientId);
            }
        }

        for (const auto& clientId: staleClientIds) {
            db.RemoveClient(clientId);
            State->RemoveClient(clientId, TActorId());
            db.WriteHistory(
                State->LogRemoveClient(ctx.Now(), clientId, "Stale", {}));
        }

        db.WriteClient(args.Info);
    }
}

void TVolumeActor::CompleteAddClient(
    const TActorContext& ctx,
    TTxVolume::TAddClient& args)
{
    Y_DEFER {
        PendingClientRequests.pop_front();
        ProcessNextPendingClientRequest(ctx);
    };

    const auto& clientId = args.Info.GetClientId();
    const auto& diskId = args.DiskId;

    if (FAILED(args.Error.GetCode())) {
        auto response = std::make_unique<TEvVolume::TEvAddClientResponse>(
            std::move(args.Error));
        response->Record.MutableVolume()->SetDiskId(diskId);
        response->Record.SetClientId(clientId);
        response->Record.SetTabletId(TabletID());

        BLOCKSTORE_TRACE_SENT(ctx, &args.RequestInfo->TraceId, this, response);
        NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
        return;
    }

    LOG_DEBUG(ctx, TBlockStoreComponents::VOLUME,
        "[%lu] Added client %s to volume %s",
        TabletID(),
        clientId.Quote().data(),
        diskId.Quote().data());

    auto response = std::make_unique<TEvVolume::TEvAddClientResponse>();
    *response->Record.MutableError() = std::move(args.Error);
    response->Record.SetTabletId(TabletID());
    response->Record.SetClientId(clientId);

    auto& config = State->GetMeta().GetConfig();
    auto& volumeConfig = State->GetMeta().GetVolumeConfig();
    auto* volumeInfo = response->Record.MutableVolume();
    VolumeConfigToVolume(volumeConfig, *volumeInfo);
    volumeInfo->SetInstanceId(config.GetInstanceId());  // XXX ???
    State->FillDeviceInfo(*volumeInfo);

    BLOCKSTORE_TRACE_SENT(ctx, &args.RequestInfo->TraceId, this, response);

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));

    OnClientListUpdate(ctx);
}

void TVolumeActor::OnClientListUpdate(const NActors::TActorContext& ctx)
{
    if (State->GetNonreplicatedPartitionActor()) {
        NCloud::Send(
            ctx,
            State->GetNonreplicatedPartitionActor(),
            std::make_unique<TEvVolume::TEvRWClientIdChanged>(
                State->GetReadWriteAccessClientId()));
    }
}

}   // namespace NCloud::NBlockStore::NStorage
