#include "disk_registry_actor.h"

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleCreatePlacementGroup(
    const TEvService::TEvCreatePlacementGroupRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(AllocateDisk);

    const auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    LOG_DEBUG(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received CreatePlacementGroup request: GroupId=%s",
        TabletID(),
        msg->Record.GetGroupId().c_str());

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    ExecuteTx<TCreatePlacementGroup>(
        ctx,
        std::move(requestInfo),
        msg->Record.GetGroupId());
}

bool TDiskRegistryActor::PrepareCreatePlacementGroup(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TCreatePlacementGroup& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TDiskRegistryActor::ExecuteCreatePlacementGroup(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TCreatePlacementGroup& args)
{
    Y_UNUSED(ctx);

    TDiskRegistryDatabase db(tx.DB);
    args.Error = State->CreatePlacementGroup(db, args.GroupId);
}

void TDiskRegistryActor::CompleteCreatePlacementGroup(
    const TActorContext& ctx,
    TTxDiskRegistry::TCreatePlacementGroup& args)
{
    if (HasError(args.Error)) {
        LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "CreatePlacementGroup error: %s",
            FormatError(args.Error).c_str());
    }

    auto response = std::make_unique<TEvService::TEvCreatePlacementGroupResponse>(
        std::move(args.Error)
    );

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleDestroyPlacementGroup(
    const TEvService::TEvDestroyPlacementGroupRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(AllocateDisk);

    const auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    LOG_DEBUG(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received DestroyPlacementGroup request: GroupId=%s",
        TabletID(),
        msg->Record.GetGroupId().c_str());

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    ExecuteTx<TDestroyPlacementGroup>(
        ctx,
        std::move(requestInfo),
        msg->Record.GetGroupId());
}

bool TDiskRegistryActor::PrepareDestroyPlacementGroup(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TDestroyPlacementGroup& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TDiskRegistryActor::ExecuteDestroyPlacementGroup(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TDestroyPlacementGroup& args)
{
    Y_UNUSED(ctx);

    TDiskRegistryDatabase db(tx.DB);
    args.Error = State->DestroyPlacementGroup(db, args.GroupId, args.AffectedDisks);
}

void TDiskRegistryActor::CompleteDestroyPlacementGroup(
    const TActorContext& ctx,
    TTxDiskRegistry::TDestroyPlacementGroup& args)
{
    if (HasError(args.Error)) {
        LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "DestroyPlacementGroup error: %s",
            FormatError(args.Error).c_str());
    } else {
        UpdateVolumeConfigs(ctx, args.AffectedDisks);
    }

    auto response = std::make_unique<TEvService::TEvDestroyPlacementGroupResponse>(
        std::move(args.Error)
    );

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleAlterPlacementGroupMembership(
    const TEvService::TEvAlterPlacementGroupMembershipRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(AllocateDisk);

    const auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    LOG_DEBUG(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received AlterPlacementGroupMembership request: GroupId=%s"
        ", adding %u disks, removing %u disks",
        TabletID(),
        msg->Record.GetGroupId().c_str(),
        msg->Record.DisksToAddSize(),
        msg->Record.DisksToRemoveSize());

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    ExecuteTx<TAlterPlacementGroupMembership>(
        ctx,
        std::move(requestInfo),
        msg->Record.GetGroupId(),
        msg->Record.GetConfigVersion(),
        TVector<TString>(
            msg->Record.GetDisksToAdd().begin(),
            msg->Record.GetDisksToAdd().end()
        ),
        TVector<TString>(
            msg->Record.GetDisksToRemove().begin(),
            msg->Record.GetDisksToRemove().end()
        )
    );
}

bool TDiskRegistryActor::PrepareAlterPlacementGroupMembership(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TAlterPlacementGroupMembership& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);

    return true;
}

void TDiskRegistryActor::ExecuteAlterPlacementGroupMembership(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxDiskRegistry::TAlterPlacementGroupMembership& args)
{
    Y_UNUSED(ctx);

    TDiskRegistryDatabase db(tx.DB);
    args.FailedToAdd = args.DiskIdsToAdd;
    args.Error = State->AlterPlacementGroupMembership(
        db,
        args.GroupId,
        args.ConfigVersion,
        args.FailedToAdd,
        args.DiskIdsToRemove
    );
}

void TDiskRegistryActor::CompleteAlterPlacementGroupMembership(
    const TActorContext& ctx,
    TTxDiskRegistry::TAlterPlacementGroupMembership& args)
{
    if (HasError(args.Error)) {
        LOG_ERROR(ctx, TBlockStoreComponents::DISK_REGISTRY,
            "AlterPlacementGroupMembership error: %s",
            FormatError(args.Error).c_str());
    } else {
        UpdateVolumeConfigs(ctx, args.DiskIdsToAdd);
        UpdateVolumeConfigs(ctx, args.DiskIdsToRemove);
    }

    auto response = std::make_unique<TEvService::TEvAlterPlacementGroupMembershipResponse>(
        std::move(args.Error)
    );
    for (auto& diskId: args.FailedToAdd) {
        *response->Record.AddDisksImpossibleToAdd() = std::move(diskId);
    }

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleListPlacementGroups(
    const TEvService::TEvListPlacementGroupsRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(AllocateDisk);

    const auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    LOG_DEBUG(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received ListPlacementGroups request",
        TabletID());

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    auto response = std::make_unique<TEvService::TEvListPlacementGroupsResponse>();
    for (const auto& x: State->GetPlacementGroups()) {
        *response->Record.AddGroupIds() = x.first;
    }

    NCloud::Reply(ctx, *requestInfo, std::move(response));
}

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleDescribePlacementGroup(
    const TEvService::TEvDescribePlacementGroupRequest::TPtr& ev,
    const TActorContext& ctx)
{
    BLOCKSTORE_DISK_REGISTRY_COUNTER(AllocateDisk);

    const auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    LOG_DEBUG(ctx, TBlockStoreComponents::DISK_REGISTRY,
        "[%lu] Received DescribePlacementGroup request: GroupId=%s",
        TabletID(),
        msg->Record.GetGroupId().c_str());

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    auto response = std::make_unique<TEvService::TEvDescribePlacementGroupResponse>();
    if (auto* g = State->FindPlacementGroup(msg->Record.GetGroupId())) {
        auto* group = response->Record.MutableGroup();
        group->SetGroupId(msg->Record.GetGroupId());
        group->SetPlacementStrategy(NProto::PLACEMENT_STRATEGY_SPREAD);
        THashSet<TStringBuf> racks;
        for (const auto& disk: g->GetDisks()) {
            *group->AddDiskIds() = disk.GetDiskId();
            for (const auto& rack: disk.GetDeviceRacks()) {
                if (racks.insert(rack).second) {
                    *group->AddRacks() = rack;
                }
            }
        }
        group->SetConfigVersion(g->GetConfigVersion());
    } else {
        *response->Record.MutableError() = MakeError(
            E_NOT_FOUND,
            Sprintf("no such group: %s", msg->Record.GetGroupId().c_str())
        );
    }

    NCloud::Reply(ctx, *requestInfo, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage
