#include "escape.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/random/random.h>

namespace NTskvFormat {
    Y_UNIT_TEST_SUITE(EscapeTest) {
        Y_UNIT_TEST(ShouldNotEscapeRegularChars) {
            TString res;
            Escape(" abcABC123!@#", res);
            UNIT_ASSERT_STRINGS_EQUAL(res, " abcABC123!@#");
        }

        Y_UNIT_TEST(ShouldEscapeControlChars) {
            TString res;
            Escape(TStringBuf("\t\n\r\0", 4), res);
            UNIT_ASSERT_STRINGS_EQUAL(res, "\\t\\n\\r\\0");
        }

        Y_UNIT_TEST(ShouldEscapeSpecialChars) {
            TString res;
            Escape("\\=\"", res);
            UNIT_ASSERT_STRINGS_EQUAL(res, "\\\\\\=\\\"");
        }

        Y_UNIT_TEST(EscapeShouldAppendToBuffer) {
            TString res;
            Escape("a", res);
            Escape("=", res);
            Escape("b", res);
            UNIT_ASSERT_STRINGS_EQUAL(res, "a\\=b");
        }

        Y_UNIT_TEST(UnescapeShouldAppendToBuffer) {
            TString res;
            Unescape("a", res);
            Unescape("\\=", res);
            Unescape("b", res);
            UNIT_ASSERT_STRINGS_EQUAL(res, "a=b");
        }

        Y_UNIT_TEST(UnEscapeReversibility) {
            TString src, escaped, res;
            for (size_t test = 0; test < 1000; ++test) {
                src.clear();
                escaped.clear();
                res.clear();
                auto srcSize = RandomNumber<size_t>(32);
                for (size_t i = 0; i < srcSize; ++i) {
                    src.push_back(RandomNumber<char>());
                }

                Escape(src, escaped);
                Unescape(escaped, res);
                UNIT_ASSERT_STRINGS_EQUAL(src, res);
            }
        }
    }

}
