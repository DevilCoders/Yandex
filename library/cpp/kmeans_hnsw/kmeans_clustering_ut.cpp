#include <library/cpp/kmeans_hnsw/kmeans_clustering.h>
#include <library/cpp/kmeans_hnsw/clustering_data.h>
#include <library/cpp/kmeans_hnsw/dense_vector_centroids_factory.h>
#include <library/cpp/kmeans_hnsw/dense_vector_random.h>

#include <library/cpp/hnsw/index/dense_vector_distance.h>
#include <library/cpp/hnsw/index_builder/dense_vector_distance.h>

#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/l2_distance/l2_distance.h>

#include <util/generic/algorithm.h>
#include <util/generic/ymath.h>
#include <util/memory/blob.h>
#include <util/random/normal.h>
#include <util/random/fast.h>
#include <util/random/shuffle.h>
#include <util/stream/buffer.h>

#include <utility>

Y_UNIT_TEST_SUITE(TKmeansHnswDenseVectorTest) {
    using TVectorComponent = float;

    using TBaseDistance = NHnsw::TL2SqrDistance<TVectorComponent>;
    using TDistance = NHnsw::TDistanceWithDimension<TVectorComponent, TBaseDistance>;

    using TItemStorage = NHnsw::TDenseVectorStorage<TVectorComponent>;
    using TClustering = NKmeansHnsw::TClusteringData<TItemStorage>;

    NKmeansHnsw::TBuildKmeansClustersOptions GetBuildKmeansClustersOptions(size_t numClusters, size_t numThreads = 1);

    Y_UNIT_TEST(TKmeansHnswUsageExampleTest) {
        TFastRng64 gen(17);

        const size_t dimension = 5;
        const size_t numVectors = 20;
        const size_t approximateClusterSize = 5;

        const TItemStorage vectors =
            NKmeansHnsw::GenerateRandomUniformSpherePoints<TVectorComponent>(numVectors, dimension, gen);

        NKmeansHnsw::TBuildKmeansClustersOptions kmeansClustersOptions =
            GetBuildKmeansClustersOptions(vectors.GetNumItems() / approximateClusterSize);

        const TClustering clustering = NKmeansHnsw::BuildKmeansClusters<TDistance>(
            vectors, NKmeansHnsw::TDenseVectorCentroidsFactory<TVectorComponent>(dimension),
            kmeansClustersOptions, TDistance(TBaseDistance(), dimension));

        for (size_t clusterId = 0; clusterId < clustering.ClusterIds.size(); ++clusterId) {
            const TVector<ui32>& clusterIds = clustering.ClusterIds[clusterId];

            Cout << "cluster size " << clusterIds.size() << ", cluster center ";
            for (size_t i = 0; i < dimension; ++i)
                Cout << clustering.Clusters.GetItem(clusterId)[i] << " ";
            Cout << Endl;

            Cout << "cluster vector ids: ";
            for (size_t id : clusterIds)
                Cout << id << " ";
            Cout << "\n"
                 << Endl;
        }
    }

    NKmeansHnsw::TBuildKmeansClustersOptions GetBuildKmeansClustersOptions(const size_t numClusters, const size_t numThreads) {
        NKmeansHnsw::TBuildKmeansClustersOptions kmeansClustersOptions = {};
        kmeansClustersOptions.ReportProgress = false;
        kmeansClustersOptions.NumIterations = 8;
        kmeansClustersOptions.HnswNeighborhoodSize = 16;
        kmeansClustersOptions.NumClusters = numClusters;
        kmeansClustersOptions.NumThreads = numThreads;
        kmeansClustersOptions.NumVectorsForEmptyClusters = 3;

        NHnsw::THnswBuildOptions& hnswOpts = kmeansClustersOptions.HnswBuildOptions;

        hnswOpts.NumThreads = numThreads;
        hnswOpts.MaxNeighbors = Min<ui32>(32, kmeansClustersOptions.NumClusters - 1);
        hnswOpts.BatchSize = 1000;
        hnswOpts.UpperLevelBatchSize = 40000;
        hnswOpts.SearchNeighborhoodSize = 300;
        hnswOpts.NumExactCandidates = 100;
        hnswOpts.LevelSizeDecay = Max<ui32>(hnswOpts.MaxNeighbors / 2, 2);
        hnswOpts.ReportProgress = false;

        return kmeansClustersOptions;
    }

    template <class TVectorComponent, class TGen>
    NKmeansHnsw::TClusteringData<NHnsw::TDenseVectorStorage<TVectorComponent>> GenerateRandomExactClusters(
        const size_t numClusters,
        const size_t minClusterSize,
        const size_t maxClusterSize,
        const size_t dimension,
        TGen& gen) {
        TVector<size_t> clusterSizes(numClusters);
        for (size_t i = 0; i < numClusters; ++i)
            clusterSizes[i] = minClusterSize + gen.GenRand() % (maxClusterSize - minClusterSize + 1);

        const size_t numVectors = Accumulate(clusterSizes.begin(), clusterSizes.end(), static_cast<size_t>(0));

        const size_t bufferSize = numVectors * dimension * sizeof(TVectorComponent);
        TBuffer buffer(bufferSize);
        buffer.Proceed(bufferSize);
        TVectorComponent* const data = reinterpret_cast<TVectorComponent*>(buffer.Data());

        TVector<size_t> permutation(numVectors);
        Iota(permutation.begin(), permutation.end(), static_cast<size_t>(0));
        Shuffle(permutation.begin(), permutation.end(), gen);

        TVector<TVector<ui32>> exactClusterIds(numClusters);

        size_t curVectorIndex = 0;
        for (size_t clusterId = 0; clusterId < numClusters; ++clusterId) {
            const size_t firstClusterVectorIndex = curVectorIndex;

            for (size_t inClusterId = 0; inClusterId < clusterSizes[clusterId]; ++inClusterId) {
                size_t vectorIdPermuted = permutation[curVectorIndex];
                exactClusterIds[clusterId].push_back(vectorIdPermuted);

                if (curVectorIndex == firstClusterVectorIndex) {
                    NKmeansHnsw::GenerateRandomUniformSpherePoint(dimension, gen, data + dimension * vectorIdPermuted);
                } else {
                    MemCopy(data + dimension * vectorIdPermuted, data + dimension * permutation[firstClusterVectorIndex], dimension);
                }

                ++curVectorIndex;
            }
        }
        Y_VERIFY(curVectorIndex == numVectors);

        NHnsw::TDenseVectorStorage<TVectorComponent> storage(TBlob::FromBuffer(buffer), dimension);
        return NKmeansHnsw::TClusteringData<NHnsw::TDenseVectorStorage<TVectorComponent>>{std::move(storage), exactClusterIds};
    }

    void AssertClusteringsEqual(TVector<TVector<ui32>> expected, TVector<TVector<ui32>> actual) {
        auto normalizeClustering = [](TVector<TVector<ui32>>& clustering) {
            for (TVector<ui32>& cluster : clustering)
                Sort(cluster.begin(), cluster.end());

            Sort(clustering.begin(), clustering.end());
        };

        normalizeClustering(expected);
        normalizeClustering(actual);

        if (expected != actual) {
            auto printClustering = [](const TVector<TVector<ui32>>& clustering) {
                Cerr << "Clustering into " << clustering.size() << " clusters of sizes ";
                for (const auto& it : clustering)
                    Cerr << it.size() << " ";
                Cerr << Endl;
            };

            Cerr << "Expected: ";
            printClustering(expected);
            Cerr << "Actual: ";
            printClustering(actual);
        }

        UNIT_ASSERT_EQUAL(expected, actual);
    }

    // Test kmeans on a collection of vectors that admits clustering with zero error (each cluster contains only equal vectors)
    // It must succeed provided that iteration count is sufficiently large for it to guess the correct center
    // when dealing with empty clusters
    void TestKmeansExactClusters(const size_t numClusters, const size_t minClusterSize, const size_t maxClusterSize, const size_t numIterations, const size_t dimension = 10) {
        TFastRng64 gen(17);

        NKmeansHnsw::TBuildKmeansClustersOptions kmeansClustersOptions = GetBuildKmeansClustersOptions(numClusters);
        kmeansClustersOptions.NumIterations = numIterations;

        kmeansClustersOptions.HnswNeighborhoodSize = 100;
        kmeansClustersOptions.NumVectorsForEmptyClusters = 5;
        const TClustering exactClustering = GenerateRandomExactClusters<TVectorComponent>(numClusters, minClusterSize, maxClusterSize, dimension, gen);

        const TClustering clustering = NKmeansHnsw::BuildKmeansClusters<TDistance>(exactClustering.Clusters, NKmeansHnsw::TDenseVectorCentroidsFactory<TVectorComponent>(dimension), kmeansClustersOptions, TDistance(TBaseDistance(), dimension));

        AssertClusteringsEqual(exactClustering.ClusterIds, clustering.ClusterIds);
    }

    Y_UNIT_TEST(TKmeansHnswIdentityClusterizationSmallTest) {
        TestKmeansExactClusters(3, 2, 5, 200);
        TestKmeansExactClusters(5, 10, 20, 200);
    }

    Y_UNIT_TEST(TExtremeCountsTest) {
        // check that library supports num clusters = num vectors
        TestKmeansExactClusters(100, 1, 1, 200, 10);
    }

    Y_UNIT_TEST(TKmeansHnswIdentityClusterizationLargeTest) {
        TestKmeansExactClusters(20, 100, 200, 200, 10);
        TestKmeansExactClusters(20, 100, 200, 200, 50);
    }
}
