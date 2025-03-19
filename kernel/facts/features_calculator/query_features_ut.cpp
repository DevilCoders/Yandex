#include "query_features.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>

using namespace NUnstructuredFeatures;

Y_UNIT_TEST_SUITE(TQueryCalculatorTest) {
    Y_UNIT_TEST(TLevensteinDistanceEqualStringsTest) {
        TFactFactorStorage distances;
        NUnstructuredFeatures::TQueryCalculator::BuildEditDistanceFeatures(u"walk1talk2полк3волк", u"walk1talk2полк3волк", distances);
        UNIT_ASSERT_EQUAL(distances[FI_SYMBOL_EDIT_DISTANCE], 0.0f);
        UNIT_ASSERT_EQUAL(distances[FI_WORD_EDIT_DISTANCE], 0.0f);
        UNIT_ASSERT_EQUAL(distances[FI_SORTED_WORD_EDIT_DISTANCE], 0.0f);
    }

    Y_UNIT_TEST(TLevensteinDistanceEmptyStringsTest) {
        TFactFactorStorage distances;
        NUnstructuredFeatures::TQueryCalculator::BuildEditDistanceFeatures(u"", u"", distances);
        UNIT_ASSERT_EQUAL(distances[FI_SYMBOL_EDIT_DISTANCE], 0.0f);
        UNIT_ASSERT_EQUAL(distances[FI_WORD_EDIT_DISTANCE], 0.0f);
        UNIT_ASSERT_EQUAL(distances[FI_SORTED_WORD_EDIT_DISTANCE], 0.0f);
    }

    Y_UNIT_TEST(TLevensteinDistanceEmptyVsNonEmptyStringTest) {
        TFactFactorStorage distances;
        NUnstructuredFeatures::TQueryCalculator::BuildEditDistanceFeatures(u"", u"xyz", distances);
        UNIT_ASSERT_EQUAL(distances[FI_SYMBOL_EDIT_DISTANCE], 1.0f);
        UNIT_ASSERT_EQUAL(distances[FI_WORD_EDIT_DISTANCE], 1.0f);
        UNIT_ASSERT_EQUAL(distances[FI_SORTED_WORD_EDIT_DISTANCE], 1.0f);
    }

    Y_UNIT_TEST(TLevensteinDistanceNoWordStringsTest) {
        TFactFactorStorage distances;
        NUnstructuredFeatures::TQueryCalculator::BuildEditDistanceFeatures(u" ", u";;", distances);
        UNIT_ASSERT_EQUAL(distances[FI_SYMBOL_EDIT_DISTANCE], 1.0f);
        UNIT_ASSERT_EQUAL(distances[FI_WORD_EDIT_DISTANCE], 0.0f);
        UNIT_ASSERT_EQUAL(distances[FI_SORTED_WORD_EDIT_DISTANCE], 0.0f);
    }

    Y_UNIT_TEST(TLevensteinDistanceDifferentCaseTest) {
        TFactFactorStorage distances;
        NUnstructuredFeatures::TQueryCalculator::BuildEditDistanceFeatures(u"TALK", u"talk", distances);
        UNIT_ASSERT_EQUAL(distances[FI_SYMBOL_EDIT_DISTANCE], 1.0f);
        UNIT_ASSERT_EQUAL(distances[FI_WORD_EDIT_DISTANCE], 1.0f);
        UNIT_ASSERT_EQUAL(distances[FI_SORTED_WORD_EDIT_DISTANCE], 1.0f);
    }

    Y_UNIT_TEST(TLevensteinDistanceOneAsciiWordTest) {
        TFactFactorStorage distances;
        NUnstructuredFeatures::TQueryCalculator::BuildEditDistanceFeatures(u"folk", u"talk", distances);
        UNIT_ASSERT_EQUAL(distances[FI_SYMBOL_EDIT_DISTANCE], 0.5f);
        UNIT_ASSERT_EQUAL(distances[FI_WORD_EDIT_DISTANCE], 1.0f);
        UNIT_ASSERT_EQUAL(distances[FI_SORTED_WORD_EDIT_DISTANCE], 1.0f);
    }

    Y_UNIT_TEST(TLevensteinDistanceOneCyrillicWordTest) {
        TFactFactorStorage distances;
        NUnstructuredFeatures::TQueryCalculator::BuildEditDistanceFeatures(u"волк", u"полк", distances);
        UNIT_ASSERT_EQUAL(distances[FI_SYMBOL_EDIT_DISTANCE], 0.25f);
        UNIT_ASSERT_EQUAL(distances[FI_WORD_EDIT_DISTANCE], 1.0f);
        UNIT_ASSERT_EQUAL(distances[FI_SORTED_WORD_EDIT_DISTANCE], 1.0f);
    }

    Y_UNIT_TEST(TLevensteinDistanceAsciiAndCyrillicWordsTest) {
        TFactFactorStorage distances;
        NUnstructuredFeatures::TQueryCalculator::BuildEditDistanceFeatures(u"chalk волк", u"walk полк", distances);
        UNIT_ASSERT_EQUAL(distances[FI_SYMBOL_EDIT_DISTANCE], 0.3f);
        UNIT_ASSERT_EQUAL(distances[FI_WORD_EDIT_DISTANCE], 1.0f);
        UNIT_ASSERT_EQUAL(distances[FI_SORTED_WORD_EDIT_DISTANCE], 1.0f);
    }

    Y_UNIT_TEST(TLevensteinDistanceEqualWordsTest) {
        TFactFactorStorage distances;
        NUnstructuredFeatures::TQueryCalculator::BuildEditDistanceFeatures(u"chalk волк", u"chalk волк", distances);
        UNIT_ASSERT_EQUAL(distances[FI_SYMBOL_EDIT_DISTANCE], 0.0f);
        UNIT_ASSERT_EQUAL(distances[FI_WORD_EDIT_DISTANCE], 0.0f);
        UNIT_ASSERT_EQUAL(distances[FI_SORTED_WORD_EDIT_DISTANCE], 0.0f);
    }

    Y_UNIT_TEST(TLevensteinDistanceEqualWordsUnequallySeparatedTest) {
        TFactFactorStorage distances;
        NUnstructuredFeatures::TQueryCalculator::BuildEditDistanceFeatures(u"walk,волк.talk;полк", u"walk волк\ttalk--полк", distances);
        UNIT_ASSERT_EQUAL(distances[FI_SYMBOL_EDIT_DISTANCE], 0.2f);
        UNIT_ASSERT_EQUAL(distances[FI_WORD_EDIT_DISTANCE], 0.0f);
        UNIT_ASSERT_EQUAL(distances[FI_SORTED_WORD_EDIT_DISTANCE], 0.0f);
    }

    Y_UNIT_TEST(TLevensteinDistanceEqualWordsUnequallySortedTest) {
        TFactFactorStorage distances;
        NUnstructuredFeatures::TQueryCalculator::BuildEditDistanceFeatures(u"walk talk полк волк", u"волк,полк,talk,walk", distances);
        UNIT_ASSERT_EQUAL(distances[FI_SYMBOL_EDIT_DISTANCE], 1.0f);
        UNIT_ASSERT_EQUAL(distances[FI_WORD_EDIT_DISTANCE], 0.8f);
        UNIT_ASSERT_EQUAL(distances[FI_SORTED_WORD_EDIT_DISTANCE], 0.0f);
    }
}
