#include "part_mirror_actor.h"

#include "mirror.h"

#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TMirrorPartitionActor::HandleWriteOrZeroCompleted(
    const TEvNonreplPartitionPrivate::TEvWriteOrZeroCompleted::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);
    Y_UNUSED(ctx);

    // TODO: use this completion notification to implement request intersection
    // checks during the resync process
}

////////////////////////////////////////////////////////////////////////////////

template <typename TMethod>
void TMirrorPartitionActor::MirrorRequest(
    const typename TMethod::TRequest::TPtr& ev,
    const NActors::TActorContext& ctx)
{
    auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId)
    );

    if (HasError(Status)) {
        Reply(
            ctx,
            *requestInfo,
            std::make_unique<typename TMethod::TResponse>(Status)
        );

        return;
    }

    NCloud::Register<TWriteRequestActor<TMethod>>(
        ctx,
        std::move(requestInfo),
        State.GetReplicaActors(),
        std::move(msg->Record),
        State.GetReplicaInfos()[0].Config->GetName(),
        SelfId(),
        ++RequestId
    );
}

////////////////////////////////////////////////////////////////////////////////

void TMirrorPartitionActor::HandleWriteBlocks(
    const TEvService::TEvWriteBlocksRequest::TPtr& ev,
    const TActorContext& ctx)
{
    MirrorRequest<TEvService::TWriteBlocksMethod>(ev, ctx);
}

void TMirrorPartitionActor::HandleWriteBlocksLocal(
    const TEvService::TEvWriteBlocksLocalRequest::TPtr& ev,
    const TActorContext& ctx)
{
    MirrorRequest<TEvService::TWriteBlocksLocalMethod>(ev, ctx);
}

void TMirrorPartitionActor::HandleZeroBlocks(
    const TEvService::TEvZeroBlocksRequest::TPtr& ev,
    const TActorContext& ctx)
{
    MirrorRequest<TEvService::TZeroBlocksMethod>(ev, ctx);
}

}   // namespace NCloud::NBlockStore::NStorage
