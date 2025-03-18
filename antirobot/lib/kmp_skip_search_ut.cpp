#include <library/cpp/testing/unittest/registar.h>

#include "kmp_skip_search.h"

#include <library/cpp/charset/recyr.hh>

namespace NAntiRobot {
    Y_UNIT_TEST_SUITE(TTestKmpSkipSearch) {
        Y_UNIT_TEST(Test1) {
            const char* str1 = "a little brown fox zzze bra";
            TStringBuf str1Buf(str1, 18);

            TKmpSkipSearch search1("a");
            UNIT_ASSERT_EQUAL(search1.SearchInText(str1, 18), str1);
            UNIT_ASSERT_VALUES_EQUAL(search1.SearchInText(str1Buf), str1Buf);

            TKmpSkipSearch search2("t");
            UNIT_ASSERT_EQUAL(search2.SearchInText(str1, 18), str1 + 4);
            UNIT_ASSERT_VALUES_EQUAL(search2.SearchInText(str1Buf), "ttle brown fox");

            TKmpSkipSearch search3("x");
            UNIT_ASSERT_EQUAL(search3.SearchInText(str1, 18), str1 + 17);
            UNIT_ASSERT_VALUES_EQUAL(search3.SearchInText(str1Buf), "x");

            TKmpSkipSearch search4("z");
            UNIT_ASSERT_EQUAL(search4.SearchInText(str1, 18), (char*)nullptr);
            UNIT_ASSERT_VALUES_EQUAL(search4.SearchInText(str1Buf), TStringBuf());

            TKmpSkipSearch search5("e b");
            UNIT_ASSERT_EQUAL(search5.SearchInText(str1, 18), str1 + 7);
            UNIT_ASSERT_VALUES_EQUAL(search5.SearchInText(str1Buf), "e brown fox");

            TKmpSkipSearch search6("e bra");
            UNIT_ASSERT_EQUAL(search6.SearchInText(str1, 18), (char*)nullptr);
            UNIT_ASSERT_VALUES_EQUAL(search6.SearchInText(str1Buf), TStringBuf());
        }

        static TString ToWin(const TString& s) {
            return Recode(CODES_UTF8, CODES_WIN, s);
        }

        Y_UNIT_TEST(TestNonLatin) {
            TString s = ToWin("ехали медведи на велосипеде");
            TStringBuf b(s);

            TKmpSkipSearch search1(ToWin("медвед").data());
            UNIT_ASSERT_EQUAL(search1.SearchInText(s.data(), s.size()), s.data() + 6);
            UNIT_ASSERT_VALUES_EQUAL(search1.SearchInText(b), ToWin("медведи на велосипеде"));

        }
    };
}

