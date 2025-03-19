#include "ss_proxy_actor.h"

#include "path.h"

#include <cloud/filestore/libs/storage/api/ss_proxy.h>
#include <cloud/filestore/libs/storage/core/config.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TDescribeFileStoreActor final
    : public TActorBootstrapped<TDescribeFileStoreActor>
{
private:
    const TRequestInfoPtr RequestInfo;
    const TStorageConfigPtr Config;
    const TString FileSystemId;

public:
    TDescribeFileStoreActor(
        TRequestInfoPtr requestInfo,
        TStorageConfigPtr config,
        TString fileSystemId);

    void Bootstrap(const TActorContext& ctx);

private:
    STFUNC(StateWork);

    void DescribeScheme(const TActorContext& ctx);
    void HandleDescribeSchemeResponse(
        const TEvSSProxy::TEvDescribeSchemeResponse::TPtr& ev,
        const TActorContext& ctx);

    void ReplyAndDie(
        const TActorContext& ctx,
        const NProto::TError& error = {});

    void ReplyAndDie(
        const TActorContext& ctx,
        std::unique_ptr<TEvSSProxy::TEvDescribeFileStoreResponse> response);
};

////////////////////////////////////////////////////////////////////////////////

TDescribeFileStoreActor::TDescribeFileStoreActor(
        TRequestInfoPtr requestInfo,
        TStorageConfigPtr config,
        TString fileSystemId)
    : RequestInfo(std::move(requestInfo))
    , Config(std::move(config))
    , FileSystemId(std::move(fileSystemId))
{
    ActivityType = TFileStoreActivities::SS_PROXY;
}

void TDescribeFileStoreActor::Bootstrap(const TActorContext& ctx)
{
    DescribeScheme(ctx);
    Become(&TThis::StateWork);
}

void TDescribeFileStoreActor::DescribeScheme(const TActorContext& ctx)
{
    auto path = GetFileSystemPath(
        Config->GetSchemeShardDir(),
        FileSystemId);

    auto request = std::make_unique<TEvSSProxy::TEvDescribeSchemeRequest>(path);
    NCloud::Send(ctx, MakeSSProxyServiceId(), std::move(request));
}

void TDescribeFileStoreActor::HandleDescribeSchemeResponse(
    const TEvSSProxy::TEvDescribeSchemeResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();
    const auto& error = msg->GetError();

    if (FAILED(error.GetCode())) {
        ReplyAndDie(ctx, error);
        return;
    }

    auto response = std::make_unique<TEvSSProxy::TEvDescribeFileStoreResponse>(
        msg->Path,
        msg->PathDescription);

    ReplyAndDie(ctx, std::move(response));
}

void TDescribeFileStoreActor::ReplyAndDie(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    auto response = std::make_unique<TEvSSProxy::TEvDescribeFileStoreResponse>(error);
    ReplyAndDie(ctx, std::move(response));
}

void TDescribeFileStoreActor::ReplyAndDie(
    const TActorContext& ctx,
    std::unique_ptr<TEvSSProxy::TEvDescribeFileStoreResponse> response)
{
    NCloud::Reply(ctx, *RequestInfo, std::move(response));
    Die(ctx);
}

STFUNC(TDescribeFileStoreActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvSSProxy::TEvDescribeSchemeResponse, HandleDescribeSchemeResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TFileStoreComponents::SS_PROXY);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TSSProxyActor::HandleDescribeFileStore(
    const TEvSSProxy::TEvDescribeFileStoreRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    auto actor = std::make_unique<TDescribeFileStoreActor>(
        std::move(requestInfo),
        Config,
        msg->FileSystemId);

    NCloud::Register(ctx, std::move(actor));
}

}   // namespace NCloud::NFileStore::NStorage
