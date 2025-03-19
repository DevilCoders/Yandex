#include "compaction_map.h"

#include "alloc.h"

#include <library/cpp/containers/intrusive_rb_tree/rb_tree.h>

#include <util/generic/algorithm.h>
#include <util/generic/intrlist.h>
#include <util/system/align.h>

#include <array>

namespace NCloud::NFileStore::NStorage {

namespace {

////////////////////////////////////////////////////////////////////////////////

static constexpr ui32 GroupSize = 256;

ui32 GetGroupIndex(ui32 rangeId)
{
    return AlignDown(rangeId, GroupSize);
}

ui32 GetCompactionScore(const TCompactionStats& stats)
{
    // TODO
    return stats.BlobsCount;
}

ui32 GetCleanupScore(const TCompactionStats& stats)
{
    // TODO
    return stats.DeletionsCount;
}

////////////////////////////////////////////////////////////////////////////////

template <typename T>
ui32 GetGroupIndex(const T& node);

struct TCompareByGroupIndex
{
    template <typename T1, typename T2>
    static bool Compare(const T1& l, const T2& r)
    {
        return GetGroupIndex(l) < GetGroupIndex(r);
    }
};

struct TTreeItemByGroupIndex
    : TRbTreeItem<TTreeItemByGroupIndex, TCompareByGroupIndex>
{};

using TTreeByGroupIndex = TRbTree<
    TTreeItemByGroupIndex,
    TCompareByGroupIndex>;

////////////////////////////////////////////////////////////////////////////////

template <typename T>
ui32 GetCompactionScore(const T& node);

struct TCompareByCompactionScore
{
    template <typename T1, typename T2>
    static bool Compare(const T1& l, const T2& r)
    {
        return GetCompactionScore(l) > GetCompactionScore(r);
    }
};

struct TTreeItemByCompactionScore
    : TRbTreeItem<TTreeItemByCompactionScore, TCompareByCompactionScore>
{};

using TTreeByCompactionScore = TRbTree<
    TTreeItemByCompactionScore,
    TCompareByCompactionScore>;

////////////////////////////////////////////////////////////////////////////////

template <typename T>
ui32 GetCleanupScore(const T& node);

struct TCompareByCleanupScore
{
    template <typename T1, typename T2>
    static bool Compare(const T1& l, const T2& r)
    {
        return GetCleanupScore(l) > GetCleanupScore(r);
    }
};

struct TTreeItemByCleanupScore
    : TRbTreeItem<TTreeItemByCleanupScore, TCompareByCleanupScore>
{};

using TTreeByCleanupScore = TRbTree<
    TTreeItemByCleanupScore,
    TCompareByCleanupScore>;

////////////////////////////////////////////////////////////////////////////////

struct TGroup
    : TIntrusiveListItem<TGroup>
    , TTreeItemByGroupIndex
    , TTreeItemByCompactionScore
    , TTreeItemByCleanupScore
{
    const ui32 GroupIndex;

    TCompactionCounter TopCompactionScore;
    TCompactionCounter TopCleanupScore;

    std::array<TCompactionStats, GroupSize> Stats {};

    TGroup(ui32 groupIndex)
        : GroupIndex(groupIndex)
    {}

    const TCompactionStats& Get(ui32 rangeId) const
    {
        return Stats[rangeId - GroupIndex];
    }

    void Update(ui32 rangeId, ui32 blobsCount, ui32 deletionsCount)
    {
        auto& stats = Stats[rangeId - GroupIndex];
        stats.BlobsCount = blobsCount;
        stats.DeletionsCount = deletionsCount;

        ui32 compactionScore = GetCompactionScore(stats);
        if (TopCompactionScore.second < compactionScore) {
            TopCompactionScore = { rangeId, compactionScore };
        } else if (TopCompactionScore.first == rangeId) {
            size_t i = GetTop<TCompareByCompactionScore>();
            TopCompactionScore = { GroupIndex + i, GetCompactionScore(Stats[i]) };
        }

        ui32 cleanupScore = GetCleanupScore(stats);
        if (TopCleanupScore.second < cleanupScore) {
            TopCleanupScore = { rangeId, cleanupScore };
        } else if (TopCleanupScore.first == rangeId) {
            size_t i = GetTop<TCompareByCleanupScore>();
            TopCleanupScore = { GroupIndex + i, GetCleanupScore(Stats[i]) };
        }
    }

    template <typename TCompare>
    size_t GetTop() const
    {
        size_t top = 0;
        for (size_t i = 1; i < Stats.size(); ++i) {
            if (TCompare::Compare(Stats[i], Stats[top])) {
                top = i;
            }
        }
        return top;
    }
};

struct TDelete
{
    template <typename T>
    static inline void Destroy(T* t) noexcept {
        static_assert(sizeof(T) != 0, "Type must be complete");
        t->~T();

        auto* allocator = GetAllocatorByTag(EAllocatorTag::CompactionMap);
        allocator->Release({t, sizeof(T)});
    }
};

using TGroupList = TIntrusiveListWithAutoDelete<TGroup, TDelete>;

////////////////////////////////////////////////////////////////////////////////

template <typename T>
ui32 GetGroupIndex(const T& node)
{
    return static_cast<const TGroup&>(node).GroupIndex;
}

template <typename T>
ui32 GetCompactionScore(const T& node)
{
    return static_cast<const TGroup&>(node).TopCompactionScore.second;
}

template <typename T>
ui32 GetCleanupScore(const T& node)
{
    return static_cast<const TGroup&>(node).TopCleanupScore.second;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

struct TCompactionMap::TImpl
{
    TGroupList Groups;

    TTreeByGroupIndex GroupByGroupIndex;
    TTreeByCompactionScore GroupByCompactionScore;
    TTreeByCleanupScore GroupByCleanupScore;

    TGroup* FindGroup(ui32 groupIndex) const
    {
        return static_cast<TGroup*>(GroupByGroupIndex.Find(groupIndex));
    }

    const TGroup* GetTopCompactionScore() const
    {
        if (!GroupByCompactionScore.Empty()) {
            return static_cast<const TGroup*>(&*GroupByCompactionScore.Begin());
        }
        return nullptr;
    }

    const TGroup* GetTopCleanupScore() const
    {
        if (!GroupByCleanupScore.Empty()) {
            return static_cast<const TGroup*>(&*GroupByCleanupScore.Begin());
        }
        return nullptr;
    }

    using TGetScore = std::function<double(const TCompactionRangeInfo&)>;

    TVector<TCompactionRangeInfo> GetTopRanges(
        ui32 c,
        const TGetScore& getScore) const
    {
        TVector<TCompactionRangeInfo> ranges;

        for (const auto& g: Groups) {
            for (ui32 i = 0; i < GroupSize; ++i) {
                ranges.push_back({g.GroupIndex + i, g.Stats[i]});
            }
        }

        SortBy(ranges, getScore);

        ranges.crop(c);
        return ranges;
    }

    TVector<TCompactionRangeInfo> GetTopRangesByCompactionScore(ui32 c) const
    {
        // TODO efficient implementation for any c
        if (c == 1) {
            const auto* group = GetTopCompactionScore();
            if (group) {
                ui32 index = 0;
                TCompactionStats best = group->Stats[0];
                for (ui32 i = 1; i < GroupSize; ++i) {
                    auto stats = group->Stats[i];
                    if (stats.BlobsCount > best.BlobsCount) {
                        index = i;
                        best = stats;
                    }
                }

                return {{group->GroupIndex + index, best}};
            }

            return {};
        }

        return GetTopRanges(c, [] (const TCompactionRangeInfo& r) {
            return -static_cast<double>(r.Stats.BlobsCount);
        });
    }

    TVector<TCompactionRangeInfo> GetTopRangesByCleanupScore(ui32 c) const
    {
        // TODO efficient implementation for any c
        if (c == 1) {
            const auto* group = GetTopCleanupScore();
            if (group) {
                ui32 index = 0;
                TCompactionStats best = group->Stats[0];
                for (ui32 i = 1; i < GroupSize; ++i) {
                    auto stats = group->Stats[i];
                    if (stats.DeletionsCount > best.DeletionsCount) {
                        index = i;
                        best = stats;
                    }
                }

                return {{group->GroupIndex + index, best}};
            }

            return {};
        }

        return GetTopRanges(c, [] (const TCompactionRangeInfo& r) {
            return -static_cast<double>(r.Stats.DeletionsCount);
        });
    }
};

////////////////////////////////////////////////////////////////////////////////

TCompactionMap::TCompactionMap()
    : Impl(new TImpl())
{}

TCompactionMap::~TCompactionMap()
{}

void TCompactionMap::Update(ui32 rangeId, ui32 blobsCount, ui32 deletionsCount)
{
    ui32 groupIndex = GetGroupIndex(rangeId);

    auto* group = Impl->FindGroup(groupIndex);
    if (!group) {
        auto* allocator = GetAllocatorByTag(EAllocatorTag::CompactionMap);
        auto block = allocator->Allocate(sizeof(TGroup));
        group = new (block.Data) TGroup(groupIndex);
        Impl->Groups.PushBack(group);
        Impl->GroupByGroupIndex.Insert(group);
    }

    group->Update(rangeId, blobsCount, deletionsCount);

    Impl->GroupByCompactionScore.Insert(group);
    Impl->GroupByCleanupScore.Insert(group);
}

TCompactionStats TCompactionMap::Get(ui32 rangeId) const
{
    ui32 groupIndex = GetGroupIndex(rangeId);

    const auto* group = Impl->FindGroup(groupIndex);
    if (group) {
        return group->Get(rangeId);
    }

    return {};
}

TCompactionCounter TCompactionMap::GetTopCompactionScore() const
{
    const auto* group = Impl->GetTopCompactionScore();
    if (group) {
        return group->TopCompactionScore;
    }

    return {};
}

TCompactionCounter TCompactionMap::GetTopCleanupScore() const
{
    const auto* group = Impl->GetTopCleanupScore();
    if (group) {
        return group->TopCleanupScore;
    }

    return {};
}

TVector<TCompactionRangeInfo> TCompactionMap::GetTopRangesByCompactionScore(ui32 topSize) const
{
    return Impl->GetTopRangesByCompactionScore(topSize);
}

TVector<TCompactionRangeInfo> TCompactionMap::GetTopRangesByCleanupScore(ui32 topSize) const
{
    return Impl->GetTopRangesByCleanupScore(topSize);
}

}   // namespace NCloud::NFileStore::NStorage
