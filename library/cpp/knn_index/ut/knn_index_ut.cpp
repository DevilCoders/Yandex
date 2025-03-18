#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/knn_index/ut/user_index_info.pb.h>

#include <library/cpp/knn_index/searcher.h>
#include <library/cpp/knn_index/writer.h>

#include <util/generic/hash.h>
#include <util/random/mersenne.h>
#include <util/stream/buffer.h>

#include <utility>

Y_UNIT_TEST_SUITE(TKnnIndexTest) {
    template <class TFeatureType>
    TVector<TFeatureType> GenerateRandomPoint(size_t dimension) {
        static TMersenne<ui32> rng;
        TVector<TFeatureType> point(dimension);
        for (size_t i = 0; i < dimension; ++i) {
            point[i] = (TFeatureType)rng();
        }
        return point;
    }

    template <class TFeatureType>
    struct TItem {
        ui32 DocId = 0;
        ui32 ItemId = 0;
        TVector<TFeatureType> Item;
        TItem(ui32 docId, ui32 itemId, const TVector<TFeatureType>& item)
            : DocId(docId)
            , ItemId(itemId)
            , Item(item)
        {
        }
    };

    template <class TFeatureType>
    struct TDocument {
        ui32 DocumentId = 0;
        TVector<TItem<TFeatureType>> Items;
        TDocument(ui32 docId, const TVector<TItem<TFeatureType>>& items)
            : DocumentId(docId)
            , Items(items)
        {
        }
        TDocument(ui32 docId, const TVector<TFeatureType>& item)
            : DocumentId(docId)
            , Items(1, TItem<TFeatureType>(0, item))
        {
        }
        TDocument() = default;
    };

    template <class TFeatureType>
    struct TCluster {
        TVector<TFeatureType> Centroid;
        TVector<TItem<TFeatureType>> Items;
    };

    /**
     * Check that searcher contains the same data as it was in input.
     * Also check that all the data is 16-byte aligned.
     */
    template <class TFeatureType>
    void CheckEqual(const NKNNIndex::TSearcher<TFeatureType>& searcher, const TVector<TCluster<TFeatureType>>& clusters) {
        size_t dimension = searcher.Dimension();
        UNIT_ASSERT_EQUAL(searcher.ClusterCount(), clusters.size());
        for (size_t clId = 0; clId < searcher.ClusterCount(); ++clId) {
            auto cluster = searcher.Cluster(clId);
            UNIT_ASSERT_EQUAL(cluster.ItemCount(), clusters[clId].Items.size());
            UNIT_ASSERT_EQUAL(0, memcmp(cluster.Centroid(), clusters[clId].Centroid.data(), dimension * sizeof(TFeatureType)));
            UNIT_ASSERT_EQUAL(0, size_t(cluster.Centroid()) % NKNNIndex::DataAlignment);
            UNIT_ASSERT_EQUAL(dimension, cluster.Dimension());
            TVector<TItem<TFeatureType>> items = clusters[clId].Items;
            TVector<TItem<TFeatureType>> sItems;
            for (size_t i = 0; i < cluster.ItemCount(); ++i) {
                auto item = cluster.Item(i);
                UNIT_ASSERT_EQUAL(0, size_t(item.Item()) % NKNNIndex::DataAlignment);
                sItems.emplace_back(item.DocumentId(), item.ItemId(), TVector<TFeatureType>(item.Item(), item.Item() + item.Dimension()));
            }
            Sort(items.begin(), items.end(), [&](const auto& l, const auto& r) {
                return std::make_tuple(l.DocId, l.ItemId) < std::make_tuple(r.DocId, r.ItemId);
            });
            Sort(sItems.begin(), sItems.end(), [&](const auto& l, const auto& r) {
                return std::make_tuple(l.DocId, l.ItemId) < std::make_tuple(r.DocId, r.ItemId);
            });
            UNIT_ASSERT_EQUAL(items.size(), sItems.size());
            for (size_t i = 0; i < items.size(); ++i) {
                UNIT_ASSERT_EQUAL(items[i].DocId, sItems[i].DocId);
                UNIT_ASSERT_EQUAL(items[i].ItemId, sItems[i].ItemId);
                UNIT_ASSERT_EQUAL(0, memcmp(items[i].Item.data(), sItems[i].Item.data(), items[i].Item.size() * sizeof(TFeatureType)));
            }
        }
    }

    template <class TFeatureType>
    TBuffer WriteIndex(const TVector<TCluster<TFeatureType>>& clusters, size_t dimension) {
        TBufferOutput output;
        NKNNIndex::TWriter<TFeatureType> writer(dimension, &output);
        for (const auto& cluster : clusters) {
            for (const auto& item : cluster.Items) {
                writer.WriteItem(item.DocId, item.ItemId, item.Item.data());
            }
            writer.WriteCluster(cluster.Centroid.data());
        }
        writer.Finish();
        return output.Buffer();
    }

    template <class TFeatureType>
    void CheckSmallWriteRead(size_t dimension) {
        TVector<TCluster<TFeatureType>> clusters;
        for (size_t i = 0; i < 10; ++i) {
            TCluster<TFeatureType> cluster;
            cluster.Centroid = GenerateRandomPoint<TFeatureType>(dimension);
            for (size_t j = 0; j < 10; ++j) {
                for (size_t k = 0; k < 3; ++k) {
                    cluster.Items.push_back(TItem<TFeatureType>(
                        i * 100 + j, i + j + k, GenerateRandomPoint<TFeatureType>(dimension)));
                }
            }
            clusters.push_back(cluster);
        }
        TBuffer buffer = WriteIndex(clusters, dimension);
        NKNNIndex::TSearcher<TFeatureType> searcher(TBlob::NoCopy(buffer.data(), buffer.size()));
        CheckEqual(searcher, clusters);
    }

    Y_UNIT_TEST(TSmallWriteReadTest) {
        for (size_t dimension = 1; dimension <= 64; ++dimension) {
            CheckSmallWriteRead<ui8>(dimension);
            CheckSmallWriteRead<ui16>(dimension);
            CheckSmallWriteRead<float>(dimension);
            CheckSmallWriteRead<i8>(dimension);
        }
    }

    Y_UNIT_TEST(TCheckAccessByDocId) {
        const size_t dimension = 10;
        TVector<TItem<ui8>> documents;
        TBufferOutput output;
        NKNNIndex::TWriter<ui8> writer(dimension, &output);
        for (size_t i = 0; i < 10; ++i) {
            TItem<ui8> item(/*docId=*/i * 10, 0, GenerateRandomPoint<ui8>(dimension));
            writer.WriteItem(item.DocId, 0, item.Item.data());
            documents.push_back(item);
        }
        writer.Finish();

        NKNNIndex::TSearcher<ui8> searcher(TBlob::NoCopy(output.Buffer().data(), output.Buffer().size()));
        for (size_t i = 0; i < 10; ++i) {
            const ui32 docId = i * 10;
            auto document = searcher.Document(docId);
            UNIT_ASSERT_EQUAL(document.ItemCount(), 1);
            UNIT_ASSERT_EQUAL(0, memcmp(document.Item(0), documents[i].Item.data(), dimension));
        }
        auto document = searcher.Document(1);
        UNIT_ASSERT_EQUAL(document.ItemCount(), 0);
        auto document2 = searcher.Document(1000);
        UNIT_ASSERT_EQUAL(document2.ItemCount(), 0);
    }

    Y_UNIT_TEST(TCheckComparisonCorrectness) {
        TVector<TItem<int>> docs;
        docs.push_back(TItem<int>(0, 0, TVector<int>{1, 1}));
        docs.push_back(TItem<int>(1, 0, TVector<int>{-1, 1}));
        docs.push_back(TItem<int>(2, 0, TVector<int>{-1, -1}));
        docs.push_back(TItem<int>(3, 0, TVector<int>{1, -1}));
        TBufferOutput output;
        NKNNIndex::TWriter<int> writer(2, &output);
        for (size_t i = 0; i < 4; ++i) {
            writer.WriteItem(i, 0, docs[i].Item.data());
            writer.WriteCluster(docs[i].Item.data());
        }
        writer.Finish();

        NKNNIndex::TSearcher<int> searcher(TBlob::NoCopy(output.Buffer().data(), output.Buffer().size()));
        for (size_t i = 0; i < 4; ++i) {
            const auto top = searcher.FindNearestItems<NKNNIndex::EDistanceType::L1Distance>(
                docs[i].Item.data(), 1, 1);
            UNIT_ASSERT_EQUAL(top.size(), 1);
            UNIT_ASSERT_EQUAL(top[0].DocumentId(), i);
            UNIT_ASSERT_EQUAL(top[0].Distance(), 0);

            const auto top1 = searcher.FindNearestItems<NKNNIndex::EDistanceType::L2Distance>(
                docs[i].Item.data(), 1, 1);
            UNIT_ASSERT_EQUAL(top1.size(), 1);
            UNIT_ASSERT_EQUAL(top1[0].DocumentId(), i);
            UNIT_ASSERT_EQUAL(top1[0].Distance(), 0);

            const auto top2 = searcher.FindNearestItems<NKNNIndex::EDistanceType::L2SqrDistance>(
                docs[i].Item.data(), 1, 1);
            UNIT_ASSERT_EQUAL(top2.size(), 1);
            UNIT_ASSERT_EQUAL(top2[0].DocumentId(), i);
            UNIT_ASSERT_EQUAL(top2[0].Distance(), 0);

            const auto top3 = searcher.FindNearestItems<NKNNIndex::EDistanceType::DotProduct>(
                docs[i].Item.data(), 1, 1);
            UNIT_ASSERT_EQUAL(top3.size(), 1);
            UNIT_ASSERT_EQUAL(top3[0].DocumentId(), i);
            UNIT_ASSERT_EQUAL(top3[0].Distance(), 2);
        }
    }

    Y_UNIT_TEST(TCheckUserIndexInfo) {
        TVector<int> data{0, 1, 2, 3, 4, 5};
        TTestUserIndexInfo info;
        info.SetVersion(32);
        info.SetSomeNumber(55);
        TBufferOutput output;
        NKNNIndex::TWriter<int, TTestUserIndexInfo> writer(data.size(), &output, info);
        writer.WriteItem(20, 0, data.data());
        writer.WriteCluster(data.data());
        writer.Finish();

        NKNNIndex::TSearcher<int, TTestUserIndexInfo> searcher(TBlob::NoCopy(output.Buffer().data(), output.Buffer().size()));
        UNIT_ASSERT(searcher.UserIndexInfo().HasVersion());
        UNIT_ASSERT_EQUAL(searcher.UserIndexInfo().GetVersion(), 32);
        UNIT_ASSERT(searcher.UserIndexInfo().HasSomeNumber());
        UNIT_ASSERT_EQUAL(searcher.UserIndexInfo().GetSomeNumber(), 55);
    }

    Y_UNIT_TEST(TCheckNearestDistances) {
        size_t dimension = 100;
        TCluster<i8> cluster;
        cluster.Centroid = GenerateRandomPoint<i8>(dimension);
        for (size_t i = 0; i < 1000; ++i) {
            cluster.Items.push_back(TItem<i8>(i, i, GenerateRandomPoint<i8>(dimension)));
        }
        TBuffer buffer = WriteIndex(TVector<TCluster<i8>>(1, cluster), dimension);
        NKNNIndex::TSearcher<i8> searcher(TBlob::NoCopy(buffer.data(), buffer.size()));
        for (size_t i = 0; i < 10; ++i) {
            TVector<i8> query = GenerateRandomPoint<i8>(dimension);
            TVector<i32> searchDistances;
            for (const auto& item : searcher.FindNearestItems<NKNNIndex::EDistanceType::DotProduct>(
                     query.data(), searcher.ClusterCount(), 10)) {
                searchDistances.push_back(item.Distance());
            }
            TVector<i32> bestDistances;
            for (size_t j = 0; j < cluster.Items.size(); ++j) {
                bestDistances.push_back(DotProduct(query.data(), cluster.Items[j].Item.data(), dimension));
            }
            std::sort(bestDistances.rbegin(), bestDistances.rend());
            bestDistances.resize(10);
            UNIT_ASSERT_EQUAL(bestDistances, searchDistances);
        }
    }
}
