#pragma once

#include "loss_functions.h"
#include "least_squares_tree.h"
#include "pool.h"
#include "splitter.h"

#include <util/generic/hash.h>

#include <util/random/mersenne.h>
#include <util/random/shuffle.h>

namespace NRegTree {

template <typename TFloatType>
class TPairwiseBoostCoefficientChooser : public TBoostCoefficientChooser<TFloatType> {
private:
    const TVector<TInstanceNumbersPair>& Pairs;
    size_t PoolToUseNumber;
public:
    TPairwiseBoostCoefficientChooser(const TOptions& options,
                                     const TVector<TInstance<TFloatType> >& instances,
                                     const THashSet<size_t>& testInstanceNumbers,
                                     const TVector<TFloatType>& newPredictions,
                                     const TVector<TInstanceNumbersPair>& pairs,
                                     size_t poolToUseNumber)
        : TBoostCoefficientChooser<TFloatType>(options, instances, testInstanceNumbers, newPredictions)
        , Pairs(pairs)
        , PoolToUseNumber(poolToUseNumber)
    {
    }

    double CalculateLoss(TFloatType factor) const override {
        TPairwiseMetricsCalculator<TFloatType> pairwiseMetricsCalculator;
        for (const TInstanceNumbersPair& pair : Pairs) {
            if (this->TestInstanceNumbers.contains(pair.BetterInstanceNumber) || this->TestInstanceNumbers.contains(pair.WorseInstanceNumber)) {
                continue;
            }
            const TInstance<TFloatType>& winnerInstance = this->Instances[pair.BetterInstanceNumber];
            const TInstance<TFloatType>& looserInstance = this->Instances[pair.WorseInstanceNumber];

            if (PoolToUseNumber != (size_t) -1 &&
                (winnerInstance.PoolId != PoolToUseNumber || looserInstance.PoolId != PoolToUseNumber)) {
                continue;
            }

            TFloatType winnerPrediction = winnerInstance.Prediction + factor * this->NewPredictions[pair.BetterInstanceNumber];
            TFloatType looserPrediction = looserInstance.Prediction + factor * this->NewPredictions[pair.WorseInstanceNumber];
            pairwiseMetricsCalculator.Add(winnerPrediction, looserPrediction, pair.Weight);
        }
        return pairwiseMetricsCalculator.LogLoss();
    }
};

template <typename TFloatType>
class TPairwiseLossFunction : public TLossFunctionBase<TFloatType> {
private:
public:
    TString Name() const override {
        return "PAIRWISE";
    }

    void InitializeGoals(TPool<TFloatType>& pool, const TOptions&) override {
        for (TInstance<TFloatType>& instance : pool) {
            instance.Goal = 0.;
        }

        TVector<TFloatType> sumPairWeights(pool.GetInstancesCount());
        for (const TInstanceNumbersPair& pair : pool.GetPairs()) {
            TInstance<TFloatType>& winner = pool[pair.BetterInstanceNumber];
            TInstance<TFloatType>& looser = pool[pair.WorseInstanceNumber];

            winner.Goal += 0.5 * pair.Weight;
            looser.Goal -= 0.5 * pair.Weight;

            sumPairWeights[pair.BetterInstanceNumber] += pair.Weight;
            sumPairWeights[pair.WorseInstanceNumber] += pair.Weight;
        }

        for (size_t instanceNumber = 0; instanceNumber < pool.GetInstancesCount(); ++instanceNumber) {
            pool[instanceNumber].Weight = sumPairWeights[instanceNumber];
            if (!sumPairWeights[instanceNumber]) {
                continue;
            }
            pool[instanceNumber].Goal /= sumPairWeights[instanceNumber];
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
        TVector<TFloatType> newPredictions(learnPool.GetInstancesCount());
        {
            TFloatType* newPrediction = newPredictions.begin();
            for (const TInstance<TFloatType>* instance = learnPool.begin(); instance != learnPool.end(); ++instance, ++newPrediction) {
                *newPrediction = newTree.Prediction(instance->Features);
            }
        }

        TFloatType factor = TPairwiseBoostCoefficientChooser<TFloatType>(options,
                                                                         learnPool.GetInstances(),
                                                                         testInstanceNumbers,
                                                                         newPredictions,
                                                                         learnPool.GetPairs(),
                                                                         poolToUseNumber).GetBestFactor();
        factor *= options.Shrinkage.Value;

        {
            TFloatType* newPrediction = newPredictions.begin();
            for (TInstance<TFloatType>* instance = learnPool.begin(); instance != learnPool.end(); ++instance, ++newPrediction) {
                instance->Prediction += *newPrediction * factor;
                instance->Goal = TFloatType();
            }
        }

        TPairwiseMetricsCalculator<TFloatType> learnPMC;
        TPairwiseMetricsCalculator<TFloatType> testPMC;

        TVector<TFloatType> sumPairWeights(learnPool.GetInstancesCount());
        for (const TInstanceNumbersPair& pair : learnPool.GetPairs()) {
            TInstance<TFloatType>& winner = learnPool[pair.BetterInstanceNumber];
            TInstance<TFloatType>& looser = learnPool[pair.WorseInstanceNumber];

            TFloatType margin = winner.Prediction - looser.Prediction - options.LogisticOffset;
            TFloatType absDerivative = Sigma(-margin);

            winner.Goal += absDerivative * pair.Weight;
            looser.Goal -= absDerivative * pair.Weight;

            sumPairWeights[pair.BetterInstanceNumber] += pair.Weight;
            sumPairWeights[pair.WorseInstanceNumber] += pair.Weight;

            if (testInstanceNumbers.contains(pair.BetterInstanceNumber) || testInstanceNumbers.contains(pair.WorseInstanceNumber)) {
                testPMC.Add(winner.Prediction, looser.Prediction, pair.Weight);
            } else {
                learnPMC.Add(winner.Prediction, looser.Prediction, pair.Weight);
            }
        }

        for (size_t instanceNumber = 0; instanceNumber < learnPool.GetInstancesCount(); ++instanceNumber) {
            if (!sumPairWeights[instanceNumber]) {
                continue;
            }
            learnPool[instanceNumber].Goal /= sumPairWeights[instanceNumber];
        }

        if (!!testPool) {
            for (TInstance<TFloatType>& instance : *testPool) {
                instance.Prediction += newTree.Prediction(instance.Features) * factor;
            }
            for (const TInstanceNumbersPair& pair : testPool->GetPairs()) {
                TInstance<TFloatType>& winner = (*testPool)[pair.BetterInstanceNumber];
                TInstance<TFloatType>& looser = (*testPool)[pair.WorseInstanceNumber];
                testPMC.Add(winner.Prediction, looser.Prediction, pair.Weight);
            }
        }

        TVector<TMetricWithValues> metrics;
        metrics.push_back(TMetricWithValues("LogLoss", learnPMC.LogLoss(), testPMC.LogLoss()));
        metrics.push_back(TMetricWithValues("CWP", learnPMC.CorrectWeightedPairs(), testPMC.CorrectWeightedPairs()));

        newTree.InceptWeight(factor);

        return metrics;
    }

    TVector<TMetricWithValues> GetMetrics(TPool<TFloatType>& pool, const TOptions&, size_t poolNumber = 0) override {
        TPairwiseMetricsCalculator<TFloatType> pmc;

        for (const TInstanceNumbersPair& pair : pool.GetPairs()) {
            TInstance<TFloatType>& winner = pool[pair.BetterInstanceNumber];
            TInstance<TFloatType>& looser = pool[pair.WorseInstanceNumber];
            if (winner.PoolId == poolNumber) {
                pmc.Add(winner.Prediction, looser.Prediction, pair.Weight);
            }
        }

        TVector<TMetricWithValues> metrics;
        metrics.push_back(TMetricWithValues("LogLoss", pmc.LogLoss()));
        metrics.push_back(TMetricWithValues("CWP", pmc.CorrectWeightedPairs()));
        return metrics;
    }

    TFloatType TransformPrediction(TFloatType prediction) const override {
        return prediction;
    }
};

}
