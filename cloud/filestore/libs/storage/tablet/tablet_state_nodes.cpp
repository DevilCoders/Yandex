#include "tablet_state_impl.h"

#include "helpers.h"

namespace NCloud::NFileStore::NStorage {

namespace
{

ui64 SizeSum(const TString& v1, const TString& v2)
{
    return v1.size() + v2.size();
}

ui64 SizeDiff(const TString& v1, const TString& v2)
{
    return v1.size() - v2.size();
}

} // namespace

////////////////////////////////////////////////////////////////////////////////
// Nodes

bool TIndexTabletState::HasSpaceLeft(const NProto::TNode& attrs, ui64 newSize) const
{
    i64 delta = GetBlocksDifference(attrs.GetSize(), newSize, GetBlockSize());
    if (delta > 0 && GetUsedBlocksCount() + delta > GetBlocksCount()) {
        return false;
    }

    return true;
}

bool TIndexTabletState::HasBlocksLeft(ui32 blocks) const
{
    if (blocks && GetUsedBlocksCount() + blocks > GetBlocksCount()) {
        return false;
    }

    return true;
}

void TIndexTabletState::UpdateUsedBlocksCount(
    TIndexTabletDatabase& db,
    ui64 currentSize,
    ui64 prevSize)
{
    i64 delta = GetBlocksDifference(prevSize, currentSize, GetBlockSize());
    if (delta > 0) {
        IncrementUsedBlocksCount(db, delta);
    } else if (delta < 0) {
        DecrementUsedBlocksCount(db, -delta);
    }
}

ui64 TIndexTabletState::CreateNode(
    TIndexTabletDatabase& db,
    ui64 commitId,
    const NProto::TNode& attrs)
{
    ui64 nodeId = IncrementLastNodeId(db);

    db.WriteNode(nodeId, commitId, attrs);
    IncrementUsedNodesCount(db);

    // so far symlink node has size
    UpdateUsedBlocksCount(db, attrs.GetSize(), 0);

    return nodeId;
}

void TIndexTabletState::UpdateNode(
    TIndexTabletDatabase& db,
    ui64 nodeId,
    ui64 minCommitId,
    ui64 maxCommitId,
    const NProto::TNode& attrs,
    const NProto::TNode& prevAttrs)
{
    UpdateUsedBlocksCount(db, attrs.GetSize(), prevAttrs.GetSize());

    ui64 checkpointId = Impl->Checkpoints.FindCheckpoint(nodeId, minCommitId);
    if (checkpointId == InvalidCommitId) {
        // simple in-place update
        db.WriteNode(nodeId, minCommitId, attrs);
    } else {
        // copy-on-write update
        db.WriteNode(nodeId, maxCommitId, attrs);
        db.WriteNodeVer(nodeId, checkpointId, maxCommitId, prevAttrs);

        AddCheckpointNode(db, checkpointId, nodeId);
    }
}

void TIndexTabletState::RemoveNode(
    TIndexTabletDatabase& db,
    const TIndexTabletDatabase::TNode& node,
    ui64 minCommitId,
    ui64 maxCommitId)
{
    db.DeleteNode(node.NodeId);
    DecrementUsedNodesCount(db);

    UpdateUsedBlocksCount(db, 0, node.Attrs.GetSize());

    ui64 checkpointId = Impl->Checkpoints.FindCheckpoint(node.NodeId, minCommitId);
    if (checkpointId != InvalidCommitId) {
        // keep history version
        db.WriteNodeVer(node.NodeId, checkpointId, maxCommitId, node.Attrs);
        AddCheckpointNode(db, checkpointId, node.NodeId);
    }

    Truncate(
        db,
        node.NodeId,
        maxCommitId,
        node.Attrs.GetSize(),
        0);
}

void TIndexTabletState::UnlinkNode(
    TIndexTabletDatabase& db,
    ui64 parentNodeId,
    const TString& name,
    const TIndexTabletDatabase::TNode& node,
    ui64 minCommitId,
    ui64 maxCommitId)
{
    if (node.Attrs.GetLinks() > 1 || HasOpenHandles(node.NodeId)) {
        auto attrs = CopyAttrs(node.Attrs, E_CM_CMTIME | E_CM_UNREF);
        UpdateNode(
            db,
            node.NodeId,
            minCommitId,
            maxCommitId,
            attrs,
            node.Attrs);
    } else {
        RemoveNode(
            db,
            node,
            minCommitId,
            maxCommitId);
    }

    RemoveNodeRef(
        db,
        parentNodeId,
        minCommitId,
        maxCommitId,
        name,
        node.NodeId);
}

bool TIndexTabletState::ReadNode(
    TIndexTabletDatabase& db,
    ui64 nodeId,
    ui64 commitId,
    TMaybe<TIndexTabletDatabase::TNode>& node)
{
    bool ready = db.ReadNode(nodeId, commitId, node);

    if (ready && node) {
        // fast path
        return true;
    }

    ui64 checkpointId = Impl->Checkpoints.FindCheckpoint(nodeId, commitId);
    if (checkpointId != InvalidCommitId) {
        // there could be history versions
        if (!db.ReadNodeVer(nodeId, commitId, node)) {
            ready = false;
        }
    }

    return ready;
}

void TIndexTabletState::RewriteNode(
    TIndexTabletDatabase& db,
    ui64 nodeId,
    ui64 minCommitId,
    ui64 maxCommitId,
    const NProto::TNode& attrs)
{
    ui64 checkpointId = Impl->Checkpoints.FindCheckpoint(nodeId, minCommitId);
    if (checkpointId != InvalidCommitId) {
        // keep history version
        db.WriteNodeVer(nodeId, checkpointId, maxCommitId, attrs);

        AddCheckpointNode(db, checkpointId, nodeId);
    } else {
        // no need this version any more
        db.DeleteNodeVer(nodeId, minCommitId);
    }
}

////////////////////////////////////////////////////////////////////////////////
// NodeAttrs

ui64 TIndexTabletState::CreateNodeAttr(
    TIndexTabletDatabase& db,
    ui64 nodeId,
    ui64 commitId,
    const TString& name,
    const TString& value)
{
    ui64 version = IncrementLastXAttr(db);
    db.WriteNodeAttr(nodeId, commitId, name, value, version);

    IncrementAttrsUsedBytesCount(db, SizeSum(name, value));

    return version;
}

ui64 TIndexTabletState::UpdateNodeAttr(
    TIndexTabletDatabase& db,
    ui64 nodeId,
    ui64 minCommitId,
    ui64 maxCommitId,
    const TIndexTabletDatabase::TNodeAttr& attr,
    const TString& newValue)
{
    ui64 version = IncrementLastXAttr(db);
    ui64 checkpointId = Impl->Checkpoints.FindCheckpoint(nodeId, minCommitId);
    if (checkpointId == InvalidCommitId) {
        // simple in-place update
        db.WriteNodeAttr(nodeId, minCommitId, attr.Name, newValue, version);
    } else {
        // copy-on-write update
        db.WriteNodeAttr(nodeId, maxCommitId, attr.Name, newValue, version);
        db.WriteNodeAttrVer(
            nodeId,
            checkpointId,
            maxCommitId,
            attr.Name,
            attr.Value,
            attr.Version);

        AddCheckpointNode(db, checkpointId, nodeId);
    }

    if (newValue.size() > attr.Value.size()) {
        IncrementAttrsUsedBytesCount(db, SizeDiff(newValue, attr.Value));
    } else {
        DecrementAttrsUsedBytesCount(db, SizeDiff(attr.Value, newValue));
    }

    return version;
}

void TIndexTabletState::RemoveNodeAttr(
    TIndexTabletDatabase& db,
    ui64 nodeId,
    ui64 minCommitId,
    ui64 maxCommitId,
    const TIndexTabletDatabase::TNodeAttr& attr)
{
    db.DeleteNodeAttr(nodeId, attr.Name);

    ui64 checkpointId = Impl->Checkpoints.FindCheckpoint(nodeId, minCommitId);
    if (checkpointId != InvalidCommitId) {
        // keep history version
        db.WriteNodeAttrVer(
            nodeId,
            checkpointId,
            maxCommitId,
            attr.Name,
            attr.Value,
            attr.Version);

        AddCheckpointNode(db, checkpointId, nodeId);
    }

    DecrementAttrsUsedBytesCount(db, SizeSum(attr.Name, attr.Value));
}

bool TIndexTabletState::ReadNodeAttr(
    TIndexTabletDatabase& db,
    ui64 nodeId,
    ui64 commitId,
    const TString& name,
    TMaybe<TIndexTabletDatabase::TNodeAttr>& attr)
{
    bool ready = db.ReadNodeAttr(nodeId, commitId, name, attr);

    if (ready && attr) {
        // fast path
        return true;
    }

    ui64 checkpointId = Impl->Checkpoints.FindCheckpoint(nodeId, commitId);
    if (checkpointId != InvalidCommitId) {
        // there could be history versions
        if (!db.ReadNodeAttrVer(nodeId, commitId, name, attr)) {
            ready = false;
        }
    }

    return ready;
}

bool TIndexTabletState::ReadNodeAttrs(
    TIndexTabletDatabase& db,
    ui64 nodeId,
    ui64 commitId,
    TVector<TIndexTabletDatabase::TNodeAttr>& attrs)
{
    bool ready = db.ReadNodeAttrs(nodeId, commitId, attrs);

    ui64 checkpointId = Impl->Checkpoints.FindCheckpoint(nodeId, commitId);
    if (checkpointId != InvalidCommitId) {
        // there could be history versions
        if (!db.ReadNodeAttrVers(nodeId, commitId, attrs)) {
            ready = false;
        }
    }

    return ready;
}

void TIndexTabletState::RewriteNodeAttr(
    TIndexTabletDatabase& db,
    ui64 nodeId,
    ui64 minCommitId,
    ui64 maxCommitId,
    const TIndexTabletDatabase::TNodeAttr& attr)
{
    ui64 checkpointId = Impl->Checkpoints.FindCheckpoint(nodeId, minCommitId);
    if (checkpointId != InvalidCommitId) {
        // keep history version
        db.WriteNodeAttrVer(
            nodeId,
            checkpointId,
            maxCommitId,
            attr.Name,
            attr.Value,
            attr.Version);

        AddCheckpointNode(db, checkpointId, nodeId);
    } else {
        // no need this version any more
        db.DeleteNodeAttrVer(nodeId, minCommitId, attr.Name);
    }
}

////////////////////////////////////////////////////////////////////////////////
// NodeRefs

void TIndexTabletState::CreateNodeRef(
    TIndexTabletDatabase& db,
    ui64 nodeId,
    ui64 commitId,
    const TString& childName,
    ui64 childNodeId)
{
    db.WriteNodeRef(nodeId, commitId, childName, childNodeId);
}

void TIndexTabletState::RemoveNodeRef(
    TIndexTabletDatabase& db,
    ui64 nodeId,
    ui64 minCommitId,
    ui64 maxCommitId,
    const TString& childName,
    ui64 prevChildNodeId)
{
    db.DeleteNodeRef(nodeId, childName);

    ui64 checkpointId = Impl->Checkpoints.FindCheckpoint(nodeId, minCommitId);
    if (checkpointId != InvalidCommitId) {
        // keep history version
        db.WriteNodeRefVer(
            nodeId,
            checkpointId,
            maxCommitId,
            childName,
            prevChildNodeId);

        AddCheckpointNode(db, checkpointId, nodeId);
    }
}

bool TIndexTabletState::ReadNodeRef(
    TIndexTabletDatabase& db,
    ui64 nodeId,
    ui64 commitId,
    const TString& name,
    TMaybe<TIndexTabletDatabase::TNodeRef>& ref)
{
    bool ready = db.ReadNodeRef(nodeId, commitId, name, ref);

    if (ready && ref) {
        // fast path
        return true;
    }

    ui64 checkpointId = Impl->Checkpoints.FindCheckpoint(nodeId, commitId);
    if (checkpointId != InvalidCommitId) {
        // there could be history versions
        if (!db.ReadNodeRefVer(nodeId, commitId, name, ref)) {
            ready = false;
        }
    }

    return ready;
}

bool TIndexTabletState::ReadNodeRefs(
    TIndexTabletDatabase& db,
    ui64 nodeId,
    ui64 commitId,
    const TString& name,
    TVector<TIndexTabletDatabase::TNodeRef>& refs,
    ui32 maxBytes,
    TString* next)
{
    bool ready = db.ReadNodeRefs(nodeId, commitId, name, refs, maxBytes, next);

    ui64 checkpointId = Impl->Checkpoints.FindCheckpoint(nodeId, commitId);
    if (checkpointId != InvalidCommitId) {
        // there could be history versions
        if (!db.ReadNodeRefVers(nodeId, commitId, refs)) {
            ready = false;
        }
    }

    return ready;
}

void TIndexTabletState::RewriteNodeRef(
    TIndexTabletDatabase& db,
    ui64 nodeId,
    ui64 minCommitId,
    ui64 maxCommitId,
    const TString& childName,
    ui64 childNodeId)
{
    ui64 checkpointId = Impl->Checkpoints.FindCheckpoint(nodeId, minCommitId);
    if (checkpointId != InvalidCommitId) {
        // keep history version
        db.WriteNodeRefVer(
            nodeId,
            checkpointId,
            maxCommitId,
            childName,
            childNodeId);

        AddCheckpointNode(db, checkpointId, nodeId);
    } else {
        // no need this version any more
        db.DeleteNodeRefVer(nodeId, minCommitId, childName);
    }
}

}   // namespace NCloud::NFileStore::NStorage
