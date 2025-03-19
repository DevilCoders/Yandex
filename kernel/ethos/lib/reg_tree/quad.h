#pragma once

#include "loss_functions.h"

namespace NRegTree {

template <typename TFloatType>
class TQuadLossFunction : public TLossFunctionBase<TFloatType> {
private:
public:
    TString Name() const override {
        return "QUAD";
    }

    void InitializeGoals(TPool<TFloatType>& pool, const TOptions&) override {
        for (TInstance<TFloatType>& instance : pool) {
            instance.Goal = instance.OriginalGoal;
        }
    }

    TVector<TMetricWithValues> Process(TPool<TFloatType>& learnPool,
                                               TLeastSquaresTree<TFloatType>& newTree,
                                               const TOptions& options,
                                               const THashSet<size_t>& testInstanceNumbers,
                                               TPool<TFloatType>* testPool,
                                               const size_t poolToUseNumber,
                                               const bool /* treeIsFirst */,
                                               TFloatType* /* bias */ = nullptr) override
    {
        TFloatType up = TFloatType();
        TFloatType down = TFloatType();

        TVector<TFloatType> newPredictions(learnPool.GetInstancesCount());
        TFloatType* newPrediction = newPredictions.begin();
        for (const TInstance<TFloatType>* instance = learnPool.begin(); instance != learnPool.end(); ++instance, ++newPrediction) {
            *newPrediction = newTree.Prediction(instance->Features);

            if (testInstanceNumbers.contains(instance - learnPool.begin()) ||
                (poolToUseNumber != (size_t)-1 && instance->PoolId != poolToUseNumber))
            {
                continue;
            }

            up += instance->Goal * instance->Weight * *newPrediction;
            down += *newPrediction * instance->Weight * *newPrediction;
        }

        TFloatType factor = down ? options.Shrinkage.Value * up / down : TFloatType();

        TRegressionMetricsCalculator<TFloatType> learnRMC, testRMC;

        newPrediction = newPredictions.begin();
        for (TInstance<TFloatType>* instance = learnPool.begin(); instance != learnPool.end(); ++instance, ++newPrediction) {
            instance->Goal -= *newPrediction * factor;
            instance->Prediction += *newPrediction * factor;

            if (instance->PoolId == 0) {
                (testInstanceNumbers.contains(instance - learnPool.begin()) ? testRMC : learnRMC).
                    Add(instance->OriginalGoal, instance->Prediction + options.Bias, instance->Weight);
            }
        }

        if (!!testPool) {
            for (TInstance<TFloatType>* instance = testPool->begin(); instance != testPool->end(); ++instance) {
                instance->Goal -= newTree.Prediction(instance->Features) * factor;
                instance->Prediction += newTree.Prediction(instance->Features) * factor;
                testRMC.Add(instance->OriginalGoal, instance->Prediction + options.Bias, instance->Weight);
            }
        }

        TVector<TMetricWithValues> metrics;
        metrics.push_back(TMetricWithValues("rmse", learnRMC.RootMeanSquaredError(), testRMC.RootMeanSquaredError()));
        metrics.push_back(TMetricWithValues("R^2", learnRMC.DeterminationCoefficient(), testRMC.DeterminationCoefficient()));

        newTree.InceptWeight(factor);

        return metrics;
    }

    TVector<TMetricWithValues> GetMetrics(TPool<TFloatType>& pool, const TOptions& options, size_t poolNumber = 0) override {
        TRegressionMetricsCalculator<TFloatType> rmc;

        for (TInstance<TFloatType>* instance = pool.begin(); instance != pool.end(); ++instance) {
            if (instance->PoolId == poolNumber) {
                rmc.Add(instance->OriginalGoal, instance->Prediction + options.Bias, instance->Weight);
            }
        }
        TVector<TMetricWithValues> metrics;
        metrics.push_back(TMetricWithValues("rmse", rmc.RootMeanSquaredError()));
        metrics.push_back(TMetricWithValues("R^2", rmc.DeterminationCoefficient()));

        return metrics;
    }

    TFloatType TransformPrediction(TFloatType prediction) const override {
        return prediction;
    }
};

}
