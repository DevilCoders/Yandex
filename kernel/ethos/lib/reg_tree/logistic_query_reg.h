#pragma once

#include <library/cpp/linear_regression/welford.h>

#include "loss_functions.h"
#include "least_squares_tree.h"
#include "pool.h"
#include "splitter.h"

#include <util/generic/hash.h>

namespace NRegTree {

template <typename TFloatType>
class TQueryRegularizedLogLossBoostCoefficientChooser : public TBoostCoefficientChooser<TFloatType> {
private:
    size_t PoolToUseNumber;

    const THashMap<size_t, TMeanCalculator>& QueryAvgOldPredictions;
    const THashMap<size_t, TMeanCalculator>& QueryAvgNewPredictions;
public:
    TQueryRegularizedLogLossBoostCoefficientChooser(const TOptions& options,
                                                    const TVector<TInstance<TFloatType> >& instances,
                                                    const THashSet<size_t>& testInstanceNumbers,
                                                    const TVector<TFloatType>& newPredictions,
                                                    const size_t poolToUseNumber,
                                                    const THashMap<size_t, TMeanCalculator>& queryAvgOldPredictions,
                                                    const THashMap<size_t, TMeanCalculator>& queryAvgNewPredictions)
        : TBoostCoefficientChooser<TFloatType>(options, instances, testInstanceNumbers, newPredictions)
        , PoolToUseNumber(poolToUseNumber)
        , QueryAvgOldPredictions(queryAvgOldPredictions)
        , QueryAvgNewPredictions(queryAvgNewPredictions)
    {
    }

    double CalculateLoss(TFloatType factor) const override {
        TMeanCalculator logisticErrorCalculator;
        TMeanCalculator regularizationErrorCalculator;

        const TInstance<TFloatType>* instance = this->Instances.begin();
        const TFloatType* newPrediction = this->NewPredictions.begin();

        for (; newPrediction != this->NewPredictions.end(); ++newPrediction, ++instance) {
            const TFloatType prediction = instance->Prediction + factor * *newPrediction;

            if (instance->PoolId == 0) {
                const TFloatType margin = instance->OriginalGoal > this->Options.ClassificationThreshold ? prediction : -prediction;
                const TFloatType logisticError = FastLogError(margin - this->Options.LogisticOffset);
                logisticErrorCalculator.Add(logisticError, instance->Weight);
            }
            if (instance->PoolId == 1) {
                const TFloatType queryAvgPrediction = QueryAvgOldPredictions.FindPtr(instance->QueryId)->GetMean() + factor * QueryAvgNewPredictions.FindPtr(instance->QueryId)->GetMean();
                const TFloatType queryRegularizationError = prediction - queryAvgPrediction;
                regularizationErrorCalculator.Add(queryRegularizationError * queryRegularizationError, instance->Weight);
            }
        }

        const double logisticError = logisticErrorCalculator.GetMean();
        const double regularizationError = sqrt(Max(regularizationErrorCalculator.GetMean(), 0.));

        return logisticError + this->Options.QueryRegularizationFactor * regularizationError;
    }
};

template <typename TFloatType>
class TQueryRegularizedLogLossLossFunction : public TLossFunctionBase<TFloatType> {
private:
public:
    TString Name() const override {
        return "QUERY_REGULARIZED_LOGISTIC";
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
                                       TFloatType* /* bias */ = nullptr) override
    {
        TVector<TFloatType> predictions(learnPool.GetInstancesCount());

        THashMap<size_t, TMeanCalculator> queryAvgOldPredictions;
        THashMap<size_t, TMeanCalculator> queryAvgNewPredictions;

        TFloatType* prediction = predictions.begin();
        for (const TInstance<TFloatType>* instance = learnPool.begin(); instance != learnPool.end(); ++instance, ++prediction) {
            *prediction = newTree.Prediction(instance->Features);

            if (instance->PoolId != 1) {
                continue;
            }

            queryAvgOldPredictions[instance->QueryId].Add(instance->Prediction);
            queryAvgNewPredictions[instance->QueryId].Add(*prediction);
        }

        TFloatType factor = TQueryRegularizedLogLossBoostCoefficientChooser<TFloatType>(options,
                                                                                        learnPool.GetInstances(),
                                                                                        testInstanceNumbers,
                                                                                        predictions,
                                                                                        poolToUseNumber,
                                                                                        queryAvgOldPredictions,
                                                                                        queryAvgNewPredictions).GetBestFactor();
        factor *= options.Shrinkage.Value;
        newTree.InceptWeight(factor);

        auto prepareInstances = [&](TVector<TInstance<TFloatType> >& instances,
                                    TSurplusMetricsCalculator<TFloatType>& surplusMetricsCalculator,
                                    TClassificationMetricsCalculator<TFloatType>& classificationMetricsCalculator,
                                    TRegressionMetricsCalculator<TFloatType>& regressionMetricsCalculator)
        {
            THolder<IThreadPool> queue = CreateThreadPool(options.ThreadsCount);
            for (size_t threadNumber = 0; threadNumber < options.ThreadsCount; ++threadNumber) {
                queue->SafeAddFunc([&, threadNumber]() {
                    for (size_t instanceNumber = threadNumber; instanceNumber < instances.size(); instanceNumber += options.ThreadsCount) {
                        TInstance<TFloatType>& instance = instances[instanceNumber];
                        instance.Prediction += newTree.Prediction(instance.Features);

                        if (instance.PoolId == 0) {
                            TFloatType instanceIW = instance.Features.front();
                            for (size_t rwNumber = 0; rwNumber < instance.RandomWeights.size(); ++rwNumber) {
                                instance.Features.front() = instance.RandomWeights[rwNumber];
                                instance.PredictionsForRandomWeights[rwNumber] += newTree.Prediction(instance.Features);
                            }
                            instance.Features.front() = instanceIW;

                            TFloatType margin = instance.OriginalGoal > 0 ? instance.Prediction : -instance.Prediction;
                            TFloatType absDerivative = Sigma(-margin);

                            instance.Goal = instance.OriginalGoal > 0 ? absDerivative : -absDerivative;
                        }
                    }
                });
            }
            queue->Stop();
            for (TInstance<TFloatType>& instance : instances) {
                if (instance.PoolId == 0) {
                    classificationMetricsCalculator.Add(instance, options);
                    surplusMetricsCalculator.Add(instance);
                    regressionMetricsCalculator.Add(instance.OriginalGoal > 0 ? 1 : 0, Sigma(instance.Prediction), 1.);
                }
                if (instance.PoolId == 1) {
                    const TFloatType queryAvgPrediction = queryAvgOldPredictions[instance.QueryId].GetMean() + factor * queryAvgNewPredictions[instance.QueryId].GetMean();
                    instance.Goal = queryAvgPrediction - instance.Prediction;
                }
            }
        };

        TSurplusMetricsCalculator<TFloatType> learnSMC(options), testSMC(options);
        TClassificationMetricsCalculator<TFloatType> learnCMC(learnPool.GetQueriesCount()), testCMC(testPool ? testPool->GetQueriesCount() : 0);
        TRegressionMetricsCalculator<TFloatType> learnRMC, testRMC;

        prepareInstances(learnPool.GetInstances(), learnSMC, learnCMC, learnRMC);
        if (testPool) {
            prepareInstances(testPool->GetInstances(), testSMC, testCMC, testRMC);
        }

        TVector<TMetricWithValues> metrics;

        metrics.push_back(TMetricWithValues("Surplus", learnSMC.Surplus(), testSMC.Surplus()));
        metrics.push_back(TMetricWithValues("WinsPart", learnSMC.WinsFraction(), testSMC.WinsFraction()));
        metrics.push_back(TMetricWithValues("Wins", learnSMC.Wins(), testSMC.Wins()));
        metrics.push_back(TMetricWithValues("Losses", learnSMC.Losses(), testSMC.Losses()));

        metrics.push_back(TMetricWithValues("Logistic", learnCMC.LogisticLoss(), testCMC.LogisticLoss()));
        metrics.push_back(TMetricWithValues("RMSE", learnRMC.RootMeanSquaredError(), testRMC.RootMeanSquaredError()));
        metrics.push_back(TMetricWithValues("Precision", learnCMC.Precision(), testCMC.Precision()));
        metrics.push_back(TMetricWithValues("Recall", learnCMC.Recall(), testCMC.Recall()));
        metrics.push_back(TMetricWithValues("F1", learnCMC.F1(), testCMC.F1()));
        metrics.push_back(TMetricWithValues("BestF1", learnCMC.BestF1(), testCMC.BestF1()));

        return metrics;
    }

    TVector<TMetricWithValues> GetMetrics(TPool<TFloatType>& pool, const TOptions& options, size_t poolNumber = 0) override {
        TClassificationMetricsCalculator<TFloatType> classificationMetricsCalculator(pool.GetInstancesCount());
        TRegressionMetricsCalculator<TFloatType> regressionMetricsCalculator;

        THashMap<size_t, TMeanCalculator> queryAvgPredictions;

        TSurplusMetricsCalculator<TFloatType> surplusMetricsCalculator(options);
        for (TInstance<TFloatType>* instance = pool.begin(); instance != pool.end(); ++instance) {
            if (instance->PoolId == poolNumber) {
                queryAvgPredictions[instance->QueryId].Add(instance->Prediction);

                classificationMetricsCalculator.Add(*instance, options);
                regressionMetricsCalculator.Add(instance->OriginalGoal, Sigma(instance->Prediction + options.Bias), instance->Weight);
                surplusMetricsCalculator.Add(*instance);
            }
        }

        TRegressionMetricsCalculator<TFloatType> regularizationRMC;
        for (TInstance<TFloatType>* instance = pool.begin(); instance != pool.end(); ++instance) {
            if (instance->PoolId == poolNumber) {
                regularizationRMC.Add(queryAvgPredictions[instance->QueryId].GetMean(), instance->Prediction, 1.);
            }
        }

        TVector<TMetricWithValues> metrics;
        if (poolNumber == 0) {
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
        }

        if (poolNumber == 1) {
            metrics.push_back(TMetricWithValues("REG", regularizationRMC.RootMeanSquaredError()));
        }

        return metrics;
    }

    TFloatType TransformPrediction(TFloatType prediction) const override {
        return prediction;
    }
};

}
