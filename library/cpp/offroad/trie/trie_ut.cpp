#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/buffer.h>

#include <library/cpp/offroad/codec/multi_table.h>

#include "trie_writer.h"
#include "trie_reader.h"
#include "trie_sampler.h"

using namespace NOffroad;

Y_UNIT_TEST_SUITE(TTrieTest) {
    using TTrieIndex = std::vector<std::pair<TString, TString>>;

    TTrieIndex MakeTestTrie(size_t size) {
        TTrieIndex result;
        for (size_t i = 0; i < size; i++)
            result.emplace_back(ToString(i), ToString(100 - i));
        return result;
    }

    template <class Index, class Writer>
    void WriteTrie(const Index& index, Writer* writer) {
        for (const auto& pair : index)
            writer->Write(pair.first, pair.second);
    }

    Y_UNIT_TEST(TestSimple) {
        auto trie = MakeTestTrie(1000);

        TTrieSampler sampler;
        WriteTrie(trie, &sampler);
        auto table = NewMultiTable(sampler.Finish());

        TBufferStream stream;
        TTrieWriter writer(table.Get(), &stream);
        WriteTrie(trie, &writer);
        writer.Finish();

        TTrieReader reader(table.Get(), TArrayRef<const char>(stream.Buffer().data(), stream.Buffer().size()));
        TStringBuf key, data;
        for (size_t i = 0; i < trie.size(); i++) {
            reader.Read(&key, &data);

            UNIT_ASSERT_VALUES_EQUAL(key, trie[i].first);
            UNIT_ASSERT_VALUES_EQUAL(data, trie[i].second);
        }
        UNIT_ASSERT(!reader.Read(&key, &data));
    }
}
