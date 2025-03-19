#pragma once

#include "public.h"

#include "tablet_database.h"
#include "tablet_private.h"

#include <cloud/filestore/libs/diagnostics/events/profile_events.ev.pb.h>
#include <cloud/filestore/libs/service/request.h>
#include <cloud/filestore/libs/storage/api/tablet.h>
#include <cloud/filestore/libs/storage/core/request_info.h>
#include <cloud/filestore/libs/storage/tablet/model/block.h>
#include <cloud/filestore/libs/storage/tablet/model/block_buffer.h>
#include <cloud/filestore/libs/storage/tablet/model/range.h>
#include <cloud/filestore/libs/storage/tablet/model/range_locks.h>
#include <cloud/filestore/libs/storage/tablet/protos/tablet.pb.h>

#include <cloud/storage/core/libs/common/error.h>

#include <util/folder/pathsplit.h>
#include <util/generic/hash_set.h>
#include <util/generic/intrlist.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

#define FILESTORE_VALIDATE_EVENT_SESSION(event, request)                                       \
    if (auto session = FindSession(GetClientId(request), GetSessionId(request)); !session) {   \
        auto response = std::make_unique<TEvService::TEv##event##Response>(                    \
            ErrorInvalidSession(GetClientId(request), GetSessionId(request)));                 \
        return NCloud::Reply(ctx, *ev, std::move(response));                                   \
    }                                                                                          \
// FILESTORE_VALIDATE_EVENT_SESSION

#define FILESTORE_VALIDATE_DUPEVENT_SESSION(event, request)                                    \
    FILESTORE_VALIDATE_EVENT_SESSION(event, request)                                           \
    else if (auto entry = session->LookupDupEntry(GetRequestId(request))) {                    \
        auto response = std::make_unique<TEvService::TEv##event##Response>();                  \
        GetDupCacheEntry(entry, response->Record);                                             \
        return NCloud::Reply(ctx, *ev, std::move(response));                                   \
    }                                                                                          \
// FILESTORE_VALIDATE_DUPEVENT_SESSION

#define FILESTORE_VALIDATE_TX_SESSION(event, args)                             \
    if (!FindSession(args.ClientId, args.SessionId)) {                         \
        args.Error = ErrorInvalidSession(args.ClientId, args.SessionId);       \
        return true;                                                           \
    }                                                                          \
// FILESTORE_VALIDATE_TX_SESSION

#define FILESTORE_VALIDATE_TX_ERROR(event, args)                               \
    if (FAILED(args.Error.GetCode())) {                                        \
        return;                                                                \
    }                                                                          \
// FILESTORE_VALIDATE_TX_ERROR

////////////////////////////////////////////////////////////////////////////////

#define FILESTORE_TABLET_TRANSACTIONS(xxx, ...)                                \
    xxx(InitSchema,                         __VA_ARGS__)                       \
    xxx(LoadState,                          __VA_ARGS__)                       \
    xxx(UpdateConfig,                       __VA_ARGS__)                       \
                                                                               \
    xxx(CreateSession,                      __VA_ARGS__)                       \
    xxx(ResetSession,                       __VA_ARGS__)                       \
    xxx(DestroySession,                     __VA_ARGS__)                       \
                                                                               \
    xxx(CreateCheckpoint,                   __VA_ARGS__)                       \
    xxx(DeleteCheckpoint,                   __VA_ARGS__)                       \
                                                                               \
    xxx(ResolvePath,                        __VA_ARGS__)                       \
    xxx(CreateNode,                         __VA_ARGS__)                       \
    xxx(UnlinkNode,                         __VA_ARGS__)                       \
    xxx(RenameNode,                         __VA_ARGS__)                       \
    xxx(AccessNode,                         __VA_ARGS__)                       \
    xxx(ListNodes,                          __VA_ARGS__)                       \
    xxx(ReadLink,                           __VA_ARGS__)                       \
                                                                               \
    xxx(SetNodeAttr,                        __VA_ARGS__)                       \
    xxx(GetNodeAttr,                        __VA_ARGS__)                       \
    xxx(SetNodeXAttr,                       __VA_ARGS__)                       \
    xxx(GetNodeXAttr,                       __VA_ARGS__)                       \
    xxx(ListNodeXAttr,                      __VA_ARGS__)                       \
    xxx(RemoveNodeXAttr,                    __VA_ARGS__)                       \
                                                                               \
    xxx(CreateHandle,                       __VA_ARGS__)                       \
    xxx(DestroyHandle,                      __VA_ARGS__)                       \
                                                                               \
    xxx(AcquireLock,                        __VA_ARGS__)                       \
    xxx(ReleaseLock,                        __VA_ARGS__)                       \
    xxx(TestLock,                           __VA_ARGS__)                       \
                                                                               \
    xxx(ReadData,                           __VA_ARGS__)                       \
    xxx(WriteData,                          __VA_ARGS__)                       \
    xxx(WriteBatch,                         __VA_ARGS__)                       \
    xxx(AllocateData,                       __VA_ARGS__)                       \
                                                                               \
    xxx(AddBlob,                            __VA_ARGS__)                       \
    xxx(FlushBytes,                         __VA_ARGS__)                       \
    xxx(TrimBytes,                          __VA_ARGS__)                       \
    xxx(Compaction,                         __VA_ARGS__)                       \
    xxx(Cleanup,                            __VA_ARGS__)                       \
    xxx(DeleteGarbage,                      __VA_ARGS__)                       \
// FILESTORE_TABLET_TRANSACTIONS

////////////////////////////////////////////////////////////////////////////////

struct TSessionAware
{
    const TString ClientId;
    const TString SessionId;
    const ui64 RequestId;

    NProto::TError Error;

    template<typename T>
    TSessionAware(const T& request) noexcept
        : ClientId(GetClientId(request))
        , SessionId(GetSessionId(request))
        , RequestId(GetRequestId(request))
    {}
};

////////////////////////////////////////////////////////////////////////////////

struct TWriteRequest
    : public TIntrusiveListItem<TWriteRequest>
    , public TSessionAware
{
    const TRequestInfoPtr RequestInfo;
    const ui64 Handle;
    const TByteRange ByteRange;
    const IBlockBufferPtr Buffer;

    ui64 NodeId = InvalidNodeId;
    NProto::TError Error;

    TWriteRequest(
            TRequestInfoPtr requestInfo,
            const NProto::TWriteDataRequest& request,
            TByteRange byteRange,
            IBlockBufferPtr buffer)
        : TSessionAware(request)
        , RequestInfo(std::move(requestInfo))
        , Handle(request.GetHandle())
        , ByteRange(byteRange)
        , Buffer(std::move(buffer))
    {}
};

using TWriteRequestList = TIntrusiveListWithAutoDelete<TWriteRequest, TDelete>;

////////////////////////////////////////////////////////////////////////////////

struct TNodeOps
{
    template <typename T>
    static auto GetNodeId(const T& value)
    {
        return value;
    }

    static auto GetNodeId(const TIndexTabletDatabase::TNode& node)
    {
        return node.NodeId;
    }

    struct TNodeSetHash
    {
        template <typename T>
        size_t operator ()(const T& value) const noexcept
        {
            return IntHash(GetNodeId(value));
        }
    };

    struct TNodeSetEqual
    {
        template <typename T1, typename T2>
        bool operator ()(const T1& lhs, const T2& rhs) const noexcept
        {
            return GetNodeId(lhs) == GetNodeId(rhs);
        }
    };
};

using TNodeSet = THashSet<TIndexTabletDatabase::TNode, TNodeOps::TNodeSetHash, TNodeOps::TNodeSetEqual>;

////////////////////////////////////////////////////////////////////////////////

struct TTxIndexTablet
{
    //
    // InitSchema
    //

    struct TInitSchema
    {
        // actually unused, needed in tablet_tx.h to avoid sophisticated
        // template tricks
        const TRequestInfoPtr RequestInfo;

        void Clear()
        {
            // nothing to do
        }
    };

    //
    // LoadState
    //

    struct TLoadState
    {
        // actually unused, needed in tablet_tx.h to avoid sophisticated
        // template tricks
        const TRequestInfoPtr RequestInfo;

        NProto::TFileSystem FileSystem;
        NProto::TFileSystemStats FileSystemStats;
        NCloud::NProto::TTabletStorageInfo TabletStorageInfo;
        TMaybe<TIndexTabletDatabase::TNode> RootNode;
        TVector<NProto::TSession> Sessions;
        TVector<NProto::TSessionHandle> Handles;
        TVector<NProto::TSessionLock> Locks;
        TVector<TIndexTabletDatabase::TFreshBytesEntry> FreshBytes;
        TVector<TIndexTabletDatabase::TFreshBlock> FreshBlocks;
        TVector<TPartialBlobId> NewBlobs;
        TVector<TPartialBlobId> GarbageBlobs;
        TVector<NProto::TCheckpoint> Checkpoints;
        TVector<TCompactionRangeInfo> CompactionMap;
        TVector<NProto::TDupCacheEntry> DupCache;

        NProto::TError Error;

        void Clear()
        {
            FileSystem.Clear();
            FileSystemStats.Clear();
            TabletStorageInfo.Clear();
            RootNode.Clear();
            Sessions.clear();
            Handles.clear();
            Locks.clear();
            FreshBytes.clear();
            FreshBlocks.clear();
            NewBlobs.clear();
            GarbageBlobs.clear();
            Checkpoints.clear();
            CompactionMap.clear();
            DupCache.clear();
        }
    };

    //
    // UpdateConfig
    //

    struct TUpdateConfig
    {
        const TRequestInfoPtr RequestInfo;
        const ui64 TxId;
        const NProto::TFileSystem FileSystem;

        TUpdateConfig(
                TRequestInfoPtr requestInfo,
                ui64 txId,
                NProto::TFileSystem fileSystem)
            : RequestInfo(std::move(requestInfo))
            , TxId(txId)
            , FileSystem(std::move(fileSystem))
        {}

        void Clear()
        {
            // nothing to do
        }
    };

    //
    // CreateSession
    //

    struct TCreateSession
    {
        const TRequestInfoPtr RequestInfo;
        const NProtoPrivate::TCreateSessionRequest Request;

        NProto::TError Error;
        TString SessionId;

        TCreateSession(
                TRequestInfoPtr requestInfo,
                const NProtoPrivate::TCreateSessionRequest& request)
            : RequestInfo(std::move(requestInfo))
            , Request(request)
        {}

        void Clear()
        {
            Error.Clear();
            SessionId.clear();
        }
    };

    //
    // DestroySession
    //

    struct TResetSession
    {
        const TRequestInfoPtr RequestInfo;
        const TString SessionId;
        const TString SessionState;

        TNodeSet Nodes;

        TResetSession(TRequestInfoPtr requestInfo, TString sessionId, TString sessionState)
            : RequestInfo(std::move(requestInfo))
            , SessionId(std::move(sessionId))
            , SessionState(std::move(sessionState))
        {}

        void Clear()
        {
            Nodes.clear();
        }
    };

    //
    // DestroySession
    //

    struct TDestroySession
    {
        const TRequestInfoPtr RequestInfo;
        const TString SessionId;

        TNodeSet Nodes;

        TDestroySession(TRequestInfoPtr requestInfo, TString sessionId)
            : RequestInfo(std::move(requestInfo))
            , SessionId(std::move(sessionId))
        {}

        void Clear()
        {
            Nodes.clear();
        }
    };

    //
    // CreateCheckpoint
    //

    struct TCreateCheckpoint
    {
        const TRequestInfoPtr RequestInfo;
        const TString CheckpointId;
        const ui64 NodeId;

        NProto::TError Error;
        ui64 CommitId = 0;

        TCreateCheckpoint(
                TRequestInfoPtr requestInfo,
                TString checkpointId,
                ui64 nodeId)
            : RequestInfo(std::move(requestInfo))
            , CheckpointId(std::move(checkpointId))
            , NodeId(nodeId)
        {}

        void Clear()
        {
            Error.Clear();
            CommitId = 0;
        }
    };

    //
    // DeleteCheckpoint
    //

    struct TDeleteCheckpoint
    {
        const TRequestInfoPtr RequestInfo;
        const TString CheckpointId;
        const EDeleteCheckpointMode Mode;
        const ui64 CollectBarrier;

        NProto::TError Error;
        ui64 CommitId = 0;

        TVector<ui64> NodeIds;
        TVector<TIndexTabletDatabase::TNode> Nodes;
        TVector<TIndexTabletDatabase::TNodeAttr> NodeAttrs;
        TVector<TIndexTabletDatabase::TNodeRef> NodeRefs;

        TVector<TIndexTabletDatabase::TCheckpointBlob> Blobs;
        TVector<TIndexTabletDatabase::TMixedBlob> MixedBlobs;

        TDeleteCheckpoint(
                TRequestInfoPtr requestInfo,
                TString checkpointId,
                EDeleteCheckpointMode mode,
                ui64 collectBarrier)
            : RequestInfo(std::move(requestInfo))
            , CheckpointId(std::move(checkpointId))
            , Mode(mode)
            , CollectBarrier(collectBarrier)
        {}

        void Clear()
        {
            Error.Clear();
            CommitId = 0;

            NodeIds.clear();
            Nodes.clear();
            NodeAttrs.clear();
            NodeRefs.clear();

            Blobs.clear();
            MixedBlobs.clear();
        }
    };

    //
    // ResolvePath
    //

    struct TResolvePath : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const NProto::TResolvePathRequest Request;
        const TString Path;

        ui64 CommitId = 0;

        TResolvePath(
                TRequestInfoPtr requestInfo,
                const NProto::TResolvePathRequest& request)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , Request(request)
            , Path(request.GetPath())
        {}

        void Clear()
        {
            Error.Clear();
            CommitId = 0;
        }
    };

    //
    // CreateNode
    //

    struct TCreateNode : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const ui64 ParentNodeId;
        const ui64 TargetNodeId;
        const TString Name;
        const NProto::TNode Attrs;

        ui64 CommitId = 0;
        TMaybe<TIndexTabletDatabase::TNode> ParentNode;
        ui64 ChildNodeId = InvalidNodeId;
        TMaybe<TIndexTabletDatabase::TNode> ChildNode;

        NProto::TCreateNodeResponse Response;

        TCreateNode(
                TRequestInfoPtr requestInfo,
                const NProto::TCreateNodeRequest request,
                ui64 parentNodeId,
                ui64 targetNodeId,
                TString name,
                const NProto::TNode& attrs)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , ParentNodeId(parentNodeId)
            , TargetNodeId(targetNodeId)
            , Name(std::move(name))
            , Attrs(attrs)
        {}

        void Clear()
        {
            Error.Clear();
            CommitId = 0;
            ParentNode.Clear();
            ChildNodeId = InvalidNodeId;
            ChildNode.Clear();
            Response.Clear();
        }
    };

    //
    // UnlinkNode
    //

    struct TUnlinkNode : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const NProto::TUnlinkNodeRequest Request;
        const ui64 ParentNodeId;
        const TString Name;

        ui64 CommitId = 0;
        TMaybe<TIndexTabletDatabase::TNode> ParentNode;
        TMaybe<TIndexTabletDatabase::TNode> ChildNode;
        TMaybe<TIndexTabletDatabase::TNodeRef> ChildRef;

        NProto::TUnlinkNodeResponse Response;

        TUnlinkNode(
                TRequestInfoPtr requestInfo,
                const NProto::TUnlinkNodeRequest& request)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , Request(request)
            , ParentNodeId(request.GetNodeId())
            , Name(request.GetName())
        {}

        void Clear()
        {
            Error.Clear();
            CommitId = 0;
            ParentNode.Clear();
            ChildNode.Clear();
            ChildRef.Clear();
            Response.Clear();
        }
    };

    //
    // RenameNode
    //

    struct TRenameNode : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const NProto::TRenameNodeRequest Request;
        const ui64 ParentNodeId;
        const TString Name;
        const ui64 NewParentNodeId;
        const TString NewName;

        ui64 CommitId = 0;
        TMaybe<TIndexTabletDatabase::TNode> ParentNode;
        TMaybe<TIndexTabletDatabase::TNode> ChildNode;
        TMaybe<TIndexTabletDatabase::TNodeRef> ChildRef;

        TMaybe<TIndexTabletDatabase::TNode> NewParentNode;
        TMaybe<TIndexTabletDatabase::TNode> NewChildNode;
        TMaybe<TIndexTabletDatabase::TNodeRef> NewChildRef;

        NProto::TRenameNodeResponse Response;

        TRenameNode(
                TRequestInfoPtr requestInfo,
                const NProto::TRenameNodeRequest& request)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , ParentNodeId(request.GetNodeId())
            , Name(request.GetName())
            , NewParentNodeId(request.GetNewParentId())
            , NewName(request.GetNewName())
        {}

        void Clear()
        {
            Error.Clear();
            CommitId = 0;
            ParentNode.Clear();
            ChildNode.Clear();
            ChildRef.Clear();
            NewParentNode.Clear();
            NewChildNode.Clear();
            NewChildRef.Clear();
            Response.Clear();
        }
    };

    //
    // AccessNode
    //

    struct TAccessNode : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const NProto::TAccessNodeRequest Request;
        const ui64 NodeId;

        ui64 CommitId = 0;
        TMaybe<TIndexTabletDatabase::TNode> Node;

        TAccessNode(
                TRequestInfoPtr requestInfo,
                const NProto::TAccessNodeRequest& request)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , Request(request)
            , NodeId(request.GetNodeId())
        {}

        void Clear()
        {
            Error.Clear();
            CommitId = 0;
            Node.Clear();
        }
    };

    //
    // ReadLink
    //

    struct TReadLink : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const NProto::TReadLinkRequest Request;
        const ui64 NodeId;

        ui64 CommitId = 0;
        TMaybe<TIndexTabletDatabase::TNode> Node;

        TReadLink(
                TRequestInfoPtr requestInfo,
                const NProto::TReadLinkRequest& request)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , NodeId(request.GetNodeId())
        {}

        void Clear()
        {
            Error.Clear();
            CommitId = 0;
            Node.Clear();
        }
    };

    //
    // ListNodes
    //

    struct TListNodes : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const NProto::TListNodesRequest Request;
        const ui64 NodeId;
        const TString Cookie;
        const ui32 MaxBytes;

        ui64 CommitId = 0;
        TMaybe<TIndexTabletDatabase::TNode> Node;
        TVector<TIndexTabletDatabase::TNodeRef> ChildRefs;
        TVector<TIndexTabletDatabase::TNode> ChildNodes;
        TString Next;

        TListNodes(
                TRequestInfoPtr requestInfo,
                const NProto::TListNodesRequest& request,
                ui32 maxBytes)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , Request(request)
            , NodeId(request.GetNodeId())
            , Cookie(request.GetCookie())
            , MaxBytes(maxBytes)
        {}

        void Clear()
        {
            Error.Clear();
            CommitId = 0;
            Node.Clear();
            ChildRefs.clear();
            ChildNodes.clear();
            Next.clear();
        }
    };

    //
    // SetNodeAttr
    //

    struct TSetNodeAttr : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const NProto::TSetNodeAttrRequest Request;
        const ui64 NodeId;

        ui64 CommitId = 0;
        TMaybe<TIndexTabletDatabase::TNode> Node;

        TSetNodeAttr(
                TRequestInfoPtr requestInfo,
                const NProto::TSetNodeAttrRequest& request)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , Request(request)
            , NodeId(request.GetNodeId())
        {}

        void Clear()
        {
            Error.Clear();
            CommitId = 0;
            Node.Clear();
        }
    };

    //
    // GetNodeAttr
    //

    struct TGetNodeAttr : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const NProto::TGetNodeAttrRequest Request;
        const ui64 NodeId;
        const TString Name;

        ui64 CommitId = 0;
        TMaybe<TIndexTabletDatabase::TNode> ParentNode;
        ui64 TargetNodeId = InvalidNodeId;
        TMaybe<TIndexTabletDatabase::TNode> TargetNode;

        TGetNodeAttr(
                TRequestInfoPtr requestInfo,
                const NProto::TGetNodeAttrRequest& request)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , Request(request)
            , NodeId(request.GetNodeId())
            , Name(request.GetName())
        {}

        void Clear()
        {
            Error.Clear();
            CommitId = 0;
            ParentNode.Clear();
            TargetNodeId = InvalidNodeId;
            TargetNode.Clear();
        }
    };

    //
    // SetNodeXAttr
    //

    struct TSetNodeXAttr : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const NProto::TSetNodeXAttrRequest Request;
        const ui64 NodeId;
        const TString Name;
        const TString Value;

        ui64 Version = 0;
        ui64 CommitId = 0;
        TMaybe<TIndexTabletDatabase::TNode> Node;
        TMaybe<TIndexTabletDatabase::TNodeAttr> Attr;

        TSetNodeXAttr(
                TRequestInfoPtr requestInfo,
                const NProto::TSetNodeXAttrRequest& request)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , Request(request)
            , NodeId(request.GetNodeId())
            , Name(request.GetName())
            , Value(request.GetValue())
        {}

        void Clear()
        {
            Error.Clear();
            CommitId = 0;
            Node.Clear();
            Attr.Clear();
        }
    };

    //
    // GetNodeXAttr
    //

    struct TGetNodeXAttr : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const NProto::TGetNodeXAttrRequest Request;
        const ui64 NodeId;
        const TString Name;

        ui64 CommitId = 0;

        TMaybe<TIndexTabletDatabase::TNode> Node;
        TMaybe<TIndexTabletDatabase::TNodeAttr> Attr;

        TGetNodeXAttr(
                TRequestInfoPtr requestInfo,
                const NProto::TGetNodeXAttrRequest& request)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , Request(request)
            , NodeId(request.GetNodeId())
            , Name(request.GetName())
        {}

        void Clear()
        {
            Error.Clear();
            CommitId = 0;
            Node.Clear();
            Attr.Clear();
        }
    };

    //
    // ListNodeXAttr
    //

    struct TListNodeXAttr : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const NProto::TListNodeXAttrRequest Request;
        const ui64 NodeId;

        ui64 CommitId = 0;
        TMaybe<TIndexTabletDatabase::TNode> Node;
        TVector<TIndexTabletDatabase::TNodeAttr> Attrs;

        TListNodeXAttr(
                TRequestInfoPtr requestInfo,
                const NProto::TListNodeXAttrRequest& request)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , Request(request)
            , NodeId(request.GetNodeId())
        {}

        void Clear()
        {
            Error.Clear();
            CommitId = 0;
            Node.Clear();
            Attrs.clear();
        }
    };

    //
    // RemoveNodeXAttr
    //

    struct TRemoveNodeXAttr : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const NProto::TRemoveNodeXAttrRequest Request;
        const ui64 NodeId;
        const TString Name;

        ui64 CommitId = 0;
        TMaybe<TIndexTabletDatabase::TNode> Node;
        TMaybe<TIndexTabletDatabase::TNodeAttr> Attr;

        TRemoveNodeXAttr(
                TRequestInfoPtr requestInfo,
                const NProto::TRemoveNodeXAttrRequest& request)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , Request(request)
            , NodeId(request.GetNodeId())
            , Name(request.GetName())
        {}

        void Clear()
        {
            Error.Clear();
            CommitId = 0;
            Node.Clear();
            Attr.Clear();
        }
    };

    //
    // CreateHandle
    //

    struct TCreateHandle : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const ui64 NodeId;
        const TString Name;
        const ui32 Flags;
        const ui32 Mode;
        const ui32 Uid;
        const ui32 Gid;

        ui64 ReadCommitId = 0;
        ui64 WriteCommitId = 0;
        ui64 TargetNodeId = InvalidNodeId;
        TMaybe<TIndexTabletDatabase::TNode> TargetNode;
        TMaybe<TIndexTabletDatabase::TNode> ParentNode;

        NProto::TCreateHandleResponse Response;

        TCreateHandle(
                TRequestInfoPtr requestInfo,
                const NProto::TCreateHandleRequest& request)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , NodeId(request.GetNodeId())
            , Name(request.GetName())
            , Flags(request.GetFlags())
            , Mode(request.GetMode())
            , Uid(request.GetUid())
            , Gid(request.GetGid())
        {}

        void Clear()
        {
            Error.Clear();
            ReadCommitId = 0;
            WriteCommitId = 0;
            TargetNodeId = InvalidNodeId;
            TargetNode.Clear();
            ParentNode.Clear();
            Response.Clear();
        }
    };

    //
    // DestroyHandle
    //

    struct TDestroyHandle : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const NProto::TDestroyHandleRequest Request;

        TMaybe<TIndexTabletDatabase::TNode> Node;

        TDestroyHandle(
                TRequestInfoPtr requestInfo,
                const NProto::TDestroyHandleRequest& request)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , Request(request)
        {}

        void Clear()
        {
            Node.Clear();
        }
    };

    //
    // AcquireLock
    //

    struct TAcquireLock : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const NProto::TAcquireLockRequest Request;

        TAcquireLock(
                TRequestInfoPtr requestInfo,
                const NProto::TAcquireLockRequest& request)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , Request(request)
        {}

        void Clear()
        {
            Error.Clear();
        }
    };

    //
    // ReleaseLock
    //

    struct TReleaseLock : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const NProto::TReleaseLockRequest Request;

        TReleaseLock(
                TRequestInfoPtr requestInfo,
                const NProto::TReleaseLockRequest& request)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , Request(request)
        {}

        void Clear()
        {
            Error.Clear();
        }
    };

    //
    // TestLock
    //

    struct TTestLock : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const NProto::TTestLockRequest Request;

        TMaybe<TLockRange> Conflicting;

        TTestLock(
                TRequestInfoPtr requestInfo,
                const NProto::TTestLockRequest& request)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , Request(request)
        {}

        void Clear()
        {
            Error.Clear();
            Conflicting.Clear();
        }
    };

    //
    // ReadData
    //

    struct TReadData : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const ui64 Handle;
        const TByteRange OriginByteRange;
        const TByteRange AlignedByteRange;
        /*const*/ IBlockBufferPtr Buffer;

        ui64 CommitId = InvalidCommitId;
        ui64 NodeId = InvalidNodeId;
        TMaybe<TIndexTabletDatabase::TNode> Node;
        TVector<TBlockDataRef> Blocks;
        TVector<TBlockBytes> Bytes;

        TReadData(
                TRequestInfoPtr requestInfo,
                const NProto::TReadDataRequest& request,
                TByteRange originByteRange,
                TByteRange alignedByteRange,
                IBlockBufferPtr buffer)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , Handle(request.GetHandle())
            , OriginByteRange(originByteRange)
            , AlignedByteRange(alignedByteRange)
            , Buffer(std::move(buffer))
            , Blocks(AlignedByteRange.BlockCount())
            , Bytes(AlignedByteRange.BlockCount())
        {
            Y_VERIFY_DEBUG(AlignedByteRange.IsAligned());
        }

        void Clear()
        {
            Error.Clear();
            CommitId = 0;
            NodeId = InvalidNodeId;
            Node.Clear();
            std::fill(Blocks.begin(), Blocks.end(), TBlockDataRef());
            std::fill(Bytes.begin(), Bytes.end(), TBlockBytes());
        }
    };

    //
    // WriteData
    //

    struct TWriteData : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const ui32 WriteBlobThreshold;
        const ui64 Handle;
        const TByteRange ByteRange;
        /*const*/ IBlockBufferPtr Buffer;

        ui64 CommitId = 0;
        ui64 NodeId = InvalidNodeId;
        TMaybe<TIndexTabletDatabase::TNode> Node;

        TWriteData(
                TRequestInfoPtr requestInfo,
                const ui32 writeBlobThreshold,
                const NProto::TWriteDataRequest& request,
                TByteRange byteRange,
                IBlockBufferPtr buffer)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , WriteBlobThreshold(writeBlobThreshold)
            , Handle(request.GetHandle())
            , ByteRange(byteRange)
            , Buffer(std::move(buffer))
        {}

        void Clear()
        {
            Error.Clear();
            CommitId = 0;
            NodeId = InvalidNodeId;
            Node.Clear();
        }

        bool ShouldWriteBlob() const
        {
            // skip fresh completely for large aligned writes
            return ByteRange.IsAligned()
                && ByteRange.Length >= WriteBlobThreshold;
        }
    };

    //
    // WriteBatch
    //

    struct TWriteBatch
    {
        const TRequestInfoPtr RequestInfo;
        const bool SkipFresh;
        /*const*/ TWriteRequestList WriteBatch;

        ui64 CommitId = 0;
        TMap<ui64, ui64> WriteRanges;
        TNodeSet Nodes;

        NProto::TError Error;

        TWriteBatch(
                TRequestInfoPtr requestInfo,
                bool skipFresh,
                TWriteRequestList writeBatch)
            : RequestInfo(std::move(requestInfo))
            , SkipFresh(skipFresh)
            , WriteBatch(std::move(writeBatch))
        {}

        void Clear()
        {
            CommitId = 0;
            WriteRanges.clear();
            Nodes.clear();
        }
    };

    //
    // AllocateData
    //

    struct TAllocateData : TSessionAware
    {
        const TRequestInfoPtr RequestInfo;
        const ui64 Handle;
        const ui64 Offset;
        const ui64 Length;

        ui64 CommitId = 0;
        ui64 NodeId = InvalidNodeId;
        TMaybe<TIndexTabletDatabase::TNode> Node;

        TAllocateData(
                TRequestInfoPtr requestInfo,
                const NProto::TAllocateDataRequest& request)
            : TSessionAware(request)
            , RequestInfo(std::move(requestInfo))
            , Handle(request.GetHandle())
            , Offset(request.GetOffset())
            , Length(request.GetLength())
        {}

        void Clear()
        {
            Error.Clear();
            CommitId = 0;
            NodeId = InvalidNodeId;
            Node.Clear();
        }
    };

    //
    // AddBlob
    //

    struct TAddBlob
    {
        const TRequestInfoPtr RequestInfo;
        const EAddBlobMode Mode;
        /*const*/ TVector<TMixedBlobMeta> SrcBlobs;
        /*const*/ TVector<TBlock> SrcBlocks;
        /*const*/ TVector<TMixedBlobMeta> MixedBlobs;
        /*const*/ TVector<TMergedBlobMeta> MergedBlobs;

        const TVector<TWriteRange> WriteRanges;

        ui64 CommitId = 0;
        TNodeSet Nodes;

        TAddBlob(
                TRequestInfoPtr requestInfo,
                EAddBlobMode mode,
                TVector<TMixedBlobMeta> srcBlobs,
                TVector<TBlock> srcBlocks,
                TVector<TMixedBlobMeta> mixedBlobs,
                TVector<TMergedBlobMeta> mergedBlobs,
                TVector<TWriteRange> writeRanges)
            : RequestInfo(std::move(requestInfo))
            , Mode(mode)
            , SrcBlobs(std::move(srcBlobs))
            , SrcBlocks(std::move(srcBlocks))
            , MixedBlobs(std::move(mixedBlobs))
            , MergedBlobs(std::move(mergedBlobs))
            , WriteRanges(std::move(writeRanges))
        {}

        void Clear()
        {
            CommitId = 0;
            Nodes.clear();
        }
    };

    //
    // FlushBytes
    //

    struct TFlushBytes
    {
        const TRequestInfoPtr RequestInfo;
        const ui64 ReadCommitId;
        const ui64 ChunkId;
        const TVector<TBytes> Bytes;

        ui64 CollectCommitId = 0;
        NProto::TProfileLogRequestInfo ProfileLogRequest;

        TFlushBytes(
                TRequestInfoPtr requestInfo,
                ui64 readCommitId,
                ui64 chunkId,
                TVector<TBytes> bytes)
            : RequestInfo(std::move(requestInfo))
            , ReadCommitId(readCommitId)
            , ChunkId(chunkId)
            , Bytes(std::move(bytes))
        {
            ProfileLogRequest.SetRequestType(
                static_cast<ui32>(EFileStoreSystemRequest::FlushBytes));
        }

        void Clear()
        {
            CollectCommitId = 0;
        }
    };

    //
    // TrimBytes
    //

    struct TTrimBytes
    {
        const TRequestInfoPtr RequestInfo;
        const ui64 ChunkId;

        NProto::TProfileLogRequestInfo ProfileLogRequest;

        TTrimBytes(TRequestInfoPtr requestInfo, ui64 chunkId)
            : RequestInfo(std::move(requestInfo))
            , ChunkId(chunkId)
        {
            ProfileLogRequest.SetRequestType(
                static_cast<ui32>(EFileStoreSystemRequest::TrimBytes));
        }

        void Clear()
        {
        }
    };

    //
    // Compaction
    //

    struct TCompaction
    {
        const TRequestInfoPtr RequestInfo;
        const ui32 RangeId;

        ui64 CommitId = 0;
        TInstant StartTs;

        TCompaction(TRequestInfoPtr requestInfo, ui32 rangeId)
            : RequestInfo(std::move(requestInfo))
            , RangeId(rangeId)
        {}

        void Clear()
        {
            CommitId = 0;
        }
    };

    //
    // Cleanup
    //

    struct TCleanup
    {
        const TRequestInfoPtr RequestInfo;
        const ui32 RangeId;
        const ui64 CollectBarrier;

        ui64 CommitId = 0;
        NProto::TProfileLogRequestInfo ProfileLogRequest;

        TCleanup(TRequestInfoPtr requestInfo, ui32 rangeId, ui64 collectBarrier)
            : RequestInfo(std::move(requestInfo))
            , RangeId(rangeId)
            , CollectBarrier(collectBarrier)
        {
            ProfileLogRequest.SetRequestType(
                static_cast<ui32>(EFileStoreSystemRequest::Cleanup));
        }

        void Clear()
        {
            CommitId = 0;
        }
    };

    //
    // DeleteGarbage
    //

    struct TDeleteGarbage
    {
        const TRequestInfoPtr RequestInfo;
        const ui64 CollectCommitId;
        TVector<TPartialBlobId> NewBlobs;
        TVector<TPartialBlobId> GarbageBlobs;
        TVector<TPartialBlobId> RemainingNewBlobs;
        TVector<TPartialBlobId> RemainingGarbageBlobs;

        TDeleteGarbage(
                TRequestInfoPtr requestInfo,
                ui64 collectCommitId,
                TVector<TPartialBlobId> newBlobs,
                TVector<TPartialBlobId> garbageBlobs)
            : RequestInfo(std::move(requestInfo))
            , CollectCommitId(collectCommitId)
            , NewBlobs(std::move(newBlobs))
            , GarbageBlobs(std::move(garbageBlobs))
        {}

        void Clear()
        {
            // nothing to do
        }
    };
};

}   // namespace NCloud::NFileStore::NStorage
