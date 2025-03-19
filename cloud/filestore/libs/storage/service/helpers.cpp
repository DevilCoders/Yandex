#include "helpers.h"

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

void Convert(const NKikimrFileStore::TConfig& config, NProto::TFileStore& fileStore)
{
    fileStore.SetFileSystemId(config.GetFileSystemId());
    fileStore.SetProjectId(config.GetProjectId());
    fileStore.SetFolderId(config.GetFolderId());
    fileStore.SetCloudId(config.GetCloudId());
    fileStore.SetBlockSize(config.GetBlockSize());
    fileStore.SetBlocksCount(config.GetBlocksCount());
    fileStore.SetNodesCount(config.GetNodesCount());
    fileStore.SetStorageMediaKind(config.GetStorageMediaKind());
    fileStore.SetConfigVersion(config.GetVersion());
}

}   // namespace NCloud::NFileStore::NStorage
