#include "disk_registry_actor.h"

#include <ydb/core/base/appdata.h>

#include <util/generic/algorithm.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

namespace {

////////////////////////////////////////////////////////////////////////////////

bool AllSucceeded(std::initializer_list<bool> ls)
{
    auto identity = [] (bool x) {
        return x;
    };

    return std::all_of(std::begin(ls), std::end(ls), identity);
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

bool TDiskRegistryActor::LoadState(
    TDiskRegistryDatabase& db,
    TTxDiskRegistry::TLoadDBState& args)
{
    return AllSucceeded({
        db.ReadDiskRegistryConfig(args.Config),
        db.ReadDirtyDevices(args.DirtyDevices),
        db.ReadOldAgents(args.OldAgents),
        db.ReadAgents(args.Agents),
        db.ReadDisks(args.Disks),
        db.ReadPlacementGroups(args.PlacementGroups),
        db.ReadBrokenDisks(args.BrokenDisks),
        db.ReadDisksToNotify(args.DisksToNotify),
        db.ReadErrorNotifications(args.ErrorNotifications),
        db.ReadDiskStateChanges(args.DiskStateChanges),
        db.ReadLastDiskStateSeqNo(args.LastDiskStateSeqNo),
        db.ReadDiskAllocationAllowed(args.DiskAllocationAllowed),
        db.ReadDisksToCleanup(args.DisksToCleanup),
        db.ReadOutdatedVolumeConfigs(args.OutdatedVolumeConfigs),
        db.ReadSuspendedDevices(args.SuspendedDevices),
    });
}

////////////////////////////////////////////////////////////////////////////////

bool TDiskRegistryActor::PrepareLoadState(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TLoadState& args)
{
    Y_UNUSED(ctx);

    TDiskRegistryDatabase db(tx.DB);
    return LoadState(db, args);
}

void TDiskRegistryActor::ExecuteLoadState(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TLoadState& args)
{
    // Move OldAgents to Agents

    THashSet<TString> ids;
    for (const auto& agent: args.Agents) {
        ids.insert(agent.GetAgentId());
    }

    TDiskRegistryDatabase db(tx.DB);

    for (auto& agent: args.OldAgents) {
        if (!ids.insert(agent.GetAgentId()).second) {
            continue;
        }

        LOG_INFO(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "Agent %s:%d moved to new table",
            agent.GetAgentId().c_str(),
            agent.GetNodeId());

        args.Agents.push_back(agent);

        db.UpdateAgent(agent);
    }
}

void TDiskRegistryActor::CompleteLoadState(
    const TActorContext& ctx,
    TTxDiskRegistry::TLoadState& args)
{
    Y_VERIFY(CurrentState == STATE_INIT);
    BecomeAux(ctx, STATE_WORK);

    // allow pipes to connect
    SignalTabletActive(ctx);

    // resend pending requests
    SendPendingRequests(ctx, PendingRequests);

    for (const auto& agent: args.Agents) {
        if (agent.GetState() != NProto::AGENT_STATE_UNAVAILABLE) {
            ScheduleRejectAgent(ctx, agent.GetAgentId(), 0);
        }
    }

    // initialize state
    State.reset(new TDiskRegistryState(
        Config,
        ComponentGroup,
        std::move(args.Config),
        std::move(args.Agents),
        std::move(args.Disks),
        std::move(args.PlacementGroups),
        std::move(args.BrokenDisks),
        std::move(args.DisksToNotify),
        std::move(args.DiskStateChanges),
        args.LastDiskStateSeqNo,
        std::move(args.DirtyDevices),
        args.DiskAllocationAllowed,
        std::move(args.DisksToCleanup),
        std::move(args.ErrorNotifications),
        std::move(args.OutdatedVolumeConfigs),
        std::move(args.SuspendedDevices)
    ));

    SecureErase(ctx);

    ScheduleCleanup(ctx);

    DestroyBrokenDisks(ctx);

    NotifyDisks(ctx);

    NotifyUsers(ctx);

    PublishDiskStates(ctx);

    UpdateCounters(ctx);

    StartMigration(ctx);

    UpdateVolumeConfigs(ctx);
}

}   // namespace NCloud::NBlockStore::NStorage
