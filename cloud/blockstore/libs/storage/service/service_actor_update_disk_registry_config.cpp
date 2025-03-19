#include "service_actor.h"

#include <cloud/blockstore/libs/storage/api/disk_registry.h>
#include <cloud/blockstore/libs/storage/api/disk_registry_proxy.h>
#include <cloud/blockstore/libs/storage/protos/disk.pb.h>

#include <util/string/join.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

NProto::TAgentConfig CreateConfig(const NProto::TKnownDiskAgent& knownAgent)
{
    NProto::TAgentConfig config;

    config.SetAgentId(knownAgent.GetAgentId());

    for (auto& uuid: knownAgent.GetDevices()) {
        config.AddDevices()->SetDeviceUUID(uuid);
    }

    return config;
}

////////////////////////////////////////////////////////////////////////////////

class TUpdateDiskRegistryConfigActor final
    : public TActorBootstrapped<TUpdateDiskRegistryConfigActor>
{
private:
    const TRequestInfoPtr RequestInfo;
    const NProto::TDiskRegistryConfig Config;
    const bool IgnoreVersion;

public:
    TUpdateDiskRegistryConfigActor(
        TRequestInfoPtr requestInfo,
        NProto::TDiskRegistryConfig config,
        bool ignoreVersion);

    void Bootstrap(const TActorContext& ctx);

private:
    void UpdateConfig(const TActorContext& ctx);

    void HandleUpdateConfigResponse(
        const TEvDiskRegistry::TEvUpdateConfigResponse::TPtr& ev,
        const TActorContext& ctx);

    void ReplyAndDie(
        const TActorContext& ctx,
        std::unique_ptr<TEvService::TEvUpdateDiskRegistryConfigResponse> response);

private:
    STFUNC(StateWork);
};

////////////////////////////////////////////////////////////////////////////////

TUpdateDiskRegistryConfigActor::TUpdateDiskRegistryConfigActor(
        TRequestInfoPtr requestInfo,
        NProto::TDiskRegistryConfig config,
        bool ignoreVersion)
    : RequestInfo(std::move(requestInfo))
    , Config(std::move(config))
    , IgnoreVersion(ignoreVersion)
{
    ActivityType = TBlockStoreActivities::SERVICE;
}

void TUpdateDiskRegistryConfigActor::Bootstrap(const TActorContext& ctx)
{
    Become(&TThis::StateWork);

    UpdateConfig(ctx);
}

void TUpdateDiskRegistryConfigActor::UpdateConfig(const TActorContext& ctx)
{
    auto traceId = RequestInfo->TraceId.Clone();
    auto request = std::make_unique<TEvDiskRegistry::TEvUpdateConfigRequest>();

    *request->Record.MutableConfig() = Config;
    request->Record.SetIgnoreVersion(IgnoreVersion);

    BLOCKSTORE_TRACE_SENT(ctx, &traceId, this, request);

    NCloud::Send(
        ctx,
        MakeDiskRegistryProxyServiceId(),
        std::move(request),
        RequestInfo->Cookie,
        std::move(traceId));
}

void TUpdateDiskRegistryConfigActor::HandleUpdateConfigResponse(
    const TEvDiskRegistry::TEvUpdateConfigResponse::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();

    BLOCKSTORE_TRACE_RECEIVED(ctx, &RequestInfo->TraceId, this, msg, &ev->TraceId);

    const auto& error = msg->GetError();
    if (FAILED(error.GetCode())) {
        LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
            "Update Disk Registry config failed: %s. Affected disks: %s",
            FormatError(error).data(),
            JoinSeq(", ", msg->Record.GetAffectedDisks()).c_str());
    }

    auto response = std::make_unique<TEvService::TEvUpdateDiskRegistryConfigResponse>(
        error);

    response->Record.MutableAffectedDisks()->Swap(msg->Record.MutableAffectedDisks());

    ReplyAndDie(ctx, std::move(response));
}

void TUpdateDiskRegistryConfigActor::ReplyAndDie(
    const TActorContext& ctx,
    std::unique_ptr<TEvService::TEvUpdateDiskRegistryConfigResponse> response)
{
    BLOCKSTORE_TRACE_SENT(ctx, &RequestInfo->TraceId, this, response);
    NCloud::Reply(ctx, *RequestInfo, std::move(response));
    Die(ctx);
}

////////////////////////////////////////////////////////////////////////////////

STFUNC(TUpdateDiskRegistryConfigActor::StateWork)
{
    switch (ev->GetTypeRewrite()) {
        HFunc(TEvDiskRegistry::TEvUpdateConfigResponse, HandleUpdateConfigResponse);

        default:
            HandleUnexpectedEvent(ctx, ev, TBlockStoreComponents::SERVICE);
            break;
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TServiceActor::HandleUpdateDiskRegistryConfig(
    const TEvService::TEvUpdateDiskRegistryConfigRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto* msg = ev->Get();

    auto requestInfo = CreateRequestInfo(
        ev->Sender,
        ev->Cookie,
        msg->CallContext,
        std::move(ev->TraceId));

    const auto& request = msg->Record;

    BLOCKSTORE_TRACE_RECEIVED(ctx, &requestInfo->TraceId, this, msg);

    LOG_DEBUG(ctx, TBlockStoreComponents::SERVICE,
        "Update Disk Registry config");

    NProto::TDiskRegistryConfig config;

    config.SetVersion(request.GetVersion());

    for (const auto& agent: request.GetKnownAgents()) {
        *config.AddKnownAgents() = CreateConfig(agent);
    }

    for (const auto& deviceOverride: request.GetDeviceOverrides()) {
        *config.AddDeviceOverrides() = deviceOverride;
    }

    for (const auto& devicePool: request.GetKnownDevicePools()) {
        auto* pool = config.AddDevicePoolConfigs();
        pool->SetName(devicePool.GetName());
        pool->SetKind(devicePool.GetKind());
        pool->SetAllocationUnit(devicePool.GetAllocationUnit());
    }

    NCloud::Register<TUpdateDiskRegistryConfigActor>(
        ctx,
        std::move(requestInfo),
        std::move(config),
        request.GetIgnoreVersion());
}

}   // namespace NCloud::NBlockStore::NStorage
