#include "part_actor.h"

#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/storage/core/config.h>
#include <cloud/blockstore/libs/storage/core/monitoring_utils.h>
#include <cloud/blockstore/libs/storage/core/probes.h>
#include <cloud/blockstore/private/api/protos/volume.pb.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <util/datetime/base.h>
#include <util/generic/algorithm.h>
#include <util/generic/guid.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/str.h>

namespace NCloud::NBlockStore::NStorage::NPartition {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

////////////////////////////////////////////////////////////////////////////////

void TPartitionActor::HandleHttpInfo_RebuildMetadata(
    const TActorContext& ctx,
    TRequestInfoPtr requestInfo,
    const TCgiParameters& params)
{
    using namespace NMonitoringUtils;

    const auto& batchSizeParam = params.Get("BatchSize");
    ui32 batchSize = 0;
    if (batchSizeParam) {
        TryFromString(batchSizeParam, batchSize);
    }

    auto result = DoHandleMetadataRebuildBatch(
        ctx,
        NProto::USED_BLOCKS,
        batchSize);

    TStringStream out;
    BuildTabletNotifyPageWithRedirect(out, result.GetMessage(), TabletID());

    auto response = std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str());
    NCloud::Reply(ctx, *requestInfo, std::move(response));
}

////////////////////////////////////////////////////////////////////////////////

void TPartitionActor::HandleRebuildMetadata(
    const TEvVolume::TEvRebuildMetadataRequest::TPtr& ev,
    const TActorContext& ctx)
{
    NProto::TError result;

    const auto* msg = ev->Get();

    result = DoHandleMetadataRebuildBatch(
        ctx,
        msg->Record.GetMetadataType(),
        msg->Record.GetBatchSize());

    auto response = std::make_unique<TEvVolume::TEvRebuildMetadataResponse>(result);
    NCloud::Reply(ctx, *ev, std::move(response));
}

void TPartitionActor::HandleGetRebuildMetadataStatus(
    const TEvVolume::TEvGetRebuildMetadataStatusRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto response = std::make_unique<TEvVolume::TEvGetRebuildMetadataStatusResponse>();
    NProto::TError result;

    if (State) {
        if (State->GetMetadataRebuildType() == EMetadataRebuildType::NoOperation)
        {
            result = MakeError(E_NOT_FOUND, "No operation found");
        } else {
            auto& progress = *response->Record.MutableProgress();
            const auto p = State->GetMetadataRebuildProgress();
            progress.SetProcessed(p.Processed);
            progress.SetTotal(p.Total);
            progress.SetIsCompleted(p.IsCompleted);
        }
    } else {
        result = MakeError(E_REJECTED, "Tablet is dead");
    };

    *response->Record.MutableError() = std::move(result);
    NCloud::Reply(ctx, *ev, std::move(response));
}

NProto::TError TPartitionActor::DoHandleMetadataRebuildBatch(
    const TActorContext& ctx,
    NProto::ERebuildMetadataType type,
    ui32 batchSize)
{
    if (State->IsMetadataRebuildStarted()) {
        return MakeError(S_ALREADY, "Metadata rebuild is already running");
    }

    if (!batchSize) {
        return MakeError(E_ARGUMENT, "Batch size is 0");
    }

    switch (type) {
        case NProto::USED_BLOCKS: {
            if (State->GetMetadataRebuildType() == EMetadataRebuildType::UsedBlocks)
            {
                return MakeError(S_ALREADY, "Used blocks are already calculated");
            }
            State->StartRebuildUsedBlocks();

            auto actorId = NCloud::Register(
                ctx,
                CreateMetadataRebuildUsedBlocksActor(
                    SelfId(),
                    static_cast<ui64>(batchSize) * State->GetUsedBlocks().CHUNK_SIZE,
                    State->GetBlocksCount(),
                    Config->GetCompactionRetryTimeout()));

            Actors.insert(actorId);

            return MakeError(S_OK, "Metadata rebuild(used blocks) has been started");
        }
        case NProto::BLOCK_COUNT: {
            if (State->GetMetadataRebuildType() == EMetadataRebuildType::BlockCount)
            {
                return MakeError(S_ALREADY, "Block count is already calculated");
            }

            State->StartRebuildBlockCount();

            auto actorId = NCloud::Register(
                ctx,
                CreateMetadataRebuildBlockCountActor(
                    SelfId(),
                    batchSize,
                    State->GetLastCommitId(),
                    State->GetMixedBlocksCount(),
                    State->GetMergedBlocksCount(),
                    Config->GetCompactionRetryTimeout()));

            Actors.insert(actorId);

            return MakeError(S_OK, "Metadata rebuild has been started");
        }
        default: {
            return MakeError(
                E_ARGUMENT,
                TStringBuilder() << "Unknown metadata type: " << static_cast<ui32>(type));
        }
    }
}

void TPartitionActor::HandleMetadataRebuildCompleted(
    const TEvPartitionPrivate::TEvMetadataRebuildCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ctx);

    if (State && State->IsMetadataRebuildStarted()) {
        bool runCleanup =
            State->GetMetadataRebuildType() == EMetadataRebuildType::BlockCount;

        State->CompleteMetadataRebuild();
        if (runCleanup) {
            EnqueueCleanupIfNeeded(ctx);
        }
    }
    Actors.erase(ev->Sender);
}

}   // namespace NCloud::NBlockStore::NStorage::NPartition
