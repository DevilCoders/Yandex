#include "offroad_doc_wad_ut.h"

#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/offroad/test/test_md5.h>

using namespace NDoom;

template <>
void Out<TOffroadWadKey>(IOutputStream& out, const TOffroadWadKey& key) {
    out << "TOffroadWadKey { Id = " << key.Id.Id << ", PackedKey = " << key.PackedKey << " }";
}

Y_UNIT_TEST_SUITE(TestOffroadWadDocumentWriter) {

    TString CalcMD5(const TBuffer& buffer) {
        char md5[33];
        TBufferInput input(buffer);
        MD5::Stream(&input, md5);
        return TString(md5);
    }

    Y_UNIT_TEST(TestEmptyIterator) {
        using TSearcher = TOffroadDocWadSearcher<FactorAnnIndexType, TReqBundleHit, TReqBundleHitVectorizer, TReqBundleHitSubtractor, TReqBundleHitPrefixVectorizer>;
        using TIterator = typename TSearcher::TIterator;

        bool success = true;
        TReqBundleHit hit;
        TIterator iterator;

        success = iterator.ReadHit(&hit);
        UNIT_ASSERT(!success);

        success = iterator.LowerBound(hit, &hit);
        UNIT_ASSERT(!success);

        success = iterator.ReadHit(&hit);
        UNIT_ASSERT(!success);

        iterator.Reset();
        success = iterator.ReadHit(&hit);
        UNIT_ASSERT(!success);
    }

    Y_UNIT_TEST(TestSimpleIndex) {
        TIndex index = GenerateSimpleIndex();
        TBuffer buffer = WriteIndex(index);

        //Cerr << "MD5 = " << CalcMD5(buffer) << Endl;
        UNIT_ASSERT_MD5_EQUAL(buffer, "391c88d8e974718fe17e9d3b0625066c");

        THolder<IWad> wad = IWad::Open(TArrayRef<const char>(buffer.data(), buffer.size()));
        TSearcher searcher(wad.Get());

        TSearcher::TIterator iterator;
        TVector<TOffroadWadKey> keys;

        UNIT_ASSERT(searcher.FindTerms("Key1", &iterator, &keys));
        TVector<TOffroadWadKey> expectedKeys = { TOffroadWadKey({1, 0}, "Key1") };
        UNIT_ASSERT_VALUES_EQUAL(expectedKeys, keys);

        UNIT_ASSERT(searcher.Find(0, {1, 0}, &iterator));
        TReqBundleHit hit;
        UNIT_ASSERT(iterator.ReadHit(&hit));
        UNIT_ASSERT_VALUES_EQUAL(TReqBundleHit(1, 2, 13, 1, 3, 0), hit);
        UNIT_ASSERT(!iterator.ReadHit(&hit));

        UNIT_ASSERT(searcher.Find(1, {1, 0}, &iterator));
        UNIT_ASSERT(iterator.ReadHit(&hit));
        UNIT_ASSERT_VALUES_EQUAL(TReqBundleHit(1, 18, 19, 2, 19, 0), hit);
        UNIT_ASSERT(!iterator.ReadHit(&hit));

        keys.clear();
        UNIT_ASSERT(searcher.FindTerms("Key2", &iterator, &keys));
        expectedKeys = { TOffroadWadKey({0, 0}, "Key2") };
        UNIT_ASSERT_VALUES_EQUAL(expectedKeys, keys);

        UNIT_ASSERT(searcher.Find(0, {0, 0}, &iterator));
        UNIT_ASSERT(iterator.ReadHit(&hit));
        UNIT_ASSERT_VALUES_EQUAL(TReqBundleHit(0, 1, 1, 0, 1, 0), hit);
        UNIT_ASSERT(!iterator.ReadHit(&hit));

        UNIT_ASSERT(!searcher.Find(1, {0, 0}, &iterator));

        keys.clear();
        UNIT_ASSERT(searcher.FindTerms("Lalala", &iterator, &keys));
        expectedKeys = { TOffroadWadKey({2, 0}, "Lalala") };
        UNIT_ASSERT_VALUES_EQUAL(expectedKeys, keys);

        UNIT_ASSERT(!searcher.Find(0, {2, 0}, &iterator));

        UNIT_ASSERT(searcher.Find(1, {2, 0}, &iterator));
        UNIT_ASSERT(iterator.ReadHit(&hit));
        UNIT_ASSERT_VALUES_EQUAL(TReqBundleHit(2, 13, 15, 3, 10, 0), hit);
        UNIT_ASSERT(iterator.ReadHit(&hit));
        UNIT_ASSERT_VALUES_EQUAL(TReqBundleHit(2, 14, 7, 1, 123, 0), hit);
        UNIT_ASSERT(!iterator.ReadHit(&hit));

        keys.clear();
        UNIT_ASSERT(!searcher.FindTerms("Azaza", &iterator, &keys));
    }

    void TestSearcher(const TIndex& index, size_t docsCount, const TBuffer& buffer) {
        THolder<IWad> wad = IWad::Open(TArrayRef<const char>(buffer.data(), buffer.size()));
        TSearcher searcher(wad.Get());
        TSearcher::TIterator iterator;

        TVector<std::pair<TString, ui32>> keys;
        for (size_t i = 0; i < index.Keys.size(); ++i) {
            keys.push_back(index.Keys[i]);
        }
        Shuffle(keys.begin(), keys.end());

        THashMap<ui32, size_t> indexByDocId;
        for (size_t i = 0; i < index.DocumentWithHits.size(); ++i) {
            indexByDocId[index.DocumentWithHits[i].first] = i;
        }

        for (const auto& key : keys) {
            TVector<TOffroadWadKey> expectedKeys = { TOffroadWadKey({key.second, 0}, key.first) };

            TVector<TOffroadWadKey> searcherKeys;
            UNIT_ASSERT(searcher.FindTerms(key.first, &iterator, &searcherKeys));
            UNIT_ASSERT_VALUES_EQUAL(expectedKeys, searcherKeys);

            for (const auto& offroadKey : searcherKeys) {
                TVector<ui32> docIds(docsCount);
                Iota(docIds.begin(), docIds.end(), 0);
                Shuffle(docIds.begin(), docIds.end());
                for (ui32 docId : docIds) {
                    if (indexByDocId.find(docId) == indexByDocId.end()) {
                        UNIT_ASSERT(!searcher.Find(docId, offroadKey.Id, &iterator));
                    } else {
                        TVector<TReqBundleHit> expectedHits;
                        for (const auto& hit : index.DocumentWithHits[indexByDocId[docId]].second) {
                            if (hit.DocId() == offroadKey.Id.Id) {
                                expectedHits.push_back(hit);
                            }
                        }
                        UNIT_ASSERT(searcher.Find(docId, offroadKey.Id, &iterator) == !expectedHits.empty());
                        if (expectedHits.empty()) {
                            continue;
                        }
                        TVector<TReqBundleHit> hits;
                        TReqBundleHit hit;
                        while (iterator.ReadHit(&hit)) {
                            hits.push_back(hit);
                        }
                        UNIT_ASSERT_VALUES_EQUAL(expectedHits, hits);
                    }
                }
            }
        }
    }

    Y_UNIT_TEST(TestEmptyDocs) {
        for (size_t docsCount = 0; docsCount <= 8; ++docsCount) {
            TIndex index = GenerateRandomIndex(13, docsCount);
            for (size_t docsMask = 0; docsMask < (1 << docsCount); ++docsMask) {
                TIndex index2;
                index2.Keys = index.Keys;
                ui32 maxDoc = 0;
                for (size_t doc = 0; doc < docsCount; ++doc) {
                    if ((docsMask & (1 << doc))) {
                        index2.DocumentWithHits.push_back(index.DocumentWithHits[doc]);
                        maxDoc = doc;
                    }
                }
                TBuffer buffer = WriteIndex(index2);
                TestSearcher(index2, (docsMask == 0 ? 0 : maxDoc + 1), buffer);
            }
        }
    }

    Y_UNIT_TEST(TestRandomIndex) {
        TIndex index = GenerateRandomIndex(43, 4243);
        TBuffer buffer = WriteIndex(index);
        TestSearcher(index, 4243, buffer);
    }

}

Y_UNIT_TEST_SUITE(TestOffroadWadDocumentReader) {
    Y_UNIT_TEST(TestReadHits) {
        using TReader = TOffroadDocWadReader<FactorAnnIndexType, TReqBundleHit, TReqBundleHitVectorizer, TReqBundleHitSubtractor, TReqBundleHitPrefixVectorizer>;
        using THit = TReader::THit;

        TIndex index = GenerateRandomIndex(43, 4243);
        TBuffer buffer = WriteIndex(index);
        THolder<IWad> wad = IWad::Open(TArrayRef<const char>(buffer.data(), buffer.size()));

        TReader reader(wad.Get());
        ui32 docId = 0;

        size_t i = 0;
        while (reader.ReadDoc(&docId)) {
            while (i < index.DocumentWithHits.size() && index.DocumentWithHits[i].second.empty()) {
                ++i;
            }
            UNIT_ASSERT(i < index.DocumentWithHits.size());
            UNIT_ASSERT_VALUES_EQUAL(docId, index.DocumentWithHits[i].first);
            size_t j = 0;
            while (reader.ReadHits([&](const THit& hit) {
                UNIT_ASSERT(j < index.DocumentWithHits[i].second.size());
                UNIT_ASSERT_VALUES_EQUAL(index.DocumentWithHits[i].second[j], hit);
                ++j;
                return true;
            })) {}
            ++i;
        }
    }
}
