#include <kernel/stringmatch_tracker/matchers/matcher.h>
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TBinaryMaskTrigramMatcher) {

    Y_UNIT_TEST(EmptyQuery) {
        NRefSequences::TBinaryMaskTrigramMatcher matcher;
        ui32 textTrigrams[] = {1, 20, 3, 8, 4, 3, 20, 40, 3, 1, 1};
        for (size_t i = 0; i < Y_ARRAY_SIZE(textTrigrams); ++i) {
            matcher.AddTextTrigram(textTrigrams[i]);
        }
        float queryInText = 0;
        float textInQuery = 0;
        size_t numMatched = 0;
        matcher.CalcFactors(queryInText, textInQuery, numMatched);
        UNIT_ASSERT_EQUAL(queryInText, 0);
        UNIT_ASSERT_EQUAL(textInQuery, 0);
        UNIT_ASSERT_EQUAL(numMatched, 0);
    }

    Y_UNIT_TEST(EmptyText) {
        NRefSequences::TBinaryMaskTrigramMatcher matcher;
        ui32 queryTrigrams[] = {1, 20, 3, 8, 4, 3, 20, 40, 3, 1, 1};
        for (size_t i = 0; i < Y_ARRAY_SIZE(queryTrigrams); ++i) {
            matcher.AddQueryTrigram(queryTrigrams[i]);
        }
        float queryInText = 0;
        float textInQuery = 0;
        size_t numMatched = 0;
        matcher.CalcFactors(queryInText, textInQuery, numMatched);
        UNIT_ASSERT_EQUAL(queryInText, 0);
        UNIT_ASSERT_EQUAL(textInQuery, 0);
        UNIT_ASSERT_EQUAL(numMatched, 0);
    }

    Y_UNIT_TEST(BestMatch) {
        NRefSequences::TBinaryMaskTrigramMatcher matcher;
        ui32 queryTrigrams[] = {1, 20, 3, 8, 4, 3, 20, 40, 3, 1, 1};
        for (size_t i = 0; i < Y_ARRAY_SIZE(queryTrigrams); ++i) {
            matcher.AddQueryTrigram(queryTrigrams[i]);
        }
        ui32 textTrigrams[] = {1, 20, 3, 8, 4, 3, 20, 40, 3, 1, 1};
        for (size_t i = 0; i < Y_ARRAY_SIZE(textTrigrams); ++i) {
            matcher.AddTextTrigram(textTrigrams[i]);
        }
        float queryInText = 0;
        float textInQuery = 0;
        size_t numMatched = 0;
        matcher.CalcFactors(queryInText, textInQuery, numMatched);
        UNIT_ASSERT_EQUAL(queryInText, 1);
        UNIT_ASSERT_EQUAL(textInQuery, 1);
        UNIT_ASSERT_EQUAL(numMatched, 6);
    }

    Y_UNIT_TEST(CasualMatch) {
        NRefSequences::TBinaryMaskTrigramMatcher matcher;
        ui32 queryTrigrams[] = {1, 20, 3, 8, 4, 3, 20, 40, 3, 1, 1};
        for (size_t i = 0; i < Y_ARRAY_SIZE(queryTrigrams); ++i) {
            matcher.AddQueryTrigram(queryTrigrams[i]);
        }
        ui32 textTrigrams[] = {20, 3, 8, 3, 20, 40, 3, 12, 12, 100, 786, 100, 1000, 500};
        for (size_t i = 0; i < Y_ARRAY_SIZE(textTrigrams); ++i) {
            matcher.AddTextTrigram(textTrigrams[i]);
        }
        float queryInText = 0;
        float textInQuery = 0;
        size_t numMatched = 0;
        matcher.CalcFactors(queryInText, textInQuery, numMatched);
        UNIT_ASSERT(queryInText > 0.66 && queryInText < 0.67);
        UNIT_ASSERT(textInQuery > 0.44 && textInQuery < 0.45);
        UNIT_ASSERT_EQUAL(numMatched, 4);
    }
}
