#include <library/cpp/hnsw/index/index_item_storage_base.h>

#include <library/cpp/hnsw/index_builder/index_builder.h>
#include <library/cpp/hnsw/index_builder/index_writer.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>
#include <util/generic/ymath.h>
#include <util/memory/blob.h>
#include <util/random/fast.h>
#include <util/random/shuffle.h>
#include <util/stream/buffer.h>

Y_UNIT_TEST_SUITE(THnswTest) {
    NHnsw::THnswBuildOptions SkipListBuildOptions() {
        NHnsw::THnswBuildOptions opts;
        opts.MaxNeighbors = 2;
        opts.BatchSize = 31;
        opts.UpperLevelBatchSize = 63;
        opts.SearchNeighborhoodSize = 15;
        opts.NumExactCandidates = 17;
        opts.NumThreads = 3;
        // LevelSizeDecay set to 2 means that each level will contain at most half of items
        // of the previous level. In that way our index will represent an ordinary skip-list.
        opts.LevelSizeDecay = 2;
        return opts;
    }

    void CheckIndexData(const NHnsw::THnswBuildOptions& opts,
                        size_t numItems,
                        const NHnsw::THnswIndexData& indexData) {
        UNIT_ASSERT_EQUAL(indexData.NumItems, numItems);
        UNIT_ASSERT_EQUAL(indexData.MaxNeighbors, opts.MaxNeighbors);
        UNIT_ASSERT_EQUAL(indexData.LevelSizeDecay, opts.LevelSizeDecay);
        size_t expectedSize = 0;
        for (; numItems > 1; numItems /= opts.LevelSizeDecay) {
            size_t numNeighbors = Min(opts.MaxNeighbors, numItems - 1);
            expectedSize += numItems * numNeighbors;
        }
        UNIT_ASSERT_EQUAL(indexData.FlatLevels.size(), expectedSize);
    }

    void CheckWriteRead(const NHnsw::THnswIndexData& indexData, const TBlob& indexBlob) {
        const ui32* data = reinterpret_cast<const ui32*>(indexBlob.Begin());
        const ui32* dataEnd = reinterpret_cast<const ui32*>(indexBlob.End());

        ui32 numItems = *data++;
        UNIT_ASSERT_EQUAL(numItems, indexData.NumItems);
        ui32 maxNeighbors = *data++;
        UNIT_ASSERT_EQUAL(maxNeighbors, indexData.MaxNeighbors);
        ui32 levelSizeDecay = *data++;
        UNIT_ASSERT_EQUAL(levelSizeDecay, indexData.LevelSizeDecay);

        UNIT_ASSERT_EQUAL(static_cast<size_t>(dataEnd - data), indexData.FlatLevels.size());
        UNIT_ASSERT_EQUAL(std::memcmp(data, indexData.FlatLevels.data(), indexData.FlatLevels.size() * sizeof(ui32)), 0);
    }

    // This particular test contains everything you need
    // to build your own indexes with custom items and custom metrics.
    Y_UNIT_TEST(TSkipListExample) {
        // First, you need to build your index.
        // In order to do so you start with defining storage for your items.
        class TItemStorage {
        public:
            // Define a type for item in index
            using TItem = int;
            TItemStorage()
                : Items(10113)
            {
                std::iota(Items.begin(), Items.end(), 0);
                TFastRng<ui64> rng(0);
                Shuffle(Items.begin(), Items.end(), rng);
            }
            // The following two methods are mandatory for TItemStorage.
            // Provides access to items. Typically it should return TItem or const TItem&.
            int GetItem(size_t id) const {
                return Items[id];
            }
            // Just the number of items in index.
            size_t GetNumItems() const {
                return Items.size();
            }

        private:
            TVector<int> Items;
        };
        TItemStorage itemStorage;

        // Then you proceed with defining custom metric.
        struct TDistance {
            // The return type of your custom distance.
            using TResult = int;
            // The comparison operator for TResult.
            // Typically is ::TLess<TResult>: the smaller the distance the closer the items are.
            using TLess = ::TLess<TResult>;
            // Operator that is called to evaluate distance between two items.
            // Typically has signature:
            // TResult operator()(const TItemStorage::TItem& a, const TItemStorage::TItem& b)
            TResult operator()(int a, int b) const {
                return Abs(a - b);
            }
        };

        // Now you're all set to build index.
        auto opts = SkipListBuildOptions();
        NHnsw::THnswIndexData indexData = NHnsw::BuildIndex<TDistance>(opts, itemStorage);

        CheckIndexData(opts, itemStorage.GetNumItems(), indexData);

        // After that you should save your index.
        TBufferOutput out;
        NHnsw::WriteIndex(indexData, out);

        // Index is saved as a memory blob.
        TBlob indexBlob(TBlob::FromBuffer(out.Buffer()));

        CheckWriteRead(indexData, indexBlob);

        // You are almost ready to perform searches.
        // In order to do so you should define your custom index class.
        // THnswIndex will inherit GetNearestNeighbors method from the base class.
        class THnswIndex: public NHnsw::THnswItemStorageIndexBase<THnswIndex> {
            using TBase = NHnsw::THnswItemStorageIndexBase<THnswIndex>;

        public:
            using TNeighbor = TBase::TNeighbor<int>;

            THnswIndex(const TBlob& indexBlob, const TItemStorage& itemStorage)
                : TBase(indexBlob) // The base is initialized from a memory blob
                , ItemStorage(itemStorage)
            {
            }
            // In order to perform searches THnswIndex must implement only one method - GetItem.
            int GetItem(ui32 id) const {
                return ItemStorage.GetItem(id);
            }
            // This method is not necessary for querying index.
            // It's provided here for the sake of completeness of THnswIndex class.
            size_t GetNumItems() const {
                return ItemStorage.GetNumItems();
            }

        private:
            TItemStorage ItemStorage;
        };
        THnswIndex index(indexBlob, itemStorage);

        // We want to retieve only the closest item from index.
        const size_t topSize = 1;
        // In this exapmle we expect to find the closest item using hierarchical structure of index,
        // while examining the smallest neighborhood possible.
        const size_t searchNeighborhoodSize = 1;

        const int maxItem = index.GetNumItems() - 1;
        for (int item = 0; item < maxItem + 100; ++item) {
            const int expectedItem = Min(item, maxItem);
            const int expectedDist = Abs(item - expectedItem);

            // Ready, set, search!
            TVector<THnswIndex::TNeighbor> bestMatch = index.GetNearestNeighbors<TDistance>(item, topSize, searchNeighborhoodSize);

            UNIT_ASSERT_EQUAL(bestMatch.size(), 1);
            const int resultItem = index.GetItem(bestMatch[0].Id);
            UNIT_ASSERT_EQUAL(resultItem, expectedItem);
            const int resultDist = bestMatch[0].Dist;
            UNIT_ASSERT_EQUAL(resultDist, expectedDist);
        }
    }
}
