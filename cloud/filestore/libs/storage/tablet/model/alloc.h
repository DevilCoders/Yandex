#pragma once

#include <cloud/storage/core/libs/common/alloc.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

enum class EAllocatorTag
{
    BlobMetaMap,
    BlockList,
    CompactionMap,
    DeletionMarkers,
    FreshBlocks,
    GarbageQueue,

    Max
};

////////////////////////////////////////////////////////////////////////////////

TProfilingAllocator* GetAllocatorByTag(EAllocatorTag tag);

}   // namespace NCloud::NFileStore::NStorage
