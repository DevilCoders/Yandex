#include <kernel/urlid/urlid.h>
#include <kernel/urlid/urlhash.h>
#include <kernel/urlid/url2docid.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/random/mersenne.h>

using namespace NUrlId;

Y_UNIT_TEST_SUITE(TUrlIdTest) {
    struct TTest {
        TUrlId Hash;
        TString Str;
    };

    Y_UNIT_TEST(TestHash) {
        const TTest tests[] = {
            {123456                , "0000000040E20100"},
            {1234567               , "0000000087D61200"},
            {((TUrlId)123456) << 24, "E201000000000040"}
        };

        TString buffer;
        for (size_t i = 0; i < Y_ARRAY_SIZE(tests); ++i) {
            UNIT_ASSERT_EQUAL(Str2Hash(tests[i].Str), tests[i].Hash);
            UNIT_ASSERT_EQUAL(Hash2Str(tests[i].Hash), tests[i].Str);
            UNIT_ASSERT_EQUAL(Hash2Str(tests[i].Hash, buffer), tests[i].Str);
            UNIT_ASSERT_EQUAL(Hash2StrZ(tests[i].Hash, buffer), TString::Join("Z", tests[i].Str));
        }
    }

    Y_UNIT_TEST(TestRandomHash) {
        TMersenne<TUrlId> rnd;

        TString buffer;
        for (size_t i = 0; i < 100000; ++i) {
            const TUrlId id = rnd.GenRand();

            UNIT_ASSERT_EQUAL(Str2Hash(Hash2Str(id)), id);
            UNIT_ASSERT_EQUAL(Str2Hash(Hash2Str(id, buffer)), id);
        }
    }

    Y_UNIT_TEST(ShortUrlids) {
        // Test for some defined behavior. The reference value may change, but the result should be defined.
        UNIT_ASSERT_VALUES_EQUAL(Str2Hash(TString("ABCD000000000000")), Str2Hash(TString("ABCD")));
        UNIT_ASSERT_VALUES_EQUAL(Str2Hash(TString("abcd000000000000")), Str2Hash(TString("ABCD")));
        UNIT_ASSERT_VALUES_EQUAL(Str2Hash(TString("0000000000000000")), Str2Hash(TString("00")));
        UNIT_ASSERT_VALUES_EQUAL(Str2Hash(TString("0000000000000000")), Str2Hash(TString("")));
    }

    Y_UNIT_TEST(NonFormatStrHashes) {
        UNIT_ASSERT_EXCEPTION(Str2Hash(TString("hello")), yexception);
        // not sure it's sane if 3 digits are considered not valid, but that's how it works now.
        // remove this check if the situation changes.
        UNIT_ASSERT_EXCEPTION(Str2Hash(TString("ABC")), yexception);
        UNIT_ASSERT_EXCEPTION(Str2Hash(TString("0")), yexception);
    }
}

Y_UNIT_TEST_SUITE(TUrlHashTest) {

    Y_UNIT_TEST(TestHashPack) {
        const ui64 h = 0x0123456789abcdef;
        const ui32 h1 = 0x01234567, h2 = 0x89abcdef;

        ui64 r = PackUrlHash64(h1, h2);
        UNIT_ASSERT_EQUAL(r, h);

        ui32 r1, r2;
        UnPackUrlHash64(h, r1, r2);
        UNIT_ASSERT_EQUAL(r1, h1);
        UNIT_ASSERT_EQUAL(r2, h2);
    }
}

Y_UNIT_TEST_SUITE(TUrl2DocIdHelperTest) {
    struct TOnDocId {
        TString DocIds;

        void operator()(TStringBuf docid, TStringBuf url, TStringBuf /*lang*/) {
            DocIds.append(docid).append("=").append(url).append(" ");
        }
    };

    Y_UNIT_TEST(TestUrl2DocId) {
        UNIT_ASSERT_VALUES_EQUAL("Z612A952099CD8FE2", Url2DocIdSimple("https://twitter.com/segalovich"));
        UNIT_ASSERT_VALUES_EQUAL("Z3E9C56E652B9AD1F", Url2DocIdSimple("https://twitter.com/segalovich", "de"));
    }

    Y_UNIT_TEST(TestLowerHost) {
        TString buf;
        UNIT_ASSERT_VALUES_EQUAL(Url2DocIdSimple("https://YANDEX.RU/TEST"), Url2DocIdRaw("https://YANDEX.RU/TEST", buf));
        UNIT_ASSERT_VALUES_EQUAL(Url2DocIdSimpleHost2Lower("https://YANDEX.RU/TEST"), Url2DocIdRaw("https://yandex.ru/TEST", buf));
    }
}
