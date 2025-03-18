#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/map.h>
#include <util/stream/buffer.h>

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/offroad/test/test_key_data.h>
#include <library/cpp/offroad/test/test_md5.h>

#include "fat_writer.h"
#include "fat_searcher.h"

using namespace NOffroad;

Y_UNIT_TEST_SUITE(TFatTest) {
    using TFatWriter = NOffroad::TFatWriter<TTestKeyData, TTestKeyDataSerializer>;
    using TFatSearcher = NOffroad::TFatSearcher<TTestKeyData, TTestKeyDataSerializer>;

    Y_UNIT_TEST(TestSimple) {
        TMap<TString, TTestKeyData> index = MakeTestKeyData(10000, "blablabla!!!111");

        TBufferStream fatStream;
        TBufferStream fatSubStream;

        TFatWriter writer(&fatStream, &fatSubStream);
        for (const auto& pair : index)
            writer.WriteKey(pair.first, pair.second);
        writer.Finish();

        TFatSearcher searcher(TArrayRef<const char>(fatStream.Buffer().data(), fatStream.Buffer().size()), TArrayRef<const char>(fatSubStream.Buffer().data(), fatSubStream.Buffer().size()));

        for (auto pos = index.begin(); pos != index.end(); pos++) {
            size_t i = searcher.LowerBound(pos->first);
            TStringBuf key = searcher.ReadKey(i);
            TTestKeyData data = searcher.ReadData(i);

            UNIT_ASSERT_VALUES_EQUAL(key, pos->first);
            UNIT_ASSERT_VALUES_EQUAL(data.ToEncoded(), pos->second.ToEncoded());
        }

        {
            size_t i = searcher.LowerBound("z");
            UNIT_ASSERT_VALUES_EQUAL(i, searcher.Size());
        }

        UNIT_ASSERT_MD5_EQUAL(fatStream.Buffer(), "6b6fbb427b9e98f541d52e08a2fdcc77");
        UNIT_ASSERT_MD5_EQUAL(fatSubStream.Buffer(), "7855bf2782db98ae76653c2e338d75c6");
    }

    Y_UNIT_TEST(TestEmpty) {
        TBufferStream fatStream;
        TBufferStream fatSubStream;

        TFatWriter writer(&fatStream, &fatSubStream);

        writer.Finish();

        TFatSearcher searcher(TArrayRef<const char>(fatStream.Buffer().data(), fatStream.Buffer().size()), TArrayRef<const char>(fatSubStream.Buffer().data(), fatSubStream.Buffer().size()));

        UNIT_ASSERT_VALUES_EQUAL(1, searcher.Size());
        UNIT_ASSERT_VALUES_EQUAL("", searcher.ReadKey(0));
        UNIT_ASSERT(TTestKeyData() == searcher.ReadData(0));
    }
}
