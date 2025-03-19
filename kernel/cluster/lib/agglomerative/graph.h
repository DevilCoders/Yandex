#pragma once

#include <library/cpp/containers/dense_hash/dense_hash.h>

#include <contrib/libs/sparsehash/src/sparsehash/dense_hash_set>

#include <util/generic/vector.h>
#include <library/cpp/containers/intrusive_rb_tree/rb_tree.h>

namespace NAgglomerativePrivate {

struct TClustersCross {
    float PrecisionCross;
    float RecallCross;

    explicit TClustersCross(float similarity = 0)
        : PrecisionCross(similarity)
        , RecallCross(similarity)
    {
    }

    TClustersCross(const TClustersCross& source)
        : PrecisionCross(source.PrecisionCross)
        , RecallCross(source.RecallCross)
    {
    }

    void operator += (const TClustersCross& other) {
        PrecisionCross += other.PrecisionCross;
        RecallCross += other.RecallCross;
    }
};

struct TEdgeSimilarity;

struct TEdgeSimilarityGreater {
    bool Compare(const TEdgeSimilarity& l, const TEdgeSimilarity& r) const;
};

struct TEdgeSimilarity : public TRbTreeItem<TEdgeSimilarity, TEdgeSimilarityGreater> {
    ui32 NodeIdx; // for stability of sorting
    float CurrentValue; // tree is sorted by this value
    float UpdatedValue; // but this the real value

    TEdgeSimilarity(ui32 nodeIdx, float value)
        : NodeIdx(nodeIdx)
        , CurrentValue(value)
        , UpdatedValue(value)
    {
    }

    TEdgeSimilarity& operator=(const TEdgeSimilarity& rhs) {
        NodeIdx = rhs.NodeIdx;
        CurrentValue = rhs.CurrentValue;
        UpdatedValue = rhs.UpdatedValue;
        return *this;
    }

    bool operator> (const TEdgeSimilarity& r) const {
        if (CurrentValue != r.CurrentValue) {
            return CurrentValue > r.CurrentValue;
        }
        return NodeIdx < r.NodeIdx;
    }

    bool operator== (const TEdgeSimilarity& r) const {
        return CurrentValue == r.CurrentValue && NodeIdx == r.NodeIdx;
    }

private:
    TEdgeSimilarity(const TEdgeSimilarity&);
};

inline bool TEdgeSimilarityGreater::Compare(const TEdgeSimilarity& l, const TEdgeSimilarity& r) const {
    return l > r;
}

struct TEdge {
    TClustersCross ClustersCross;
    TAutoPtr<TEdgeSimilarity> Similarity;

    TEdge() {
    }
};

/*
struct THighBitsHash {
    size_t operator() (size_t val) const {
        return val >> 3;
    }
};
*/

typedef THash<ui32> THighBitsHash;
typedef TDenseHash<ui32, TEdge, THighBitsHash, 50, 2> TEdgesHash;

struct TMaxSimilarity {
    ui32 NodeIdx;
    TEdgeSimilarity Similarity; // TODO: rename to "Neighbour?"

    TMaxSimilarity()
        : NodeIdx(0)
        , Similarity(0, 0)
    {
    }

    bool operator> (const TMaxSimilarity& r) const {
        if (!(Similarity == r.Similarity)) {
            return Similarity > r.Similarity;
        }
        return NodeIdx < r.NodeIdx;
    }

    bool operator== (const TMaxSimilarity& r) const {
        return Similarity == r.Similarity && NodeIdx == r.NodeIdx;
    }
};

struct TNode;

struct TNodeMaxSimilarityGreater {
    bool Compare(const TNode& l, const TNode& r) const;
};

struct TNode : public TRbTreeItem<TNode, TNodeMaxSimilarityGreater> {
    size_t Idx;
    TEdgesHash Edges;
    TMaxSimilarity MaxSimilarity;
    TRbTree<TEdgeSimilarity, TEdgeSimilarityGreater> Similarities;

    TNode() {
    }

    TNode(const TNode& other)
        : Idx(other.Idx)
    {
        Y_ASSERT(other.Similarities.Empty());
    }

    // Invalidates references to edges outside!
    void PushNewSimilarity(TEdge& edge, float similarity, ui32 right) {
        Y_ASSERT(!edge.Similarity);
        edge.Similarity = new TEdgeSimilarity(right, similarity);
        Similarities.Insert(edge.Similarity.Get());
        if (*edge.Similarity > MaxSimilarity.Similarity) {
            MaxSimilarity.Similarity = *edge.Similarity;
        }
    }
};

inline bool TNodeMaxSimilarityGreater::Compare(const TNode& l, const TNode& r) const {
    return l.MaxSimilarity > r.MaxSimilarity;
}

struct TSimilarityGraph {
    TVector<TNode> Nodes;
    TRbTree<TNode, TNodeMaxSimilarityGreater> NodeMaxSimilarities;

    TSimilarityGraph(size_t size = 0) {
        Init(size);
    }

    void Init(size_t size) {
        Nodes.resize(size);
        for (size_t i = 0; i < size; ++i) {
            Nodes[i].MaxSimilarity.NodeIdx = i;
            Nodes[i].Idx = i;
        }
    }

    TEdge& GetEdge(ui32 left, ui32 right) {
        Y_ASSERT(left < Nodes.size() && right < Nodes.size());

        if (left > right) {
            DoSwap(left, right);
        }
        return Nodes[left].Edges[right];
    }

    void ThreadSafePushSimilarity(TEdge& edge, float similarity, ui32 left, ui32 right) {
        if (similarity > 0) {
            if (left > right) {
                DoSwap(left, right);
            }
            Nodes[left].PushNewSimilarity(edge, similarity, right);
        }
    };

    void FinishThreadSafePushes() {
        for (size_t i = 0; i < Nodes.size(); ++i) {
            TNode& node = Nodes[i];
            if (!node.Similarities.Empty()) {
                NodeMaxSimilarities.Insert(&node);
            }
        }
    }

    void Push(float similarity, ui32 left, ui32 right, TEdge* edgePtr = nullptr) {
        Y_ASSERT(similarity >= 0 && left <= Nodes.size() && right <= Nodes.size());

        if (left > right) {
            DoSwap(left, right);
        }

        TNode& node = Nodes[left];
        TEdge& edge = edgePtr ? *edgePtr : node.Edges[right];

        if (edge.Similarity && edge.Similarity->CurrentValue > 0) {
            if (similarity > 0 && similarity < edge.Similarity->CurrentValue) {
                edge.Similarity->UpdatedValue = similarity;
                return;
            }
            edge.Similarity.Destroy();
        }

        if (similarity > 0) {
            if (edge.Similarity) {
                edge.Similarity->NodeIdx = right;
                edge.Similarity->CurrentValue = similarity;
                edge.Similarity->UpdatedValue = similarity;
            } else {
                edge.Similarity = new TEdgeSimilarity(right, similarity);
            }
            node.Similarities.Insert(edge.Similarity.Get());
        }

        TEdgeSimilarity& maxSimilarity = node.MaxSimilarity.Similarity;

        if (node.Similarities.Empty()) {
            node.UnLink();
            maxSimilarity = TEdgeSimilarity(0, 0);
            return;
        }

        auto updateMaxSimilarity = [&](const TEdgeSimilarity& newSimilarity) {
            node.UnLink();
            maxSimilarity = newSimilarity;
            NodeMaxSimilarities.Insert(node);
        };

        if (edge.Similarity && *edge.Similarity > maxSimilarity) {
            updateMaxSimilarity(*edge.Similarity);
        } else if (maxSimilarity.NodeIdx == right) {
            updateMaxSimilarity(*node.Similarities.Begin());
        }
    }

    void PopMaxSimilarity() {
        TNode& maxSimNode = *NodeMaxSimilarities.Begin();
        auto& similarities = Nodes[maxSimNode.Idx].Similarities;

        Y_ASSERT(*similarities.Begin() == maxSimNode.MaxSimilarity.Similarity);

        maxSimNode.UnLink();
        similarities.Begin()->UnLink();

        if (similarities.Empty()) {
            maxSimNode.MaxSimilarity.Similarity = TEdgeSimilarity(0, 0);
        } else {
            maxSimNode.MaxSimilarity.Similarity = *similarities.Begin();
            NodeMaxSimilarities.Insert(&maxSimNode);
        }
    }

    const TMaxSimilarity& GetMaxSimilarity() {
        Y_ASSERT(!NodeMaxSimilarities.Empty());
        return NodeMaxSimilarities.Begin()->MaxSimilarity;
    }

    void ActualizeMaxSimilarity() {
        if (NodeMaxSimilarities.Empty()) {
            return;
        }
        bool actual = false;
        while (!actual) {
            actual = true;

            TNode& maxNode = *NodeMaxSimilarities.Begin();
            auto& similarities = Nodes[maxNode.Idx].Similarities;

            while (similarities.Begin()->CurrentValue != similarities.Begin()->UpdatedValue) {
                TEdgeSimilarity& similarity = *similarities.Begin();
                similarity.UnLink();
                similarity.CurrentValue = similarity.UpdatedValue;
                similarities.Insert(&similarity);
                actual = false;
            }

            if (!actual) {
                maxNode.UnLink();
                maxNode.MaxSimilarity.Similarity = *similarities.Begin();
                NodeMaxSimilarities.Insert(&maxNode);
            }
        }
    }


    void ClearNode(size_t idx) {
        Nodes[idx].UnLink();
        Nodes[idx].Edges.MakeEmpty();
    }

    void Clear() {
        NodeMaxSimilarities.Clear();
        for (size_t i = 0; i < Nodes.size(); ++i) {
            ClearNode(i);
        }
    }
};

}
