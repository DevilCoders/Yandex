#include "fresh_bytes.h"

#include <cloud/storage/core/libs/tablet/model/commit.h>

namespace NCloud::NFileStore::NStorage {

namespace {

////////////////////////////////////////////////////////////////////////////////

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TFreshBytes::TFreshBytes(IAllocator* allocator)
    : Allocator(allocator)
{
    Y_UNUSED(Allocator);    // TODO

    Chunks.emplace_back();
    Chunks.back().Id = ++LastChunkId;
}

TFreshBytes::~TFreshBytes()
{
}

void TFreshBytes::DeleteBytes(
    TChunk& c,
    ui64 nodeId,
    ui64 offset,
    ui64 len,
    ui64 commitId)
{
    auto end = offset + len;

    auto lo = c.Refs.upper_bound({nodeId, offset});
    auto hi = c.Refs.upper_bound({nodeId, end});

    if (lo != c.Refs.end() && lo->first.NodeId == nodeId) {
        if (lo->second.Offset == offset && lo->first.End == end) {
            // special case
            c.Refs.erase(lo);
            return;
        }

        if (lo->second.Offset < offset) {
            // cutting lo from the right side
            Y_VERIFY_DEBUG(lo->second.CommitId < commitId);
            auto& loRef = c.Refs[{nodeId, offset}];
            loRef = lo->second;
            loRef.Buf = loRef.Buf.substr(0, offset - loRef.Offset);

            if (lo->first.End <= end) {
                // blockRange is not contained strictly inside lo
                Y_VERIFY_DEBUG(lo != hi);
                c.Refs.erase(lo++);
            }
        }

        if (hi != c.Refs.end()
                && hi->first.NodeId == nodeId
                && hi->second.Offset < end)
        {
            // cutting hi from the left side
            // hi might be equal to lo - it's not a problem
            Y_VERIFY_DEBUG(hi->second.CommitId < commitId);
            const auto shift = end - hi->second.Offset;
            hi->second.Buf = hi->second.Buf.substr(
                shift,
                hi->second.Buf.Size() - shift
            );
            hi->second.Offset = end;
        }

        // deleting ranges between lo and hi
        while (lo != c.Refs.end() && lo->first.NodeId == nodeId && lo != hi) {
            c.Refs.erase(lo++);
        }
    }
}

void TFreshBytes::AddBytes(
    ui64 nodeId,
    ui64 offset,
    TStringBuf data,
    ui64 commitId)
{
    auto& c = Chunks.back();
    if (c.FirstCommitId == InvalidCommitId) {
        c.FirstCommitId = commitId;
    } else {
        Y_VERIFY(commitId >= c.FirstCommitId);
    }
    c.TotalBytes += data.Size();
    c.Data.push_back({
        {nodeId, offset, data.Size(), commitId, InvalidCommitId},
        TString(data)
    });

    TKey key{nodeId, offset + data.Size()};
    TRef ref{TStringBuf(c.Data.back().Data), offset, commitId};

    DeleteBytes(c, nodeId, offset, data.Size(), commitId);

    c.Refs[key] = ref;
}

void TFreshBytes::AddDeletionMarker(
    ui64 nodeId,
    ui64 offset,
    ui64 len,
    ui64 commitId)
{
    auto& c = Chunks.back();
    if (c.FirstCommitId != InvalidCommitId) {
        Y_VERIFY(commitId >= c.FirstCommitId);
    }

    DeleteBytes(c, nodeId, offset, len, commitId);
}

void TFreshBytes::Barrier(ui64 commitId)
{
    if (Chunks.back().TotalBytes) {
        Y_VERIFY(Chunks.back().Data.back().Descriptor.MinCommitId <= commitId);
        Chunks.back().ClosingCommitId = commitId;
        Chunks.emplace_back();
        Chunks.back().Id = ++LastChunkId;
    }
}

void TFreshBytes::OnCheckpoint(ui64 commitId)
{
    Barrier(commitId);
}

TFlushBytesCleanupInfo TFreshBytes::StartCleanup(
    ui64 commitId,
    TVector<TBytes>* entries)
{
    if (Chunks.size() == 1) {
        Barrier(commitId);
    }

    for (auto& e: Chunks.front().Data) {
        entries->push_back(e.Descriptor);
    }

    return {Chunks.front().Id, Chunks.front().ClosingCommitId};
}

void TFreshBytes::VisitTop(const TChunkVisitor& visitor)
{
    for (const auto& e: Chunks.front().Data) {
        visitor(e.Descriptor);
    }
}

void TFreshBytes::FinishCleanup(ui64 chunkId)
{
    Y_VERIFY(Chunks.size() > 1);
    Y_VERIFY(Chunks.front().Id == chunkId);

    Chunks.pop_front();
}

void TFreshBytes::FindBytes(
    IFreshBytesVisitor& visitor,
    ui64 nodeId,
    TByteRange byteRange,
    ui64 commitId) const
{
    for (const auto& c: Chunks) {
        if (c.FirstCommitId > commitId) {
            break;
        }

        FindBytes(c, visitor, nodeId, byteRange, commitId);
    }
}

void TFreshBytes::FindBytes(
    const TChunk& chunk,
    IFreshBytesVisitor& visitor,
    ui64 nodeId,
    TByteRange byteRange,
    ui64 commitId) const
{
    auto it = chunk.Refs.upper_bound({nodeId, byteRange.Offset});
    while (it != chunk.Refs.end()
            && it->first.NodeId == nodeId
            && it->second.Offset < byteRange.End())
    {
        TByteRange itRange(
            it->second.Offset,
            it->first.End - it->second.Offset,
            byteRange.BlockSize
        );

        const auto intersection = itRange.Intersect(byteRange);
        if (it->second.CommitId <= commitId && intersection.Length) {
            TStringBuf data = it->second.Buf.substr(
                intersection.Offset - itRange.Offset,
                intersection.Length);

            visitor.Accept({
                nodeId,
                intersection.Offset,
                intersection.Length,
                it->second.CommitId,
                InvalidCommitId
            }, data);
        }

        ++it;
    }
}

bool TFreshBytes::Intersects(ui64 nodeId, TByteRange byteRange) const
{
    struct TVisitor: IFreshBytesVisitor
    {
        bool FoundBytes = false;

        void Accept(const TBytes& bytes, TStringBuf data) override
        {
            Y_UNUSED(bytes);
            Y_UNUSED(data);

            FoundBytes = true;
        }
    } visitor;

    FindBytes(visitor, nodeId, byteRange, InvalidCommitId);

    return visitor.FoundBytes;
}

}   // namespace NCloud::NFileStore::NStorage
