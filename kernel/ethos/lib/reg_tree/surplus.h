#pragma once

#include "loss_functions.h"
#include "least_squares_tree.h"
#include "pool.h"

#include "logistic.h"

namespace NRegTree {

template <typename TFloatType>
class TSurplusLossFunction : public TLossFunctionBase<TFloatType> {
private:
public:
    TString Name() const override {
        return "SURPLUS";
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
                                    TSurplusMetricsCalculator<TFloatType>& surplusMetricsCalculator,
                                    TClassificationMetricsCalculator<TFloatType>& classificationMetricsCalculator,
                                    TRegressionMetricsCalculator<TFloatType>& regressionMetricsCalculator)
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

                        TFloatType margin = instance.OriginalGoal > 0 ? instance.Prediction : -instance.Prediction;
                        TFloatType absDerivative = Sigma(-margin);
                        instance.Goal = instance.OriginalGoal > 0 ? absDerivative : -absDerivative;
                    }
                });
            }
            queue->Stop();
            for (const TInstance<TFloatType>& instance : instances) {
                if (instance.PoolId == 0) {
                    classificationMetricsCalculator.Add(instance, options);
                    surplusMetricsCalculator.Add(instance);
                    regressionMetricsCalculator.Add(instance.OriginalGoal > 0 ? 1 : 0, Sigma(instance.Prediction), 1.);
                }
            }
        };

        TSurplusMetricsCalculator<TFloatType> learnSMC(options), testSMC(options);
        TRegressionMetricsCalculator<TFloatType> learnRMC, testRMC;
        TClassificationMetricsCalculator<TFloatType> learnCMC(learnPool.GetQueriesCount()), testCMC(testPool ? testPool->GetQueriesCount() : 0);
        prepareInstances(learnPool.GetInstances(), learnSMC, learnCMC, learnRMC);
        if (testPool) {
            prepareInstances(testPool->GetInstances(), testSMC, testCMC, learnRMC);
        }

        TVector<TMetricWithValues> metrics;
        metrics.push_back(TMetricWithValues("Surplus", learnSMC.Surplus(), testSMC.Surplus()));
        metrics.push_back(TMetricWithValues("WinsPart", learnSMC.WinsFraction(), testSMC.WinsFraction()));
        metrics.push_back(TMetricWithValues("Wins", learnSMC.Wins(), testSMC.Wins()));
        metrics.push_back(TMetricWithValues("Losses", learnSMC.Losses(), testSMC.Losses()));

        metrics.push_back(TMetricWithValues("Logistic", learnCMC.LogisticLoss(), testCMC.LogisticLoss()));
        metrics.push_back(TMetricWithValues("RMSE", learnRMC.RootMeanSquaredError(), learnRMC.RootMeanSquaredError()));
        metrics.push_back(TMetricWithValues("Precision", learnCMC.Precision(), testCMC.Precision()));
        metrics.push_back(TMetricWithValues("Recall", learnCMC.Recall(), testCMC.Recall()));
        metrics.push_back(TMetricWithValues("F1", learnCMC.F1(), testCMC.F1()));
        metrics.push_back(TMetricWithValues("BestF1", learnCMC.BestF1(), testCMC.BestF1()));

        return metrics;
    }

    TVector<TMetricWithValues> GetMetrics(TPool<TFloatType>& pool, const TOptions& options, size_t poolNumber = 0) override {
        TSurplusMetricsCalculator<TFloatType> surplusMetricsCalculator(options);
        TClassificationMetricsCalculator<TFloatType> classificationMetricsCalculator(pool.GetQueriesCount());
        TRegressionMetricsCalculator<TFloatType> regressionMetricsCalculator;
        for (TInstance<TFloatType>& instance : pool) {
            if (instance.PoolId == poolNumber) {
                surplusMetricsCalculator.Add(instance);
                classificationMetricsCalculator.Add(instance, options);
                regressionMetricsCalculator.Add(instance.OriginalGoal > 0 ? 1 : 0, Sigma(instance.Prediction), 1.);
            }
        }

        TVector<TMetricWithValues> metrics;
        metrics.push_back(TMetricWithValues("Surplus", surplusMetricsCalculator.Surplus()));
        metrics.push_back(TMetricWithValues("WinsPart", surplusMetricsCalculator.WinsFraction()));
        metrics.push_back(TMetricWithValues("Wins", surplusMetricsCalculator.Wins()));
        metrics.push_back(TMetricWithValues("Losses", surplusMetricsCalculator.Losses()));

        metrics.push_back(TMetricWithValues("Logistic", classificationMetricsCalculator.LogisticLoss()));
        metrics.push_back(TMetricWithValues("RMSE", regressionMetricsCalculator.RootMeanSquaredError()));
        metrics.push_back(TMetricWithValues("Precision", classificationMetricsCalculator.Precision()));
        metrics.push_back(TMetricWithValues("Recall", classificationMetricsCalculator.Recall()));
        metrics.push_back(TMetricWithValues("F1", classificationMetricsCalculator.F1()));
        metrics.push_back(TMetricWithValues("BestF1", classificationMetricsCalculator.BestF1()));

        return metrics;
    }

    TFloatType TransformPrediction(TFloatType prediction) const override {
        return prediction;
    }
};

}
