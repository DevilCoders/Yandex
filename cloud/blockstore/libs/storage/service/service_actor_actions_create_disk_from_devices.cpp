#include "service_actor.h"

#include <cloud/blockstore/libs/storage/api/disk_registry.h>
#include <cloud/blockstore/libs/storage/api/disk_registry_proxy.h>
#include <cloud/blockstore/private/api/protos/disk.pb.h>
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

class TCreateDiskActor final
    : public TActorBootstrapped<TCreateDiskActor>
{
private:
    const TRequestInfoPtr RequestInfo;
    const TString Input;

    NProto::TError Error;

public:
    TCreateDiskActor(
        TRequestInfoPtr requestInfo,
        TString input);

    void Bootstrap(const TActorContext& ctx);

private:
    void ReplyAndDie(
        const TActorContext& ctx,
        const NProto::TError& error = {});

private:
    STFUNC(StateWork);

    void HandleCreateDiskFromDevicesResponse(
        const TEvDiskRegistry::TEvCreateDiskFromDevicesResponse::TPtr& ev,
        const TActorContext& ctx);
};

////////////////////////////////////////////////////////////////////////////////

TCreateDiskActor::TCreateDiskActor(
        TRequestInfoPtr requestInfo,
        TString input)
    : RequestInfo(std::move(requestInfo))
    , Input(std::move(input))
{
    ActivityType = TBlockStoreActivities::SERVICE;
}

void TCreateDiskActor::Bootstrap(const TActorContext& ctx)
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

    if (!input.Has("DiskId")) {
        ReplyAndDie(ctx,
            MakeError(E_ARGUMENT, "DiskId is required"));
        return;
    }

    if (!input.Has("Devices")) {
        ReplyAndDie(ctx,
            MakeError(E_ARGUMENT, "Devices is required"));
        return;
    }

    for (const auto& device: input["Devices"].GetArray()) {
        if (!device.Has("AgentId")) {
            ReplyAndDie(ctx,
                MakeError(E_ARGUMENT, "AgentId is required"));
            return;
        }

        if (!device.Has("Path")) {
            ReplyAndDie(ctx,
                MakeError(E_ARGUMENT, "Path is required"));
            return;
        }
    }

    auto request =
        std::make_unique<TEvDiskRegistry::TEvCreateDiskFromDevicesRequest>();

    request->Record.SetForce(
        input.Has("Force") && input["Force"].GetBoolean());

    request->Record.SetDiskId(input["DiskId"].GetString());

    request->Record.SetBlockSize(input.Has("BlockSize")
        ? input["BlockSize"].GetUInteger()
        : 4096);

    for (const auto& device: input["Devices"].GetArray()) {
        auto& config = *request->Record.MutableDevices()->Add();

        config.SetAgentId(device["AgentId"].GetString());
        config.SetDeviceName(device["Path"].GetString());
    }

    Become(&TThis::StateWork);

    ctx.Send(MakeDiskRegistryProxyServiceId(), request.release());
}

void TCreateDiskActor::ReplyAndDie(
    const TActorContext& ctx,
    const NProto::TError& error)
{
    auto response =
        std::make_unique<TEvService::TEvExecuteActionResponse>(error);

    LWTRACK(
        ResponseSent_Service,
        RequestInfo->CallContext->LWOrbit,
        "ExecuteAction_creatediskfromdevices",
        RequestInfo->CallContext->RequestId);

    NCloud::Reply(ctx, *RequestInfo, std::move(response));
    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

void TCreateDiskActor::HandleCreateDiskFromDevicesResponse(
    const TEvDiskRegistry::TEvCreateDiskFromDevicesResponse::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    ReplyAndDie(ctx, msg->GetError());
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TCreateDiskActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(
            TEvDiskRegistry::TEvCreateDiskFromDevicesResponse,
            HandleCreateDiskFromDevicesResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IActorPtr TServiceActor::CreateCreateDiskFromDevices(
    TRequestInfoPtr requestInfo,
    TString input)
{
    return std::make_unique<TCreateDiskActor>(
        std::move(requestInfo),
        std::move(input)
    );
}

}   // namespace NCloud::NBlockStore::NStorage
