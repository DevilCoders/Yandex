#include "service_actor.h"

#include "helpers.h"

#include <cloud/filestore/libs/storage/api/ss_proxy.h>
#include <cloud/filestore/libs/storage/api/tablet.h>
#include <cloud/filestore/libs/storage/api/tablet_proxy.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

#include <memory>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TStatFileStoreActor final
    : public TActorBootstrapped<TStatFileStoreActor>
{
private:
    const TRequestInfoPtr RequestInfo;
    const TString FileSystemId;

    NProto::TFileStore FileStore;

public:
    TStatFileStoreActor(
        TRequestInfoPtr requestInfo,
        TString fileSystemId);

    void Bootstrap(const TActorContext& ctx);

private:
    STFUNC(StateWork);

    void DescribeFileStore(const TActorContext& ctx);
    void HandleDescribeFileStoreResponse(
        const TEvSSProxy::TEvDescribeFileStoreResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandleStorageStats(
        const TEvIndexTablet::TEvGetStorageStatsResponse::TPtr& ev,
        const TActorContext& ctx);

    void HandlePoisonPill(
        const TEvents::TEvPoisonPill::TPtr& ev,
        const TActorContext& ctx);

    void ReplyAndDie(
        const TActorContext& ctx,
        const NProto::TError& error = {});

    void ReplyAndDie(
        const TActorContext& ctx,
        std::unique_ptr<TEvService::TEvStatFileStoreResponse> response);
};

////////////////////////////////////////////////////////////////////////////////

TStatFileStoreActor::TStatFileStoreActor(
        TRequestInfoPtr requestInfo,
        TString fileSystemId)
    : RequestInfo(std::move(requestInfo))
    , FileSystemId(std::move(fileSystemId))
{
    ActivityType = TFileStoreActivities::SERVICE_WORKER;
}

void TStatFileStoreActor::Bootstrap(const TActorContext& ctx)
{
    DescribeFileStore(ctx);
    Become(&TThis::StateWork);
}

void TStatFileStoreActor::DescribeFileStore(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvSSProxy::TEvDescribeFileStoreRequest>(
        FileSystemId);

    NCloud::Send(ctx, MakeSSProxyServiceId(), std::move(request));
}

void TStatFileStoreActor::HandleDescribeFileStoreResponse(
    const TEvSSProxy::TEvDescribeFileStoreResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    if (FAILED(msg->GetStatus())) {
        ReplyAndDie(ctx, msg->GetError());
        return;
    }

    const auto& fileStore = msg->PathDescription.GetFileStoreDescription();
    const auto& config = fileStore.GetConfig();
    Convert(config, FileStore);

    auto request = std::make_unique<TEvIndexTablet::TEvGetStorageStatsRequest>();
    request->Record.SetFileSystemId(FileSystemId);

    // forward request through tablet proxy
    ctx.Send(MakeIndexTabletProxyServiceId(), request.release());
}

void TStatFileStoreActor::HandleStorageStats(
    const TEvIndexTablet::TEvGetStorageStatsResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    if (FAILED(msg->GetStatus())) {
        ReplyAndDie(ctx, msg->GetError());
        return;
    }

    auto response = std::make_unique<TEvService::TEvStatFileStoreResponse>();
    response->Record.MutableFileStore()->CopyFrom(FileStore);

    auto stats = response->Record.MutableStats();
    stats->SetUsedNodesCount(msg->Record.GetStats().GetUsedNodesCount());
    stats->SetUsedBlocksCount(msg->Record.GetStats().GetUsedBlocksCount());

    ReplyAndDie(ctx, std::move(response));
}

void TStatFileStoreActor::HandlePoisonPill(
    const TEvents::TEvPoisonPill::TPtr& ev,
    const TActorContext& ctx)
{
    Y_UNUSED(ev);
    ReplyAndDie(ctx, MakeError(E_REJECTED, "request cancelled"));
}

void TStatFileStoreActor::ReplyAndDie(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    auto response = std::make_unique<TEvService::TEvStatFileStoreResponse>(error);
    ReplyAndDie(ctx, std::move(response));
}

void TStatFileStoreActor::ReplyAndDie(
    const TActorContext& ctx,
    std::unique_ptr<TEvService::TEvStatFileStoreResponse> response)
{
    NCloud::Reply(ctx, *RequestInfo, std::move(response));
    Die(ctx);
}

STFUNC(TStatFileStoreActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvents::TEvPoisonPill, HandlePoisonPill);

        HFunc(TEvSSProxy::TEvDescribeFileStoreResponse, HandleDescribeFileStoreResponse);
        HFunc(TEvIndexTablet::TEvGetStorageStatsResponse, HandleStorageStats);

        default:
            HandleUnexpectedEvent(ctx, ev, TFileStoreComponents::SERVICE_WORKER);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TStorageServiceActor::HandleStatFileStore(
    const TEvService::TEvStatFileStoreRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    auto actor = std::make_unique<TStatFileStoreActor>(
        std::move(requestInfo),
        msg->Record.GetFileSystemId());

    NCloud::Register(ctx, std::move(actor));
}

}   // namespace NCloud::NFileStore::NStorage
