#pragma once

#include "public.h"

#include "block.h"
#include "range.h"

#include <util/generic/map.h>
#include <util/generic/deque.h>
#include <util/generic/strbuf.h>
#include <util/memory/alloc.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

using TChunkVisitor = std::function<void(const TBytes& bytes)>;

////////////////////////////////////////////////////////////////////////////////

class TFreshBytes
{
public:
    struct TKey
    {
        ui64 NodeId;
        ui64 End;

        bool operator<(const TKey& rhs) const
        {
            // (NodeId, End) ASC
            return NodeId < rhs.NodeId
                || NodeId == rhs.NodeId && End < rhs.End;
        }
    };

private:
    struct TElement
    {
        TBytes Descriptor;
        TString Data;
    };

    struct TRef
    {
        TStringBuf Buf;
        ui64 Offset = 0;
        ui64 CommitId = 0;
    };

    struct TChunk
    {
        TMap<TKey, TRef> Refs;
        TDeque<TElement> Data;
        ui64 FirstCommitId = InvalidCommitId;
        ui64 TotalBytes = 0;
        ui64 Id = 0;
        ui64 ClosingCommitId = 0;
    };

private:
    IAllocator* Allocator;
    TDeque<TChunk> Chunks;
    ui64 LastChunkId = 0;

public:
    TFreshBytes(IAllocator* allocator = TDefaultAllocator::Instance());
    ~TFreshBytes();

    size_t GetTotalBytes() const
    {
        ui64 bytes = 0;
        for (const auto& c: Chunks) {
            bytes += c.TotalBytes;
        }
        return bytes;
    }

    void AddBytes(ui64 nodeId, ui64 offset, TStringBuf data, ui64 commitId);
    void AddDeletionMarker(ui64 nodeId, ui64 offset, ui64 len, ui64 commitId);

    void OnCheckpoint(ui64 commitId);

    TFlushBytesCleanupInfo StartCleanup(ui64 commitId, TVector<TBytes>* entries);
    void VisitTop(const TChunkVisitor& visitor);
    void FinishCleanup(ui64 chunkId);

    void FindBytes(
        IFreshBytesVisitor& visitor,
        ui64 nodeId,
        TByteRange byteRange,
        ui64 commitId) const;

    bool Intersects(ui64 nodeId, TByteRange byteRange) const;

private:
    void DeleteBytes(TChunk& c, ui64 nodeId, ui64 offset, ui64 len, ui64 commitId);

    void Barrier(ui64 commitId);

    void FindBytes(
        const TChunk& chunk,
        IFreshBytesVisitor& visitor,
        ui64 nodeId,
        TByteRange byteRange,
        ui64 commitId) const;
};

}   // namespace NCloud::NFileStore::NStorage
