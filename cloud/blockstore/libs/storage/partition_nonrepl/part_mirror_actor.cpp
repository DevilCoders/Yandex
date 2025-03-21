#include "part_mirror_actor.h"
#include "part_nonrepl.h"
#include "part_nonrepl_migration.h"

#include <cloud/blockstore/libs/diagnostics/critical_events.h>

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

TMirrorPartitionActor::TMirrorPartitionActor(
        TStorageConfigPtr config,
        IProfileLogPtr profileLog,
        IBlockDigestGeneratorPtr digestGenerator,
        TString rwClientId,
        TNonreplicatedPartitionConfigPtr partConfig,
        TVector<TDevices> replicas,
        NRdma::IClientPtr rdmaClient)
    : Config(std::move(config))
    , ProfileLog(std::move(profileLog))
    , BlockDigestGenerator(std::move(digestGenerator))
    , RdmaClient(std::move(rdmaClient))
    , State(Config, rwClientId, std::move(partConfig), std::move(replicas))
{
    ActivityType = TBlockStoreActivities::PARTITION;
}

TMirrorPartitionActor::~TMirrorPartitionActor()
{
}

void TMirrorPartitionActor::Bootstrap(const TActorContext& ctx)
{
    SetupPartitions(ctx);
    ScheduleCountersUpdate(ctx);

    Become(&TThis::StateWork);
}

void TMirrorPartitionActor::KillActors(const TActorContext& ctx)
{
    for (const auto& actorId: State.GetReplicaActors()) {
        NCloud::Send<TEvents::TEvPoisonPill>(ctx, actorId);
    }
}

void TMirrorPartitionActor::SetupPartitions(const TActorContext& ctx)
{
    Status = State.Validate();
    if (HasError(Status)) {
        return;
    }

    Status = State.PrepareMigrationConfig();
    if (HasError(Status)) {
        return;
    }

    for (const auto& replicaInfo: State.GetReplicaInfos()) {
        IActorPtr actor;
        if (replicaInfo.Migrations.size()) {
            actor = CreateNonreplicatedPartitionMigration(
                Config,
                ProfileLog,
                BlockDigestGenerator,
                0,  // initialMigrationIndex
                State.GetRWClientId(),
                replicaInfo.Config,
                replicaInfo.Migrations,
                RdmaClient);
        } else {
            actor = CreateNonreplicatedPartition(
                Config,
                replicaInfo.Config,
                SelfId(),
                RdmaClient);
        }

        State.AddReplicaActor(NCloud::Register(ctx, std::move(actor)));
    }

    ReplicaCounters.resize(State.GetReplicaInfos().size());
}

void TMirrorPartitionActor::ReplyAndDie(const TActorContext& ctx)
{
    NCloud::Reply(ctx, *Poisoner, std::make_unique<TEvents::TEvPoisonTaken>());
    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TMirrorPartitionActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    Become(&TThis::StateZombie);

    KillActors(ctx);

    AliveReplicas = State.GetReplicaActors().size();

    Poisoner = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        MakeIntrusive<TCallContext>(),
        std::move(ev->TraceId));

    Y_VERIFY_DEBUG(AliveReplicas != 0);

    if (AliveReplicas == 0) {
        ReplyAndDie(ctx);
    }
}

void TMirrorPartitionActor::HandlePoisonTaken(
    const TEvents::TEvPoisonTaken::TPtr& ev,
    const TActorContext& ctx)
{
    if (!FindPtr(State.GetReplicaActors(), ev->Sender)) {
        return;
    }

    Y_VERIFY(AliveReplicas > 0);
    --AliveReplicas;

    if (AliveReplicas == 0) {
        ReplyAndDie(ctx);
    }
}

////////////////////////////////////////////////////////////////////////////////

void TMirrorPartitionActor::ScheduleCountersUpdate(
    const TActorContext& ctx)
{
    if (!UpdateCountersScheduled) {
        ctx.Schedule(UpdateCountersInterval,
            new TEvNonreplPartitionPrivate::TEvUpdateCounters());
        UpdateCountersScheduled = true;
    }
}

void TMirrorPartitionActor::HandleUpdateCounters(
    const TEvNonreplPartitionPrivate::TEvUpdateCounters::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    UpdateCountersScheduled = false;

    SendStats(ctx);
    ScheduleCountersUpdate(ctx);
}

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_HANDLE_UNIMPLEMENTED_REQUEST(name, ns)                      \
    void TMirrorPartitionActor::Handle##name(                           \
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

STFUNC(TMirrorPartitionActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(
            TEvNonreplPartitionPrivate::TEvUpdateCounters,
            HandleUpdateCounters);

        HFunc(TEvService::TEvReadBlocksRequest, HandleReadBlocks);
        HFunc(TEvService::TEvWriteBlocksRequest, HandleWriteBlocks);
        HFunc(TEvService::TEvZeroBlocksRequest, HandleZeroBlocks);

        HFunc(TEvService::TEvReadBlocksLocalRequest, HandleReadBlocksLocal);
        HFunc(TEvService::TEvWriteBlocksLocalRequest, HandleWriteBlocksLocal);

        HFunc(TEvVolume::TEvDescribeBlocksRequest, HandleDescribeBlocks);
        HFunc(TEvVolume::TEvGetCompactionStatusRequest, HandleGetCompactionStatus);
        HFunc(TEvVolume::TEvCompactRangeRequest, HandleCompactRange);
        HFunc(TEvVolume::TEvRebuildMetadataRequest, HandleRebuildMetadata);
        HFunc(TEvVolume::TEvGetRebuildMetadataStatusRequest, HandleGetRebuildMetadataStatus);

        HFunc(
            TEvNonreplPartitionPrivate::TEvWriteOrZeroCompleted,
            HandleWriteOrZeroCompleted);

        HFunc(
            TEvVolume::TEvRWClientIdChanged,
            HandleRWClientIdChanged);
        HFunc(
            TEvVolume::TEvNonreplicatedPartitionCounters,
            HandlePartCounters);

        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::PARTITION);
            break;
    }
}

STFUNC(TMirrorPartitionActor::StateZombie)
{
    switch (ev->GetTypeRewrite()) {
        IgnoreFunc(TEvNonreplPartitionPrivate::TEvUpdateCounters);

        HFunc(TEvService::TEvReadBlocksRequest, RejectReadBlocks);
        HFunc(TEvService::TEvWriteBlocksRequest, RejectWriteBlocks);
        HFunc(TEvService::TEvZeroBlocksRequest, RejectZeroBlocks);

        HFunc(TEvService::TEvReadBlocksLocalRequest, RejectReadBlocksLocal);
        HFunc(TEvService::TEvWriteBlocksLocalRequest, RejectWriteBlocksLocal);

        HFunc(TEvVolume::TEvDescribeBlocksRequest, RejectDescribeBlocks);
        HFunc(TEvVolume::TEvGetCompactionStatusRequest, RejectGetCompactionStatus);
        HFunc(TEvVolume::TEvCompactRangeRequest, RejectCompactRange);
        HFunc(TEvVolume::TEvRebuildMetadataRequest, RejectRebuildMetadata);
        HFunc(TEvVolume::TEvGetRebuildMetadataStatusRequest, RejectGetRebuildMetadataStatus);

        IgnoreFunc(TEvNonreplPartitionPrivate::TEvWriteOrZeroCompleted);

        IgnoreFunc(TEvVolume::TEvRWClientIdChanged);
        IgnoreFunc(TEvVolume::TEvNonreplicatedPartitionCounters);

        IgnoreFunc(TEvents::TEvPoisonPill);
        HFunc(TEvents::TEvPoisonTaken, HandlePoisonTaken);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::PARTITION);
            break;
    }
}

}   // namespace NCloud::NBlockStore::NStorage
