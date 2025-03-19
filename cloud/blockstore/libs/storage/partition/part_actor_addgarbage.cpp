#include "part_actor.h"

#include <cloud/blockstore/libs/storage/core/probes.h>

#include <util/generic/algorithm.h>

namespace NCloud::NBlockStore::NStorage::NPartition {

using namespace NActors;

using namespace NCloud::NStorage;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

////////////////////////////////////////////////////////////////////////////////

void TPartitionActor::HandleAddGarbage(
    const TEvPartitionPrivate::TEvAddGarbageRequest::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo<TEvPartitionPrivate::TAddGarbageMethod>(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    TRequestScope timer(*requestInfo);

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    LWPROBE(
        RequestReceived_Partition,
        "AddGarbage",
        GetRequestId(requestInfo->TraceId));

    AddTransaction(*requestInfo);

    ExecuteTx<TAddGarbage>(
        ctx,
        requestInfo,
        std::move(msg->BlobIds));
}

bool TPartitionActor::PrepareAddGarbage(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxPartition::TAddGarbage& args)
{
    Y_UNUSED(ctx);

    TRequestScope timer(*args.RequestInfo);
    TPartitionDatabase db(tx.DB);

    return db.ReadNewBlobs(args.KnownBlobIds)
        && db.ReadGarbageBlobs(args.KnownBlobIds);
}

void TPartitionActor::ExecuteAddGarbage(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxPartition::TAddGarbage& args)
{
    Y_UNUSED(ctx);

    TRequestScope timer(*args.RequestInfo);
    TPartitionDatabase db(tx.DB);

    Y_VERIFY(IsSorted(args.BlobIds.begin(), args.BlobIds.end()));
    SortUnique(args.KnownBlobIds);

    TVector<TPartialBlobId> diff;
    std::set_difference(
        args.BlobIds.begin(), args.BlobIds.end(),
        args.KnownBlobIds.begin(), args.KnownBlobIds.end(),
        std::inserter(diff, diff.begin()));

    for (const auto& blobId: diff) {
        LOG_INFO(ctx, TBlockStoreComponents::PARTITION,
            "[%lu] Add garbage blob: %s",
            TabletID(),
            ToString(MakeBlobId(TabletID(), blobId)).data());

        bool added = State->GetGarbageQueue().AddGarbageBlob(blobId);
        Y_VERIFY(added);

        db.WriteGarbageBlob(blobId);
    }
}

void TPartitionActor::CompleteAddGarbage(
    const TActorContext& ctx,
    TTxPartition::TAddGarbage& args)
{
    TRequestScope timer(*args.RequestInfo);

    auto response = std::make_unique<TEvPartitionPrivate::TEvAddGarbageResponse>();

    BLOCKSTORE_TRACE_SENT(ctx, &args.RequestInfo->TraceId, this, response);

    LWPROBE(
        ResponseSent_Partition,
        "AddGarbage",
        GetRequestId(args.RequestInfo->TraceId));

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
    RemoveTransaction(*args.RequestInfo);

    EnqueueCollectGarbageIfNeeded(ctx);
}

}   // namespace NCloud::NBlockStore::NStorage::NPartition
