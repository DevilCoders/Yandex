#include "binary_classification_metrics.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TTestMetrics) {
    Y_UNIT_TEST(TestAUC) {
        // examples from http://cling.csd.uwo.ca/papers/ijcai03.pdf
        using namespace NEthos;
        {
            EBinaryClassLabel goals[] = {EBinaryClassLabel::BCL_NEGATIVE,
                                         EBinaryClassLabel::BCL_NEGATIVE,
                                         EBinaryClassLabel::BCL_NEGATIVE,
                                         EBinaryClassLabel::BCL_NEGATIVE,
                                         EBinaryClassLabel::BCL_POSITIVE,
                                         EBinaryClassLabel::BCL_NEGATIVE,
                                         EBinaryClassLabel::BCL_POSITIVE,
                                         EBinaryClassLabel::BCL_POSITIVE,
                                         EBinaryClassLabel::BCL_POSITIVE,
                                         EBinaryClassLabel::BCL_POSITIVE};

            TVector<EBinaryClassLabel> weightedGoals;
            TVector<double> weights;

            size_t count = sizeof(goals) / sizeof(EBinaryClassLabel);
            for (size_t i = 0; i < count; ++i) {
                if (weightedGoals.empty() || weightedGoals.back() != goals[i]) {
                    weightedGoals.push_back(goals[i]);
                    weights.push_back(1.);
                    continue;
                }
                weights.back() += 1.;
            }

            NEthos::TBcMetricsCalculator binaryClassificationMetricsCalculator;
            for (size_t i = 0; i < count; ++i) {
                EBinaryClassLabel fakePrediction = EBinaryClassLabel::BCL_NEGATIVE;
                binaryClassificationMetricsCalculator.Add(i, fakePrediction, goals[i]);
            }

            NEthos::TBcMetricsCalculator binaryClassificationWeightedMetricsCalculator;
            for (size_t i = 0; i < weightedGoals.size(); ++i) {
                EBinaryClassLabel fakePrediction = EBinaryClassLabel::BCL_NEGATIVE;
                binaryClassificationWeightedMetricsCalculator.Add(i, fakePrediction, weightedGoals[i], weights[i]);
            }

            const double auc = binaryClassificationMetricsCalculator.AUC();
            const double weightedAUC = binaryClassificationWeightedMetricsCalculator.AUC();

            UNIT_ASSERT_DOUBLES_EQUAL(auc, 0.96, 1e-3);
            UNIT_ASSERT_DOUBLES_EQUAL(auc, weightedAUC, 1e-3);
        }
        {
            EBinaryClassLabel goals[] = {EBinaryClassLabel::BCL_POSITIVE,
                                         EBinaryClassLabel::BCL_NEGATIVE,
                                         EBinaryClassLabel::BCL_NEGATIVE,
                                         EBinaryClassLabel::BCL_NEGATIVE,
                                         EBinaryClassLabel::BCL_NEGATIVE,
                                         EBinaryClassLabel::BCL_POSITIVE,
                                         EBinaryClassLabel::BCL_POSITIVE,
                                         EBinaryClassLabel::BCL_POSITIVE,
                                         EBinaryClassLabel::BCL_POSITIVE,
                                         EBinaryClassLabel::BCL_NEGATIVE};

            TVector<EBinaryClassLabel> weightedGoals;
            TVector<double> weights;

            size_t count = sizeof(goals) / sizeof(EBinaryClassLabel);
            for (size_t i = 0; i < count; ++i) {
                if (weightedGoals.empty() || weightedGoals.back() != goals[i]) {
                    weightedGoals.push_back(goals[i]);
                    weights.push_back(1.);
                    continue;
                }
                weights.back() += 1.;
            }

            NEthos::TBcMetricsCalculator binaryClassificationMetricsCalculator;
            for (size_t i = 0; i < count; ++i) {
                EBinaryClassLabel fakePrediction = EBinaryClassLabel::BCL_NEGATIVE;
                binaryClassificationMetricsCalculator.Add(i, fakePrediction, goals[i]);
            }

            NEthos::TBcMetricsCalculator binaryClassificationWeightedMetricsCalculator;
            for (size_t i = 0; i < weightedGoals.size(); ++i) {
                EBinaryClassLabel fakePrediction = EBinaryClassLabel::BCL_NEGATIVE;
                binaryClassificationWeightedMetricsCalculator.Add(i, fakePrediction, weightedGoals[i], weights[i]);
            }

            const double auc = binaryClassificationMetricsCalculator.AUC();
            const double weightedAUC = binaryClassificationWeightedMetricsCalculator.AUC();

            UNIT_ASSERT_DOUBLES_EQUAL(auc, 0.64, 1e-3);
            UNIT_ASSERT_DOUBLES_EQUAL(auc, weightedAUC, 1e-3);
        }
    }

    Y_UNIT_TEST(TestBinaryClassificationMetrics) {
        using namespace NEthos;

        EBinaryClassLabel goals[]       = {EBinaryClassLabel::BCL_NEGATIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_NEGATIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE};

        EBinaryClassLabel predictions[] = {EBinaryClassLabel::BCL_NEGATIVE,
                                           EBinaryClassLabel::BCL_NEGATIVE,
                                           EBinaryClassLabel::BCL_NEGATIVE,
                                           EBinaryClassLabel::BCL_NEGATIVE,
                                           EBinaryClassLabel::BCL_NEGATIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE};

        TVector<EBinaryClassLabel> weightedGoals;
        TVector<EBinaryClassLabel> weightedPredictions;
        TVector<double> weights;

        size_t count = sizeof(goals) / sizeof(EBinaryClassLabel);
        for (size_t i = 0; i < count; ++i) {
            if (weightedGoals.empty() || weightedGoals.back() != goals[i] || weightedPredictions.back() != predictions[i]) {
                weightedGoals.push_back(goals[i]);
                weightedPredictions.push_back(predictions[i]);
                weights.push_back(1.);
                continue;
            }
            weights.back() += 1.;
        }

        NEthos::TBcMetricsCalculator binaryClassificationMetricsCalculator;
        for (size_t i = 0; i < count; ++i) {
            binaryClassificationMetricsCalculator.Add(i, predictions[i], goals[i]);
        }

        NEthos::TBcMetricsCalculator binaryClassificationWeightedMetricsCalculator;
        for (size_t i = 0; i < weightedGoals.size(); ++i) {
            binaryClassificationWeightedMetricsCalculator.Add(i, weightedPredictions[i], weightedGoals[i], weights[i]);
        }

        const double precision = binaryClassificationMetricsCalculator.Precision();
        const double recall = binaryClassificationMetricsCalculator.Recall();
        const double f1 = binaryClassificationMetricsCalculator.F1();
        const double falsePositiveRate = binaryClassificationMetricsCalculator.FalsePositiveRate();
        const double accuracy = binaryClassificationMetricsCalculator.Accuracy();

        const double instancesCount = binaryClassificationMetricsCalculator.GetInstancesCount();
        const double positivePredicitonsCount = binaryClassificationMetricsCalculator.GetPositivePredictionsCount();

        UNIT_ASSERT_DOUBLES_EQUAL(precision,         0.8,   1e-3);
        UNIT_ASSERT_DOUBLES_EQUAL(recall,            0.5,   1e-3);
        UNIT_ASSERT_DOUBLES_EQUAL(f1,                0.615, 1e-3);
        UNIT_ASSERT_DOUBLES_EQUAL(falsePositiveRate, 0.5,   1e-3);
        UNIT_ASSERT_DOUBLES_EQUAL(accuracy,          0.5,   1e-3);

        UNIT_ASSERT_DOUBLES_EQUAL(instancesCount,           10, 1e-5);
        UNIT_ASSERT_DOUBLES_EQUAL(positivePredicitonsCount, 5,  1e-5);

        const double weightedPrecision = binaryClassificationWeightedMetricsCalculator.Precision();
        const double weightedRecall = binaryClassificationWeightedMetricsCalculator.Recall();
        const double weightedF1 = binaryClassificationWeightedMetricsCalculator.F1();
        const double weightedFalsePositiveRate = binaryClassificationWeightedMetricsCalculator.FalsePositiveRate();
        const double weightedAccuracy = binaryClassificationWeightedMetricsCalculator.Accuracy();

        const double weightedInstancesCount = binaryClassificationWeightedMetricsCalculator.GetInstancesCount();
        const double weightedPositivePredicitonsCount = binaryClassificationWeightedMetricsCalculator.GetPositivePredictionsCount();

        UNIT_ASSERT_DOUBLES_EQUAL(precision,         weightedPrecision,         1e-3);
        UNIT_ASSERT_DOUBLES_EQUAL(recall,            weightedRecall,            1e-3);
        UNIT_ASSERT_DOUBLES_EQUAL(f1,                weightedF1,                1e-3);
        UNIT_ASSERT_DOUBLES_EQUAL(falsePositiveRate, weightedFalsePositiveRate, 1e-3);
        UNIT_ASSERT_DOUBLES_EQUAL(accuracy,          weightedAccuracy,          1e-3);

        UNIT_ASSERT_DOUBLES_EQUAL(instancesCount,           weightedInstancesCount,           1e-5);
        UNIT_ASSERT_DOUBLES_EQUAL(positivePredicitonsCount, weightedPositivePredicitonsCount, 1e-5);

    }

    Y_UNIT_TEST(TestBinaryClassificationArithmetics) {
        using namespace NEthos;

        EBinaryClassLabel goals[]       = {EBinaryClassLabel::BCL_NEGATIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_NEGATIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE};

        EBinaryClassLabel predictions[] = {EBinaryClassLabel::BCL_NEGATIVE,
                                           EBinaryClassLabel::BCL_NEGATIVE,
                                           EBinaryClassLabel::BCL_NEGATIVE,
                                           EBinaryClassLabel::BCL_NEGATIVE,
                                           EBinaryClassLabel::BCL_NEGATIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_POSITIVE};

        NEthos::TBcMetricsCalculator binaryClassificationMetricsCalculator;
        NEthos::TBcMetricsCalculator binaryClassificationMetricsCalculatorRight;

        size_t count = sizeof(goals) / sizeof(EBinaryClassLabel);
        for (size_t i = 0; i < count; ++i) {
            if (i * 2 < count) {
                binaryClassificationMetricsCalculator.Add(i, predictions[i], goals[i]);
            } else {
                binaryClassificationMetricsCalculatorRight.Add(i, predictions[i], goals[i]);
            }
        }

        binaryClassificationMetricsCalculator += binaryClassificationMetricsCalculatorRight;

        UNIT_ASSERT_DOUBLES_EQUAL(binaryClassificationMetricsCalculator.Precision(),         0.8,   0.001);
        UNIT_ASSERT_DOUBLES_EQUAL(binaryClassificationMetricsCalculator.Recall(),            0.5,   0.001);
        UNIT_ASSERT_DOUBLES_EQUAL(binaryClassificationMetricsCalculator.F1(),                0.615, 0.001);
        UNIT_ASSERT_DOUBLES_EQUAL(binaryClassificationMetricsCalculator.FalsePositiveRate(), 0.5,   0.001);
        UNIT_ASSERT_DOUBLES_EQUAL(binaryClassificationMetricsCalculator.Accuracy(),          0.5,   0.001);
        UNIT_ASSERT_DOUBLES_EQUAL(binaryClassificationMetricsCalculator.AUC(),               0.75,  0.001);

        UNIT_ASSERT_EQUAL(binaryClassificationMetricsCalculator.GetInstancesCount(), 10);
        UNIT_ASSERT_EQUAL(binaryClassificationMetricsCalculator.GetPositivePredictionsCount(), 5);
    }

    Y_UNIT_TEST(TestBinaryClassificationThresholdSelection) {
        using namespace NEthos;

        EBinaryClassLabel goals[]       = {EBinaryClassLabel::BCL_NEGATIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_NEGATIVE,
                                           EBinaryClassLabel::BCL_POSITIVE,
                                           EBinaryClassLabel::BCL_NEGATIVE,
                                           EBinaryClassLabel::BCL_POSITIVE};

        double weights[] = {1.,
                            1.,
                            2.,
                            1.,
                            2.,
                            3.};

        NEthos::TBcMetricsCalculator binaryClassificationMetricsCalculator;
        size_t count = sizeof(goals) / sizeof(EBinaryClassLabel);
        for (size_t i = 0; i < count; ++i) {
            EBinaryClassLabel fakePrediction = EBinaryClassLabel::BCL_NEGATIVE;
            binaryClassificationMetricsCalculator.Add(i, fakePrediction, goals[i], weights[i]);
        }

        const double bestF1 = binaryClassificationMetricsCalculator.BestF1();
        const double bestThreshold = binaryClassificationMetricsCalculator.BestThreshold();

        UNIT_ASSERT_DOUBLES_EQUAL(bestF1,        0.75, 0.001);
        UNIT_ASSERT_DOUBLES_EQUAL(bestThreshold, 4.50, 0.001);
    }
}
