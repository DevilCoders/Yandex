#include "bigrams.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/printf.h>

namespace {
    void GenerateAndCompare(const TString& query, const TVector<TString>& etalon, bool addOneWord=true) {
        TVector<TString> res;
        NWizardsClicks::GetBigrams(res, query, addOneWord);
        UNIT_ASSERT_EQUAL_C(etalon.size(), res.size(), Sprintf("\"query = [%s], %lu != %lu\"",  query.data(), res.size(), etalon.size()));
        for (size_t i = 0; i < etalon.size(); ++i) {
            UNIT_ASSERT_STRINGS_EQUAL_C(res[i], etalon[i], Sprintf("\"query = [%s]\"",  query.data()));
        }
    }
}

Y_UNIT_TEST_SUITE(TBigramIterTest) {
    Y_UNIT_TEST(TestEmpty) {
        GenerateAndCompare("", TVector<TString>());
    }

    Y_UNIT_TEST(TestOneWord) {
        GenerateAndCompare("a",  {"a"});
        GenerateAndCompare("a ",  {"a"});
        GenerateAndCompare("a", TVector<TString>(), false);
        GenerateAndCompare("a ", TVector<TString>(), false);
    }

    Y_UNIT_TEST(TestNormal) {
        GenerateAndCompare("a b c", {"a b", "b c"});
        GenerateAndCompare("a b", {"a b"});
    }
}
