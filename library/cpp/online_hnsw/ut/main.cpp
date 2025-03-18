#include <library/cpp/hnsw/index/index_base.h>
#include <library/cpp/hnsw/index/index_item_storage_base.h>
#include <library/cpp/hnsw/index/dense_vector_distance.h>
#include <library/cpp/hnsw/index/dense_vector_index.h>
#include <library/cpp/online_hnsw/base/item_storage_index.h>
#include <library/cpp/online_hnsw/base/index_reader.h>
#include <library/cpp/online_hnsw/base/index_writer.h>
#include <library/cpp/online_hnsw/dense_vectors/index.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>
#include <util/generic/ymath.h>
#include <util/memory/blob.h>
#include <util/random/fast.h>
#include <util/random/shuffle.h>
#include <util/stream/buffer.h>
#include <util/generic/hash_set.h>

template <class TDistance, class TNeighbors>
void CheckNeighborsSorted(const TNeighbors& neighbors) {
    using TLess = typename TDistance::TLess;
    for (size_t pos = 1; pos < neighbors.size(); ++pos) {
        UNIT_ASSERT(!TLess()(neighbors[pos].Dist, neighbors[pos - 1].Dist));
    }
}

Y_UNIT_TEST_SUITE(TOnlineHnswTest) {
    Y_UNIT_TEST(TEmptyIndex) {
        struct TDistance {
            using TResult = int;
            using TLess = ::TLess<TResult>;
            TResult operator()(int a, int b) const {
                return Abs(a - b);
            }
        };

        class TOnlineHnswIndex: public NOnlineHnsw::TOnlineHnswItemStorageIndexBase<TOnlineHnswIndex, int, TDistance> {
            using TBase = NOnlineHnsw::TOnlineHnswItemStorageIndexBase<TOnlineHnswIndex, int, TDistance>;
        public:
            using TItem = int;

            using TBase::TBase;

            TItem GetItem(size_t id) const {
                return Items[id];
            }
            size_t GetNumItems() const {
                return Items.size();
            }
            void AddItem(const TItem& item) {
                Items.push_back(item);
            }

        private:
            TVector<TItem> Items;
        };

        NOnlineHnsw::TOnlineHnswBuildOptions opts;
        opts.MaxNeighbors = 2;
        opts.SearchNeighborhoodSize = 15;
        opts.LevelSizeDecay = 2;

        TOnlineHnswIndex onlineIndex(opts);

        for (size_t item = 0; item < 10; ++item) {
            auto neighbors = onlineIndex.GetNearestNeighbors(item);
            UNIT_ASSERT(neighbors.empty());
        }

        NOnlineHnsw::TOnlineHnswIndexData indexData = onlineIndex.ConstructIndexData();

        TBufferOutput out;
        NOnlineHnsw::WriteIndex(indexData, out);
        TBlob indexBlob(TBlob::FromBuffer(out.Buffer()));

        using TItemStorage = TOnlineHnswIndex;

        class THnswIndex: public NHnsw::THnswItemStorageIndexBase<THnswIndex> {
            using TBase = NHnsw::THnswItemStorageIndexBase<THnswIndex>;

        public:
            THnswIndex(const TBlob& indexBlob, const TItemStorage& itemStorage)
                : TBase(indexBlob, NOnlineHnsw::TOnlineHnswIndexReader())
                , ItemStorage(itemStorage)
            {
            }
            int GetItem(ui32 id) const {
                return ItemStorage.GetItem(id);
            }
            size_t GetNumItems() const {
                return ItemStorage.GetNumItems();
            }

        private:
            const TItemStorage& ItemStorage;
        };

        THnswIndex staticIndex(indexBlob, onlineIndex);

        const size_t topSize = 1;
        const size_t searchNeighborhoodSize = 1;

        for (int item = 0; item < 10; ++item) {
            auto neighbors = staticIndex.GetNearestNeighbors<TDistance>(item, topSize, searchNeighborhoodSize);
            UNIT_ASSERT(neighbors.empty());
        }
    }


    Y_UNIT_TEST(TSkipListExample) {
        struct TDistance {
            using TResult = int;
            using TLess = ::TLess<TResult>;
            TResult operator()(int a, int b) const {
                return Abs(a - b);
            }
        };

        class TOnlineHnswIndex: public NOnlineHnsw::TOnlineHnswItemStorageIndexBase<TOnlineHnswIndex, int, TDistance> {
            using TBase = NOnlineHnsw::TOnlineHnswItemStorageIndexBase<TOnlineHnswIndex, int, TDistance>;
        public:
            using TItem = int;

            using TBase::TBase;

            TItem GetItem(size_t id) const {
                return Items[id];
            }
            size_t GetNumItems() const {
                return Items.size();
            }
            void AddItem(const TItem& item) {
                Items.push_back(item);
            }

        private:
            TVector<TItem> Items;
        };

        NOnlineHnsw::TOnlineHnswBuildOptions opts;
        opts.MaxNeighbors = 2;
        opts.SearchNeighborhoodSize = 15;
        opts.LevelSizeDecay = 2;

        TOnlineHnswIndex onlineIndex(opts);

        TVector<int> permutation(2000);
        Iota(permutation.begin(), permutation.end(), 0);
        TFastRng<ui64> rng(0);
        Shuffle(permutation.begin(), permutation.end(), rng);

        for (size_t itemId = 0; itemId < permutation.size(); ++itemId) {
            auto neighbors = onlineIndex.GetNearestNeighborsAndAddItem(permutation[itemId]);

            CheckNeighborsSorted<TDistance>(neighbors);
            UNIT_ASSERT_EQUAL(neighbors.size(), Min(opts.SearchNeighborhoodSize, itemId));
            if (neighbors.size() == 0) {
                continue;
            }

            TDistance::TResult maxDistance = neighbors.back().Dist;
            THashSet<size_t> takenItems;
            for (const auto& neighbor : neighbors) {
                takenItems.insert(neighbor.Id);
            }
            for (size_t notTakenId = 0; notTakenId < itemId; ++notTakenId) {
                if (takenItems.contains(notTakenId)) {
                    continue;
                }
                const auto distance = TDistance()(permutation[itemId], permutation[notTakenId]);
                UNIT_ASSERT_C(!TDistance::TLess()(distance, maxDistance), "closer neighbor exists");
            }
        }

        NOnlineHnsw::TOnlineHnswIndexData indexData = onlineIndex.ConstructIndexData();

        TBufferOutput out;
        NOnlineHnsw::WriteIndex(indexData, out);
        TBlob indexBlob(TBlob::FromBuffer(out.Buffer()));

        using TItemStorage = TOnlineHnswIndex;

        class THnswIndex: public NHnsw::THnswItemStorageIndexBase<THnswIndex> {
            using TBase = NHnsw::THnswItemStorageIndexBase<THnswIndex>;

        public:
            using TNeighbor = TBase::TNeighbor<int>;

            THnswIndex(const TBlob& indexBlob, const TItemStorage& itemStorage)
                : TBase(indexBlob, NOnlineHnsw::TOnlineHnswIndexReader())
                , ItemStorage(itemStorage)
            {
            }
            int GetItem(ui32 id) const {
                return ItemStorage.GetItem(id);
            }
            size_t GetNumItems() const {
                return ItemStorage.GetNumItems();
            }

        private:
            const TItemStorage& ItemStorage;
        };

        THnswIndex staticIndex(indexBlob, onlineIndex);

        const size_t topSize = 1;
        const size_t searchNeighborhoodSize = 1;

        const int maxItem = staticIndex.GetNumItems() - 1;
        for (int item = 0; item < maxItem + 100; ++item) {
            const int expectedItem = Min(item, maxItem);
            const int expectedDist = Abs(item - expectedItem);

            TVector<THnswIndex::TNeighbor> bestMatch =
                staticIndex.GetNearestNeighbors<TDistance>(item, topSize, searchNeighborhoodSize);

            UNIT_ASSERT_EQUAL(bestMatch.size(), 1);
            const int resultItem = staticIndex.GetItem(bestMatch[0].Id);
            UNIT_ASSERT_EQUAL(resultItem, expectedItem);
            const int resultDist = bestMatch[0].Dist;
            UNIT_ASSERT_EQUAL(resultDist, expectedDist);
        }
    }


    Y_UNIT_TEST(TDenseVectorsExample) {
        using TVectorComponent = int;

        size_t dimension = 20;
        size_t numVectors = 2000;

        TVector<int> permutation(dimension);
        Iota(permutation.begin(), permutation.end(), 0);
        TFastRng<ui64> rng(0);

        NOnlineHnsw::TOnlineHnswBuildOptions opts = {
            32, // MaxNeighbors
            300 // SearchNeighborhoodSize
            // LevelSizeDecay and NumberOfVertices are optional
        };
        NOnlineHnsw::TOnlineHnswDenseVectorIndex<TVectorComponent, NHnsw::TL1Distance<TVectorComponent>> onlineIndex(opts, dimension);
        const auto& storage = onlineIndex;
        using TDistance = NHnsw::TDistanceWithDimension<TVectorComponent,
                                                        NHnsw::TL1Distance<TVectorComponent>>;
        auto distance = TDistance(NHnsw::TL1Distance<TVectorComponent>(), dimension);

        size_t fails = 0;
        for (size_t itemId = 0; itemId < numVectors; ++itemId) {
            Shuffle(permutation.begin(), permutation.end(), rng);
            auto neighbors = onlineIndex.GetNearestNeighborsAndAddItem(permutation.data());
            UNIT_ASSERT_EQUAL(neighbors.size(), Min(opts.SearchNeighborhoodSize, itemId));
            CheckNeighborsSorted<TDistance>(neighbors);
            if (neighbors.size() == 0) {
                continue;
            }

            for (size_t neighborId = 0; neighborId < itemId; ++neighborId) {
                auto neighborDistance = distance(storage.GetItem(neighborId), storage.GetItem(itemId));
                fails += TDistance::TLess()(neighborDistance, neighbors[0].Dist);
            }
        }
        UNIT_ASSERT(double(fails) / numVectors < 0.1);

        auto indexData = onlineIndex.ConstructIndexData();

        TBufferOutput out;
        NOnlineHnsw::WriteIndex(indexData, out);
        out.Flush();
        TBlob indexBlob = TBlob::FromBuffer(out.Buffer());
        TBlob vectorsBlob = TBlob::NoCopy(storage.GetData(), storage.GetSize());
        NHnsw::THnswDenseVectorIndex<TVectorComponent> staticIndex(indexBlob,
                                                                   vectorsBlob,
                                                                   dimension,
                                                                   NOnlineHnsw::TOnlineHnswIndexReader());
        fails = 0;
        for (size_t itemId = 0; itemId < numVectors; ++itemId) {
            const auto& expectedItem = storage.GetItem(itemId);
            auto found = staticIndex.GetNearestNeighbors<NHnsw::TL1Distance<TVectorComponent>>(expectedItem,
                                                                                   /*topSize*/ 1,
                                                                     /*searchNeighborhoodSize*/1);
            UNIT_ASSERT_EQUAL(found.size(), 1);

            const auto& foundItem = storage.GetItem(found[0].Id);
            fails += distance(expectedItem, foundItem) != 0;
        }
        UNIT_ASSERT(double(fails) / numVectors < 0.1);
    }
}
