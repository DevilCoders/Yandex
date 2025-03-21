#pragma once

#include "public.h"

#include <cloud/filestore/libs/storage/tablet/model/block_list.h>
#include <cloud/filestore/libs/storage/tablet/model/compaction_map.h>
#include <cloud/filestore/libs/storage/tablet/model/deletion_markers.h>
#include <cloud/filestore/libs/storage/tablet/protos/tablet.pb.h>

#include <cloud/storage/core/libs/tablet/model/commit.h>
#include <cloud/storage/core/libs/tablet/model/partial_blob_id.h>
#include <cloud/storage/core/protos/tablet.pb.h>

#include <ydb/core/tablet_flat/flat_cxx_database.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

#define FILESTORE_FILESYSTEM_STATS(xxx, ...)                                   \
    xxx(LastNodeId,             __VA_ARGS__)                                   \
    xxx(LastLockId,             __VA_ARGS__)                                   \
    xxx(LastCollectCommitId,    __VA_ARGS__)                                   \
    xxx(LastXAttr,              __VA_ARGS__)                                   \
                                                                               \
    xxx(UsedNodesCount,         __VA_ARGS__)                                   \
    xxx(UsedSessionsCount,      __VA_ARGS__)                                   \
    xxx(UsedHandlesCount,       __VA_ARGS__)                                   \
    xxx(UsedLocksCount,         __VA_ARGS__)                                   \
    xxx(UsedBlocksCount,        __VA_ARGS__)                                   \
                                                                               \
    xxx(FreshBlocksCount,       __VA_ARGS__)                                   \
    xxx(MixedBlocksCount,       __VA_ARGS__)                                   \
    xxx(MixedBlobsCount,        __VA_ARGS__)                                   \
    xxx(DeletionMarkersCount,   __VA_ARGS__)                                   \
    xxx(GarbageQueueSize,       __VA_ARGS__)                                   \
    xxx(GarbageBlocksCount,     __VA_ARGS__)                                   \
    xxx(CheckpointNodesCount,   __VA_ARGS__)                                   \
    xxx(CheckpointBlocksCount,  __VA_ARGS__)                                   \
    xxx(CheckpointBlobsCount,   __VA_ARGS__)                                   \
    xxx(FreshBytesCount,        __VA_ARGS__)                                   \
    xxx(AttrsUsedBytesCount,    __VA_ARGS__)                                   \
// FILESTORE_FILESYSTEM_STATS

#define FILESTORE_DUPCACHE_REQUESTS(xxx, ...)                                  \
    xxx(CreateHandle,   __VA_ARGS__)                                           \
    xxx(CreateNode,     __VA_ARGS__)                                           \
    xxx(RenameNode,     __VA_ARGS__)                                           \
    xxx(UnlinkNode,     __VA_ARGS__)                                           \
// FILESTORE_DUPCACHE_REQUESTS

////////////////////////////////////////////////////////////////////////////////

class TIndexTabletDatabase
    : public NKikimr::NIceDb::TNiceDb
{
public:
    TIndexTabletDatabase(NKikimr::NTable::TDatabase& database)
        : NKikimr::NIceDb::TNiceDb(database)
    {}

    void InitSchema();

    //
    // FileSystem
    //

    void WriteFileSystem(const NProto::TFileSystem& fileSystem);
    bool ReadFileSystem(NProto::TFileSystem& fileSystem);
    bool ReadFileSystemStats(NProto::TFileSystemStats& stats);

#define FILESTORE_DECLARE_STATS(name, ...)                                     \
    void Write##name(ui64 value);                                              \
// FILESTORE_DECLARE_STATS

FILESTORE_FILESYSTEM_STATS(FILESTORE_DECLARE_STATS)

#undef FILESTORE_DECLARE_STATS

    bool ReadTabletStorageInfo(
        NCloud::NProto::TTabletStorageInfo& tabletStorageInfo);
    void WriteTabletStorageInfo(
        const NCloud::NProto::TTabletStorageInfo& tabletStorageInfo);

    //
    // Nodes
    //

    void WriteNode(ui64 nodeId, ui64 commitId, const NProto::TNode& attrs);
    void DeleteNode(ui64 nodeId);

    struct TNode
    {
        ui64 NodeId;
        NProto::TNode Attrs;
        ui64 MinCommitId;
        ui64 MaxCommitId;
    };

    bool ReadNode(ui64 nodeId, ui64 commitId, TMaybe<TNode>& node);

    //
    // Nodes_Ver
    //

    void WriteNodeVer(
        ui64 nodeId,
        ui64 minCommitId,
        ui64 maxCommitId,
        const NProto::TNode& attrs);

    void DeleteNodeVer(ui64 nodeId, ui64 commitId);

    bool ReadNodeVer(ui64 nodeId, ui64 commitId, TMaybe<TNode>& node);

    //
    // NodeAttrs
    //

    void WriteNodeAttr(
        ui64 nodeId,
        ui64 commitId,
        const TString& name,
        const TString& value,
        ui64 version);

    void DeleteNodeAttr(ui64 nodeId, const TString& name);

    struct TNodeAttr
    {
        ui64 NodeId;
        TString Name;
        TString Value;
        ui64 MinCommitId;
        ui64 MaxCommitId;
        ui64 Version;
    };

    bool ReadNodeAttr(
        ui64 nodeId,
        ui64 commitId,
        const TString& name,
        TMaybe<TNodeAttr>& attr);

    bool ReadNodeAttrs(ui64 nodeId, ui64 commitId, TVector<TNodeAttr>& attrs);

    //
    // NodeAttrs_Ver
    //

    void WriteNodeAttrVer(
        ui64 nodeId,
        ui64 minCommitId,
        ui64 maxCommitId,
        const TString& name,
        const TString& value,
        ui64 version);

    void DeleteNodeAttrVer(ui64 nodeId, ui64 commitId, const TString& name);

    bool ReadNodeAttrVer(
        ui64 nodeId,
        ui64 commitId,
        const TString& name,
        TMaybe<TNodeAttr>& attr);

    bool ReadNodeAttrVers(
        ui64 nodeId,
        ui64 commitId,
        TVector<TNodeAttr>& attrs);

    //
    // NodeRefs
    //

    void WriteNodeRef(
        ui64 nodeId,
        ui64 commitId,
        const TString& name,
        ui64 childNode);

    void DeleteNodeRef(ui64 nodeId, const TString& name);

    struct TNodeRef
    {
        ui64 NodeId;
        TString Name;
        ui64 ChildNodeId;
        ui64 MinCommitId;
        ui64 MaxCommitId;
    };

    bool ReadNodeRef(
        ui64 nodeId,
        ui64 commitId,
        const TString& name,
        TMaybe<TNodeRef>& ref);

    bool ReadNodeRefs(
        ui64 nodeId,
        ui64 commitId,
        const TString& name,
        TVector<TNodeRef>& refs,
        ui32 maxBytes,
        TString* next = nullptr);

    //
    // NodeRefs_Ver
    //

    void WriteNodeRefVer(
        ui64 nodeId,
        ui64 minCommitId,
        ui64 maxCommitId,
        const TString& name,
        ui64 childNode);

    void DeleteNodeRefVer(ui64 nodeId, ui64 commitId, const TString& name);

    bool ReadNodeRefVer(
        ui64 nodeId,
        ui64 commitId,
        const TString& name,
        TMaybe<TNodeRef>& ref);

    bool ReadNodeRefVers(ui64 nodeId, ui64 commitId, TVector<TNodeRef>& refs);

    //
    // Sessions
    //

    void WriteSession(const NProto::TSession& session);
    void DeleteSession(const TString& sessionId);
    bool ReadSessions(TVector<NProto::TSession>& sessions);

    //
    // SessionHandles
    //

    void WriteSessionHandle(const NProto::TSessionHandle& handle);
    void DeleteSessionHandle(const TString& sessionId, ui64 handle);
    bool ReadSessionHandles(TVector<NProto::TSessionHandle>& handles);

    bool ReadSessionHandles(
        const TString& sessionId,
        TVector<NProto::TSessionHandle>& handles);

    //
    // SessionLocks
    //

    void WriteSessionLock(const NProto::TSessionLock& lock);
    void DeleteSessionLock(const TString& sessionId, ui64 lockId);
    bool ReadSessionLocks(TVector<NProto::TSessionLock>& locks);

    bool ReadSessionLocks(
        const TString& sessionId,
        TVector<NProto::TSessionLock>& locks);

    //
    // SessionDuplicateCache
    //

    void WriteSessionDupCacheEntry(const NProto::TDupCacheEntry& entry);
    void DeleteSessionDupCacheEntry(const TString& sessionId, ui64 entryId);
    bool ReadSessionDupCacheEntries(TVector<NProto::TDupCacheEntry>& entries);

    //
    // FreshBytes
    //

    void WriteFreshBytes(
        ui64 nodeId,
        ui64 commitId,
        ui64 offset,
        TStringBuf data);

    void WriteFreshBytesDeletionMarker(
        ui64 nodeId,
        ui64 commitId,
        ui64 offset,
        ui64 len);

    void DeleteFreshBytes(ui64 nodeId, ui64 commitId, ui64 offset);

    struct TFreshBytesEntry
    {
        ui64 NodeId;
        ui64 MinCommitId;
        ui64 Offset;
        TString Data;
        ui64 Len;
    };

    bool ReadFreshBytes(TVector<TFreshBytesEntry>& bytes);

    //
    // FreshBlocks
    //

    void WriteFreshBlock(
        ui64 nodeId,
        ui64 commitId,
        ui32 blockIndex,
        TStringBuf blockData);

    void MarkFreshBlockDeleted(
        ui64 nodeId,
        ui64 minCommitId,
        ui64 maxCommitId,
        ui32 blockIndex);

    void DeleteFreshBlock(ui64 nodeId, ui64 commitId, ui32 blockIndex);

    struct TFreshBlock
    {
        ui64 NodeId;
        ui32 BlockIndex;
        ui64 MinCommitId;
        ui64 MaxCommitId;
        TString BlockData;
    };

    bool ReadFreshBlocks(TVector<TFreshBlock>& blocks);

    //
    // MixedBlocks
    //

    void WriteMixedBlocks(
        ui32 rangeId,
        const TPartialBlobId& blobId,
        const TBlockList& blockList,
        ui32 garbageBlocks,
        ui32 checkpointBlocks);

    void DeleteMixedBlocks(ui32 rangeId, const TPartialBlobId& blobId);

    struct TMixedBlob
    {
        TPartialBlobId BlobId;
        TBlockList BlockList;
        ui32 GarbageBlocks;
        ui32 CheckpointBlocks;
    };

    bool ReadMixedBlocks(
        ui32 rangeId,
        const TPartialBlobId& blobId,
        TMaybe<TMixedBlob>& blob);

    bool ReadMixedBlocks(ui32 rangeId, TVector<TMixedBlob>& blobs);

    //
    // DeletionMarkers
    //

    void WriteDeletionMarkers(
        ui32 rangeId,
        ui64 nodeId,
        ui64 commitId,
        ui32 blockIndex,
        ui32 blocksCount);

    void DeleteDeletionMarker(
        ui32 rangeId,
        ui64 nodeId,
        ui64 commitId,
        ui32 blockIndex);

    bool ReadDeletionMarkers(
        ui32 rangeId,
        TVector<TDeletionMarker>& deletionMarkers);

    //
    // NewBlobs
    //

    void WriteNewBlob(const TPartialBlobId& blobId);
    void DeleteNewBlob(const TPartialBlobId& blobId);
    bool ReadNewBlobs(TVector<TPartialBlobId>& blobIds);

    //
    // GarbageBlobs
    //

    void WriteGarbageBlob(const TPartialBlobId& blobId);
    void DeleteGarbageBlob(const TPartialBlobId& blobId);
    bool ReadGarbageBlobs(TVector<TPartialBlobId>& blobIds);

    //
    // Checkpoints
    //

    void WriteCheckpoint(const NProto::TCheckpoint& checkpoint);
    void DeleteCheckpoint(const TString& checkpointId);
    bool ReadCheckpoints(TVector<NProto::TCheckpoint>& checkpoints);

    //
    // CheckpointNodes
    //

    void WriteCheckpointNode(ui64 checkpointId, ui64 nodeId);
    void DeleteCheckpointNode(ui64 checkpointId, ui64 nodeId);

    bool ReadCheckpointNodes(
        ui64 checkpointId,
        TVector<ui64>& nodes,
        size_t maxCount = 100);

    //
    // CheckpointBlobs
    //

    void WriteCheckpointBlob(
        ui64 checkpointId,
        ui32 rangeId,
        const TPartialBlobId& blobId);

    void DeleteCheckpointBlob(
        ui64 checkpointId,
        ui32 rangeId,
        const TPartialBlobId& blobId);

    struct TCheckpointBlob
    {
        ui32 RangeId;
        TPartialBlobId BlobId;
    };

    bool ReadCheckpointBlobs(
        ui64 checkpointId,
        TVector<TCheckpointBlob>& blobs,
        size_t maxCount = 100);

    //
    // CompactionMap
    //

    void WriteCompactionMap(ui32 rangeId, ui32 blobsCount, ui32 deletionsCount);
    bool ReadCompactionMap(TVector<TCompactionRangeInfo>& compactionMap);
};

}   // namespace NCloud::NFileStore::NStorage
