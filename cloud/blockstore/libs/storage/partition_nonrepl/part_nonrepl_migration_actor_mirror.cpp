#include "part_nonrepl_migration_actor.h"

#include "mirror.h"

#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/storage/api/undelivered.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

TBlockRange64 BuildRequestBlockRange(
    const TEvService::TEvWriteBlocksRequest& request,
    const ui32 blockSize)
{
    ui64 totalSize = CalculateBytesCount(request.Record, blockSize);
    Y_VERIFY(totalSize % blockSize == 0);

    return TBlockRange64::WithLength(
        request.Record.GetStartIndex(),
        totalSize / blockSize
    );
}

TBlockRange64 BuildRequestBlockRange(
    const TEvService::TEvWriteBlocksLocalRequest& request,
    const ui32 blockSize)
{
    Y_UNUSED(blockSize);

    return TBlockRange64::WithLength(
        request.Record.GetStartIndex(),
        request.Record.BlocksCount
    );
}

TBlockRange64 BuildRequestBlockRange(
    const TEvService::TEvZeroBlocksRequest& request,
    const ui32 blockSize)
{
    Y_UNUSED(blockSize);

    return TBlockRange64::WithLength(
        request.Record.GetStartIndex(),
        request.Record.GetBlocksCount()
    );
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TNonreplicatedPartitionMigrationActor::HandleWriteOrZeroCompleted(
    const TEvNonreplPartitionPrivate::TEvWriteOrZeroCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    const auto counter = ev->Get()->RequestCounter;
    if (!WriteAndZeroRequest2Range.erase(counter)) {
        Y_VERIFY_DEBUG(0);
    }

    ContinueMigrationIfNeeded(ctx);
}

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
void TNonreplicatedPartitionMigrationActor::MirrorRequest(
    const typename TMethod::TRequest::TPtr& ev,
    const NActors::TActorContext& ctx)
{
    if (!DstActorId) {
        ForwardRequestWithNondeliveryTracking(
            ctx,
            SrcActorId,
            *ev);

        return;
    }

    auto replyError = [&] (ui32 errorCode, TString errorMessage)
    {
        auto response = std::make_unique<typename TMethod::TResponse>(
            MakeError(errorCode, std::move(errorMessage)));
        NCloud::Reply(ctx, *ev, std::move(response));
    };

    auto* msg = ev->Get();

    const auto range = BuildRequestBlockRange(
        *msg,
        SrcConfig->GetBlockSize());

    if (State.IsMigrationStarted()) {
        const auto migrationRange = State.BuildMigrationRange();
        if (range.Overlaps(migrationRange)) {
            replyError(E_REJECTED, TStringBuilder()
                << "Request " << TMethod::Name
                << " intersects with currently migrated range");
            return;
        }
    }

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId)
    );

    const auto counter = ++WriteAndZeroRequestCounter;
    WriteAndZeroRequest2Range[counter] = range;

    NCloud::Register<TWriteRequestActor<TMethod>>(
        ctx,
        std::move(requestInfo),
        TVector<TActorId>{SrcActorId, DstActorId},
        std::move(msg->Record),
        SrcConfig->GetName(),
        SelfId(),
        counter
    );
}

////////////////////////////////////////////////////////////////////////////////

void TNonreplicatedPartitionMigrationActor::HandleWriteBlocks(
    const TEvService::TEvWriteBlocksRequest::TPtr& ev,
    const TActorContext& ctx)
{
    MirrorRequest<TEvService::TWriteBlocksMethod>(ev, ctx);
}

void TNonreplicatedPartitionMigrationActor::HandleWriteBlocksLocal(
    const TEvService::TEvWriteBlocksLocalRequest::TPtr& ev,
    const TActorContext& ctx)
{
    MirrorRequest<TEvService::TWriteBlocksLocalMethod>(ev, ctx);
}

void TNonreplicatedPartitionMigrationActor::HandleZeroBlocks(
    const TEvService::TEvZeroBlocksRequest::TPtr& ev,
    const TActorContext& ctx)
{
    MirrorRequest<TEvService::TZeroBlocksMethod>(ev, ctx);
}

}   // namespace NCloud::NBlockStore::NStorage
