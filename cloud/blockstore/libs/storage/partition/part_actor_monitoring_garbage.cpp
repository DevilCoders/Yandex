#include "part_actor.h"

#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/storage/core/monitoring_utils.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

#include <library/cpp/monlib/service/pages/templates.h>

#include <util/generic/algorithm.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/str.h>
#include <util/string/split.h>

namespace NCloud::NBlockStore::NStorage::NPartition {

using namespace NKikimr;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

class THttpGarbageActor final
    : public TActorBootstrapped<THttpGarbageActor>
{
public:
    enum EAction
    {
        AddGarbage,
        CollectGarbage,
    };

private:
    const TRequestInfoPtr RequestInfo;

    const TActorId Tablet;
    const ui64 TabletId;
    const EAction Action;
    const TVector<TPartialBlobId> BlobIds;

public:
    THttpGarbageActor(
        TRequestInfoPtr requestInfo,
        const TActorId& tablet,
        ui64 tabletId,
        EAction action,
        TVector<TPartialBlobId> blobIds = {});

    void Bootstrap(const TActorContext& ctx);

private:
    void ReplyAndDie(const TActorContext& ctx, const NProto::TError& error);

private:
    STFUNC(StateWork);

    void HandleAddGarbageResponse(
        const TEvPartitionPrivate::TEvAddGarbageResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleCollectGarbageResponse(
        const TEvPartitionPrivate::TEvCollectGarbageResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleAddGarbageRequest(
        const TEvPartitionPrivate::TEvAddGarbageRequest::TPtr& ev,
        const TActorContext& ctx);

    void HandleCollectGarbageRequest(
        const TEvPartitionPrivate::TEvCollectGarbageRequest::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

THttpGarbageActor::THttpGarbageActor(
        TRequestInfoPtr requestInfo,
        const TActorId& tablet,
        ui64 tabletId,
        EAction action,
        TVector<TPartialBlobId> blobIds)
    : RequestInfo(std::move(requestInfo))
    , Tablet(tablet)
    , TabletId(tabletId)
    , Action(action)
    , BlobIds(std::move(blobIds))
{
    ActivityType = TBlockStoreActivities::PARTITION_WORKER;
}

void THttpGarbageActor::Bootstrap(const TActorContext& ctx)
{
    switch (Action) {
        case AddGarbage: {
            auto request = std::make_unique<TEvPartitionPrivate::TEvAddGarbageRequest>(
                MakeIntrusive<TCallContext>(),
                BlobIds);

            NCloud::SendWithUndeliveryTracking(ctx, Tablet, std::move(request));
            break;
        }
        case CollectGarbage: {
            auto request =
                std::make_unique<TEvPartitionPrivate::TEvCollectGarbageRequest>();

            NCloud::SendWithUndeliveryTracking(ctx, Tablet, std::move(request));
            break;
        }
        default:
            Y_FAIL("Invalid action");
    }

    Become(&TThis::StateWork);
}

void THttpGarbageActor::ReplyAndDie(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    using namespace NMonitoringUtils;

    TStringStream msg;
    if (FAILED(error.GetCode())) {
        msg << "[" << TabletId << "] ";
        msg << "Operation completed with error : " << FormatError(error);
        LOG_ERROR_S(ctx, TBlockStoreComponents::PARTITION, msg.Str());
    } else {
        msg << "Operation successfully completed";
    }

    TStringStream out;
    BuildTabletNotifyPageWithRedirect(out, msg.Str(), TabletId);

    auto response = std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str());
    NCloud::Reply(ctx, *RequestInfo, std::move(response));

    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void THttpGarbageActor::HandleAddGarbageResponse(
    const TEvPartitionPrivate::TEvAddGarbageResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    ReplyAndDie(ctx, msg->GetError());
}

void THttpGarbageActor::HandleCollectGarbageResponse(
    const TEvPartitionPrivate::TEvCollectGarbageResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    ReplyAndDie(ctx, msg->GetError());
}

void THttpGarbageActor::HandleAddGarbageRequest(
    const TEvPartitionPrivate::TEvAddGarbageRequest::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    ReplyAndDie(ctx, MakeError(E_REJECTED, "Tablet is dead"));
}

void THttpGarbageActor::HandleCollectGarbageRequest(
    const TEvPartitionPrivate::TEvCollectGarbageRequest::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);

    ReplyAndDie(ctx, MakeError(E_REJECTED, "Tablet is dead"));
}

STFUNC(THttpGarbageActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvPartitionPrivate::TEvAddGarbageResponse, HandleAddGarbageResponse);
        HFunc(TEvPartitionPrivate::TEvCollectGarbageResponse, HandleCollectGarbageResponse);

        HFunc(TEvPartitionPrivate::TEvAddGarbageRequest, HandleAddGarbageRequest);
        HFunc(TEvPartitionPrivate::TEvCollectGarbageRequest, HandleCollectGarbageRequest);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::PARTITION_WORKER);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TPartitionActor::HandleHttpInfo_AddGarbage(
    const TActorContext& ctx,
    TRequestInfoPtr requestInfo,
    const TCgiParameters& params)
{
    TStringBuilder errors;

    TVector<ui32> allowedChannels = State->GetChannelsByKind([](auto kind) {
        return kind == EChannelDataKind::Mixed
            || kind == EChannelDataKind::Merged;
    });

    TVector<TPartialBlobId> blobIds;
    for (auto it: StringSplitter(params.Get("blobs")).SplitBySet(" ,;").SkipEmpty()) {
        TString part(it);

        TLogoBlobID blobId;
        TString errorExplanation;
        if (TLogoBlobID::Parse(blobId, part, errorExplanation)) {
            if (blobId.TabletID() != TabletID()) {
                errorExplanation = "tablet does not match";
            } else if (Find(allowedChannels, blobId.Channel()) == allowedChannels.end()) {
                errorExplanation = "channel does not match";
            } else if (blobId.Generation() >= Executor()->Generation()) {
                errorExplanation = "generation does not match";
            } else {
                blobIds.push_back(MakePartialBlobId(blobId));
                continue;
            }
        }

        errors << "Invalid blob " << part.Quote() << ": " << errorExplanation << Endl;
    }

    if (!blobIds || errors) {
        TStringStream out;
        HTML(out) {
            if (errors) {
                out << "Could not parse some blob IDs: "
                    << "<pre>" << errors << "</pre>";
            } else {
                out << "You should specify some blob IDs to add";
            }
        }

        auto response = std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str());
        BLOCKSTORE_TRACE_SENT(ctx, &requestInfo->TraceId, this, response);

        LWPROBE(
            ResponseSent_Partition,
            "HttpInfo",
            GetRequestId(requestInfo->TraceId));

        NCloud::Reply(ctx, *requestInfo, std::move(response));
        return;
    }

    SortUnique(blobIds);
    NCloud::Register<THttpGarbageActor>(
        ctx,
        std::move(requestInfo),
        SelfId(),
        TabletID(),
        THttpGarbageActor::AddGarbage,
        std::move(blobIds));
}

void TPartitionActor::HandleHttpInfo_CollectGarbage(
    const TActorContext& ctx,
    TRequestInfoPtr requestInfo,
    const TCgiParameters& params)
{
    const auto& type = params.Get("type");
    if (type == "hard") {
        State->CollectGarbageHardRequested = true;
    }

    NCloud::Register<THttpGarbageActor>(
        ctx,
        std::move(requestInfo),
        SelfId(),
        TabletID(),
        THttpGarbageActor::CollectGarbage);
}

}   // namespace NCloud::NBlockStore::NStorage::NPartition
