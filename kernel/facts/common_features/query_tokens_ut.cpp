#include <kernel/facts/common_features/query_tokens.cpp>
#include <kernel/ethos/lib/data/dataset.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/vector.h>
#include <limits>

using namespace NFacts;

#define UNIT_ASSERT_VECTOR_OF_DOUBLES_EQUAL(A, B) { \
    UNIT_ASSERT_EQUAL(A.size(), B.size()); \
    for (size_t i = 0; i < A.size(); ++i) \
        UNIT_ASSERT_DOUBLES_EQUAL(A[i], B[i], 0.001); \
    }

class TQueryTokensTest : public NUnitTest::TTestBase {
    UNIT_TEST_SUITE(TQueryTokensTest);
        UNIT_TEST(SubstNumericTokenOrGetLemmaTest);
    UNIT_TEST_SUITE_END();

    TQueryTokensTest() {}

    void SubstNumericTokenOrGetLemmaTest() {
        TVector<TTypedLemmedToken> tokens = GetTypedLemmedTokens("000 123 -4 5.6 078 0x9 1e-5 abc three один");
        UNIT_ASSERT_EQUAL(SubstNumericTokenOrGetLemma(tokens[0]), u"INTEGER");
        UNIT_ASSERT_EQUAL(SubstNumericTokenOrGetLemma(tokens[1]), u"INTEGER");
        UNIT_ASSERT_EQUAL(SubstNumericTokenOrGetLemma(tokens[2]), u"INTEGER");
        UNIT_ASSERT_EQUAL(SubstNumericTokenOrGetLemma(tokens[3]), u"REAL");
        UNIT_ASSERT_EQUAL(SubstNumericTokenOrGetLemma(tokens[4]), u"INTEGER");
        UNIT_ASSERT_EQUAL(SubstNumericTokenOrGetLemma(tokens[5]), u"");
        UNIT_ASSERT_EQUAL(SubstNumericTokenOrGetLemma(tokens[6]), u"");
        UNIT_ASSERT_EQUAL(SubstNumericTokenOrGetLemma(tokens[7]), u"INTEGER");
        UNIT_ASSERT_EQUAL(SubstNumericTokenOrGetLemma(tokens[8]), u"abc");
        UNIT_ASSERT_EQUAL(SubstNumericTokenOrGetLemma(tokens[9]), u"three");
        UNIT_ASSERT_EQUAL(SubstNumericTokenOrGetLemma(tokens[10]), u"один");
    }

private:
    TVector<TTypedLemmedToken> GetTypedLemmedTokens(TString input) {
        TVector<TTypedLemmedToken> tokens;
        TTokenizerSplitParams tokenizerSplitParams(TTokenizerSplitParams::NOT_PUNCT);
        TTypedLemmedTokenHandler handler(&tokens, tokenizerSplitParams);
        TNlpTokenizer tokenizer(handler);
        tokenizer.Tokenize(UTF8ToWide(input));
        return tokens;
    }
};

UNIT_TEST_SUITE_REGISTRATION(TQueryTokensTest);


