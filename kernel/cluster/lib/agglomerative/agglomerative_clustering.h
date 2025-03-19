#pragma once

#include <kernel/cluster/lib/cluster_metrics/base.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>

#include <util/ysaveload.h>

#include "graph.h"

namespace NAgglomerative {

static float DefaultSimilarityThreshold = 0.2f;

typedef TDenseHash<ui32, float, THash<ui32>, 50, 2> TRawSimilaritiesHash;

class TClusterization {
public:
    typedef TDenseHashSet<ui32, THash<ui32>, 50, 2> TNeighboursHash;
    typedef NAgglomerativePrivate::TClustersCross TClustersCross;
    typedef NAgglomerativePrivate::TEdge TEdge;
    typedef NAgglomerativePrivate::TEdgesHash TEdgesHash;
    typedef NAgglomerativePrivate::TNode TNode;
    typedef NAgglomerativePrivate::TSimilarityGraph TSimilarityGraph;

private:
    enum {
        MaxThreads = 16,
    };

    // settings
    size_t ThreadsCount;
    ui32 ShardMask;
    float SimilarityThreshold;
    float RecallFactor;
    float RecallDecayFactor;

    // Node attributes
    size_t ElementsCount;
    TVector<ui32> ClusterSizes;
    TVector<float> ClusterPrecisions;
    TVector<float> ClusterRecalls;
    TVector<float> DocumentRecalls;
    TVector<ui32> Parents;

    // Graph data

    // some stuff for external usage
    TVector<TRawSimilaritiesHash>* RawSimilarities;

    struct TShard {
        TVector<TNeighboursHash> Neighbours; // square matrix
        TSimilarityGraph SimilarityGraph; // triangular matrix

        void Clear() {
            TVector<TNeighboursHash>().swap(Neighbours);
            SimilarityGraph.Clear();
        }
    };

    TShard Shards[MaxThreads];

    // resulting clusters data
    TVector<TVector<std::pair<ui32, float> > > Clusters;

    // Some global statistics
    float SumRecalls;
    float SumPrecisions;
    size_t IterationsCount;

    bool FinishedAdditions;
public:
    TClusterization(size_t elementsCount,
                    float recallFactor = 1.f,
                    float recallDecayFactor = 0.f,
                    float similarityThreshold = DefaultSimilarityThreshold,
                    TVector<TRawSimilaritiesHash>* rawSimilaritiesHolder = nullptr,
                    size_t threadsCount = 1)
        : ThreadsCount(threadsCount)
        , ShardMask(threadsCount - 1)
        , SimilarityThreshold(similarityThreshold)
        , RecallFactor(recallFactor)
        , RecallDecayFactor(recallDecayFactor)
        , ElementsCount(elementsCount)
        , ClusterSizes(elementsCount, 1)
        , ClusterPrecisions(elementsCount, 1)
        , ClusterRecalls(elementsCount, 0)
        , DocumentRecalls(elementsCount, 1)
        , Parents(elementsCount)
        , RawSimilarities(rawSimilaritiesHolder)
        , SumRecalls(0)
        , SumPrecisions(elementsCount)
        , IterationsCount(0)
        , FinishedAdditions(false)
    {
        if (ThreadsCount > MaxThreads) {
            Cerr << "lazy@ decided that " << (int)MaxThreads
                 << " are enough for clustering. Using it instead of " << ThreadsCount << "\n";
            ThreadsCount = MaxThreads;
            ShardMask = ThreadsCount - 1;
        } else if (ThreadsCount & ShardMask) {
            ThreadsCount = FastClp2(ThreadsCount) / 2;
            ShardMask = ThreadsCount - 1;
        }

        if (RawSimilarities) {
            TVector<TRawSimilaritiesHash>(elementsCount).swap(*RawSimilarities);
        }

        for (size_t i = 0; i < elementsCount; ++i) {
            Parents[i] = i;
        }
        for (size_t i = 0; i < ThreadsCount; ++i) {
            TShard& shard = Shards[i];
            shard.Neighbours.resize(elementsCount, TNeighboursHash((ui32)-1));
            shard.SimilarityGraph.Init(elementsCount);
        }
    }

    bool Add(ui32 left, ui32 right, float similarity) {
        if (left == right) {
            return false;
        }

        ui32 low = Min(left, right);
        ui32 high = Max(left, right);
        if (similarity < SimilarityThreshold) {
            return false;
        }

        TShard& shard = Shards[(left ^ right) & ShardMask];

        if (RawSimilarities) {
            (*RawSimilarities)[low][high] = similarity;
        }

        float normalizedSimilarity = ::Min(1.f, similarity);
        TClustersCross& clustersCross = shard.SimilarityGraph.GetEdge(low, high).ClustersCross;

        if (!clustersCross.PrecisionCross) {
            shard.Neighbours[low].Insert(high);
        } else {
            normalizedSimilarity -= clustersCross.PrecisionCross;
            if (normalizedSimilarity <= 0) {
                return false;
            }
        }

        clustersCross.PrecisionCross += normalizedSimilarity;
        clustersCross.RecallCross += normalizedSimilarity;

        DocumentRecalls[low] += normalizedSimilarity;

        return true;
    }

    float UniteMetric(size_t left, size_t right, const TClustersCross& cross) const;

    float GetPrecision() const {
        return SumPrecisions / ElementsCount;
    }

    float GetRecall() const {
        return SumRecalls / ElementsCount;
    }

    size_t GetIterationsCount() const {
        return IterationsCount;
    }

    bool SetupRelevances();
    bool Build();

    size_t GetClustersCount() const {
        return Clusters.size();
    }

    const TVector<TVector<std::pair<ui32, float> > >& GetClusters() const {
        return Clusters;
    }

    TVector<TVector<std::pair<ui32, float> > > GetNonTrivialClusters() const {
        TVector<TVector<std::pair<ui32, float> > > result;
        result.reserve(Clusters.size());
        for (const TVector<std::pair<ui32, float> >* cluster = Clusters.begin(); cluster != Clusters.end(); ++cluster) {
            if (cluster->size() > 1) {
                result.push_back(*cluster);
            }
        }
        result.shrink_to_fit();
        return result;
    }

    const TVector<std::pair<ui32, float> >& GetCluster(size_t clusterNumber) const {
        return Clusters[clusterNumber];
    }

    ui32 GetClusterNumber(ui32 elementNumber) const {
        return GetRootConst(elementNumber);
    }
private:
    void FinishAdding();

    struct TSharedParallelContext;

    void UniteCommon(ui32 left, ui32 right); // unites not sharded data (not graph)
    void UniteShard(ui32 shard, ui32 left, ui32 right, TSharedParallelContext& context);
    void FindClustersForUnite(TSharedParallelContext& context);
    void IterateShard(ui32 shard, TSharedParallelContext& context);
    void Iterate();

    bool IsRoot(ui32 elementNumber) const {
        return Parents[elementNumber] == elementNumber;
    }

    ui32 GetRoot(ui32 elementNumber) {
        ui32 root = GetRootConst(elementNumber);

        ui32 current = elementNumber;
        while (Parents[current] != root) {
            ui32 next = Parents[current];
            Parents[current] = root;
            current = next;
        }

        return root;
    }

    ui32 GetRootConst(ui32 elementNumber) const {
        ui32 current = elementNumber;
        ui32 root = Parents[current];

        while (root != current) {
            current = root;
            root = Parents[current];
        }

        return root;
    }
};

}
