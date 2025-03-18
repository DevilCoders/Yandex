#include <library/cpp/unicode/utf8_iter/utf8_iter.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(UnicodeSet) {
    Y_UNIT_TEST(UtfIter) {
        TString s = "привет! \xF0\x9F\x98\xB8";
        TString expect = "п;р;и;в;е;т;!; ;\xF0\x9F\x98\xB8;";

        TString result1;
        for (TStringBuf ch : TUtfIterBuf(s)) {
            result1 += ch;
            result1 += ';';
        }
        TString result2 = Accumulate(
            TUtfIterBuf(s).begin(), TUtfIterBuf(s).end(),
            TString(),
            [](auto&& acc, TStringBuf add) { return acc + add + ";"; });

        UNIT_ASSERT_EQUAL(expect, result1);
        UNIT_ASSERT_EQUAL(expect, result2);

        TVector<wchar32> result3;
        Copy(TUtfIterCode(s).begin(), TUtfIterCode(s).end(), std::back_inserter(result3));

        wchar32 resultExpectedA[] = {1087, 1088, 1080, 1074, 1077, 1090, 33, 32, 128568};
        TVector<wchar32> resultExpected(resultExpectedA, resultExpectedA + Y_ARRAY_SIZE(resultExpectedA));
        UNIT_ASSERT_EQUAL(result3, resultExpected);
    }
}
