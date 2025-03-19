#include "levenstein_dist_with_bigrams.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NLevenstein;

Y_UNIT_TEST_SUITE(Levenstein) {
    Y_UNIT_TEST(SimpleDistance) {
        auto replacer = [](const TString& str1, const TString& str2) -> float { return (str1.find(" ") != TString::npos || str2.find(" ") != TString::npos) ? FORBIDDEN_BIGRAM_WEIGHT : 1; };
        auto inserter = [](const TString& str) -> float { return str.find(" ") != TString::npos ? FORBIDDEN_BIGRAM_WEIGHT : 1; };
        auto deleter = [](const TString& str) -> float { return str.find(" ") != TString::npos ? FORBIDDEN_BIGRAM_WEIGHT : 1; };

        UNIT_ASSERT_VALUES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"hello"}, TVector<TString>{"hello", "world"}, " ", replacer,
                                         deleter, inserter), 1);
        UNIT_ASSERT_VALUES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"hello", "world"}, TVector<TString>{"hello"}, " ", replacer,
                                         deleter, inserter), 1);
        UNIT_ASSERT_VALUES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"hello", "world"}, TVector<TString>{"hello", "world"},
                                         " ", replacer, deleter, inserter), 0);
        UNIT_ASSERT_VALUES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"hello", "a", "strange"}, TVector<TString>{}, " ", replacer,
                                         deleter, inserter), 3);
        UNIT_ASSERT_VALUES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{}, TVector<TString>{"hello", "a", "strange"}, " ", replacer,
                                         deleter, inserter), 3);
        UNIT_ASSERT_VALUES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"hello", "a", "strange"}, TVector<TString>{"hello", "world"},
                                         " ", replacer, deleter, inserter), 2);
    }

    Y_UNIT_TEST(WeightedDistance) {

        auto replacer = [](const TString& str1, const TString& str2) -> float { return (str1.find(" ") != TString::npos || str2.find(" ") != TString::npos) ? FORBIDDEN_BIGRAM_WEIGHT : 0.5f; };
        auto inserter = [](const TString& str) -> float { return str.find(" ") != TString::npos ? FORBIDDEN_BIGRAM_WEIGHT : 2; };
        auto deleter = [](const TString& str) -> float { return str.find(" ") != TString::npos ? FORBIDDEN_BIGRAM_WEIGHT : 1; };

        UNIT_ASSERT_VALUES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"hello"}, TVector<TString>{"hello", "world"}, " ", replacer,
                                         deleter, inserter), 2);
        UNIT_ASSERT_VALUES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"hello", "world"}, TVector<TString>{"hello"}, " ", replacer,
                                         deleter, inserter), 1);
        UNIT_ASSERT_VALUES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"hello", "world"}, TVector<TString>{"hello", "world"},
                                         " ", replacer, deleter, inserter), 0);
        UNIT_ASSERT_VALUES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"hello", "a", "strange"}, TVector<TString>{"hello", "world"},
                                         " ", replacer, deleter, inserter), 1.5);
        UNIT_ASSERT_VALUES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"a", "b", "c"}, TVector<TString>{"d", "e", "f"}, " ", replacer,
                                         deleter, inserter), 1.5);
    }

    Y_UNIT_TEST(BigramDistanceBasic) {
        auto replacer = [](const TString& , const TString& ) -> float { return 1; };
        auto inserter = [](const TString& ) -> float { return 1; };
        auto deleter = [](const TString& ) -> float { return 1; };

        UNIT_ASSERT_VALUES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{}, TVector<TString>{"hello", "world"}, " ", replacer, deleter, inserter), 1);

        UNIT_ASSERT_VALUES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"hello", "world"}, TVector<TString>{}, " ", replacer, deleter, inserter), 1);

        UNIT_ASSERT_VALUES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"hello", "world", "1"}, TVector<TString>{"1"}, " ", replacer, deleter, inserter), 1);

        UNIT_ASSERT_VALUES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"1", "hello", "world", "1"}, TVector<TString>{"1"}, " ", replacer, deleter, inserter), 2);

        UNIT_ASSERT_VALUES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"1", "hello", "world", "1"}, TVector<TString>{}, " ", replacer, deleter, inserter), 2);

        UNIT_ASSERT_VALUES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"1", "2", "3", "4"}, TVector<TString>{"1", "2", "3", "4"}, " ", replacer, deleter, inserter), 0);

        UNIT_ASSERT_VALUES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"4", "2", "3", "1"}, TVector<TString>{"1", "2", "3", "4"}, " ", replacer, deleter, inserter), 2);
    }

    Y_UNIT_TEST(BigramDistanceAdv) {
        auto replacer = [](const TString& str1, const TString& str2) -> float {
            if ((str1 == "уголовном кодексе" && str2 == "ук") || (str1 == "ук" && str2 == "уголовном кодексе")) {
                return 0.6673150743789701f;
            }
            if ((str1 == "кодексе россии" && str2 == "россии матушки") || (str1 == "россии матушки" && str2 == "кодексе россии")) {
                return 0.8;
            }
            if ((str1 == "это" && str2 == "что такое") || (str1 == "что такое" && str2 == "это")) {
                return 0.5f;
            }
            if (str1.find(" ") != TString::npos || str2.find(" ") != TString::npos) {
                return FORBIDDEN_BIGRAM_WEIGHT;
            }
            return 1;
        };
        auto inserter = [](const TString& str) -> float {
            if (str == "рф") {
                return 0.5;
            }
            if (str == "мальдивы") {
                return 0.02;
            }
            if (str == "супер пупер") {
                return 0.1;
            }
            if (str.find(" ") != TString::npos) {
                return FORBIDDEN_BIGRAM_WEIGHT;
            }
            return 1;
        };
        auto deleter = [](const TString& str) -> float {
            if (str == "рф") {
                return 0.5;
            }
            if (str == "мальдивы") {
                return 0.02;
            }
            if (str == "супер пупер") {
                return 0.1;
            }
            if (str.find(" ") != TString::npos) {
                return FORBIDDEN_BIGRAM_WEIGHT;
            }
            return 1;
        };

        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"ук"},
                                         TVector<TString>{"уголовном", "кодексе"},
                                         " ", replacer, deleter, inserter), 0.6673150743789701, 0.0001);

        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"ук", "рф"},
                                         TVector<TString>{"уголовном", "кодексе", "рф"},
                                         " ", replacer, deleter, inserter), 0.6673150743789701, 0.0001);
        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"в", "ук"},
                        TVector<TString>{"в", "уголовном", "кодексе"},
                        " ", replacer, deleter, inserter), 0.6673150743789701, 0.0001);

        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"сколько", "статей", "в", "ук", "рф"},
                                         TVector<TString>{"сколько", "статей", "в", "уголовном", "кодексе", "рф"},
                                         " ", replacer, deleter, inserter), 0.6673150743789701, 0.0001);

        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"сколько", "статей", "в", "ук"},
                                         TVector<TString>{"сколько", "статей", "в", "уголовном", "кодексе", "рф"},
                                         " ", replacer, deleter, inserter), 0.6673150743789701 + 0.5, 0.0001);

        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"сколько", "статей", "в", "ук", "рф"},
                                         TVector<TString>{"сколько", "статей", "в", "уголовном", "кодексе"},
                                         " ", replacer, deleter, inserter), 0.6673150743789701 + 0.5, 0.0001);

        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"рф"},
                                         TVector<TString>{},
                                         " ", replacer, deleter, inserter), 0.5, 0.0001);

        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"мальдивы", "это"},
                                         TVector<TString>{"что", "такое", "мальдивы"},
                                         " ", replacer, deleter, inserter), 0.5 + 2 * 0.02, 0.0001);

        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"танцы", "супер", "пупер"},
                                         TVector<TString>{"танцы"},
                                         " ", replacer, deleter, inserter), 0.1, 0.0001);

        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"танцы"},
                                         TVector<TString>{"танцы", "супер", "пупер"},
                                         " ", replacer, deleter, inserter), 0.1, 0.0001);

        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"супер", "пупер"},
                                         TVector<TString>{},
                                         " ", replacer, deleter, inserter), 0.1, 0.0001);

        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{},
                                         TVector<TString>{"супер", "пупер"},
                                         " ", replacer, deleter, inserter), 0.1, 0.0001);

        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"супер", "пупер", "танцы"},
                                         TVector<TString>{"танцы"},
                                         " ", replacer, deleter, inserter), 0.1, 0.0001);

        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"танцы"},
                                         TVector<TString>{"супер", "пупер", "танцы"},
                                         " ", replacer, deleter, inserter), 0.1, 0.0001);

        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"танцы", "в", "августе", "супер", "пупер"},
                                         TVector<TString>{"танцы", "в", "августе"},
                                         " ", replacer, deleter, inserter), 0.1, 0.0001);

        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"танцы", "в", "августе"},
                                         TVector<TString>{"танцы", "в", "августе", "супер", "пупер"},
                                         " ", replacer, deleter, inserter), 0.1, 0.0001);

        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"танцы", "в", "августе", "это", "точно"},
                                         TVector<TString>{"танцы", "в", "августе", "супер", "пупер", "это", "точно"},
                                         " ", replacer, deleter, inserter), 0.1, 0.0001);

        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"танцы", "в", "августе", "супер", "пупер", "это", "точно"},
                                         TVector<TString>{"танцы", "в", "августе", "это", "точно"},
                                         " ", replacer, deleter, inserter), 0.1, 0.0001);

        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"уголовном", "кодексе", "россии"},
                                         TVector<TString>{"ук", "россии", "матушки"},
                                         " ", replacer, deleter, inserter), 1 + 0.6673150743789701f, 0.0001);

        UNIT_ASSERT_DOUBLES_EQUAL(
                BigramLevensteinDistance(TVector<TString>{"ук", "россии", "матушки"},
                                         TVector<TString>{"уголовном", "кодексе", "россии"},
                                         " ", replacer, deleter, inserter), 1 + 0.6673150743789701f, 0.0001);
    }
}
