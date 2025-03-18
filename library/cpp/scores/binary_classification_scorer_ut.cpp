#include <library/cpp/scores/binary_classification_scorer.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/typetraits.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/system/yassert.h>

// Helpers BEGIN

template <typename T, typename U>
static void Compare(
    const NScores::TTrueFalseScores<T>& lhs,
    const NScores::TTrueFalseScores<U>& rhs) {
    UNIT_ASSERT_VALUES_EQUAL(lhs.TruePositive, rhs.TruePositive);
    UNIT_ASSERT_VALUES_EQUAL(lhs.FalsePositive, rhs.FalsePositive);
    UNIT_ASSERT_VALUES_EQUAL(lhs.FalseNegative, rhs.FalseNegative);
    UNIT_ASSERT_VALUES_EQUAL(lhs.TrueNegative, rhs.TrueNegative);
}

static constexpr double EPSILON = 1e-6;

template <typename T, typename C, typename TOther, typename COther>
static void Compare(
    const NScores::TBinaryClassificationScores<T, C>& lhs,
    const NScores::TBinaryClassificationScores<TOther, COther>& rhs,
    const T epsilon = static_cast<T>(EPSILON)) {
    Compare(lhs.TrueFalse, rhs.TrueFalse);
    UNIT_ASSERT_DOUBLES_EQUAL(lhs.Prevalence, rhs.Prevalence, epsilon);
    UNIT_ASSERT_DOUBLES_EQUAL(lhs.Precision, rhs.Precision, epsilon);
    UNIT_ASSERT_DOUBLES_EQUAL(lhs.FalseDiscoveryRate, rhs.FalseDiscoveryRate, epsilon);
    UNIT_ASSERT_DOUBLES_EQUAL(lhs.FalseOmissionRate, rhs.FalseOmissionRate, epsilon);
    UNIT_ASSERT_DOUBLES_EQUAL(lhs.NegativePredictiveValue, rhs.NegativePredictiveValue, epsilon);
    UNIT_ASSERT_DOUBLES_EQUAL(lhs.PositiveLikelihoodRatio, rhs.PositiveLikelihoodRatio, epsilon);
    UNIT_ASSERT_DOUBLES_EQUAL(lhs.NegativeLikelihoodRatio, rhs.NegativeLikelihoodRatio, epsilon);
    UNIT_ASSERT_DOUBLES_EQUAL(lhs.DiagnosticOddsRatio, rhs.DiagnosticOddsRatio, epsilon);
    UNIT_ASSERT_DOUBLES_EQUAL(lhs.Recall, rhs.Recall, epsilon);
    UNIT_ASSERT_DOUBLES_EQUAL(lhs.FalseNegativeRate, rhs.FalseNegativeRate, epsilon);
    UNIT_ASSERT_DOUBLES_EQUAL(lhs.FalsePositiveRate, rhs.FalsePositiveRate, epsilon);
    UNIT_ASSERT_DOUBLES_EQUAL(lhs.TrueNegativeRate, rhs.TrueNegativeRate, epsilon);
    UNIT_ASSERT_DOUBLES_EQUAL(lhs.Accuracy, rhs.Accuracy, epsilon);
    UNIT_ASSERT_DOUBLES_EQUAL(lhs.FOne, rhs.FOne, epsilon);
}

template <typename T>
static NScores::TTrueFalseScores<T> MakeTrueFalseScores(
    const T truePositive,
    const T falsePositive,
    const T falseNegative,
    const T trueNegative) {
    NScores::TTrueFalseScores<T> result;
    result.TruePositive = truePositive;
    result.FalsePositive = falsePositive;
    result.FalseNegative = falseNegative;
    result.TrueNegative = trueNegative;
    return result;
}

template <typename T, typename C>
static NScores::TBinaryClassificationScores<T, C> MakeBinaryClassificationScores(
    const NScores::TTrueFalseScores<C> trueFalse,
    const T prevalence = 0,
    const T precision = 0,
    const T falseDiscoveryRate = 0,
    const T falseOmissionRate = 0,
    const T negativePredictiveValue = 0,
    const T positiveLikelihoodRatio = 0,
    const T negativeLikelihoodRatio = 0,
    const T diagnosticOddsRatio = 0,
    const T recall = 0,
    const T falseNegativeRate = 0,
    const T falsePositiveRate = 0,
    const T trueNegativeRate = 0,
    const T accuracy = 0,
    const T fOne = 0) {
    NScores::TBinaryClassificationScores<T, C> result;
    result.TrueFalse = trueFalse;
    result.Prevalence = prevalence;
    result.Precision = precision;
    result.FalseDiscoveryRate = falseDiscoveryRate;
    result.FalseOmissionRate = falseOmissionRate;
    result.NegativePredictiveValue = negativePredictiveValue;
    result.PositiveLikelihoodRatio = positiveLikelihoodRatio;
    result.NegativeLikelihoodRatio = negativeLikelihoodRatio;
    result.DiagnosticOddsRatio = diagnosticOddsRatio;
    result.Recall = recall;
    result.FalseNegativeRate = falseNegativeRate;
    result.FalsePositiveRate = falsePositiveRate;
    result.TrueNegativeRate = trueNegativeRate;
    result.Accuracy = accuracy;
    result.FOne = fOne;
    return result;
}

template <>
void Out<NScores::TTrueFalseScores<ui32>>(
    IOutputStream& output,
    typename TTypeTraits<NScores::TTrueFalseScores<ui32>>::TFuncParam value) {
    output << value.ToJson();
}

template <>
void Out<NScores::TBinaryClassificationScores<double, ui32>>(
    IOutputStream& output,
    typename TTypeTraits<NScores::TBinaryClassificationScores<double, ui32>>::TFuncParam value) {
    output << value.ToJson();
}

// Helpers END

struct TDiscreteSample {
    TVector<int> Predicted;
    TVector<int> Expected;
};

static const TDiscreteSample DISCRETE_SAMPLE_ONE{
    {0, 1, 0, 0, 0, 1, 1, 0, 1, 0},
    {0, 0, 0, 1, 1, 1, 1, 1, 0, 0}};

static const size_t DISCRETE_SAMPLE_ONE_SIZE = DISCRETE_SAMPLE_ONE.Predicted.size();

static const TVector<NScores::TTrueFalseScores<ui32>> DISCRETE_SAMPLE_ONE_TRUE_FALSE_SCORES = {
    MakeTrueFalseScores<ui32>(0, 0, 0, 1),
    MakeTrueFalseScores<ui32>(0, 1, 0, 1),
    MakeTrueFalseScores<ui32>(0, 1, 0, 2),
    MakeTrueFalseScores<ui32>(0, 1, 1, 2),
    MakeTrueFalseScores<ui32>(0, 1, 2, 2),
    MakeTrueFalseScores<ui32>(1, 1, 2, 2),
    MakeTrueFalseScores<ui32>(2, 1, 2, 2),
    MakeTrueFalseScores<ui32>(2, 1, 3, 2),
    MakeTrueFalseScores<ui32>(2, 2, 3, 2),
    MakeTrueFalseScores<ui32>(2, 2, 3, 3)};

static const TVector<NScores::TTrueFalseScores<ui32>> DISCRETE_SAMPLE_ONE_INVERTED_TRUE_FALSE_SCORES = {
    MakeTrueFalseScores<ui32>(1, 0, 0, 0),
    MakeTrueFalseScores<ui32>(1, 0, 1, 0),
    MakeTrueFalseScores<ui32>(2, 0, 1, 0),
    MakeTrueFalseScores<ui32>(2, 1, 1, 0),
    MakeTrueFalseScores<ui32>(2, 2, 1, 0),
    MakeTrueFalseScores<ui32>(2, 2, 1, 1),
    MakeTrueFalseScores<ui32>(2, 2, 1, 2),
    MakeTrueFalseScores<ui32>(2, 3, 1, 2),
    MakeTrueFalseScores<ui32>(2, 3, 2, 2),
    MakeTrueFalseScores<ui32>(3, 3, 2, 2)};

static const TVector<NScores::TBinaryClassificationScores<double, ui32>> DISCRETE_SAMPLE_ONE_SCORES = {
    MakeBinaryClassificationScores(
        DISCRETE_SAMPLE_ONE_TRUE_FALSE_SCORES[0],
        0.000000, 0.000000, 0.000000, 0.000000, 1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 1.000000, 1.000000, 0.000000),
    MakeBinaryClassificationScores(
        DISCRETE_SAMPLE_ONE_TRUE_FALSE_SCORES[1],
        0.000000, 0.000000, 1.000000, 0.000000, 1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.500000, 0.500000, 0.500000, 0.000000),
    MakeBinaryClassificationScores(
        DISCRETE_SAMPLE_ONE_TRUE_FALSE_SCORES[2],
        0.000000, 0.000000, 1.000000, 0.000000, 1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.333333, 0.666667, 0.666667, 0.000000),
    MakeBinaryClassificationScores(
        DISCRETE_SAMPLE_ONE_TRUE_FALSE_SCORES[3],
        0.250000, 0.000000, 1.000000, 0.333333, 0.666667, 0.000000, 0.500000, 0.000000, 0.000000, 1.000000, 0.333333, 0.666667, 0.500000, 0.000000),
    MakeBinaryClassificationScores(
        DISCRETE_SAMPLE_ONE_TRUE_FALSE_SCORES[4],
        0.400000, 0.000000, 1.000000, 0.500000, 0.500000, 0.000000, 1.000000, 0.000000, 0.000000, 1.000000, 0.333333, 0.666667, 0.400000, 0.000000),
    MakeBinaryClassificationScores(
        DISCRETE_SAMPLE_ONE_TRUE_FALSE_SCORES[5],
        0.500000, 0.500000, 0.500000, 0.500000, 0.500000, 1.000000, 1.000000, 1.000000, 0.333333, 0.666667, 0.333333, 0.666667, 0.500000, 0.400000),
    MakeBinaryClassificationScores(
        DISCRETE_SAMPLE_ONE_TRUE_FALSE_SCORES[6],
        0.571429, 0.666667, 0.333333, 0.500000, 0.500000, 2.000000, 1.000000, 2.000000, 0.500000, 0.500000, 0.333333, 0.666667, 0.571429, 0.571429),
    MakeBinaryClassificationScores(
        DISCRETE_SAMPLE_ONE_TRUE_FALSE_SCORES[7],
        0.625000, 0.666667, 0.333333, 0.600000, 0.400000, 2.000000, 1.500000, 1.333333, 0.400000, 0.600000, 0.333333, 0.666667, 0.500000, 0.500000),
    MakeBinaryClassificationScores(
        DISCRETE_SAMPLE_ONE_TRUE_FALSE_SCORES[8],
        0.555556, 0.500000, 0.500000, 0.600000, 0.400000, 1.000000, 1.500000, 0.666667, 0.400000, 0.600000, 0.500000, 0.500000, 0.444444, 0.444444),
    MakeBinaryClassificationScores(
        DISCRETE_SAMPLE_ONE_TRUE_FALSE_SCORES[9],
        0.500000, 0.500000, 0.500000, 0.500000, 0.500000, 1.000000, 1.000000, 1.000000, 0.400000, 0.600000, 0.400000, 0.600000, 0.500000, 0.444444)};

Y_UNIT_TEST_SUITE(TBinaryClassificationScorerTest) {
    Y_UNIT_TEST(TestTrueFalseScoresAtEachElementWithDefaultChecker) {
        Y_ASSERT(DISCRETE_SAMPLE_ONE.Predicted.size() == DISCRETE_SAMPLE_ONE_SIZE);
        Y_ASSERT(DISCRETE_SAMPLE_ONE.Expected.size() == DISCRETE_SAMPLE_ONE_SIZE);
        Y_ASSERT(DISCRETE_SAMPLE_ONE_TRUE_FALSE_SCORES.size() == DISCRETE_SAMPLE_ONE_SIZE);

        NScores::TBinaryClassificationScorer<int> scorer;
        for (size_t index = 0; index < DISCRETE_SAMPLE_ONE_SIZE; ++index) {
            scorer.Push(DISCRETE_SAMPLE_ONE.Predicted[index], DISCRETE_SAMPLE_ONE.Expected[index]);
            Compare(
                scorer.TrueFalseScores(),
                DISCRETE_SAMPLE_ONE_TRUE_FALSE_SCORES[index]);
        }
    }

    Y_UNIT_TEST(TestTrueFalseScoresAtEachElementWithCustomChecker) {
        Y_ASSERT(DISCRETE_SAMPLE_ONE.Predicted.size() == DISCRETE_SAMPLE_ONE_SIZE);
        Y_ASSERT(DISCRETE_SAMPLE_ONE.Expected.size() == DISCRETE_SAMPLE_ONE_SIZE);
        Y_ASSERT(DISCRETE_SAMPLE_ONE_TRUE_FALSE_SCORES.size() == DISCRETE_SAMPLE_ONE_SIZE);

        NScores::TBinaryClassificationScorer<int> scorer;
        scorer.SetChecker([](const int sample) { return 1 == sample; });

        for (size_t index = 0; index < DISCRETE_SAMPLE_ONE_SIZE; ++index) {
            scorer.Push(DISCRETE_SAMPLE_ONE.Predicted[index], DISCRETE_SAMPLE_ONE.Expected[index]);
            Compare(
                scorer.TrueFalseScores(),
                DISCRETE_SAMPLE_ONE_TRUE_FALSE_SCORES[index]);
        }
    }

    Y_UNIT_TEST(TestAllScoresAtEachElementWithDefaultChecker) {
        Y_ASSERT(DISCRETE_SAMPLE_ONE.Predicted.size() == DISCRETE_SAMPLE_ONE_SIZE);
        Y_ASSERT(DISCRETE_SAMPLE_ONE.Expected.size() == DISCRETE_SAMPLE_ONE_SIZE);
        Y_ASSERT(DISCRETE_SAMPLE_ONE_SCORES.size() == DISCRETE_SAMPLE_ONE_SIZE);

        NScores::TBinaryClassificationScorer<int> scorer;
        for (size_t index = 0; index < DISCRETE_SAMPLE_ONE_SIZE; ++index) {
            scorer.Push(DISCRETE_SAMPLE_ONE.Predicted[index], DISCRETE_SAMPLE_ONE.Expected[index]);
            Compare(
                scorer.Get<double>(),
                DISCRETE_SAMPLE_ONE_SCORES[index]);
        }
    }

    Y_UNIT_TEST(TestAllScoresAtEachElementWithCustomChecker) {
        Y_ASSERT(DISCRETE_SAMPLE_ONE.Predicted.size() == DISCRETE_SAMPLE_ONE_SIZE);
        Y_ASSERT(DISCRETE_SAMPLE_ONE.Expected.size() == DISCRETE_SAMPLE_ONE_SIZE);
        Y_ASSERT(DISCRETE_SAMPLE_ONE_SCORES.size() == DISCRETE_SAMPLE_ONE_SIZE);

        NScores::TBinaryClassificationScorer<int> scorer;
        scorer.SetChecker([](const int sample) { return 1 == sample; });
        for (size_t index = 0; index < DISCRETE_SAMPLE_ONE_SIZE; ++index) {
            scorer.Push(DISCRETE_SAMPLE_ONE.Predicted[index], DISCRETE_SAMPLE_ONE.Expected[index]);
            Compare(
                scorer.Get<double>(),
                DISCRETE_SAMPLE_ONE_SCORES[index]);
        }
    }

    Y_UNIT_TEST(TestInvertedScorer) {
        Y_ASSERT(DISCRETE_SAMPLE_ONE.Predicted.size() == DISCRETE_SAMPLE_ONE_SIZE);
        Y_ASSERT(DISCRETE_SAMPLE_ONE.Expected.size() == DISCRETE_SAMPLE_ONE_SIZE);
        Y_ASSERT(DISCRETE_SAMPLE_ONE_INVERTED_TRUE_FALSE_SCORES.size() == DISCRETE_SAMPLE_ONE_SIZE);

        NScores::TBinaryClassificationScorer<int> scorer;
        scorer = scorer.Invert();
        for (size_t index = 0; index < DISCRETE_SAMPLE_ONE_SIZE; ++index) {
            scorer.Push(DISCRETE_SAMPLE_ONE.Predicted[index], DISCRETE_SAMPLE_ONE.Expected[index]);
            Compare(
                scorer.TrueFalseScores(),
                DISCRETE_SAMPLE_ONE_INVERTED_TRUE_FALSE_SCORES[index]);
        }
    }

    Y_UNIT_TEST(TestInvertedInvertedScorer) {
        Y_ASSERT(DISCRETE_SAMPLE_ONE.Predicted.size() == DISCRETE_SAMPLE_ONE_SIZE);
        Y_ASSERT(DISCRETE_SAMPLE_ONE.Expected.size() == DISCRETE_SAMPLE_ONE_SIZE);
        Y_ASSERT(DISCRETE_SAMPLE_ONE_INVERTED_TRUE_FALSE_SCORES.size() == DISCRETE_SAMPLE_ONE_SIZE);

        NScores::TBinaryClassificationScorer<int> scorer;
        scorer = scorer.Invert().Invert();
        for (size_t index = 0; index < DISCRETE_SAMPLE_ONE_SIZE; ++index) {
            scorer.Push(DISCRETE_SAMPLE_ONE.Predicted[index], DISCRETE_SAMPLE_ONE.Expected[index]);
            Compare(
                scorer.TrueFalseScores(),
                DISCRETE_SAMPLE_ONE_TRUE_FALSE_SCORES[index]);
        }
    }

    Y_UNIT_TEST(TestCalculateScoresForContainerWithDefaultChecker) {
        Y_ASSERT(DISCRETE_SAMPLE_ONE.Predicted.size() == DISCRETE_SAMPLE_ONE_SIZE);
        Y_ASSERT(DISCRETE_SAMPLE_ONE.Expected.size() == DISCRETE_SAMPLE_ONE_SIZE);
        Y_ASSERT(DISCRETE_SAMPLE_ONE_INVERTED_TRUE_FALSE_SCORES.size() == DISCRETE_SAMPLE_ONE_SIZE);

        for (size_t index = 0; index < DISCRETE_SAMPLE_ONE_SIZE; ++index) {
            const TVector<int> predicted(
                DISCRETE_SAMPLE_ONE.Predicted.begin(),
                DISCRETE_SAMPLE_ONE.Predicted.begin() + index + 1);
            const TVector<int> expected(
                DISCRETE_SAMPLE_ONE.Expected.begin(),
                DISCRETE_SAMPLE_ONE.Expected.begin() + index + 1);
            Compare(
                NScores::CalculateBinaryClassificationScores<double>(predicted, expected),
                DISCRETE_SAMPLE_ONE_SCORES[index]);
        }
    }

    Y_UNIT_TEST(TestCalculateScoresForContainerWithCustomChecker) {
        Y_ASSERT(DISCRETE_SAMPLE_ONE.Predicted.size() == DISCRETE_SAMPLE_ONE_SIZE);
        Y_ASSERT(DISCRETE_SAMPLE_ONE.Expected.size() == DISCRETE_SAMPLE_ONE_SIZE);
        Y_ASSERT(DISCRETE_SAMPLE_ONE_INVERTED_TRUE_FALSE_SCORES.size() == DISCRETE_SAMPLE_ONE_SIZE);

        for (size_t index = 0; index < DISCRETE_SAMPLE_ONE_SIZE; ++index) {
            const TVector<int> predicted(
                DISCRETE_SAMPLE_ONE.Predicted.begin(),
                DISCRETE_SAMPLE_ONE.Predicted.begin() + index + 1);
            const TVector<int> expected(
                DISCRETE_SAMPLE_ONE.Expected.begin(),
                DISCRETE_SAMPLE_ONE.Expected.begin() + index + 1);
            Compare(
                NScores::CalculateBinaryClassificationScores<double>(
                    predicted, expected,
                    [](const int sample) { return 1 == sample; }),
                DISCRETE_SAMPLE_ONE_SCORES[index]);
        }
    }
}
