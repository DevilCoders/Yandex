#pragma once

#include <library/cpp/hnsw/index_builder/index_builder.h>
#include <library/cpp/hnsw/index_builder/index_writer.h>
#include <library/cpp/hnsw/index/index_base.h>

#include <library/cpp/threading/local_executor/local_executor.h>

#include <util/generic/buffer.h>
#include <util/random/fast.h>
#include <util/stream/buffer.h>
#include <util/ysaveload.h>

#include "clustering_data.h"

namespace NKmeansHnsw {
    struct TBuildKmeansClustersOptions {
        size_t NumClusters = 0;
        size_t NumIterations = 8;

        // If the cluster becomes empty,
        // set its center to centroid of NumVectorsForEmptyClusters random vectors
        size_t NumVectorsForEmptyClusters = 3;

        size_t HnswNeighborhoodSize = 15;
        NHnsw::THnswBuildOptions HnswBuildOptions = {};

        size_t NumThreads = 1;

        size_t RandomSeed = 0;
        bool ReportProgress = false;

        Y_SAVELOAD_DEFINE(
            NumClusters,
            NumIterations,
            NumVectorsForEmptyClusters,
            HnswNeighborhoodSize,
            HnswBuildOptions,
            NumThreads,
            RandomSeed,
            ReportProgress);
    };

    template <class TGen>
    TVector<size_t> GetRandomEquiprobableSubsetIndices(const size_t setSize, const size_t subsetSize, TGen&& gen) {
        Y_ENSURE(setSize >= subsetSize);

        // Reservoir sampling algorithm (refer to wiki)

        TVector<size_t> subset;
        subset.reserve(subsetSize);

        // End of iteration 'i' invariant is:
        // subset is a random equiprobable subset of the set { 0, 1, .., i } <=>
        // indices are unique, each index appears with the probability 1 / (i + 1)
        for (size_t i = 0; i < setSize; ++i) {
            if (i < subsetSize) {
                subset.push_back(i);
            } else {
                // the probability for this new index to be added to the subset is:
                //     subsetSize / (i + 1)
                // the probability for an old index to remain in the subset is:
                //     subsetSize / i * (1 - 1 / (i + 1)) = subsetSize / (i + 1)
                const size_t index = gen.GenRand() % (i + 1);
                if (index < subsetSize) {
                    subset[index] = i;
                }
            }
        }

        return subset;
    }

    template <class TItemStorage, class TGen, class TCentroidsFactory>
    void InitializeKmeansRandomOneVectorClusters(const TItemStorage& itemStorage, const size_t numClusters, TGen&& gen, TCentroidsFactory& centroidsFactory) {
        const size_t numVectors = itemStorage.GetNumItems();
        Y_ENSURE(numClusters <= numVectors);

        const TVector<size_t> subset = GetRandomEquiprobableSubsetIndices(numVectors, numClusters, gen);

        centroidsFactory.Reset(numClusters);
        for (size_t clusterId = 0; clusterId < numClusters; ++clusterId) {
            centroidsFactory.AddItemToCluster(itemStorage.GetItem(subset[clusterId]), clusterId);
        }
    }

    template <
        class TDistance,
        class TDistanceResult,
        class TDistanceLess,
        class TItemStorage,
        class TItem,
        class TClustersItemStorage>
    TVector<ui32> GetNearestClusters(const TItemStorage& itemStorage,
                                     const TClustersItemStorage& clustersStorage,
                                     const NHnsw::THnswBuildOptions hnswBuildOptions,
                                     const size_t hnswNeighborhoodSize,
                                     NPar::TLocalExecutor& executor,
                                     const TDistance& distance = {},
                                     const TDistanceLess& distanceLess = {}) {
        Y_ENSURE(clustersStorage.GetNumItems() > 0);

        const NHnsw::THnswIndexData indexData = NHnsw::BuildIndex<
            TDistance,
            TDistanceResult,
            TDistanceLess,
            TItemStorage>(hnswBuildOptions, clustersStorage, distance, distanceLess);

        TBufferOutput buffer;
        NHnsw::WriteIndex(indexData, buffer);

        NHnsw::THnswIndexBase index(TBlob::NoCopy(buffer.Buffer().Data(), buffer.Buffer().Size()));

        TVector<ui32> vectorToNearestCluster(itemStorage.GetNumItems());

        const auto findNearestClusterTask = [&](const size_t vectorIndex) {
            TVector<NHnsw::THnswIndexBase::TNeighbor<TDistanceResult>> neighbors =
                index.GetNearestNeighbors<TItemStorage, TDistance, TDistanceResult, TDistanceLess, TItem>(
                    itemStorage.GetItem(vectorIndex),
                    1,
                    hnswNeighborhoodSize,
                    clustersStorage,
                    distance,
                    distanceLess);

            Y_ASSERT(neighbors.size() == 1);
            vectorToNearestCluster[vectorIndex] = neighbors[0].Id;
        };

        NPar::ParallelFor(executor, 0, itemStorage.GetNumItems(), findNearestClusterTask);

        return vectorToNearestCluster;
    }

    template <
        class TDistance,
        class TDistanceResult = typename TDistance::TResult,
        class TDistanceLess = typename TDistance::TLess,
        class TItemStorage,
        class TItem = typename TItemStorage::TItem,
        class TCentroidsFactory>
    auto BuildKmeansClusters(const TItemStorage& itemStorage,
                             TCentroidsFactory&& centroidsFactory,
                             const TBuildKmeansClustersOptions& options,
                             const TDistance& distance = {},
                             const TDistanceLess& distanceLess = {}) -> TClusteringData<decltype(centroidsFactory.GetClusterCentroids())> {
        using TClustersItemStorage = decltype(centroidsFactory.GetClusterCentroids());
        using TClustering = TClusteringData<TClustersItemStorage>;

        Y_ENSURE(options.NumThreads >= 1);
        Y_ENSURE(options.NumClusters >= 1 && options.NumClusters <= itemStorage.GetNumItems());

        TFastRng<ui64> rng(options.RandomSeed);

        InitializeKmeansRandomOneVectorClusters(itemStorage, options.NumClusters, rng, centroidsFactory);

        NPar::TLocalExecutor localExecutor;
        localExecutor.RunAdditionalThreads(options.NumThreads - 1);

        for (size_t iterationIndex = 0; iterationIndex < options.NumIterations; ++iterationIndex) {
            if (options.ReportProgress) {
                Cerr << "Iteration " << iterationIndex + 1 << " of " << options.NumIterations << " planned" << Endl;
            }

            const TClustersItemStorage storage = centroidsFactory.GetClusterCentroids();
            const TVector<ui32> nearestCluster =
                GetNearestClusters<TDistance, TDistanceResult, TDistanceLess, TItemStorage, TItem, TClustersItemStorage>(
                    itemStorage, storage, options.HnswBuildOptions, options.HnswNeighborhoodSize,
                    localExecutor, distance, distanceLess);

            centroidsFactory.Reset(options.NumClusters);

            TVector<ui32> numItemsInCluster(options.NumClusters);

            for (size_t vectorId = 0; vectorId < itemStorage.GetNumItems(); ++vectorId) {
                centroidsFactory.AddItemToCluster(itemStorage.GetItem(vectorId), nearestCluster[vectorId]);
                ++numItemsInCluster[nearestCluster[vectorId]];
            }

            for (size_t clusterId = 0; clusterId < options.NumClusters; ++clusterId) {
                if (!numItemsInCluster[clusterId]) {
                    for (size_t j = 0; j < options.NumVectorsForEmptyClusters; ++j) {
                        const size_t randomItem = rng.GenRand() % itemStorage.GetNumItems();

                        centroidsFactory.AddItemToCluster(itemStorage.GetItem(randomItem), clusterId);
                        ++numItemsInCluster[clusterId];
                    }
                }
            }
        }

        // FIXME: add option to purge empty clusters!
        TClustering clustering = {centroidsFactory.GetClusterCentroids(), {}};

        if (options.ReportProgress) {
            Cerr << "Building final clusters" << Endl;
        }

        const TVector<ui32> nearestCluster =
            GetNearestClusters<TDistance, TDistanceResult, TDistanceLess, TItemStorage, TItem, TClustersItemStorage>(
                itemStorage, clustering.Clusters, options.HnswBuildOptions, options.HnswNeighborhoodSize,
                localExecutor, distance, distanceLess);

        clustering.ClusterIds.resize(options.NumClusters);
        for (size_t vectorId = 0; vectorId < itemStorage.GetNumItems(); ++vectorId) {
            clustering.ClusterIds[nearestCluster[vectorId]].push_back(vectorId);
        }

        if (options.ReportProgress) {
            Cerr << "Clustering done" << Endl;
        }

        return clustering;
    }

}
