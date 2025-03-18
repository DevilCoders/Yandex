#include "utf8_char.h"

#include <library/cpp/unicode/utf8_iter/utf8_iter.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/str.h>
#include <util/string/cast.h>

using namespace std::string_view_literals;

constexpr TStringBuf DATA = ".\0dд\xE2\x82\xAC\xF0\x90\x8D\x88$"sv;
const TVector<size_t> DATA_CP_LENGTHS = {1, 1, 1, 2, 3, 4, 1};

Y_UNIT_TEST_SUITE(TUtf8CharTest) {
    Y_UNIT_TEST(TestDefaultCtor) {
        TUtf8Char chDefault;
        UNIT_ASSERT_VALUES_EQUAL(chDefault.length(), 0);

        TUtf8Char chZero{'\0'};
        UNIT_ASSERT_VALUES_EQUAL(chZero.length(), 1);
        UNIT_ASSERT_VALUES_EQUAL(chZero, "\0"sv);
    }

    Y_UNIT_TEST(TestLengths) {
        TVector<size_t> cpLengths;
        for (wchar32 c : TUtfIterCode(DATA)) {
            cpLengths.push_back(TUtf8Char(c).length());
        }
        UNIT_ASSERT_EQUAL(cpLengths, DATA_CP_LENGTHS);
    }

    Y_UNIT_TEST(TestPlusEquals) {
        TString result;
        for (wchar32 c : TUtfIterCode(DATA)) {
            result += TUtf8Char(c);
        }
        UNIT_ASSERT_VALUES_EQUAL(result, DATA);
    }

    Y_UNIT_TEST(TestAppend) {
        TString result;
        for (wchar32 c : TUtfIterCode(DATA)) {
            result.append(TUtf8Char(c));
        }
        UNIT_ASSERT_VALUES_EQUAL(result, DATA);
    }

    Y_UNIT_TEST(TestToString) {
        TString result;
        for (wchar32 c : TUtfIterCode(DATA)) {
            result += ToString(TUtf8Char(c));
        }
        UNIT_ASSERT_VALUES_EQUAL(result, DATA);
    }

    Y_UNIT_TEST(TestOutputToStream) {
        TStringStream sstream;
        for (wchar32 c : TUtfIterCode(DATA)) {
            sstream << TUtf8Char(c);
        }
        UNIT_ASSERT_VALUES_EQUAL(sstream.Str(), DATA);
    }
}
