#include "disk_registry_actor.h"
#include "disk_registry_database.h"

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleRegisterAgent(
    const TEvDiskRegistry::TEvRegisterAgentRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(RegisterAgent);

    auto* msg = ev->Get();
    const auto& agentConfig = msg->Record.GetAgentConfig();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received RegisterAgent request: NodeId=%u, AgentId=%s"
        ", SeqNo=%lu, Dedicated=%s, Devices=[%s]",
        TabletID(),
        agentConfig.GetNodeId(),
        agentConfig.GetAgentId().c_str(),
        agentConfig.GetSeqNumber(),
        agentConfig.GetDedicatedDiskAgent() ? "true" : "false",
        [&agentConfig] {
            TStringStream out;
            for (const auto& config: agentConfig.GetDevices()) {
                out << config.GetDeviceUUID()
                    << " ("
                    << config.GetDeviceName() << " "
                    << config.GetBlocksCount() << " x "
                    << config.GetBlockSize()
                    << " " << config.GetPoolName()
                    << "); ";
            }
            return out.Str();
        }().c_str());

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    ExecuteTx<TAddAgent>(
        ctx,
        std::move(requestInfo),
        std::move(*msg->Record.MutableAgentConfig()),
        ctx.Now(),
        ev->Recipient);
}

bool TDiskRegistryActor::PrepareAddAgent(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TAddAgent& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TDiskRegistryActor::ExecuteAddAgent(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TAddAgent& args)
{
    Y_UNUSED(ctx);

    TDiskRegistryDatabase db(tx.DB);
    args.Error = State->RegisterAgent(
        db,
        args.Config,
        args.Timestamp,
        &args.AffectedDisks,
        &args.NotifiedDisks);

    if (!HasError(args.Error)) {
        for (auto it = ServerToAgentId.begin(); it != ServerToAgentId.end(); ) {
            const auto& agentId = it->second;

            if (agentId == args.Config.GetAgentId()) {
                const auto& serverId = it->first;
                NCloud::Send<TEvents::TEvPoisonPill>(ctx, serverId);

                ServerToAgentId.erase(it++);
            } else {
                ++it;
            }
        }

        Y_VERIFY_DEBUG(ServerToAgentId.contains(args.RegisterActorId));
        ServerToAgentId[args.RegisterActorId] = args.Config.GetAgentId();

        auto& info = AgentRegInfo[args.Config.GetAgentId()];
        info.Connected = true;
        info.SeqNo += 1;

    }
}

void TDiskRegistryActor::CompleteAddAgent(
    const TActorContext& ctx,
    TTxDiskRegistry::TAddAgent& args)
{
    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Complete register agent: NodeId=%u, AgentId=%s"
        ", AffectedDisks=%lu, Error=%s",
        TabletID(),
        args.Config.GetNodeId(),
        args.Config.GetAgentId().c_str(),
        args.AffectedDisks.size(),
        FormatError(args.Error).c_str());

    if (HasError(args.Error)) {
        LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "AddAgent error: %s",
            FormatError(args.Error).c_str());
    }

    for (const auto& x: args.AffectedDisks) {
        LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "[%lu] AffectedDiskID=%s",
            TabletID(),
            x.State.GetDiskId().Quote().c_str());
    }

    for (const auto& [diskId, seqNo]: args.NotifiedDisks) {
        LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "[%lu] NotifiedDiskID=%s SeqNo=%lu",
            TabletID(),
            diskId.Quote().c_str(),
            seqNo);
    }

    auto response = std::make_unique<TEvDiskRegistry::TEvRegisterAgentResponse>();
    *response->Record.MutableError() = std::move(args.Error);

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));

    NotifyDisks(ctx);
    NotifyUsers(ctx);
    PublishDiskStates(ctx);
    SecureErase(ctx);
    StartMigration(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleUnregisterAgent(
    const TEvDiskRegistry::TEvUnregisterAgentRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(UnregisterAgent);

    const auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received UnregisterAgent request: NodeId=%u",
        TabletID(),
        msg->Record.GetNodeId());

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    ExecuteTx<TRemoveAgent>(
        ctx,
        std::move(requestInfo),
        msg->Record.GetNodeId(),
        ctx.Now());
}

bool TDiskRegistryActor::PrepareRemoveAgent(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TRemoveAgent& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TDiskRegistryActor::ExecuteRemoveAgent(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TRemoveAgent& args)
{
    Y_UNUSED(ctx);

    TDiskRegistryDatabase db(tx.DB);
    // TODO: affected disks
    args.Error = State->UnregisterAgent(db, args.NodeId);
}

void TDiskRegistryActor::CompleteRemoveAgent(
    const TActorContext& ctx,
    TTxDiskRegistry::TRemoveAgent& args)
{
    if (HasError(args.Error)) {
        LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "RemoveAgent error: %s",
            FormatError(args.Error).c_str());
    }

    auto response = std::make_unique<TEvDiskRegistry::TEvUnregisterAgentResponse>();

    *response->Record.MutableError() = std::move(args.Error);

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage
