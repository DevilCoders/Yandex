#include <library/cpp/testing/unittest/registar.h>

#include "transforming_key_writer.h"

#include <library/cpp/offroad/fat/fat_searcher.h>
#include <library/cpp/offroad/fat/fat_writer.h>
#include <library/cpp/offroad/offset/data_offset.h>

#include <util/generic/buffer.h>
#include <util/stream/buffer.h>

using namespace NOffroad;

Y_UNIT_TEST_SUITE(TKeyTransformingIndexWriter) {
    struct TTestKeyPrefixGetter {
        TStringBuf operator()(const TStringBuf& key) const {
            return key.SubStr(0, 1);
        }
    };

    Y_UNIT_TEST(TestFatWriter) {
        using TFatWriter = TTransformingKeyWriter<TTestKeyPrefixGetter, NOffroad::TFatWriter<TDataOffset, TDataOffsetSerializer>>;
        using TFatSearcher = TFatSearcher<TDataOffset, TDataOffsetSerializer>;

        TBufferStream fatStream;
        TBufferStream fatSubStream;

        TFatWriter writer(&fatStream, &fatSubStream);

        for (size_t i = 0; i < 26; ++i) {
            for (size_t j = 0; j < 26; ++j) {
                TString key = TStringBuilder() << char('a' + i) << char('a' + j);
                writer.WriteKey(key, TDataOffset());
            }
        }
        writer.Finish();

        TFatSearcher searcher(TArrayRef<const char>(fatStream.Buffer().data(), fatStream.Buffer().size()), TArrayRef<const char>(fatSubStream.Buffer().data(), fatSubStream.Buffer().size()));
        UNIT_ASSERT_VALUES_EQUAL(26 * 26 + 1, searcher.Size());
        size_t index = 1;
        for (size_t i = 0; i < 26; ++i) {
            for (size_t j = 0; j < 26; ++j) {
                TStringBuf key = searcher.ReadKey(index);
                TString expectedKey = TStringBuilder() << char('a' + i);
                UNIT_ASSERT_VALUES_EQUAL(expectedKey, key);
                ++index;
            }
        }
    }
}
