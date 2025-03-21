#pragma once

#include "public.h"

#include "block.h"

#include <util/generic/array_ref.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TDeletionMarker
{
    ui64 NodeId;
    ui64 CommitId;
    ui32 BlockIndex;
    ui32 BlockCount;

    TDeletionMarker(
            ui64 nodeId,
            ui64 commitId,
            ui32 blockIndex,
            ui32 blockCount)
        : NodeId(nodeId)
        , CommitId(commitId)
        , BlockIndex(blockIndex)
        , BlockCount(blockCount)
    {}

    bool operator==(const TDeletionMarker& other) const
    {
        return NodeId == other.NodeId
            && CommitId == other.CommitId
            && BlockIndex == other.BlockIndex
            && BlockCount == other.BlockCount;
    }
};

////////////////////////////////////////////////////////////////////////////////

class TDeletionMarkers
{
private:
    class TImpl;
    std::unique_ptr<TImpl> Impl;

public:
    TDeletionMarkers();
    ~TDeletionMarkers();

    void Add(TDeletionMarker deletionMarker);
    ui32 Apply(TBlock& block) const;
    ui32 Apply(TArrayRef<TBlock> blocks) const;
    TVector<TDeletionMarker> Extract();
};

}   // namespace NCloud::NFileStore::NStorage
