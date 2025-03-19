#include "disk_registry_actor.h"

#include <cloud/blockstore/libs/storage/api/service.h>

namespace NCloud::NBlockStore::NStorage {

using namespace NActors;

namespace {

////////////////////////////////////////////////////////////////////////////////

NProto::EDevicePoolKind ToDevicePoolKind(NProto::EStoragePoolKind kind)
{
    switch (kind) {
        case NProto::STORAGE_POOL_KIND_LOCAL:
            return NProto::DEVICE_POOL_KIND_LOCAL;

        case NProto::STORAGE_POOL_KIND_GLOBAL:
            return NProto::DEVICE_POOL_KIND_GLOBAL;

        case NProto::STORAGE_POOL_KIND_DEFAULT:
            return NProto::DEVICE_POOL_KIND_DEFAULT;

        default:
            Y_FAIL("enexpected storage pool kind %d", kind);
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

void TDiskRegistryActor::HandleQueryAvailableStorage(
    const TEvService::TEvQueryAvailableStorageRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto& request = ev->Get()->Record;

    auto response = std::make_unique<TEvService::TEvQueryAvailableStorageResponse>();

    const THashSet<TString> agentIds{
        request.GetAgentIds().begin(),
        request.GetAgentIds().end()
    };

    for (const auto& agentId: agentIds) {
        auto [infos, error] = State->QueryAvailableStorage(
            agentId,
            request.GetStoragePoolName(),
            ToDevicePoolKind(request.GetStoragePoolKind())
        );

        if (HasError(error)) {
            LOG_WARN(ctx, TBlockStoreComponents::DISK_REGISTRY,
                "[%lu] Can't get storage info for %s",
                TabletID(),
                agentId.Quote().c_str());
            continue;
        }

        if (infos.empty()) {
            auto& dst = *response->Record.MutableAvailableStorage()->Add();
            dst.SetAgentId(agentId);
        }

        for (auto& info: infos) {
            auto& dst = *response->Record.MutableAvailableStorage()->Add();

            dst.SetAgentId(agentId);
            dst.SetChunkSize(info.ChunkSize);
            dst.SetChunkCount(info.ChunkCount);
        }
    }

    NCloud::Reply(ctx, *ev, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage
