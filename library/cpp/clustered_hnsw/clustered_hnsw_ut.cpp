#include <library/cpp/dot_product/dot_product.h>
#include <library/cpp/l2_distance/l2_distance.h>
#include <library/cpp/kmeans_hnsw/kmeans_clustering.h>
#include <library/cpp/kmeans_hnsw/dense_vector_random.h>
#include <library/cpp/kmeans_hnsw/dense_vector_centroids_factory.h>
#include <library/cpp/hnsw/index/dense_vector_distance.h>
#include <library/cpp/hnsw/index_builder/dense_vector_distance.h>
#include <library/cpp/clustered_hnsw/index_builder.h>
#include <library/cpp/clustered_hnsw/index_writer.h>
#include <library/cpp/clustered_hnsw/index_base.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>
#include <util/generic/ymath.h>
#include <util/memory/blob.h>
#include <util/random/normal.h>
#include <util/random/fast.h>
#include <util/random/shuffle.h>
#include <util/stream/buffer.h>

Y_UNIT_TEST_SUITE(TClusteredHnswTest) {
    template <class TDistanceResult>
    using TNeighbor = NHnsw::THnswIndexBase::TNeighbor<TDistanceResult>;

    template <
        class TDistance,
        class TDistanceResult = typename TDistance::TResult,
        class TDistanceLess = typename TDistance::TLess,
        class TItemStorage,
        class TItem = typename TItemStorage::TItem>
    TVector<TNeighbor<TDistanceResult>> GetNearestNeighborsStupid(const TItem& query,
                                                                  const TItemStorage& vectors,
                                                                  const size_t topSize,
                                                                  const TDistance& distance = {},
                                                                  const TDistanceLess& less = {}) {
        TVector<TNeighbor<TDistanceResult>> neighbors;
        neighbors.reserve(vectors.GetNumItems());

        Y_ENSURE(vectors.GetNumItems() >= topSize);

        for (size_t i = 0; i < vectors.GetNumItems(); ++i) {
            neighbors.push_back(TNeighbor<TDistanceResult>{distance(vectors.GetItem(i), query), static_cast<ui32>(i)});
        }

        NthElement(neighbors.begin(), neighbors.begin() + topSize - 1, neighbors.end(),
                   [&less](const TNeighbor<TDistanceResult>& lhs, const TNeighbor<TDistanceResult>& rhs) {
                       return less(lhs.Dist, rhs.Dist);
                   });
        neighbors.resize(topSize);

        return neighbors;
    }

    template <class TDistanceResult>
    size_t CalculateAccuracy(const TVector<TNeighbor<TDistanceResult>>& expected,
                             const TVector<TNeighbor<TDistanceResult>>& actual) {
        THashSet<size_t> expectedIds;
        expectedIds.reserve(expected.size());
        for (const TNeighbor<TDistanceResult>& neighbor : expected)
            UNIT_ASSERT(expectedIds.insert(neighbor.Id).second);

        size_t accuracy = 0;

        THashSet<size_t> actualIds;
        for (const TNeighbor<TDistanceResult>& neighbor : actual) {
            UNIT_ASSERT(actualIds.insert(neighbor.Id).second);
            if (expectedIds.count(neighbor.Id))
                ++accuracy;
        }

        return accuracy;
    }

    template <class TVectorComponent>
    class TDenseVectorStorageSerializer {
    public:
        static void Save(IOutputStream* out, const NHnsw::TDenseVectorStorage<TVectorComponent>& vectors) {
            ::Save(out, static_cast<size_t>(vectors.GetDimension()));

            for (size_t vectorId = 0; vectorId < vectors.GetNumItems(); ++vectorId) {
                const TVectorComponent* const vec = vectors.GetItem(vectorId);
                out->Write(vec, sizeof(TVectorComponent) * vectors.GetDimension());
            }
        }
    };

    template <class TVectorComponent>
    class TDenseVectorStorageDeserializer {
    public:
        static NHnsw::TDenseVectorStorage<TVectorComponent> Load(const TBlob& blob) {
            Y_ENSURE(blob.Length() >= sizeof(size_t));
            const size_t dimension = ReadUnaligned<size_t>(blob.Data());
            return NHnsw::TDenseVectorStorage<TVectorComponent>(blob.SubBlob(sizeof(size_t), blob.Length()), dimension);
        }
    };

    struct TTestOptions {
        size_t Dimension = 50;
        size_t NumVectors = 32 * 10000;
        size_t NumQueries = 100;
        size_t TopSize = 100;

        // Clustered index options
        size_t ClusterSize = 32;
        size_t MaxCandidates = 15000;
        size_t ClusterTopSize = 150;
        size_t ClusterNeighborhoodSize = 180;

        // Results options
        double MinimalMeanAccuracy = -1;

        // Checked if not empty
        TVector<TVector<ui32>> ExpectedOutput;

        ui32 RandomSeed = 17;
        size_t NumThreads = 1;

        bool Verbose = false;
    };

    void FillDefaultHnswOptions(const size_t numThreads, NHnsw::THnswBuildOptions& hnswOptions) {
        hnswOptions.NumThreads = numThreads;
        hnswOptions.MaxNeighbors = 32;
        hnswOptions.BatchSize = 1000;
        hnswOptions.UpperLevelBatchSize = 40000;
        hnswOptions.SearchNeighborhoodSize = 300;
        hnswOptions.NumExactCandidates = 100;
        hnswOptions.LevelSizeDecay = hnswOptions.MaxNeighbors / 2;
        hnswOptions.ReportProgress = false;
    }

    void DoTest(const TTestOptions& opts) {
        using TVectorComponent = float;
        using TBaseDistance = NHnsw::TL2SqrDistance<TVectorComponent>;
        using TDistance = NHnsw::TDistanceWithDimension<TVectorComponent, TBaseDistance>;
        using TItemStorage = NHnsw::TDenseVectorStorage<TVectorComponent>;
        using TItemStorageSerializer = TDenseVectorStorageSerializer<TVectorComponent>;
        using TItemStorageDeserializer = TDenseVectorStorageDeserializer<float>;
        using TNeighbor = TNeighbor<TDistance::TResult>;

        TFastRng64 gen(opts.RandomSeed);
        TItemStorage vectors = NKmeansHnsw::GenerateRandomUniformSpherePoints<TVectorComponent>(opts.NumVectors, opts.Dimension, gen);

        NKmeansHnsw::TBuildKmeansClustersOptions kmeansClustersOptions = {};
        kmeansClustersOptions.NumIterations = 20;
        kmeansClustersOptions.HnswNeighborhoodSize = 16;
        kmeansClustersOptions.NumClusters = vectors.GetNumItems() / opts.ClusterSize;
        kmeansClustersOptions.NumThreads = opts.NumThreads;

        NHnsw::THnswBuildOptions& hnswOpts = kmeansClustersOptions.HnswBuildOptions;
        FillDefaultHnswOptions(opts.NumThreads, hnswOpts);
        hnswOpts.MaxNeighbors = Min<ui32>(32, kmeansClustersOptions.NumClusters - 1);
        hnswOpts.LevelSizeDecay = Max<ui32>(hnswOpts.MaxNeighbors / 2, 2);

        const TDistance distance = TDistance(TBaseDistance(), opts.Dimension);

        NKmeansHnsw::TDenseVectorCentroidsFactory<TVectorComponent> factory(opts.Dimension);
        NKmeansHnsw::TClusteringData<TItemStorage> clusters = NKmeansHnsw::BuildKmeansClusters<TDistance>(vectors, factory, kmeansClustersOptions, distance);

        NHnsw::TDenseVectorStorage<TVectorComponent> queries = NKmeansHnsw::GenerateRandomUniformSpherePoints<TVectorComponent>(opts.NumQueries, opts.Dimension, gen);

        NClusteredHnsw::TClusteredHnswBuildOptions options;
        options.NumThreads = opts.NumThreads;
        FillDefaultHnswOptions(opts.NumThreads, options.HnswOptions);
        options.HnswOptions.MaxNeighbors = Min<ui32>(options.HnswOptions.MaxNeighbors, clusters.Clusters.GetNumItems() - 1);
        options.Verbose = opts.Verbose;
        options.Verbose = true;
        options.NumOverlappingClustersCandidates = 20;
        options.OverlappingClustersNeighborhoodSize = 40;

        NClusteredHnsw::TClusteredHnswIndexData<TItemStorage> data = NClusteredHnsw::BuildIndex<TDistance>(options, vectors, clusters.Clusters, clusters.ClusterIds, distance);

        TBufferOutput buffer;
        NClusteredHnsw::WriteIndex<TItemStorageSerializer>(data, buffer);

        NClusteredHnsw::TClusteredHnswIndexBase<TItemStorage, TItemStorageDeserializer> index(TBlob::NoCopy(buffer.Buffer().Data(), buffer.Buffer().Size()));

        size_t totalAccuracy = 0;

        TVector<TVector<ui32>> resultIds;
        for (size_t queryIndex = 0; queryIndex < opts.NumQueries; ++queryIndex) {
            TVector<TNeighbor> actualResult = index.GetNearestNeighbors(
                queries.GetItem(queryIndex),
                opts.TopSize,
                opts.ClusterTopSize,
                opts.ClusterNeighborhoodSize, vectors, distance);

            TVector<TNeighbor> expectedResult = GetNearestNeighborsStupid(
                queries.GetItem(queryIndex), vectors, opts.TopSize, distance);

            const size_t accuracy = CalculateAccuracy(expectedResult, actualResult);
            totalAccuracy += accuracy;

            TVector<ui32> ids;
            ids.reserve(actualResult.size());
            for (const TNeighbor& neighbor : actualResult)
                ids.push_back(neighbor.Id);

            resultIds.push_back(std::move(ids));
        }

        const double averageAccuracy = totalAccuracy / (double)opts.NumQueries;
        Cerr << "average accuracy: " << averageAccuracy << " found of " << opts.TopSize << " requested" << Endl;

        if (!opts.ExpectedOutput.empty()) {
            Cerr << "This is a precise output test. The actual results are below (ready to replace the test if needed):\n"
                 << Endl;

            Cerr << "const TVector<TVector<ui32>> expectedResults = {" << Endl;
            for (const TVector<ui32>& ids : resultIds) {
                Cerr << "    { ";
                for (const ui32 id : ids)
                    Cerr << id << ", ";
                Cerr << "}," << Endl;
            }
            Cerr << "};\n"
                 << Endl;

            for (size_t queryId = 0; queryId < opts.NumQueries; ++queryId) {
                Sort(resultIds[queryId].begin(), resultIds[queryId].end());
                TVector<ui32> expected = opts.ExpectedOutput[queryId];
                Sort(expected.begin(), expected.end());
                UNIT_ASSERT_EQUAL(resultIds[queryId], expected);
            }
        }

        UNIT_ASSERT(averageAccuracy / (double)opts.TopSize >= opts.MinimalMeanAccuracy);
    }

    TTestOptions MakeTest(const size_t numVectors, const size_t numQueries, const size_t dimension = 50) {
        TTestOptions options;
        options.NumVectors = numVectors;
        options.NumQueries = numQueries;
        options.Dimension = dimension;
        return options;
    }

    Y_UNIT_TEST(TPerfectAccuracyTest) {
        // Num clusters = num vectors = cluster top size
        TTestOptions test = MakeTest(50, 100);

        // The accuracy must be 100%
        test.MinimalMeanAccuracy = 1.0;

        test.ClusterTopSize = 50;
        test.ClusterNeighborhoodSize = test.ClusterTopSize;
        test.ClusterSize = 1;

        for (const size_t topSize : {2, 5, 10, 20}) {
            test.TopSize = topSize;
            DoTest(test);
        }
    }

    Y_UNIT_TEST(TVerySmallTest) {
        TTestOptions test = MakeTest(200, 10);
        test.MinimalMeanAccuracy = 0.5;
        test.ClusterTopSize = 2000 / 32 / 4;
        test.TopSize = 10;
        DoTest(test);
    }

    Y_UNIT_TEST(TSmallExactOutputTest) {
        TTestOptions test = MakeTest(2000, 10);
        test.MinimalMeanAccuracy = 0.5;
        test.ClusterTopSize = 2000 / 32 / 4;
        test.TopSize = 10;

        // The test outputs actual results to Cerr
        // Feel free to replace these if the diff is expected
        const TVector<TVector<ui32>> expectedResults = {
            {
                1616,
                149,
                1755,
                579,
                663,
                2,
                1804,
                1921,
                94,
                1299,
            },
            {
                1824,
                32,
                200,
                713,
                982,
                623,
                1623,
                1217,
                425,
                1255,
            },
            {
                532,
                1212,
                774,
                1367,
                842,
                1429,
                308,
                1093,
                446,
                856,
            },
            {
                172,
                438,
                1820,
                770,
                1067,
                1754,
                1460,
                1270,
                117,
                835,
            },
            {
                929,
                145,
                1250,
                1702,
                557,
                579,
                1211,
                1744,
                312,
                981,
            },
            {
                284,
                1899,
                1138,
                1216,
                776,
                338,
                1616,
                1254,
                1574,
                842,
            },
            {
                424,
                1346,
                1771,
                316,
                818,
                1159,
                33,
                166,
                1805,
                767,
            },
            {
                19,
                1583,
                1277,
                961,
                1051,
                1651,
                714,
                1709,
                1012,
                301,
            },
            {
                310,
                1268,
                713,
                1853,
                1213,
                1832,
                1799,
                1285,
                63,
                1265,
            },
            {
                1508,
                1926,
                1243,
                279,
                1103,
                1666,
                1497,
                828,
                738,
                1761,
            },
        };

        test.ExpectedOutput = expectedResults;
        DoTest(test);
    }

    Y_UNIT_TEST(TRandomSmallTest) {
        TTestOptions test = MakeTest(1000, 200);
        test.MinimalMeanAccuracy = 0.5;
        test.ClusterTopSize = 2000 / 32 / 2;
        DoTest(test);
    }
}
