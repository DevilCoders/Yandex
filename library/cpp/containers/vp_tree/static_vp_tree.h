#pragma once

#include <util/system/yassert.h>

#include <util/random/fast.h>
#include <util/random/random.h>

#include <util/memory/pool.h>

#include <utility>
#include <util/generic/ptr.h>
#include <util/generic/intrlist.h>
#include <util/generic/vector.h>
#include <util/generic/algorithm.h>
#include <util/generic/noncopyable.h>

/**
 * A simple vantage point selecting strategy that takes a random item in the array as a new vantage point.
 */
template <class T, class TMetric, typename TNumberType = double>
class TVantagePointSelectStrategyRandom {
private:
    struct TItemWithDist {
        const T* Item;
        TNumberType Dist;

        TItemWithDist(const T* item, TNumberType dist)
            : Item(item)
            , Dist(dist)
        {
        }
    };

    TReallyFastRng32 Rng;
    TVector<TItemWithDist> Distances;

public:
    TVantagePointSelectStrategyRandom()
        : Rng(3703)
    {
    }

    void SelectVantagePoint(const TMetric& metric, const T** items, const size_t count, size_t& innerNodeStart, size_t& outerNodeStart, TNumberType& radius) {
        Y_ASSERT(count > 0);
        SelectVantagePoint(metric, items, count, innerNodeStart, outerNodeStart, radius, Rng.Uniform(count), 0.5f);
    }

    void SelectVantagePoint(const TMetric& metric, const T** items, const size_t count, size_t& innerNodeStart, size_t& outerNodeStart, TNumberType& radius,
                            size_t vpItemIndex, float nodeSplitFraction) {
        Y_ASSERT(vpItemIndex < count);
        Y_ASSERT(nodeSplitFraction > 0 && nodeSplitFraction < 1.0);

        DoSwap(items[0], items[vpItemIndex]);

        Distances.clear();
        Distances.reserve(count);
        Distances.push_back(TItemWithDist(items[0], 0));

        for (size_t i = 1; i < count; ++i) {
            const TNumberType d = metric.Distance(**items, *(items[i]));
            Distances.push_back(TItemWithDist(items[i], d));
        }

        TItemWithDist* beg = Distances.begin() + 1;
        TItemWithDist* end = Distances.end() - 1;

        // Move items with distance = 0 forward
        while (beg <= end) {
            if (end->Dist <= 0) {
                DoSwap(*beg, *end);
                ++beg;
            } else {
                --end;
            }
        }

        innerNodeStart = beg - Distances.begin();
        const size_t remainingCount = count - innerNodeStart;
        outerNodeStart = innerNodeStart + nodeSplitFraction * remainingCount;

        if (remainingCount > 0) {
            auto&& cmpByDistance = [](const TItemWithDist& item1, const TItemWithDist& item2) {
                return item1.Dist < item2.Dist;
            };
            std::nth_element(Distances.begin() + innerNodeStart, Distances.begin() + outerNodeStart, Distances.end(), cmpByDistance);
            radius = Distances[outerNodeStart].Dist;
            ++outerNodeStart;
            size_t endIndex = count - 1;

            while (outerNodeStart <= endIndex) {
                if (Distances[endIndex].Dist <= radius) {
                    DoSwap(Distances[outerNodeStart], Distances[endIndex]);
                    ++outerNodeStart;
                } else {
                    --endIndex;
                }
            }
        }

        for (size_t i = 1; i < count; ++i) {
            items[i] = Distances[i].Item;
        }
    }
};

/**
 * Vantage-point tree implementation. See http://en.wikipedia.org/wiki/Vantage-point_tree
 *
 * It allows search through elements in any metric space. Metric defines a distance between any two elements,
 * such as
 *     for any element A : distance(A,A) = 0
 *     for any elements A != B : distance(A,B) = distance(B,A) > 0
 *     for any elements A, B, C : distance(A,C) <= distance(A,B) + distance(B,C)   (triangle inequality)
 *
 * A TMetric class must implement method Distance() that returns a distance between two elements.
 * A TVantagePointSelectStrategy class must implement SelectVantagePoint() which
 *    - selects new vantage point item and moves it to the first index of items array
 *    - moves all (other) items with 0 distance to the head of the array (starting form index 1)
 *    - moves all items for inner node after 0-distance elements
 *    - moves all items for outer node to the tail of the items array
 *    - fills innerNodeStart, outerNodeStart, radius argumenst correctly
 */
template <class T, class TMetric, typename TNumberType = double,
          class TVantagePointSelectStrategy = TVantagePointSelectStrategyRandom<T, TMetric, TNumberType>>
class TStaticVpTree: private TNonCopyable {
public:
    struct TNode: public TIntrusiveListItem<TNode>, public TPoolable {
        TNumberType Radius = 0;
        size_t Size = 0;
        const T** Items = nullptr;
        const TNode* Inner = nullptr;
        const TNode* Outer = nullptr;
    };

private:
    struct TItemWithDist {
        const T* Item;
        TNumberType Dist;

        TItemWithDist(const T* item, TNumberType dist)
            : Item(item)
            , Dist(dist)
        {
        }
    };

    struct TNodeBuildParams {
        const TNode** ParentRef;
        const T** Items;
        size_t Count;

        TNodeBuildParams(const TNode** parentRef, const T** items, size_t count)
            : ParentRef(parentRef)
            , Items(items)
            , Count(count)
        {
        }
    };

    using TNodeBuildParamsArray = TVector<TNodeBuildParams>;

    enum {
        MaxLeafSize = 5
    };

    const TMetric Metric;
    TVector<const T*> Items;
    TMemoryPool Pool;
    TIntrusiveListWithAutoDelete<TNode, TDestructor> Nodes; // pointers to nodes must not change, so we use list here

    template <typename TInputIterator>
    void Init(TVantagePointSelectStrategy& ctx, TInputIterator begin, TInputIterator end) {
        size_t i = 0;
        for (TInputIterator it = begin; it < end; ++it, ++i) {
            Items[i] = &(*it);
        }

        TNodeBuildParamsArray nodesToBuild;
        const TNode* root = nullptr;
        nodesToBuild.push_back(TNodeBuildParams(&root, Items.data(), Items.size()));

        while (!nodesToBuild.empty()) {
            TNodeBuildParams nodeParams = nodesToBuild.back();
            nodesToBuild.pop_back();
            *nodeParams.ParentRef = BuildNode(nodeParams.Items, nodeParams.Count, ctx, nodesToBuild);
        }
    }

public:
    /**
     * Build VP-tree from given elements. The tree does not copy elements, just pointers to them.
     */
    template <typename TInputIterator>
    TStaticVpTree(TVantagePointSelectStrategy& ctx, TInputIterator begin, TInputIterator end, const TMetric& metric = TMetric())
        : Metric(metric)
        , Items(std::distance(begin, end))
        , Pool(1024)
    {
        Init(ctx, begin, end);
    }

    /**
     * Build VP-tree from given elements. The tree does not copy elements, just pointers to them.
     */
    template <typename TInputIterator>
    TStaticVpTree(TInputIterator begin, TInputIterator end, const TMetric& metric = TMetric())
        : Metric(metric)
        , Items(std::distance(begin, end))
        , Pool(1024)
    {
        TVantagePointSelectStrategy ctx;
        Init(ctx, begin, end);
    }

    template <typename Consumer>
    void FindNearbyItems(const T& item, TNumberType maxDistance, Consumer&& op) {
        if (!Nodes.Empty()) {
            FindNearbyItems(RootNode(), item, maxDistance, op);
        }
    }

    template <class TCollection>
    void FindNearbyItemsWithLimit(const T& item, TNumberType maxDistance, TCollection& found, size_t limit) {
        if (found.size() >= limit) {
            return;
        }

        auto&& fn = [&found, limit](const T* p) {
            found.push_back(p);
            return found.size() < limit;
        };

        FindNearbyItems(item, maxDistance, fn);
    }

    size_t CountNearbyItems(const T& item, TNumberType maxDistance) {
        size_t count = 0;
        auto&& fn = [&count](const T*) {
            ++count;
            return true;
        };
        FindNearbyItems(item, maxDistance, fn);
        return count;
    }

    const TNode* GetRootNode() const noexcept {
        return Nodes.Empty() ? nullptr : Nodes.Front();
    }

private:
    inline const TNode& RootNode() const noexcept {
        Y_ASSERT(!Nodes.Empty());

        return *Nodes.Front();
    }

    TNode* BuildNode(const T** items, const size_t count, TVantagePointSelectStrategy& ctx, TNodeBuildParamsArray& nodesToBuild) {
        Nodes.PushBack(new (Pool) TNode());
        TNode& node = *Nodes.Back();

        node.Items = items;

        if (count <= MaxLeafSize) {
            node.Size = count;
            return &node;
        }

        size_t outerNodeStart = 0;
        ctx.SelectVantagePoint(Metric, items, count, node.Size, outerNodeStart, node.Radius);
        Y_ASSERT(node.Size > 0);
        Y_ASSERT(node.Size <= outerNodeStart);
        Y_ASSERT(outerNodeStart <= count);

        // Do not build nodes recursively to avoid stack overflow
        nodesToBuild.push_back(TNodeBuildParams(&node.Inner, items + node.Size, outerNodeStart - node.Size));
        nodesToBuild.push_back(TNodeBuildParams(&node.Outer, items + outerNodeStart, count - outerNodeStart));

        return &node;
    }

    template <typename Consumer>
    bool FindNearbyItems(const TNode& node, const T& item, TNumberType maxDistance, Consumer&& op) {
        if (node.Inner == nullptr) {
            for (size_t i = 0; i < node.Size; ++i) {
                if (Metric.Distance(item, *node.Items[i]) <= maxDistance) {
                    if (!op(node.Items[i])) {
                        return false;
                    }
                }
            }
            return true;
        }

        TNumberType distance = Metric.Distance(item, **node.Items);

        if (distance <= maxDistance) {
            for (size_t i = 0; i < node.Size; ++i) {
                if (!op(node.Items[i])) {
                    return false;
                }
            }
        }

        if (node.Radius >= distance + maxDistance) {
            return FindNearbyItems(*node.Inner, item, maxDistance, op);
        }

        if (distance > node.Radius + maxDistance) {
            return FindNearbyItems(*node.Outer, item, maxDistance, op);
        }

        if (!FindNearbyItems(*node.Inner, item, maxDistance, op)) {
            return false;
        }

        return FindNearbyItems(*node.Outer, item, maxDistance, op);
    }
};
