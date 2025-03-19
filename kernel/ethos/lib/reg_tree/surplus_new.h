#pragma once

#include "loss_functions.h"
#include "least_squares_tree.h"
#include "pool.h"

#include "logistic.h"

namespace NRegTree {

template <typename TFloatType>
class TNewSurplusLossFunction : public TLossFunctionBase<TFloatType> {
private:
public:
    TString Name() const override {
        return "NEW_SURPLUS";
    }

    void InitializeGoals(TPool<TFloatType>& pool, const TOptions&) override {
        for (TInstance<TFloatType>& instance : pool) {
            instance.Goal = instance.OriginalGoal > 0 ? 0.5 : -0.5;
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
        {
            TVector<TFloatType> newPredictions;
            for (TInstance<TFloatType>& instance : learnPool) {
                newPredictions.push_back(newTree.Prediction(instance.Features));
            }
            TFloatType factor = TLogLossBoostCoefficientChooser<TFloatType>(options, learnPool.GetInstances(), testInstanceNumbers, newPredictions, poolToUseNumber).GetBestFactor();
            newTree.InceptWeight(factor * options.Shrinkage.Value);
        }

        auto prepareInstances = [&](TVector<TInstance<TFloatType> >& instances,
                                    TSurplusMetricsCalculator<TFloatType>& surplusMetricsCalculator)
        {
            THolder<IThreadPool> queue = CreateThreadPool(options.ThreadsCount);
            for (size_t threadNumber = 0; threadNumber < options.ThreadsCount; ++threadNumber) {
                queue->SafeAddFunc([&, threadNumber](){
                    for (size_t instanceNumber = threadNumber; instanceNumber < instances.size(); instanceNumber += options.ThreadsCount) {
                        TInstance<TFloatType>& instance = instances[instanceNumber];
                        TFloatType instanceIW = instance.Features.front();
                        for (size_t rwNumber = 0; rwNumber < instance.RandomWeights.size(); ++rwNumber) {
                            instance.Features.front() = instance.RandomWeights[rwNumber];
                            instance.PredictionsForRandomWeights[rwNumber] += newTree.Prediction(instance.Features);
                        }
                        instance.Features.front() = instanceIW;
                        instance.Prediction += newTree.Prediction(instance.Features);

                        const double actualPrediction = 2 * instance.Prediction - Accumulate(instance.PredictionsForRandomWeights, TFloatType{});

                        TFloatType margin = instance.OriginalGoal * actualPrediction;
                        TFloatType absDerivative = Sigma(-margin);
                        instance.Goal = instance.OriginalGoal * absDerivative;
                    }
                });
            }
            queue->Stop();
            for (TInstance<TFloatType>& instance : instances) {
                if (instance.PoolId == 0) {
                    surplusMetricsCalculator.Add(instance);
                }
            }
        };

        TSurplusMetricsCalculator<TFloatType> learnSMC(options), testSMC(options);
        prepareInstances(learnPool.GetInstances(), learnSMC);
        if (testPool) {
            prepareInstances(testPool->GetInstances(), testSMC);
        }

        TVector<TMetricWithValues> metrics;
        metrics.push_back(TMetricWithValues("Surplus", learnSMC.Surplus(), testSMC.Surplus()));
        metrics.push_back(TMetricWithValues("WinsPart", learnSMC.WinsFraction(), testSMC.WinsFraction()));
        metrics.push_back(TMetricWithValues("Wins", learnSMC.Wins(), testSMC.Wins()));
        metrics.push_back(TMetricWithValues("Losses", learnSMC.Losses(), testSMC.Losses()));

        return metrics;
    }

    TVector<TMetricWithValues> GetMetrics(TPool<TFloatType>& pool, const TOptions& options, size_t poolNumber = 0) override {
        TSurplusMetricsCalculator<TFloatType> surplusMetricsCalculator(options);
        for (TInstance<TFloatType>& instance : pool) {
            if (instance.PoolId == poolNumber) {
                surplusMetricsCalculator.Add(instance);
            }
        }

        TVector<TMetricWithValues> metrics;
        metrics.push_back(TMetricWithValues("Surplus", surplusMetricsCalculator.Surplus()));
        metrics.push_back(TMetricWithValues("WinsPart", surplusMetricsCalculator.WinsFraction()));
        metrics.push_back(TMetricWithValues("Wins", surplusMetricsCalculator.Wins()));
        metrics.push_back(TMetricWithValues("Losses", surplusMetricsCalculator.Losses()));

        return metrics;
    }

    TFloatType TransformPrediction(TFloatType prediction) const override {
        return prediction;
    }
};

}
