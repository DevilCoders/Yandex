#include "agglomerative_clustering.h"

#include <library/cpp/deprecated/atomic/atomic.h>

#include <util/generic/ymath.h>
#include <util/thread/pool.h>

namespace NAgglomerative {

void TClusterization::FinishAdding() {
    THolder<IThreadPool> queue(CreateThreadPool(ThreadsCount));

    auto shardByThreads = [&](const std::function<void(size_t)>& func) {
        queue->Start(ThreadsCount);
        for (size_t threadNumber = 0; threadNumber < ThreadsCount; ++threadNumber) {
            queue->SafeAddFunc([func, threadNumber]() { func(threadNumber); } );
        }
        queue->Stop();
    };

    TVector<TVector<float> > threadsDocumentRecalls(ThreadsCount, TVector<float>(DocumentRecalls.size(), 0));

    shardByThreads(
        [this, &threadsDocumentRecalls](size_t threadNumber) {
            TShard& shard = Shards[threadNumber];
            for (size_t elementNumber = 0; elementNumber < ElementsCount; ++elementNumber) {
                for (const auto& it : shard.SimilarityGraph.Nodes[elementNumber].Edges) {
                    threadsDocumentRecalls[threadNumber][it.first] += it.second.ClustersCross.PrecisionCross;
                }
            }
        }
    );

    shardByThreads(
        [this, &threadsDocumentRecalls](size_t threadNumber) {
            for (size_t otherThreadNumber = 0; otherThreadNumber < threadsDocumentRecalls.size(); ++otherThreadNumber) {
                const TVector<float>& threadDocumentRecalls = threadsDocumentRecalls[otherThreadNumber];
                for (size_t elementNumber = threadNumber; elementNumber < DocumentRecalls.size(); elementNumber += ThreadsCount) {
                    DocumentRecalls[elementNumber] += threadDocumentRecalls[elementNumber];
                }
            }
        }
    );

    shardByThreads(
        [this](size_t threadNumber) {
            TShard& shard = Shards[threadNumber];
            for (size_t elementNumber = 0; elementNumber < ElementsCount; ++elementNumber) {
                float invertedNormalizer = 1. / DocumentRecalls[elementNumber];

                Y_ASSERT(invertedNormalizer < 1 + 1e-10);

                for (auto& it : shard.SimilarityGraph.Nodes[elementNumber].Edges) {
                    it.second.ClustersCross.RecallCross *=
                        invertedNormalizer + 1. / DocumentRecalls[it.first];
                }
            }
        }
    );

    SumPrecisions = ElementsCount;
    SumRecalls = 0;

    for (size_t elementNumber = 0; elementNumber < ElementsCount; ++elementNumber) {
        float& recall = ClusterRecalls[elementNumber];
        recall = 1. / DocumentRecalls[elementNumber];
        SumRecalls += recall;
    }

    shardByThreads(
        [this](size_t threadNumber) {
            TShard& shard = Shards[threadNumber];
            for (size_t elementNumber = 0; elementNumber < ElementsCount; ++elementNumber) {
                for (ui32 neighbour : shard.Neighbours[elementNumber]) {
                    if (neighbour > elementNumber) {
                        shard.Neighbours[neighbour].Insert(elementNumber);
                    }
                }
            }
        }
    );


    shardByThreads(
        [this](size_t threadNumber) {
            TShard& shard = Shards[threadNumber];
            for (size_t elementNumber = 0; elementNumber < ElementsCount; ++elementNumber) {
                for (auto& it : shard.SimilarityGraph.Nodes[elementNumber].Edges) {
                    ui32 neighbour = it.first;
                    TEdge& edge = it.second;

                    shard.SimilarityGraph.ThreadSafePushSimilarity(
                        edge,
                        UniteMetric(elementNumber, neighbour, edge.ClustersCross),
                        elementNumber,
                        neighbour);
                }
            }
        }
    );

    shardByThreads(
        [this](size_t threadNumber) {
            Shards[threadNumber].SimilarityGraph.FinishThreadSafePushes();
        }
    );

    FinishedAdditions = true;
}

bool TClusterization::SetupRelevances() {
    if (!RawSimilarities) {
        return false;
    }

    THolder<IThreadPool> queue(CreateThreadPool(ThreadsCount));
    for (size_t threadNumber = 0; threadNumber < ThreadsCount; ++threadNumber) {
        queue->SafeAddFunc(
            [threadNumber, this]() {
                for (size_t clusterNumber = threadNumber; clusterNumber < Clusters.size(); clusterNumber += ThreadsCount) {
                    TVector<std::pair<ui32, float> >& cluster = Clusters[clusterNumber];

                    Sort(cluster.begin(), cluster.end());

                    for (size_t i = 0; i < cluster.size(); ++i) {
                        cluster[i].second = 0;
                    }
                    for (std::pair<ui32, float>* element = cluster.begin(); element < cluster.end(); ++element) {
                        const TRawSimilaritiesHash& similarities = (*RawSimilarities)[element->first];
                        for (std::pair<ui32, float>* neighbour = element + 1; neighbour < cluster.end(); ++neighbour) {
                            if (auto* p = similarities.FindPtr(neighbour->first)) {
                                element->second += *p;
                                neighbour->second += *p;
                            }
                        }
                        element->second += 1;
                        element->second /= cluster.size();
                    }
                }
            }
        );
    }
    queue->Stop();
    return true;
}

inline float CombineMetrics(float precision, float recall, float recallFactor) {
    return pow(recall, recallFactor) * precision;
}

float TClusterization::UniteMetric(size_t left, size_t right, const TClustersCross& cross) const {
    size_t leftSize = ClusterSizes[left];
    size_t rightSize = ClusterSizes[right];
    size_t uniteSize = leftSize + rightSize;

    float leftPrecision = ClusterPrecisions[left];
    float rightPrecision = ClusterPrecisions[right];

    float oldPrecisionsSum = leftPrecision / leftSize + rightPrecision / rightSize;
    float newPrecisionsSum = (leftPrecision + rightPrecision + 2 * cross.PrecisionCross) / uniteSize;

    float oldRecallsSum = ClusterRecalls[left] + ClusterRecalls[right];
    float newRecallsSum = oldRecallsSum + cross.RecallCross;

    float recallFactor = RecallFactor;
    if (RecallDecayFactor) {
        recallFactor *= RecallDecayFactor / (RecallDecayFactor + uniteSize);
    }

    float diff = CombineMetrics(newPrecisionsSum / uniteSize, newRecallsSum / uniteSize, recallFactor) -
                 CombineMetrics(oldPrecisionsSum / uniteSize, oldRecallsSum / uniteSize, recallFactor);

    return diff > 0 ? (newPrecisionsSum + newRecallsSum * 0.01) / uniteSize : 0;
}

void TClusterization::UniteCommon(ui32 left, ui32 right) {
    Y_ASSERT(IsRoot(left) && IsRoot(right) && left != right);

    TShard& shard = Shards[(left ^ right) & ShardMask];

    Parents[left] = right;

    const TClustersCross& cross = shard.SimilarityGraph.GetEdge(left, right).ClustersCross;

    float oldPrecisionsSum = ClusterPrecisions[left] / ClusterSizes[left] + ClusterPrecisions[right] / ClusterSizes[right];
    float newPrecisionsSum = (ClusterPrecisions[left] + ClusterPrecisions[right] + 2 * cross.PrecisionCross) /
                             (ClusterSizes[left] + ClusterSizes[right]);
    float oldRecallsSum = ClusterRecalls[left] + ClusterRecalls[right];
    float newRecallsSum = ClusterRecalls[left] + ClusterRecalls[right] + cross.RecallCross;

    SumPrecisions += newPrecisionsSum - oldPrecisionsSum;
    SumRecalls += newRecallsSum - oldRecallsSum;

    ClusterSizes[right] += ClusterSizes[left];

    ClusterPrecisions[right] += ClusterPrecisions[left] + 2 * cross.PrecisionCross;
    ClusterRecalls[right] += ClusterRecalls[left] + cross.RecallCross;
}

struct TSimilarityUpdate {
    float NewValue;
    ui32 Left;
    ui32 Right;

    TSimilarityUpdate() {
    }

    TSimilarityUpdate(float newValue, ui32 left, ui32 right)
        : NewValue(newValue)
        , Left(left)
        , Right(right)
    {
    }
};

typedef TVector<TSimilarityUpdate> TSimilarityUpdateVector;

// we want all data to be aligned to cache line to prevent false sharing
struct TThreadData {
    TAtomic WakeFlag;
    ui64 WakeFlagAligner[64 - sizeof(TAtomic)];

    TAtomic Phase1Ready;
    ui64 Phase1ReadyAligner[64 - sizeof(TAtomic)];

    TVector<TSimilarityUpdate> LocalSimilarityUpdates;
    ui64 LocalAligner[64 - sizeof(TSimilarityUpdateVector)];

    TVector<TSimilarityUpdate> RemoteSimilarityUpdates;
    ui64 RemotaAligner[64 - sizeof(TSimilarityUpdateVector)];

    TThreadData() {
        AtomicSet(WakeFlag, 0);
        AtomicSet(Phase1Ready, 0);
    }

    ~TThreadData() {
    }
};

void WaitForAtomic(TAtomic& atomic) {
    // busy loop!
    while (!AtomicGet(atomic)) {
        // maybe add SchedYield?
    }
    AtomicSet(atomic, 0);
}

struct TClusterization::TSharedParallelContext {
    TThreadData ThreadData[MaxThreads];

    size_t ThreadsCount;
    TAtomic ShouldStop;
    TAtomic WorkingThreads;
    ui32 Left;
    ui32 Right;

    TSharedParallelContext(size_t threadsCount) {
        ThreadsCount = threadsCount;
        for (size_t i = 0; i < ThreadsCount; ++i) {
            AtomicSet(ThreadData[i].WakeFlag, 0);
        }
        AtomicSet(ShouldStop, 0);
        AtomicSet(WorkingThreads, 0);
    }

    void WakeAll() {
        for (size_t i = 0; i < ThreadsCount; ++i) {
            AtomicSet(ThreadData[i].WakeFlag, 1);
        }
    }
};

void TClusterization::UniteShard(ui32 localShardId, ui32 left, ui32 right, TSharedParallelContext& context) {
    AtomicBarrier();

    // Given:
    //     localShardId = (right ^ rightNeighbour) & ShardMask = (left ^ leftNeighbour) & ShardMask
    //     remoteShardId = (left ^ rightNeighbour) & ShardMask = (right ^ leftNeighbour) & ShardMask
    // Thus:
    //     (remoteShardId ^ localShardId) = (right ^ left) & ShardMask
    //     remoteShardId = (right ^ left ^ localShardId) & ShardMask

    ui32 remoteShardId = (right ^ left ^ localShardId) & ShardMask;

    TShard& remoteShard = Shards[remoteShardId];
    TShard& localShard = Shards[localShardId];

    TNeighboursHash& leftNeighbours = remoteShard.Neighbours[left];
    TNeighboursHash& rightNeighbours = localShard.Neighbours[right];

    TThreadData& leftThreadData = context.ThreadData[remoteShardId];
    TThreadData& rightThreadData = context.ThreadData[localShardId];

    for (ui32 neighbour : rightNeighbours) {
        Y_ASSERT(((neighbour ^ right) & ShardMask) == localShardId);

        if (!IsRoot(neighbour) || neighbour == left) {
            continue;
        }

        TEdge& edge = localShard.SimilarityGraph.GetEdge(right, neighbour);
        TClustersCross& cross = edge.ClustersCross;
        bool isAlsoLeftNeighbour = leftNeighbours.Has(neighbour);
        if (isAlsoLeftNeighbour) {
            cross += remoteShard.SimilarityGraph.GetEdge(left, neighbour).ClustersCross;
        }
        TSimilarityUpdate rightUpdate = { UniteMetric(right, neighbour, cross), neighbour, right };
        rightThreadData.LocalSimilarityUpdates.push_back(rightUpdate);
        if (isAlsoLeftNeighbour && neighbour < left) {
            leftThreadData.RemoteSimilarityUpdates.push_back(TSimilarityUpdate(0, neighbour, left));
        }
    }

    for (ui32 neighbour : leftNeighbours) {
        Y_ASSERT(((neighbour ^ left) & ShardMask) == remoteShardId);

        if (!IsRoot(neighbour) || neighbour == right || rightNeighbours.Has(neighbour)) {
            continue;
        }

        TNeighboursHash& neighbourNeighbours = localShard.Neighbours[neighbour];

        rightNeighbours.Insert(neighbour);
        neighbourNeighbours.Insert(right);

        TEdge& edge = localShard.SimilarityGraph.GetEdge(right, neighbour);
        TClustersCross& cross = edge.ClustersCross;
        cross += remoteShard.SimilarityGraph.GetEdge(left, neighbour).ClustersCross;

        TSimilarityUpdate rightUpdate = { UniteMetric(right, neighbour, cross), neighbour, right };
        rightThreadData.LocalSimilarityUpdates.push_back(rightUpdate);
        if (neighbour < left) {
            leftThreadData.RemoteSimilarityUpdates.push_back(TSimilarityUpdate(0, neighbour, left));
        }
    }

    AtomicSet(context.ThreadData[remoteShardId].Phase1Ready, 1);
    WaitForAtomic(context.ThreadData[localShardId].Phase1Ready);

    auto applyUpdates = [&](TSimilarityUpdateVector& updates) {
        for (const TSimilarityUpdate& update : updates) {
            localShard.SimilarityGraph.Push(update.NewValue, update.Left, update.Right);
        }
        updates.clear();
    };

    AtomicBarrier();

    applyUpdates(rightThreadData.LocalSimilarityUpdates);
    applyUpdates(rightThreadData.RemoteSimilarityUpdates);

    TNeighboursHash().Swap(localShard.Neighbours[left]);
    localShard.SimilarityGraph.ClearNode(left);
    localShard.SimilarityGraph.ActualizeMaxSimilarity();

    AtomicBarrier();
}

void TClusterization::FindClustersForUnite(TSharedParallelContext& context) {
    ++IterationsCount;

    TShard* bestShard = nullptr;
    const NAgglomerativePrivate::TMaxSimilarity* maxSimilarity = nullptr;

    for (size_t i = 0; i < MaxThreads; ++i) {
        TShard& shard = Shards[i];

        if (!shard.SimilarityGraph.NodeMaxSimilarities.Empty()) {
            const NAgglomerativePrivate::TMaxSimilarity* candidate = &shard.SimilarityGraph.GetMaxSimilarity();

            if (!maxSimilarity || *candidate > *maxSimilarity) {
                bestShard = &shard;
                maxSimilarity = candidate;
            }
        }
    }

    if (!maxSimilarity) {
        AtomicSet(context.ShouldStop, 1);
    } else {
        context.Left = maxSimilarity->NodeIdx;
        context.Right = maxSimilarity->Similarity.NodeIdx;

        bestShard->SimilarityGraph.PopMaxSimilarity();

        ui32 leftNeighboursCount = bestShard->Neighbours[context.Left].Size();
        ui32 rightNeighboursCount = bestShard->Neighbours[context.Right].Size();

        if (leftNeighboursCount > rightNeighboursCount) {
            DoSwap(context.Left, context.Right);
        }

        AtomicSet(context.WorkingThreads, context.ThreadsCount);
    }
}

void TClusterization::IterateShard(ui32 shard, TSharedParallelContext& context) {
    TThreadData& threadData = context.ThreadData[shard];
    for (;;) {
        WaitForAtomic(threadData.WakeFlag);

        if (AtomicGet(context.ShouldStop)) {
            break;
        }

        UniteShard(shard, context.Left, context.Right, context);

        if (AtomicDecrement(context.WorkingThreads) == 0) {
            FindClustersForUnite(context);
            if (!AtomicGet(context.ShouldStop)) {
                UniteCommon(context.Left, context.Right);
            }
            context.WakeAll();
        }
    }
}

void TClusterization::Iterate() {
    TSharedParallelContext context(ThreadsCount);

    // should be queue gets any jobs or single-threaded case will break
    FindClustersForUnite(context);
    if (!AtomicGet(context.ShouldStop)) {
        UniteCommon(context.Left, context.Right);
    }
    context.WakeAll();

    THolder<IThreadPool> queue(CreateThreadPool(ThreadsCount));
    for (size_t i = 0; i < ThreadsCount; ++i) {
        queue->SafeAddFunc(
            [i, this, &context]() {
                IterateShard(i, context);
            });
    }
    queue->Start(ThreadsCount);
    queue->Stop();
}

bool TClusterization::Build() {
    if (!FinishedAdditions) {
        FinishAdding();
    }

    Iterate();

    size_t clustersCount = 0;
    TVector<ui32> rootNumbers(ElementsCount);
    for (size_t i = 0; i < Parents.size(); ++i) {
        if (Parents[i] == i) {
            rootNumbers[i] = clustersCount++;
        }
    }
    Clusters.resize(clustersCount);

    for (size_t i = 0; i < ThreadsCount; ++i) {
        Shards[i].Clear();
    }

    for (size_t elementNumber = 0; elementNumber < ElementsCount; ++elementNumber) {
        Clusters[rootNumbers[GetRoot(elementNumber)]].push_back(std::make_pair(elementNumber, 1.f));
    }

    return true;
}

}
