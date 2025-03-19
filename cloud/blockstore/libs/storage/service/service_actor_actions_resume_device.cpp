#include "service_actor.h"

#include <cloud/blockstore/libs/storage/api/disk_registry.h>
#include <cloud/blockstore/libs/storage/api/disk_registry_proxy.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

#include <library/cpp/actors/core/actor_bootstrapped.h>
#include <library/cpp/actors/core/events.h>
#include <library/cpp/actors/core/hfunc.h>
#include <library/cpp/actors/core/log.h>
#include <library/cpp/json/json_reader.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER)

namespace {

////////////////////////////////////////////////////////////////////////////////

class TResumeDeviceActionActor final
    : public TActorBootstrapped<TResumeDeviceActionActor>
{
private:
    const TRequestInfoPtr RequestInfo;
    const TString Input;

public:
    TResumeDeviceActionActor(
        TRequestInfoPtr requestInfo,
        TString input);

    void Bootstrap(const TActorContext& ctx);

private:
    void ReplyAndDie(const TActorContext& ctx, const NProto::TError& error);

private:
    STFUNC(StateWork);

    void HandleResumeDeviceResponse(
        const TEvDiskRegistry::TEvResumeDeviceResponse::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TResumeDeviceActionActor::TResumeDeviceActionActor(
        TRequestInfoPtr requestInfo,
        TString input)
    : RequestInfo(std::move(requestInfo))
    , Input(std::move(input))
{
    ActivityType = TBlockStoreActivities::SERVICE;
}

void TResumeDeviceActionActor::Bootstrap(const TActorContext& ctx)
{
    if (!Input) {
        ReplyAndDie(ctx, MakeError(E_ARGUMENT, "Empty input"));
        return;
    }

    NJson::TJsonValue input;
    if (!NJson::ReadJsonTree(Input, &input, false)) {
        ReplyAndDie(ctx,
            MakeError(E_ARGUMENT, "Input should be in JSON format"));
        return;
    }

    if (!input.Has("Host") || !input.Has("Path")) {
        ReplyAndDie(ctx,
            MakeError(E_ARGUMENT, "Host and Path are required"));
        return;
    }

    Become(&TThis::StateWork);

    auto request =
        std::make_unique<TEvDiskRegistry::TEvResumeDeviceRequest>();

    request->Record.SetHost(input["Host"].GetString());
    request->Record.SetPath(input["Path"].GetString());

    ctx.Send(MakeDiskRegistryProxyServiceId(), request.release());
}

void TResumeDeviceActionActor::ReplyAndDie(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    auto response =
        std::make_unique<TEvService::TEvExecuteActionResponse>(error);

    LWTRACK(
        ResponseSent_Service,
        RequestInfo->CallContext->LWOrbit,
        "ExecuteAction_resumedevice",
        RequestInfo->CallContext->RequestId);

    NCloud::Reply(ctx, *RequestInfo, std::move(response));
    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TResumeDeviceActionActor::HandleResumeDeviceResponse(
    const TEvDiskRegistry::TEvResumeDeviceResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    if (HasError(msg->GetError())) {
        ReplyAndDie(ctx, msg->GetError());
        return;
    }

    ReplyAndDie(ctx, {});
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TResumeDeviceActionActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(
            TEvDiskRegistry::TEvResumeDeviceResponse,
            HandleResumeDeviceResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IActorPtr TServiceActor::CreateResumeDeviceActionActor(
    TRequestInfoPtr requestInfo,
    TString input)
{
    return std::make_unique<TResumeDeviceActionActor>(
        std::move(requestInfo),
        std::move(input)
    );
}

}   // namespace NCloud::NBlockStore::NStorage
