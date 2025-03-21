#include "service_actor.h"

#include <cloud/filestore/libs/storage/core/model.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

////////////////////////////////////////////////////////////////////////////////

void TStorageServiceActor::HandleDescribeFileStoreModel(
    const TEvService::TEvDescribeFileStoreModelRequest::TPtr& ev,
    const TActorContext& ctx)
{
    const auto& request = ev->Get()->Record;

    if (request.GetBlockSize() == 0 || request.GetBlocksCount() == 0) {
        auto response = std::make_unique<TEvService::TEvDescribeFileStoreModelResponse>(
            MakeError(E_ARGUMENT, "zero block count or blocks size"));

        NCloud::Reply(ctx, *ev, std::move(response));
        return;
    }

    NKikimrFileStore::TConfig config;
    config.SetBlockSize(request.GetBlockSize());
    config.SetBlocksCount(request.GetBlocksCount());
    config.SetStorageMediaKind(request.GetStorageMediaKind());

    SetupFileStorePerformanceAndChannels(
        false,  // do not allcoate mixed0 channel
        *StorageConfig,
        config);

    auto response = std::make_unique<TEvService::TEvDescribeFileStoreModelResponse>();
    auto* model = response->Record.MutableFileStoreModel();
    model->SetBlockSize(request.GetBlockSize());
    model->SetBlocksCount(request.GetBlocksCount());
    model->SetStorageMediaKind(request.GetStorageMediaKind());
    model->SetChannelsCount(config.ExplicitChannelProfilesSize());

    auto* profile = model->MutablePerformanceProfile();
    profile->SetMaxReadIops(config.GetPerformanceProfileMaxReadIops());
    profile->SetMaxWriteIops(config.GetPerformanceProfileMaxWriteIops());

    profile->SetMaxReadBandwidth(config.GetPerformanceProfileMaxReadBandwidth());
    profile->SetMaxWriteBandwidth(config.GetPerformanceProfileMaxWriteBandwidth());

    NCloud::Reply(ctx, *ev, std::move(response));
}

}   // namespace NCloud::NFileStore::NStorage
