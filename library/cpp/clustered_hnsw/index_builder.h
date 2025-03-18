#pragma once

#include "index_base.h"
#include "index_data.h"

#include <library/cpp/hnsw/index_builder/index_builder.h>
#include <library/cpp/hnsw/index_builder/index_writer.h>

#include <util/generic/algorithm.h>
#include <util/generic/buffer.h>
#include <util/stream/buffer.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>
#include <util/ysaveload.h>

namespace NClusteredHnsw {
    struct TClusteredHnswBuildOptions {
        size_t NumThreads = 32;

        bool BuildOverlappingClusters = true;
        size_t NumOverlappingClustersCandidates = 20;
        size_t OverlappingClustersNeighborhoodSize = 40;

        NHnsw::THnswBuildOptions HnswOptions = {};

        bool Verbose = false;

        Y_SAVELOAD_DEFINE(
            NumThreads,
            BuildOverlappingClusters,
            NumOverlappingClustersCandidates,
            OverlappingClustersNeighborhoodSize,
            HnswOptions,
            Verbose);
    };

    namespace NPrivate {
        template <
            class TDistance,
            class TDistanceLess,
            class TItemStorage,
            class TClustersItemStorage,
            class TItem>
        void SortClusterVectorsByDistanceToCentroid(const TItemStorage& itemStorage,
                                                    TClusteredHnswIndexData<TClustersItemStorage, TItem>& indexData,
                                                    NPar::TLocalExecutor& localExecutor,
                                                    const TDistance& distance,
                                                    const TDistanceLess& distanceLess) {
            auto sortClusterTask = [&](const size_t clusterId) {
                const TItem& clusterCenter = indexData.ClustersItemStorage.GetItem(clusterId);

                Sort(indexData.ClusterIds[clusterId].begin(), indexData.ClusterIds[clusterId].end(), [&](const ui32 lhs, const ui32 rhs) {
                    return distanceLess(distance(itemStorage.GetItem(lhs), clusterCenter),
                                        distance(itemStorage.GetItem(rhs), clusterCenter));
                });
            };

            localExecutor.ExecRange(sortClusterTask, 0, indexData.ClusterIds.size(), NPar::TLocalExecutor::WAIT_COMPLETE);
        }

        template <
            class TDistance,
            class TDistanceResult,
            class TDistanceLess,
            class TItemStorage,
            class TItem,
            class TClustersItemStorage>
        void BuildOverlappingClusters(const TItemStorage& itemStorage,
                                      TClusteredHnswIndexData<TClustersItemStorage, TItem>& indexData,
                                      NPar::TLocalExecutor& localExecutor,
                                      const size_t numOverlappingClustersCandidates,
                                      const size_t overlappingClustersNeighborhoodSize,
                                      const TDistance& distance,
                                      const TDistanceLess& distanceLess) {
            TBufferOutput buffer;
            NHnsw::WriteIndex(indexData.ClustersHnswIndexData, buffer);

            NHnsw::THnswIndexBase clustersHnswIndex(TBlob::NoCopy(buffer.Buffer().Data(), buffer.Buffer().Size()));

            TMutex clusterIdsMutex;

            TVector<ui32> originalItemCluster(itemStorage.GetNumItems(), Max<ui32>());
            for (size_t clusterId = 0; clusterId < indexData.ClustersItemStorage.GetNumItems(); ++clusterId) {
                for (const size_t itemId : indexData.ClusterIds[clusterId]) {
                    originalItemCluster[itemId] = static_cast<ui32>(clusterId);
                }
            }

            auto addVectorToNearestClustersTask = [&](const ui32 vectorId) {
                using TNeighbor = NHnsw::THnswIndexBase::TNeighbor<TDistanceResult>;

                TVector<TNeighbor> neighborClusters =
                    clustersHnswIndex.GetNearestNeighbors<TItemStorage, TDistance, TDistanceResult, TDistanceLess, TItem>(
                        itemStorage.GetItem(vectorId),
                        numOverlappingClustersCandidates,
                        overlappingClustersNeighborhoodSize,
                        indexData.ClustersItemStorage,
                        distance,
                        distanceLess);

                for (size_t i = 0; i < neighborClusters.size(); ++i) {
                    const TNeighbor& cluster = neighborClusters[i];

                    if (cluster.Id == originalItemCluster[vectorId]) {
                        continue;
                    }

                    bool skip = false;
                    for (size_t j = 0; j < i; ++j) {
                        // HNSW-like heuristic:
                        // If cluster j is closer to i than point -> skip

                        if (distanceLess(distance(indexData.ClustersItemStorage.GetItem(neighborClusters[j].Id),
                                                  indexData.ClustersItemStorage.GetItem(neighborClusters[i].Id)),
                                         neighborClusters[i].Dist))
                        {
                            skip = true;
                            break;
                        }
                    }

                    if (skip) {
                        continue;
                    }

                    auto guard = Guard(clusterIdsMutex);
                    indexData.ClusterIds[cluster.Id].push_back(vectorId);
                }
            };

            localExecutor.ExecRange(addVectorToNearestClustersTask, 0, itemStorage.GetNumItems(),
                                    NPar::TLocalExecutor::WAIT_COMPLETE);
        }

    }

    template <
        class TDistance,
        class TDistanceResult = typename TDistance::TResult,
        class TDistanceLess = typename TDistance::TLess,
        class TItemStorage,
        class TItem = typename TItemStorage::TItem,
        class TClustersItemStorage>
    TClusteredHnswIndexData<TClustersItemStorage, TItem> BuildIndex(const TClusteredHnswBuildOptions& options,
                                                                    const TItemStorage& itemStorage,
                                                                    TClustersItemStorage& clustersItemStorage,
                                                                    TVector<TVector<ui32>>& clusterIds,
                                                                    const TDistance& distance = {},
                                                                    const TDistanceLess& distanceLess = {}) {
        Y_ENSURE(options.NumThreads >= 1);

        TClusteredHnswIndexData<TClustersItemStorage, TItem> data = {std::move(clustersItemStorage), {}, std::move(clusterIds)};

        data.ClustersHnswIndexData = NHnsw::BuildIndex<
            TDistance,
            TDistanceResult,
            TDistanceLess,
            TItemStorage>(options.HnswOptions, data.ClustersItemStorage, distance, distanceLess);

        NPar::TLocalExecutor localExecutor;
        localExecutor.RunAdditionalThreads(options.NumThreads - 1);

        const auto sortClusters = [&]() {
            NPrivate::SortClusterVectorsByDistanceToCentroid<TDistance, TDistanceLess, TItemStorage, TClustersItemStorage, TItem>(
                itemStorage, data, localExecutor, distance, distanceLess);
        };

        sortClusters();

        if (options.BuildOverlappingClusters) {
            // Add each vector to several additional nearest clusters using HNSW heuristic
            NPrivate::BuildOverlappingClusters<TDistance, TDistanceResult, TDistanceLess, TItemStorage, TItem, TClustersItemStorage>(
                itemStorage,
                data,
                localExecutor,
                options.NumOverlappingClustersCandidates,
                options.OverlappingClustersNeighborhoodSize,
                distance,
                distanceLess);

            sortClusters();
        }

        if (options.Verbose) {
            size_t totalInClusters = 0;
            for (const TVector<ui32>& cluster : data.ClusterIds) {
                totalInClusters += cluster.size();
            }

            Cerr << "Average point is in " << totalInClusters / static_cast<double>(itemStorage.GetNumItems()) << " clusters" << Endl;
        }

        return data;
    }

}
