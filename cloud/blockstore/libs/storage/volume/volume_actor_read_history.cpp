#include "volume_actor.h"

#include "volume_database.h"

#include <cloud/blockstore/libs/storage/api/disk_registry.h>
#include <cloud/blockstore/libs/storage/api/disk_registry_proxy.h>
#include <cloud/blockstore/libs/storage/core/probes.h>
#include <cloud/blockstore/libs/storage/core/proto_helpers.h>
#include <cloud/blockstore/libs/storage/core/request_info.h>

#include <util/generic/scope.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

////////////////////////////////////////////////////////////////////////////////

void TVolumeActor::HandleReadHistory(
    const TEvVolumePrivate::TEvReadHistoryRequest::TPtr& ev,
    const NActors::TActorContext& ctx)
{
    auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    ProcessReadHistory(
        ctx,
        std::move(requestInfo),
        msg->Timestamp,
        msg->RecordCount,
        false);
}

void TVolumeActor::ProcessReadHistory(
    const NActors::TActorContext& ctx,
    TRequestInfoPtr requestInfo,
    TInstant ts,
    size_t recordCount,
    bool monRequest)
{
    AddTransaction(*requestInfo);

    ExecuteTx<TReadHistory>(
        ctx,
        std::move(requestInfo),
        ts,
        ctx.Now() - Config->GetVolumeHistoryDuration(),
        recordCount,
        monRequest);
}

////////////////////////////////////////////////////////////////////////////////

bool TVolumeActor::PrepareReadHistory(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxVolume::TReadHistory& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);

    TVolumeDatabase db(tx.DB);
    return db.ReadHistory(
        args.History,
        args.Ts,
        args.OldestTs,
        args.RecordCount);
}

void TVolumeActor::ExecuteReadHistory(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxVolume::TReadHistory& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);
}

void TVolumeActor::CompleteReadHistory(
    const TActorContext& ctx,
    TTxVolume::TReadHistory& args)
{
    if (args.MonRequest) {
        TDeque<THistoryLogItem> history;
        for (auto& h : args.History) {
            history.push_back(std::move(h));
        }
        TStringStream out;
        HandleHttpInfo_Default(ctx, history, "History", out);

        NCloud::Reply(
            ctx,
            *args.RequestInfo,
            std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str()));
    } else {
        auto response = std::make_unique<TEvVolumePrivate::TEvReadHistoryResponse>();
        response->History = std::move(args.History);

        BLOCKSTORE_TRACE_SENT(ctx, &args.RequestInfo->TraceId, this, response);
        NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
    }

    RemoveTransaction(*args.RequestInfo);
}

}   // namespace NCloud::NBlockStore::NStorage

