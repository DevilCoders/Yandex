#pragma once

#include "public.h"

#include "tablet_schema.h"
#include "tablet_state.h"

#include <cloud/filestore/libs/storage/tablet/model/block_list.h>
#include <cloud/filestore/libs/storage/tablet/model/channels.h>
#include <cloud/filestore/libs/storage/tablet/model/compaction_map.h>
#include <cloud/filestore/libs/storage/tablet/model/fresh_blocks.h>
#include <cloud/filestore/libs/storage/tablet/model/fresh_bytes.h>
#include <cloud/filestore/libs/storage/tablet/model/garbage_queue.h>
#include <cloud/filestore/libs/storage/tablet/model/mixed_blocks.h>
#include <cloud/filestore/libs/storage/tablet/model/range_locks.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TIndexTabletState::TImpl
{
    TSessionList Sessions;
    TSessionList OrphanSessions;
    TSessionMap SessionById;
    TSessionOwnerMap SessionByOwner;
    TSessionClientMap SessionByClient;

    TNodeRefsByHandle NodeRefsByHandle;

    // TODO: move to TSession
    TSessionHandleMap HandleById;
    TSessionLockMap LockById;
    TSessionLockMultiMap LocksByHandle;

    TRangeLocks RangeLocks;
    TWriteRequestList WriteBatch;
    TFreshBytes FreshBytes;
    TFreshBlocks FreshBlocks;
    TMixedBlocks MixedBlocks;
    TCompactionMap CompactionMap;
    TGarbageQueue GarbageQueue;
    TCheckpointStore Checkpoints;

    TChannels Channels;

    IBlockLocation2RangeIndexPtr RangeIdHasher;
};

}   // namespace NCloud::NFileStore::NStorage
