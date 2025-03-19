#include "mixed_blocks.h"

#include "alloc.h"
#include "deletion_markers.h"

#include <util/generic/hash_set.h>
#include <util/generic/intrlist.h>

#include <array>

namespace NCloud::NFileStore::NStorage {

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr size_t NumLevels = 3;

////////////////////////////////////////////////////////////////////////////////

struct TBlobMeta : TIntrusiveListItem<TBlobMeta>
{
    const TPartialBlobId BlobId;
    const TBlockList BlockList;
    const TMixedBlobStats Stats;
    const size_t Level;

    TBlobMeta(
            const TPartialBlobId& blobId,
            TBlockList blockList,
            const TMixedBlobStats& stats,
            size_t level)
        : BlobId(blobId)
        , BlockList(std::move(blockList))
        , Stats(stats)
        , Level(level)
    {}
};

using TBlobMetaList = TIntrusiveList<TBlobMeta>;

////////////////////////////////////////////////////////////////////////////////

struct TBlobMetaOps
{
    struct TEqual
    {
        template <typename T1, typename T2>
        bool operator ()(const T1& l, const T2& r) const
        {
            return GetBlobId(l) == GetBlobId(r);
        }
    };

    struct THash
    {
        template <typename T>
        size_t operator ()(const T& value) const
        {
            return GetBlobId(value).GetHash();
        }
    };

    static const TPartialBlobId& GetBlobId(const TBlobMeta& blob)
    {
        return blob.BlobId;
    }

    static const TPartialBlobId& GetBlobId(const TPartialBlobId& blobId)
    {
        return blobId;
    }
};

using TBlobMetaMap = THashSet<
    TBlobMeta,
    TBlobMetaOps::THash,
    TBlobMetaOps::TEqual,
    TStlAllocator
>;

////////////////////////////////////////////////////////////////////////////////

struct TLevel
{
    TBlobMetaList Blobs;
    size_t BlobsCount = 0;
};

////////////////////////////////////////////////////////////////////////////////

struct TRange
{
    TBlobMetaMap Blobs{GetAllocatorByTag(EAllocatorTag::BlobMetaMap)};
    TDeletionMarkers DeletionMarkers;
    std::array<TLevel, NumLevels> Levels;
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

struct TMixedBlocks::TImpl
{
    THashMap<ui32, TRange> Ranges;
};

////////////////////////////////////////////////////////////////////////////////

TMixedBlocks::TMixedBlocks(IAllocator* allocator)
    : Impl(new TImpl())
{
    Y_UNUSED(allocator);
}

TMixedBlocks::~TMixedBlocks()
{}

bool TMixedBlocks::IsLoaded(ui32 rangeId) const
{
    auto* range = Impl->Ranges.FindPtr(rangeId);
    return range;
}

void TMixedBlocks::SetLoaded(ui32 rangeId)
{
    auto& range = Impl->Ranges[rangeId];
    Y_UNUSED(range);
}

bool TMixedBlocks::AddBlocks(
    ui32 rangeId,
    const TPartialBlobId& blobId,
    TBlockList blockList,
    const TMixedBlobStats& stats)
{
    auto* range = Impl->Ranges.FindPtr(rangeId);
    Y_VERIFY(range);

    // TODO: pick level
    auto [it, inserted] = range->Blobs.emplace(
        blobId,
        std::move(blockList),
        stats,
        0);

    if (!inserted) {
        return false;
    }

    auto& blob = const_cast<TBlobMeta&>(*it);

    auto& level = range->Levels[blob.Level];
    level.Blobs.PushBack(&blob);
    ++level.BlobsCount;

    return true;
}

bool TMixedBlocks::RemoveBlocks(
    ui32 rangeId,
    const TPartialBlobId& blobId,
    TMixedBlobStats* stats)
{
    auto* range = Impl->Ranges.FindPtr(rangeId);
    Y_VERIFY(range);

    auto it = range->Blobs.find(blobId);
    if (it == range->Blobs.end()) {
        return false;
    }

    auto& blob = const_cast<TBlobMeta&>(*it);

    auto& level = range->Levels[blob.Level];
    blob.Unlink();
    --level.BlobsCount;

    if (stats) {
        *stats = blob.Stats;
    }

    range->Blobs.erase(it);
    return true;
}

void TMixedBlocks::FindBlocks(
    IMixedBlockVisitor& visitor,
    ui32 rangeId,
    ui64 nodeId,
    ui64 commitId,
    ui32 blockIndex,
    ui32 blocksCount) const
{
    const auto* range = Impl->Ranges.FindPtr(rangeId);
    Y_VERIFY(range);

    // TODO: limit range scan
    for (const auto& blob: range->Blobs) {
        auto iter = blob.BlockList.FindBlocks(
            nodeId,
            commitId,
            blockIndex,
            blocksCount);

        while (iter->Next()) {
            auto& block = iter->Block;

            Y_VERIFY(block.NodeId == nodeId);
            Y_VERIFY(block.MinCommitId <= commitId);

            range->DeletionMarkers.Apply(block);

            if (commitId < block.MaxCommitId) {
                visitor.Accept(block, blob.BlobId, iter->BlobOffset);
            }
        }
    }
}

void TMixedBlocks::AddDeletionMarker(
    ui32 rangeId,
    TDeletionMarker deletionMarker)
{
    auto* range = Impl->Ranges.FindPtr(rangeId);
    Y_VERIFY(range);

    range->DeletionMarkers.Add(deletionMarker);
}

TVector<TDeletionMarker> TMixedBlocks::ExtractDeletionMarkers(ui32 rangeId)
{
    auto* range = Impl->Ranges.FindPtr(rangeId);
    Y_VERIFY(range);

    return range->DeletionMarkers.Extract();
}

void TMixedBlocks::ApplyDeletionMarkers(
    const IBlockLocation2RangeIndex& hasher,
    TVector<TBlock>& blocks) const
{
    const auto rangeId = GetMixedRangeIndex(hasher, blocks);

    const auto* range = Impl->Ranges.FindPtr(rangeId);
    Y_VERIFY(range);

    range->DeletionMarkers.Apply(MakeArrayRef(blocks));
}

TVector<TMixedBlobMeta> TMixedBlocks::ApplyDeletionMarkers(ui32 rangeId) const
{
    const auto* range = Impl->Ranges.FindPtr(rangeId);
    Y_VERIFY(range);

    TVector<TMixedBlobMeta> result;

    for (const auto& blob: range->Blobs) {
        auto blocks = blob.BlockList.DecodeBlocks();

        if (range->DeletionMarkers.Apply(MakeArrayRef(blocks)) > 0) {
            result.emplace_back(blob.BlobId, std::move(blocks));
        }
    }

    return result;
}

TVector<TMixedBlobMeta> TMixedBlocks::GetBlobsForCompaction(ui32 rangeId) const
{
    const auto* range = Impl->Ranges.FindPtr(rangeId);
    Y_VERIFY(range);

    TVector<TMixedBlobMeta> result;

    // TODO: pick level
    for (const auto& blob: range->Blobs) {
        auto blocks = blob.BlockList.DecodeBlocks();

        range->DeletionMarkers.Apply(MakeArrayRef(blocks));
        result.emplace_back(blob.BlobId, std::move(blocks));
    }

    return result;
}

TMixedBlobMeta TMixedBlocks::FindBlob(ui32 rangeId, TPartialBlobId blobId) const
{
    const auto* range = Impl->Ranges.FindPtr(rangeId);
    Y_VERIFY(range);

    TVector<TMixedBlobMeta> result;

    auto it = range->Blobs.find(blobId);
    Y_VERIFY(it != range->Blobs.end());

    auto blocks = it->BlockList.DecodeBlocks();
    range->DeletionMarkers.Apply(MakeArrayRef(blocks));

    return {it->BlobId, std::move(blocks)};
}

}   // namespace NCloud::NFileStore::NStorage
