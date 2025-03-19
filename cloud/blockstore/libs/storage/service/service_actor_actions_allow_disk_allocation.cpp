#include "service_actor.h"

#include <cloud/blockstore/libs/storage/api/disk_registry.h>
#include <cloud/blockstore/libs/storage/api/disk_registry_proxy.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>
#include <library/cpp/actors/core/events.h>
#include <library/cpp/actors/core/hfunc.h>
#include <library/cpp/actors/core/log.h>

#include <google/protobuf/util/json_util.h>

#include <util/string/printf.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER)

namespace {

////////////////////////////////////////////////////////////////////////////////

class TAllowDiskAllocationActor final
    : public TActorBootstrapped<TAllowDiskAllocationActor>
{
private:
    const TRequestInfoPtr RequestInfo;
    const TString Input;

    NProto::TError Error;

public:
    TAllowDiskAllocationActor(
        TRequestInfoPtr requestInfo,
        TString input);

    void Bootstrap(const TActorContext& ctx);

private:
    void ReplyAndDie(const TActorContext& ctx);

private:
    STFUNC(StateWork);

    void HandleAllowDiskAllocationResponse(
        const TEvDiskRegistry::TEvAllowDiskAllocationResponse::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TAllowDiskAllocationActor::TAllowDiskAllocationActor(
        TRequestInfoPtr requestInfo,
        TString input)
    : RequestInfo(std::move(requestInfo))
    , Input(std::move(input))
{
    ActivityType = TBlockStoreActivities::SERVICE;
}

void TAllowDiskAllocationActor::Bootstrap(const TActorContext& ctx)
{
    auto request = std::make_unique<TEvDiskRegistry::TEvAllowDiskAllocationRequest>();

    if (!google::protobuf::util::JsonStringToMessage(Input, &request->Record).ok()) {
        Error = MakeError(E_ARGUMENT, "Failed to parse input");
        ReplyAndDie(ctx);
        return;
    }

    Become(&TThis::StateWork);

    NCloud::Send(ctx, MakeDiskRegistryProxyServiceId(), std::move(request));
}

void TAllowDiskAllocationActor::ReplyAndDie(const TActorContext& ctx)
{
    auto response = std::make_unique<TEvService::TEvExecuteActionResponse>(Error);
    google::protobuf::util::MessageToJsonString(
        NProto::TAllowDiskAllocationResponse(),
        response->Record.MutableOutput()
    );

    LWTRACK(
        ResponseSent_Service,
        RequestInfo->CallContext->LWOrbit,
        "ExecuteAction_allowdiskallocation",
        RequestInfo->CallContext->RequestId);

    NCloud::Reply(ctx, *RequestInfo, std::move(response));
    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TAllowDiskAllocationActor::HandleAllowDiskAllocationResponse(
    const TEvDiskRegistry::TEvAllowDiskAllocationResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    if (HasError(msg->GetError())) {
        Error = msg->GetError();
    }

    ReplyAndDie(ctx);
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TAllowDiskAllocationActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(
            TEvDiskRegistry::TEvAllowDiskAllocationResponse,
            HandleAllowDiskAllocationResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IActorPtr TServiceActor::CreateAllowDiskAllocationActionActor(
    TRequestInfoPtr requestInfo,
    TString input)
{
    return std::make_unique<TAllowDiskAllocationActor>(
        std::move(requestInfo),
        std::move(input)
    );
}

}   // namespace NCloud::NBlockStore::NStorage
