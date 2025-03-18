#include <library/cpp/testing/unittest/registar.h>

#include <random>
#include <algorithm>

#include <util/generic/map.h>
#include <util/stream/buffer.h>

#include <library/cpp/offroad/codec/multi_table.h>
#include <library/cpp/offroad/test/test_key_data.h>
#include <library/cpp/offroad/test/test_md5.h>
#include <library/cpp/digest/md5/md5.h>

#include "key_writer.h"
#include "key_sampler.h"
#include "key_reader.h"
#include "fat_key_writer.h"
#include "fat_key_reader.h"

using namespace NOffroad;

Y_UNIT_TEST_SUITE(TKeyTest) {
    template <class Index, class IndexWriter>
    void WriteIndex(const Index& index, IndexWriter* writer) {
        for (const auto& pair : index)
            writer->WriteKey(pair.first, pair.second);
    }

    template <class MemoryIndex, class KeyReader>
    void TestReader(const MemoryIndex& index, KeyReader* reader) {
        auto pos = index.begin();
        typename KeyReader::TKeyRef key;
        typename KeyReader::TKeyData data;
        while (reader->ReadKey(&key, &data)) {
            UNIT_ASSERT_UNEQUAL(pos, index.end());
            UNIT_ASSERT_VALUES_EQUAL(pos->first, key);

            auto posData = pos->second;
            UNIT_ASSERT_EQUAL(posData, data);

            pos++;
        }
        UNIT_ASSERT_EQUAL(pos, index.end());
    }

    template <class Reader, class Writer, class Sampler, class Index>
    void TestReadWrite(const Index& index) {
        Sampler sampler;
        WriteIndex(index, &sampler);
        auto table = NewMultiTable(sampler.Finish());

        TBufferStream keyStream;
        Writer writer(table.Get(), &keyStream);

        WriteIndex(index, &writer);
        writer.Finish();
        Reader reader(table.Get(), TArrayRef<const char>(keyStream.Buffer().data(), keyStream.Buffer().size()));
        TestReader(index, &reader);
        reader.Restart();
        TestReader(index, &reader);
    }

    Y_UNIT_TEST(TestEmptyTable) {
        using TKeyWriter = NOffroad::TKeyWriter<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor>;
        using TKeyReader = NOffroad::TKeyReader<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor>;
        using TModel = typename TKeyWriter::TModel;

        auto table = NewMultiTable(TModel());
        TBufferStream stream;

        TKeyWriter writer(table.Get(), &stream);
        for (size_t i = 0; i < 1000; i++)
            writer.WriteKey(ToString(i), NOffroad::TDataOffset::FromEncoded(i));
        writer.Finish();

        TKeyReader reader(table.Get(), TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
        for (size_t i = 0; i < 1000; i++) {
            TStringBuf key;
            NOffroad::TDataOffset data;
            bool success = reader.ReadKey(&key, &data);
            UNIT_ASSERT(success);
            UNIT_ASSERT_VALUES_EQUAL(key, ToString(i));
            UNIT_ASSERT_VALUES_EQUAL(data.ToEncoded(), i);
        }
        TStringBuf key2;
        NOffroad::TDataOffset data2;
        bool success2 = reader.ReadKey(&key2, &data2);
        UNIT_ASSERT(!success2);
    }

    Y_UNIT_TEST(TestSimple2) {
        using TKeyReader = NOffroad::TKeyReader<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor>;
        using TKeyWriter = NOffroad::TKeyWriter<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor>;
        using TKeySampler = NOffroad::TKeySampler<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor>;

        TestReadWrite<TKeyReader, TKeyWriter, TKeySampler>(MakeTestKeyData(10000, "arara"));
    }

    Y_UNIT_TEST(TestSamplingOverflow) {
        using TKeyReader = NOffroad::TKeyReader<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor>;
        using TKeyWriter = NOffroad::TKeyWriter<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor>;
        using TKeySampler = NOffroad::TKeySampler<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor>;
        TKeySampler sampler;
        sampler.WriteKey("Don't segfault, please", NOffroad::TDataOffset::FromEncoded(1));
        auto table = NewMultiTable(sampler.Finish());
        TBufferStream keyStream;
        TKeyWriter writer(table.Get(), &keyStream);
        writer.WriteKey("hoho, haha", NOffroad::TDataOffset::FromEncoded(ui64(1000000000000000000)));
        writer.Finish();
        TKeyReader reader(table.Get(), TArrayRef<const char>(keyStream.Buffer().data(), keyStream.Buffer().size()));

        TStringBuf key;
        TTestKeyData data;
        reader.ReadKey(&key, &data);
        UNIT_ASSERT_VALUES_EQUAL(key, "hoho, haha");
        UNIT_ASSERT_VALUES_EQUAL(data.ToEncoded(), ui64(1000000000000000000));
    }

    Y_UNIT_TEST(TestIdentityKeySubtractor) {
        using TKeyReader = NOffroad::TKeyReader<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor, IdentityKeySubtractor>;
        using TKeyWriter = NOffroad::TKeyWriter<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor, IdentityKeySubtractor>;
        using TKeySampler = NOffroad::TKeySampler<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor, IdentityKeySubtractor>;

        auto dataMap = MakeTestKeyData(10000, "arara");
        TVector<std::pair<TString, TTestKeyData>> data(dataMap.begin(), dataMap.end());

        /* We want the data to be unsorted here. */
        std::mt19937 rng(555);
        std::shuffle(data.begin(), data.end(), rng);

        TestReadWrite<TKeyReader, TKeyWriter, TKeySampler>(data);
    }

    Y_UNIT_TEST(TestHitCount) {
        using TKeySampler = NOffroad::TKeySampler<THitCountKeyData, THitCountKeyDataVectorizer, THitCountKeyDataSubtractor>;
        using TKeyWriter = NOffroad::TKeyWriter<THitCountKeyData, THitCountKeyDataVectorizer, THitCountKeyDataSubtractor>;
        using TKeyReader = NOffroad::TKeyReader<THitCountKeyData, THitCountKeyDataVectorizer, THitCountKeyDataSubtractor>;

        TestReadWrite<TKeyReader, TKeyWriter, TKeySampler>(MakeHitCountKeyData(10000, "arara"));
    }

    Y_UNIT_TEST(TestSearcher) {
        using TKeySampler = NOffroad::TKeySampler<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor>;
        using TKeyWriter = NOffroad::TFatKeyWriter<NOffroad::TFatOffsetDataWriter<TTestKeyData, TTestKeyDataSerializer>, NOffroad::TKeyWriter<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor>>;
        using TKeyReader = NOffroad::TFatKeyReader<TTestKeyDataSerializer, NOffroad::TKeyReader<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor>>;
        using TKeySeeker = NOffroad::TFatKeySeeker<TTestKeyData, TTestKeyDataSerializer>;

        auto index = MakeTestKeyData(10000, "ararara");

        TKeySampler sampler;
        WriteIndex(index, &sampler);
        auto model = sampler.Finish();
        auto table = NewMultiTable(model);

        TBufferStream keyStream, fatStream, fatSubStream;
        TKeyWriter writer(&fatStream, &fatSubStream, table.Get(), &keyStream);
        WriteIndex(index, &writer);
        writer.Finish();

        TKeyReader reader(TArrayRef<const char>(fatStream.Buffer().data(), fatStream.Buffer().size()), TArrayRef<const char>(fatSubStream.Buffer().data(), fatSubStream.Buffer().size()),
                          table.Get(), TArrayRef<const char>(keyStream.Buffer().data(), keyStream.Buffer().size()));

        for (const auto& pair : index) {
            TStringBuf key;
            TTestKeyData data;
            bool success = reader.ReadKey(&key, &data);

            UNIT_ASSERT(success);
            UNIT_ASSERT_VALUES_EQUAL(key, pair.first);
            UNIT_ASSERT_VALUES_EQUAL(data.ToEncoded(), pair.second.ToEncoded());
        }

        TVector<TString> keys;
        for (const auto& pair : index)
            keys.push_back(pair.first);

        std::minstd_rand random(31337);
        std::shuffle(keys.begin(), keys.end(), random); /* That's a deterministic shuffle. */

        for (const TString& key : keys) {
            bool success = reader.LowerBound(key);
            UNIT_ASSERT(success);

            TStringBuf key2;
            TTestKeyData data;
            success = reader.ReadKey(&key2, &data);
            UNIT_ASSERT(success);
            UNIT_ASSERT_VALUES_EQUAL(key, key2);
            UNIT_ASSERT_EQUAL(data, index.at(key));
        }

        TVector<std::pair<TString, size_t>> keysIndices;
        size_t i = 0;
        for (const auto& pair : index)
            keysIndices.emplace_back(pair.first, i++);
        std::shuffle(keysIndices.begin(), keysIndices.end(), random); /* That's a deterministic shuffle. */

        TKeySeeker seeker(TArrayRef<const char>(fatStream.Buffer().data(), fatStream.Buffer().size()), TArrayRef<const char>(fatSubStream.Buffer().data(), fatSubStream.Buffer().size()));
        for (const auto& pair : keysIndices) {
            TStringBuf key;
            TTestKeyData data;
            bool success = seeker.LowerBound(pair.first, &key, &data, &reader);
            UNIT_ASSERT(success);

            UNIT_ASSERT_VALUES_EQUAL(key, pair.first);
            UNIT_ASSERT_EQUAL(data, index.at(pair.first));
        }

        UNIT_ASSERT_MD5_EQUAL(keyStream.Buffer(), "74e5b362ac81a2459b4b078cfd6696d7");

        TBufferStream modelStream;
        model.Save(&modelStream);
        UNIT_ASSERT_MD5_EQUAL(modelStream.Buffer(), "ad2049b8125a26d0dfa9fe95f302f414");
    }

    TMap<TString, TTestKeyData> MakeSmallKeyData() {
        return {
            {"1", TDataOffset::FromEncoded(1)},
            {"2", TDataOffset::FromEncoded(2)}};
    }

    Y_UNIT_TEST(TestSeekBeyondEof) {
        using TKeySampler = NOffroad::TKeySampler<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor>;
        using TKeyWriter = NOffroad::TKeyWriter<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor>;
        using TKeyReader = NOffroad::TKeyReader<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor>;

        auto index = MakeSmallKeyData();

        TKeySampler sampler;
        WriteIndex(index, &sampler);
        auto table = NewMultiTable(sampler.Finish());

        TBufferStream stream;

        TKeyWriter writer(table.Get(), &stream);
        WriteIndex(index, &writer);
        writer.Finish();

        TKeyReader reader(table.Get(), TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));

        bool success;
        TStringBuf key;
        TDataOffset data;

        success = reader.Seek(TDataOffset(0, 1), TStringBuf(), TDataOffset::FromEncoded(0));
        UNIT_ASSERT(success);

        success = reader.ReadKey(&key, &data);
        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL(key, "2");
        UNIT_ASSERT_VALUES_EQUAL(data.ToEncoded(), 2);

        success = reader.ReadKey(&key, &data);
        UNIT_ASSERT(!success);

        success = reader.Seek(TDataOffset(0, 10), TStringBuf(), TDataOffset::FromEncoded(0));
        UNIT_ASSERT(!success);

        success = reader.ReadKey(&key, &data);
        UNIT_ASSERT(!success);

        success = reader.Seek(TDataOffset(0, 2), TStringBuf(), TDataOffset::FromEncoded(0));
        UNIT_ASSERT(success);

        success = reader.ReadKey(&key, &data);
        UNIT_ASSERT(!success);
    }

    Y_UNIT_TEST(TestEmpty) {
        using TKeyWriter = NOffroad::TFatKeyWriter<NOffroad::TFatOffsetDataWriter<TTestKeyData, TTestKeyDataSerializer>, NOffroad::TKeyWriter<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor>>;
        using TKeyReader = NOffroad::TFatKeyReader<TTestKeyDataSerializer, NOffroad::TKeyReader<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor>>;
        using TKeySeeker = NOffroad::TFatKeySeeker<TTestKeyData, TTestKeyDataSerializer>;

        using TModel = TKeyWriter::TModel;
        using TWriterTable = TKeyWriter::TTable;
        using TReaderTable = TKeyReader::TTable;

        THolder<TWriterTable> writerTable = MakeHolder<TWriterTable>(TModel());
        THolder<TReaderTable> readerTable = MakeHolder<TReaderTable>(TModel());

        TBufferStream keyStream, fatStream, fatSubStream;
        TKeyWriter writer(&fatStream, &fatSubStream, writerTable.Get(), &keyStream);
        writer.Finish();

        TKeyReader reader(TArrayRef<const char>(fatStream.Buffer().data(), fatStream.Buffer().size()), TArrayRef<const char>(fatSubStream.Buffer().data(), fatSubStream.Buffer().size()),
                          readerTable.Get(), TArrayRef<const char>(keyStream.Buffer().data(), keyStream.Buffer().size()));

        TStringBuf key;
        TTestKeyData data;
        UNIT_ASSERT(!reader.ReadKey(&key, &data));

        TKeySeeker seeker(TArrayRef<const char>(fatStream.Buffer().data(), fatStream.Buffer().size()), TArrayRef<const char>(fatSubStream.Buffer().data(), fatSubStream.Buffer().size()));
    }

    Y_UNIT_TEST(TestIndexSeek) {
        using TKeyWriter = NOffroad::TFatKeyWriter<NOffroad::TFatOffsetDataWriter<TTestKeyData, TTestKeyDataSerializer>, NOffroad::TKeyWriter<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor>>;
        using TKeyReader = NOffroad::TKeyReader<TTestKeyData, TTestKeyDataVectorizer, TTestKeyDataSubtractor>;
        using TKeySeeker = NOffroad::TFatKeySeeker<TTestKeyData, TTestKeyDataSerializer>;

        using TModel = TKeyWriter::TModel;
        using TWriterTable = TKeyWriter::TTable;
        using TReaderTable = TKeyReader::TTable;

        for (size_t size = 0; size <= 142; ++size) {
            THolder<TWriterTable> writerTable = MakeHolder<TWriterTable>(TModel());
            THolder<TReaderTable> readerTable = MakeHolder<TReaderTable>(TModel());

            TBufferStream keyStream, fatStream, fatSubStream;
            TKeyWriter writer(&fatStream, &fatSubStream, writerTable.Get(), &keyStream);

            auto index = MakeTestKeyData(size, "azaza");
            TVector<std::pair<TString, TTestKeyData>> indexList(Reserve(index.size()));
            for (const auto& entry : index) {
                writer.WriteKey(entry.first, entry.second);
                indexList.emplace_back(entry.first, entry.second);
            }

            writer.Finish();

            TKeySeeker seeker(
                TArrayRef<const char>(fatStream.Buffer().data(), fatStream.Buffer().size()),
                TArrayRef<const char>(fatSubStream.Buffer().data(), fatSubStream.Buffer().size()));

            TKeyReader reader(readerTable.Get(), TArrayRef<const char>(keyStream.Buffer().data(), keyStream.Buffer().size()));

            for (size_t index = 0; index <= size; ++index) {
                if (index < size) {
                    UNIT_ASSERT(seeker.Seek(index, &reader));
                } else if (!seeker.Seek(index, &reader)) {
                    continue;
                }
                TStringBuf key;
                TTestKeyData data;
                if (index < size) {
                    UNIT_ASSERT(reader.ReadKey(&key, &data));
                    UNIT_ASSERT_VALUES_EQUAL(key, indexList[index].first);
                    UNIT_ASSERT_VALUES_EQUAL(data.ToEncoded(), indexList[index].second.ToEncoded());

                    size_t keyIndex = 0;
                    UNIT_ASSERT(seeker.LowerBound(indexList[index].first, &key, &data, &reader, &keyIndex));
                    UNIT_ASSERT_VALUES_EQUAL(keyIndex, index);
                    UNIT_ASSERT_VALUES_EQUAL(key, indexList[index].first);
                    UNIT_ASSERT_VALUES_EQUAL(data.ToEncoded(), indexList[index].second.ToEncoded());

                    UNIT_ASSERT(reader.ReadKey(&key, &data));
                    UNIT_ASSERT_VALUES_EQUAL(key, indexList[index].first);
                    UNIT_ASSERT_VALUES_EQUAL(data.ToEncoded(), indexList[index].second.ToEncoded());
                } else {
                    UNIT_ASSERT(!reader.ReadKey(&key, &data));
                }
            }
        }
    }
}
