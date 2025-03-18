#include <library/cpp/testing/unittest/registar.h>

#include <random>
#include <algorithm>

#include <util/generic/buffer.h>
#include <util/generic/map.h>
#include <util/stream/buffer.h>

#include <library/cpp/offroad/codec/multi_table.h>
#include <library/cpp/offroad/test/test_data.h>
#include <library/cpp/offroad/tuple/tuple_writer.h>
#include <library/cpp/offroad/tuple/tuple_reader.h>
#include <library/cpp/offroad/tuple/tuple_sampler.h>
#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/digest/md5/md5.h>

#include "sub_writer.h"
#include "sub_reader.h"

using namespace NOffroad;

Y_UNIT_TEST_SUITE(TSubTest) {
    template <class Index, class Writer>
    void WriteIndex(const Index& index, Writer* writer) {
        for (const auto& data : index)
            writer->WriteHit(data);
    }

    void SeekerTest() {
        using TSampler = TTupleSampler<TTestData, TTestDataVectorizer, TINSubtractor>;
        using TWriter = TSubWriter<TTestDataVectorizer, TTupleWriter<TTestData, TTestDataVectorizer, TINSubtractor>>;
        using TReader = TSubReader<TTestDataVectorizer, TTupleReader<TTestData, TTestDataVectorizer, TINSubtractor>>;

        auto index = MakeTestData(10000, 5555);

        TSampler sampler;
        WriteIndex(index, &sampler);
        auto table = NewMultiTable(sampler.Finish());

        TBufferStream hitStream, subStream;

        TWriter writer(&subStream, table.Get(), &hitStream);
        WriteIndex(index, &writer);
        writer.Finish();

        TReader reader(TArrayRef<const char>(subStream.Buffer().data(), subStream.Buffer().size()), table.Get(), TArrayRef<const char>(hitStream.Buffer().data(), hitStream.Buffer().size()));

        TVector<size_t> indices = xrange(index.size());
        std::minstd_rand random(1123123);
        std::shuffle(indices.begin(), indices.end(), random);

        for (size_t i : indices) {
            TTestData first;
            bool success = reader.LowerBound(index[i], &first);

            UNIT_ASSERT(success);
            UNIT_ASSERT_VALUES_EQUAL(first, index[i]);

            size_t k = i;
            for (size_t j = 0; k < index.size() && j < 100; k++, j++) {
                TTestData data;
                success = reader.ReadHit(&data);

                UNIT_ASSERT(success);
                UNIT_ASSERT_VALUES_EQUAL(data, index[k]);
            }
        }
    }

    Y_UNIT_TEST(TestSimple) {
        SeekerTest();
    }

    Y_UNIT_TEST(TestUnique) {
        using TSampler = TTupleSampler<TTestData, TTestDataVectorizer, TINSubtractor>;
        using TWriter = TSubWriter<TTestDataVectorizer, TTupleWriter<TTestData, TTestDataVectorizer, TINSubtractor>>;
        using TSearcher = TFlatSearcher<TTestData, ui64, TTestDataVectorizer, TUi64Vectorizer>;

        auto index1 = MakeTestData(300, 1337);
        TVector<TTestData> index;
        for (const auto& data : index1)
            for (int i = 0; i < 128; ++i)
                index.push_back(data);
        TBufferStream hitStream, subStream;

        TSampler sampler;
        WriteIndex(index, &sampler);
        auto table = NewMultiTable(sampler.Finish());

        TWriter writer(&subStream, table.Get(), &hitStream);
        WriteIndex(index, &writer);
        writer.Finish();

        TSearcher searcher(TArrayRef<const char>(subStream.Buffer().data(), subStream.Buffer().size()));

        TTestData lastData = searcher.ReadKey(0);
        for (size_t i = 1; i < searcher.Size(); ++i) {
            TTestData curData = searcher.ReadKey(i);
            UNIT_ASSERT_VALUES_UNEQUAL(searcher.ReadKey(i), lastData);
            lastData = curData;
        }
    }
}
