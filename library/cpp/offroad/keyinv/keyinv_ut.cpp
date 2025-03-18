#include <library/cpp/testing/unittest/registar.h>

#include <random>

#include <library/cpp/offroad/codec/multi_table.h>
#include <library/cpp/offroad/test/test_index.h>
#include <library/cpp/offroad/standard/standard_index_reader.h>
#include <library/cpp/offroad/standard/standard_index_writer.h>
#include <library/cpp/offroad/standard/standard_index_searcher.h>

#include <util/stream/buffer.h>

using namespace NOffroad;

class TSimpleVectorizer {
public:
    enum {
        TupleSize = 2
    };

    template <class Slice>
    void Gather(Slice&& slice, ui64* hit) const {
        *hit = (static_cast<ui64>(slice[0]) << 32) | slice[1];
    }

    template <class Slice>
    void Scatter(ui64 hit, Slice&& slice) const {
        slice[0] = hit >> 32;
        slice[1] = hit;
    }
};

Y_UNIT_TEST_SUITE(TIndexTest) {
    template <class MemoryIndex, class IndexWriter>
    void WriteIndex(const MemoryIndex& index, IndexWriter* writer) {
        for (const auto& pair : index) {
            for (const auto& hit : pair.second)
                writer->WriteHit(hit);
            writer->WriteKey(pair.first);
        }
    }

    template <class MemoryHits, class HitReader>
    void TestHitReader(const MemoryHits& hits, HitReader* reader) {
        typename HitReader::THit hit;
        size_t i = 0;
        while (reader->ReadHit(&hit)) {
            UNIT_ASSERT_VALUES_EQUAL(hits[i], hit);
            i++;
        }
        UNIT_ASSERT_VALUES_EQUAL(i, hits.size());
    }

    template <class MemoryIndex, class IndexReader>
    void TestIndexReader(const MemoryIndex& index, IndexReader* reader) {
        auto pos = index.begin();
        typename IndexReader::TKeyRef key;
        while (reader->ReadKey(&key)) {
            UNIT_ASSERT_UNEQUAL(pos, index.end());
            UNIT_ASSERT_VALUES_EQUAL(pos->first, key);

            TestHitReader(pos->second, reader);
            pos++;
        }
        UNIT_ASSERT_EQUAL(pos, index.end());
    }

    using TIndexSampler0 = TStandardIndexHitSampler<TTestData, TOffsetKeyData, TTestDataVectorizer, TTestDataSubtractor>;
    using TIndexSampler1 = TStandardIndexKeySampler<TTestData, TOffsetKeyData, TTestDataVectorizer, TTestDataSubtractor>;
    using TIndexWriter = TStandardIndexWriter<TTestData, TOffsetKeyData, TTestDataVectorizer, TTestDataSubtractor>;
    using TIndexReader = TStandardIndexReader<TTestData, TOffsetKeyData, TTestDataVectorizer, TTestDataSubtractor>;
    using TIndexSearcher = TStandardIndexSearcher<TTestData, TOffsetKeyData, TTestDataVectorizer, TTestDataSubtractor>;

    Y_UNIT_TEST(TestAll) {
        auto index = MakeTestIndex(1000, 17);

        TIndexSampler0 sampler0;
        WriteIndex(index, &sampler0);
        auto hitTables = NewMultiTable(sampler0.Finish());

        TIndexSampler1 sampler1(hitTables.Get());
        WriteIndex(index, &sampler1);
        auto keyTables = NewMultiTable(sampler1.Finish());

        TBufferStream keyStream, hitStream, fatStream, fatSubStream;

        TIndexWriter writer;
        writer.Reset(hitTables.Get(), keyTables.Get(), &hitStream, &keyStream, &fatStream, &fatSubStream);
        WriteIndex(index, &writer);
        writer.Finish();

        TIndexReader reader;
        reader.Reset(hitTables.Get(), keyTables.Get(), TArrayRef<const char>(hitStream.Buffer().data(), hitStream.Buffer().size()), TArrayRef<const char>(keyStream.Buffer().data(), keyStream.Buffer().size()),
                     TArrayRef<const char>(fatStream.Buffer().data(), fatStream.Buffer().size()), TArrayRef<const char>(fatSubStream.Buffer().data(), fatSubStream.Buffer().size()));
        TestIndexReader(index, &reader);
        reader.Restart();
        TestIndexReader(index, &reader);

        TIndexSearcher searcher;
        searcher.Reset(hitTables.Get(), keyTables.Get(), TArrayRef<const char>(hitStream.Buffer().data(), hitStream.Buffer().size()), TArrayRef<const char>(keyStream.Buffer().data(), keyStream.Buffer().size()),
                       TArrayRef<const char>(fatStream.Buffer().data(), fatStream.Buffer().size()), TArrayRef<const char>(fatSubStream.Buffer().data(), fatSubStream.Buffer().size()));

        TVector<TString> keys;
        for (const auto& pair : index)
            keys.push_back(pair.first);

        std::minstd_rand random(12345);
        std::shuffle(keys.begin(), keys.end(), random);

        TIndexSearcher::TIterator iterator;
        for (const TString& key : keys) {
            for (size_t i = 0; i < 3; i++) {
                bool success = searcher.Find(key, &iterator);
                UNIT_ASSERT(success);

                TestHitReader(index.at(key), &iterator);
            }
        }
    }

    Y_UNIT_TEST(TestLowerBound) {
        TMap<TString, TVector<TTestData>> index;

        index["a"].push_back(1);
        index["b"].push_back(2);

        TIndexSampler0 sampler0;
        WriteIndex(index, &sampler0);
        auto hitTables = NewMultiTable(sampler0.Finish());

        TIndexSampler1 sampler1(hitTables.Get());
        WriteIndex(index, &sampler1);
        auto keyTables = NewMultiTable(sampler1.Finish());

        TBufferStream keyStream, hitStream, fatStream, fatSubStream;

        TIndexWriter writer;
        writer.Reset(hitTables.Get(), keyTables.Get(), &hitStream, &keyStream,
                     &fatStream, &fatSubStream);
        WriteIndex(index, &writer);
        writer.Finish();

        TIndexReader reader;
        reader.Reset(hitTables.Get(), keyTables.Get(), TArrayRef<const char>(hitStream.Buffer().data(), hitStream.Buffer().size()), TArrayRef<const char>(keyStream.Buffer().data(), keyStream.Buffer().size()),
                     TArrayRef<const char>(fatStream.Buffer().data(), fatStream.Buffer().size()), TArrayRef<const char>(fatSubStream.Buffer().data(), fatSubStream.Buffer().size()));

        reader.LowerBound("b");

        bool success;
        TStringBuf key;
        success = reader.ReadKey(&key);
        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL(key, "b");

        TTestData hit;
        success = reader.ReadHit(&hit);
        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL(hit, 2);

        success = reader.ReadHit(&hit);
        UNIT_ASSERT(!success);
    }
}
