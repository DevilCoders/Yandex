#pragma once

#include "loss_functions.h"
#include "least_squares_tree.h"
#include "pool.h"
#include "splitter.h"

#include <util/generic/hash.h>

namespace NRegTree {

template <typename TFloatType>
class TLogLossBoostCoefficientChooser : public TBoostCoefficientChooser<TFloatType> {
private:
    size_t PoolToUseNumber;
public:
    TLogLossBoostCoefficientChooser(const TOptions& options,
                                    const TVector<TInstance<TFloatType> >& instances,
                                    const THashSet<size_t>& testInstanceNumbers,
                                    const TVector<TFloatType>& newPredictions,
                                    const size_t poolToUseNumber)
        : TBoostCoefficientChooser<TFloatType>(options, instances, testInstanceNumbers, newPredictions)
        , PoolToUseNumber(poolToUseNumber)
    {
    }

    double CalculateLoss(TFloatType factor) const override {
        TFloatType result = TFloatType();
        const TInstance<TFloatType>* instance = this->Instances.begin();
        const TFloatType* newPrediction = this->NewPredictions.begin();
        for (; newPrediction != this->NewPredictions.end(); ++newPrediction, ++instance) {
            if (this->TestInstanceNumbers.contains(instance - this->Instances.begin())) {
                continue;
            }
            if (PoolToUseNumber != (size_t) -1 && instance->PoolId != PoolToUseNumber) {
                continue;
            }

            TFloatType prediction = instance->Prediction + factor * *newPrediction;
            TFloatType margin = instance->OriginalGoal > this->Options.ClassificationThreshold ? prediction : -prediction;
            result += FastLogError(margin - this->Options.LogisticOffset) * instance->Weight;
        }
        return result;
    }
};

template <typename TFloatType>
class TLogLossFunction : public TLossFunctionBase<TFloatType> {
private:
public:
    TString Name() const override {
        return "LOGISTIC";
    }

    void InitializeGoals(TPool<TFloatType>& learnPool, const TOptions& options) override {
        for (TInstance<TFloatType>& instance : learnPool) {
            instance.Goal = instance.OriginalGoal > options.ClassificationThreshold ? TFloatType(0.5) : TFloatType(-0.5);
        }
    }

    TVector<TMetricWithValues> Process(TPool<TFloatType>& learnPool,
                                               TLeastSquaresTree<TFloatType>& newTree,
                                               const TOptions& options,
                                               const THashSet<size_t>& testInstanceNumbers,
                                               TPool<TFloatType>* testPool,
                                               const size_t poolToUseNumber,
                                               const bool /* treeIsFirst */,
                                               TFloatType* bias = nullptr) override
    {
        TVector<TFloatType> predictions(learnPool.GetInstancesCount());
        TFloatType* prediction = predictions.begin();
        for (const TInstance<TFloatType>* instance = learnPool.begin(); instance != learnPool.end(); ++instance, ++prediction) {
            *prediction = newTree.Prediction(instance->Features);
        }

        TFloatType factor = TLogLossBoostCoefficientChooser<TFloatType>(options,
                                                                        learnPool.GetInstances(),
                                                                        testInstanceNumbers,
                                                                        predictions,
                                                                        poolToUseNumber).GetBestFactor();
        factor *= options.Shrinkage.Value;

        prediction = predictions.begin();
        for (TInstance<TFloatType>* instance = learnPool.begin(); instance != learnPool.end(); ++instance, ++prediction) {
            instance->Prediction += *prediction * factor;
            TFloatType margin = instance->OriginalGoal > options.ClassificationThreshold ? instance->Prediction : -instance->Prediction;
            margin -= options.LogisticOffset;
            TFloatType absDerivative = Sigma(-margin);
            instance->Goal = instance->OriginalGoal > options.ClassificationThreshold ? absDerivative : -absDerivative;
        }

        TFloatType additionalBias = 0;
        if (options.SelectBestThreshold && bias) {
            TSimpleClassificationMetricsCalculator<TFloatType> biasAdjuster;
            for (TInstance<TFloatType>* instance = learnPool.begin(); instance != learnPool.end(); ++instance, ++prediction) {
                if (instance->PoolId != 0) {
                    continue;
                }
                biasAdjuster.Add(*instance);
            }
            TFloatType bestThreshold;
            biasAdjuster.BestFBeta(options.FBeta, options.MinRecall, options.MinPrecision, &bestThreshold);

            *bias = -bestThreshold;

            additionalBias = *bias;
        }

        TClassificationMetricsCalculator<TFloatType> learnCMC(learnPool.GetInstancesCount()), testCMC(learnPool.GetInstancesCount());
        TRegressionMetricsCalculator<TFloatType> learnRMC, testRMC;
        for (TInstance<TFloatType>* instance = learnPool.begin(); instance != learnPool.end(); ++instance, ++prediction) {
            if (instance->PoolId != 0) {
                continue;
            }

            TFloatType normalizedGoal = instance->OriginalGoal > options.ClassificationThreshold ? TFloatType(1) : TFloatType();
            if (testInstanceNumbers.contains(instance - learnPool.begin())) {
                testCMC.Add(*instance, options, &additionalBias);
                testRMC.Add(normalizedGoal, Sigma(instance->Prediction + options.Bias + additionalBias), instance->Weight);
            } else {
                learnCMC.Add(*instance, options, &additionalBias);
                learnRMC.Add(normalizedGoal, Sigma(instance->Prediction + options.Bias + additionalBias), instance->Weight);
            }
        }

        if (!!testPool) {
            for (TInstance<TFloatType>* instance = testPool->begin(); instance != testPool->end(); ++instance) {
                instance->Prediction += newTree.Prediction(instance->Features) * factor;
                testCMC.Add(*instance, options, &additionalBias);
                TFloatType normalizedGoal = instance->OriginalGoal > options.ClassificationThreshold ? TFloatType(1) : TFloatType();
                testRMC.Add(normalizedGoal, Sigma(instance->Prediction + options.Bias + additionalBias), instance->Weight);
            }
        }

        TVector<TMetricWithValues> metrics;

        metrics.push_back(TMetricWithValues("log", learnCMC.LogisticLoss(), testCMC.LogisticLoss()));
        metrics.push_back(TMetricWithValues("acc", learnCMC.Accuracy(), testCMC.Accuracy()));
        metrics.push_back(TMetricWithValues("AUC", learnCMC.AUC(), testCMC.AUC()));
        metrics.push_back(TMetricWithValues("precision", learnCMC.Precision(), testCMC.Precision()));
        metrics.push_back(TMetricWithValues("recall", learnCMC.Recall(), testCMC.Recall()));
        metrics.push_back(TMetricWithValues("f1", learnCMC.F1(), testCMC.F1()));
        metrics.push_back(TMetricWithValues("bestF1", learnCMC.BestF1(), testCMC.BestF1()));

        metrics.push_back(TMetricWithValues("rmse", learnRMC.RootMeanSquaredError(), testRMC.RootMeanSquaredError()));
        metrics.push_back(TMetricWithValues("R^2", learnRMC.DeterminationCoefficient(), testRMC.DeterminationCoefficient()));

        newTree.InceptWeight(factor);

        return metrics;
    }

    TVector<TMetricWithValues> GetMetrics(TPool<TFloatType>& pool, const TOptions& options, size_t poolNumber = 0) override {
        TClassificationMetricsCalculator<TFloatType> cmc(pool.GetInstancesCount());
        TRegressionMetricsCalculator<TFloatType> rmc;

        for (TInstance<TFloatType>* instance = pool.begin(); instance != pool.end(); ++instance) {
            if (instance->PoolId == poolNumber) {
                cmc.Add(*instance, options);
                rmc.Add(instance->OriginalGoal, Sigma(instance->Prediction + options.Bias), instance->Weight);
            }
        }

        TVector<TMetricWithValues> metrics;
        metrics.push_back(TMetricWithValues("log", cmc.LogisticLoss()));
        metrics.push_back(TMetricWithValues("acc", cmc.Accuracy()));
        metrics.push_back(TMetricWithValues("AUC", cmc.AUC()));
        metrics.push_back(TMetricWithValues("precision", cmc.Precision()));
        metrics.push_back(TMetricWithValues("recall", cmc.Recall()));
        metrics.push_back(TMetricWithValues("f1", cmc.F1()));
        metrics.push_back(TMetricWithValues("bestF1", cmc.BestF1()));

        metrics.push_back(TMetricWithValues("rmse", rmc.RootMeanSquaredError()));
        metrics.push_back(TMetricWithValues("R^2", rmc.DeterminationCoefficient()));

        return metrics;
    }

    TFloatType TransformPrediction(TFloatType prediction) const override {
        return Sigma(prediction);
    }
};

}
