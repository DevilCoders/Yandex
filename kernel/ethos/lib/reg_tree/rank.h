#pragma once

#include "loss_functions.h"
#include "least_squares_tree.h"
#include "pool.h"
#include "splitter.h"

#include <util/generic/hash.h>
#include <util/string/printf.h>
#include <util/system/mutex.h>

namespace NRegTree {

template <typename TFloatType>
class TRankBoostCoefficientChooser : public TBoostCoefficientChooser<TFloatType> {
private:
    const TPool<TFloatType>& Pool;

    const TVector<TVector<std::pair<size_t, size_t> > >& QueryPairs;
    const TVector<TVector<size_t> >& Queries;
public:
    TRankBoostCoefficientChooser(const TOptions& options,
                                 const TPool<TFloatType>& pool,
                                 const THashSet<size_t>& testInstanceNumbers,
                                 const TVector<TFloatType>& newPredictions,
                                 const TVector<TVector<std::pair<size_t, size_t> > >& queryPairs,
                                 const TVector<TVector<size_t> >& queries)
        : TBoostCoefficientChooser<TFloatType>(options, pool.GetInstances(), testInstanceNumbers, newPredictions)
        , Pool(pool)
        , QueryPairs(queryPairs)
        , Queries(queries)
    {
    }

    double CalculateLoss(TFloatType factor) const override {
        if (this->Options.OptimizeBinaryRankLoss) {
            double loss = 0.;
            for (const TVector<std::pair<size_t, size_t> >& queryPairs : QueryPairs) {
                for (const std::pair<size_t, size_t>& queryPair : queryPairs) {
                    if (this->TestInstanceNumbers.contains(queryPair.first) || this->TestInstanceNumbers.contains(queryPair.second)) {
                        continue;
                    }
                    const TInstance<TFloatType>& leftInstance = this->Instances[queryPair.first];
                    const TInstance<TFloatType>& rightInstance = this->Instances[queryPair.second];

                    TFloatType weight = leftInstance.OriginalGoal - rightInstance.OriginalGoal;
                    weight *= Pool.GetPoolQueryWeight(leftInstance.PoolId);

                    TFloatType margin = leftInstance.Prediction + this->NewPredictions[queryPair.first] * factor -
                                        rightInstance.Prediction - this->NewPredictions[queryPair.second] * factor;

                    if (weight < TFloatType()) {
                        margin = -margin;
                    }
                    loss += fabs(weight) * FastLogError(-margin);
                }
            }
            return loss;
        }

        TRankingMetricsCalculator<TFloatType> metricsCalculator(Queries.size());
        for (size_t queryNumber = 0; queryNumber != Queries.size(); ++queryNumber) {
            for (const size_t instanceNumber : Queries[queryNumber]) {
                if (this->TestInstanceNumbers.contains(instanceNumber)) {
                    continue;
                }
                const TInstance<TFloatType>& instance = this->Instances[instanceNumber];
                TFloatType newPrediction = instance.Prediction + this->NewPredictions[instanceNumber] * factor;
                metricsCalculator.Add(queryNumber, instance.OriginalGoal, newPrediction, Pool.GetPoolQueryWeight(instance.PoolId));
            }
        }
        return -metricsCalculator.NPFound();
    }
};

template <typename TFloatType>
class TRankLossFunction : public TLossFunctionBase<TFloatType> {
private:
    struct TQueryInfo {
        double SumPredictions;
        size_t Size;

        TQueryInfo(double sumPredictions = 0., size_t size = 0)
            : SumPredictions(sumPredictions)
            , Size(size)
        {
        }

        void Add(double prediction) {
            SumPredictions += prediction;
            ++Size;
        }
    };

    TMersenne<ui64> Mersenne;

public:
    TString Name() const override {
        return "RANK";
    }

    void InitializeGoals(TPool<TFloatType>& learnPool, const TOptions&) override {
        TVector<TQueryInfo> queryInfos(learnPool.GetQueriesCount());
        for (const TInstance<TFloatType>& instance : learnPool) {
            queryInfos[instance.QueryId].Add(instance.OriginalGoal);
        }

        for (TInstance<TFloatType>& instance : learnPool) {
            const TQueryInfo& queryInfo = queryInfos[instance.QueryId];
            if (queryInfo.Size < 2) {
                instance.Goal = TFloatType();
                continue;
            }

            TFloatType sumRestGoals = queryInfo.SumPredictions - instance.OriginalGoal;
            instance.Goal = instance.OriginalGoal - sumRestGoals / (queryInfo.Size - 1);
            instance.Weight = learnPool.GetPoolQueryWeight(instance.PoolId) / queryInfo.Size;
        }
    }

    TVector<TMetricWithValues> Process(TPool<TFloatType>& learnPool,
                                               TLeastSquaresTree<TFloatType>& newTree,
                                               const TOptions& options,
                                               const THashSet<size_t>& testInstanceNumbers,
                                               TPool<TFloatType>* testPool,
                                               const size_t poolToUseNumber,
                                               const bool treeIsFirst,
                                               TFloatType* /* bias */ = nullptr) override
    {
        TVector<TVector<size_t> > queries(learnPool.GetInstancesCount());
        for (const TInstance<TFloatType>* instance = learnPool.begin(); instance != learnPool.end(); ++instance) {
            if (testInstanceNumbers.contains(instance - learnPool.begin()) ||
                (poolToUseNumber != (size_t) -1 && instance->PoolId != poolToUseNumber))
            {
                continue;
            }

            queries[instance->QueryId].push_back(instance - learnPool.begin());
        }

        TVector<TVector<std::pair<size_t, size_t> > > pairs;
        for (TVector<size_t>& query : queries) {
            TVector<std::pair<size_t, size_t> > myPairs;

            if (options.PairwiseOverhead * 2 >= query.size()) {
                for (const size_t* lhs = query.begin(); lhs != query.end(); ++lhs) {
                    for (const size_t* rhs = lhs + 1; rhs != query.end(); ++rhs) {
                        myPairs.push_back(std::make_pair(*lhs, *rhs));
                    }
                }
            } else {
                Shuffle(query.begin(), query.end(), Mersenne);

                for (size_t lhs = 0; lhs != query.size(); ++lhs) {
                    for (size_t neighbourNumber = 1; neighbourNumber <= options.PairwiseOverhead; ++neighbourNumber) {
                        size_t rhs = (lhs + neighbourNumber) % query.size();
                        myPairs.push_back(std::make_pair(query[lhs], query[rhs]));
                    }
                }
            }

            if (!myPairs.empty()) {
                pairs.push_back(myPairs);
            }
        }

        TVector<TFloatType> predictions(learnPool.GetInstancesCount());
        TFloatType* prediction = predictions.begin();
        for (const TInstance<TFloatType>* instance = learnPool.begin(); instance != learnPool.end(); ++instance, ++prediction) {
            *prediction = newTree.Prediction(instance->Features);
        }

        TFloatType factor = 1;
        if (!treeIsFirst) {
            TFloatType bestBoostFactor = TRankBoostCoefficientChooser<TFloatType>(options,
                                                                                  learnPool,
                                                                                  testInstanceNumbers,
                                                                                  predictions,
                                                                                  pairs,
                                                                                  queries).GetBestFactor();
            factor = bestBoostFactor * options.Shrinkage.Value;
        }

        TRankingMetricsCalculator<TFloatType> learnRankMetricsCalculator(learnPool.GetQueriesCount());

        size_t testQueriesCount = learnPool.GetQueriesCount();
        if (!!testPool) {
            testQueriesCount = Max(testQueriesCount, testPool->GetQueriesCount());
        }
        TRankingMetricsCalculator<TFloatType> testRankMetricsCalculator(testQueriesCount);

        prediction = predictions.begin();
        for (TInstance<TFloatType>* instance = learnPool.begin(); instance != learnPool.end(); ++instance, ++prediction) {
            instance->Prediction += *prediction * factor;
            if (instance->PoolId == 0) {
                (testInstanceNumbers.contains(instance - learnPool.begin()) ? testRankMetricsCalculator : learnRankMetricsCalculator)
                    .Add(*instance, learnPool.GetPoolQueryWeight(instance->PoolId));
            }
        }

        if (!!testPool) {
            for (TInstance<TFloatType>& instance : *testPool) {
                instance.Prediction += newTree.Prediction(instance.Features) * factor;
                testRankMetricsCalculator.Add(instance, testPool->GetPoolQueryWeight(instance.PoolId));
            }
        }

        for (TInstance<TFloatType>& instance : learnPool) {
            instance.Goal = TFloatType();
        }

        TVector<size_t> normalizers(learnPool.GetInstancesCount());
        THolder<IThreadPool> queue(CreateThreadPool(options.ThreadsCount));
        for (size_t threadNumber = 0; threadNumber < options.ThreadsCount; ++threadNumber) {
            queue->SafeAddFunc([&, threadNumber](){
                const TVector<std::pair<size_t, size_t> >* queryPairs = pairs.begin() + threadNumber;
                for (; queryPairs < pairs.end(); queryPairs += options.ThreadsCount) {
                    for (const std::pair<size_t, size_t>& queryPair : *queryPairs) {
                        TInstance<TFloatType>* leftInstance = learnPool.begin() + queryPair.first;
                        TInstance<TFloatType>* rightInstance = learnPool.begin() + queryPair.second;

                        TFloatType weight = leftInstance->OriginalGoal - rightInstance->OriginalGoal;
                        TFloatType margin = leftInstance->Prediction - rightInstance->Prediction;
                        if (weight < TFloatType()) {
                            margin = -margin;
                        }

                        leftInstance->Goal += weight * FastSigma(-margin);
                        rightInstance->Goal -= weight * FastSigma(-margin);

                        ++normalizers[queryPair.first];
                        ++normalizers[queryPair.second];
                    }
                }
            });
        }
        Delete(std::move(queue));

        for (size_t instanceNumber = 0; instanceNumber != learnPool.GetInstancesCount(); ++instanceNumber) {
            if (normalizers[instanceNumber]) {
                learnPool[instanceNumber].Goal /= normalizers[instanceNumber];
            }
        }

        TVector<TMetricWithValues> metrics;
        metrics.push_back(TMetricWithValues("PFound", learnRankMetricsCalculator.PFound(), testRankMetricsCalculator.PFound()));
        metrics.push_back(TMetricWithValues("NPFound", learnRankMetricsCalculator.NPFound(), testRankMetricsCalculator.NPFound()));
        metrics.push_back(TMetricWithValues("DCG", learnRankMetricsCalculator.DCG(), testRankMetricsCalculator.DCG()));
        metrics.push_back(TMetricWithValues("NDCG", learnRankMetricsCalculator.NDCG(), testRankMetricsCalculator.NDCG()));
        metrics.push_back(TMetricWithValues("Top@1", learnRankMetricsCalculator.TopScore(1), testRankMetricsCalculator.TopScore(1)));
        metrics.push_back(TMetricWithValues("Top@5", learnRankMetricsCalculator.TopScore(5), testRankMetricsCalculator.TopScore(5)));
        metrics.push_back(TMetricWithValues("Top@10", learnRankMetricsCalculator.TopScore(10), testRankMetricsCalculator.TopScore(10)));

        newTree.InceptWeight(factor);

        return metrics;
    }

    TVector<TMetricWithValues> GetMetrics(TPool<TFloatType>& pool, const TOptions&, size_t poolNumber = 0) override {
        TRankingMetricsCalculator<TFloatType> rankMetricsCalculator(pool.GetQueriesCount());

        for (TInstance<TFloatType>* instance = pool.begin(); instance != pool.end(); ++instance) {
            if (instance->PoolId == poolNumber) {
                rankMetricsCalculator.Add(*instance, pool.GetPoolQueryWeight(instance->PoolId));
            }
        }

        TVector<TMetricWithValues> metrics;
        metrics.push_back(TMetricWithValues("PFound", rankMetricsCalculator.PFound()));
        metrics.push_back(TMetricWithValues("NPFound", rankMetricsCalculator.NPFound()));
        metrics.push_back(TMetricWithValues("DCG", rankMetricsCalculator.DCG()));
        metrics.push_back(TMetricWithValues("NDCG", rankMetricsCalculator.NDCG()));
        metrics.push_back(TMetricWithValues("Top@1", rankMetricsCalculator.TopScore(1)));
        metrics.push_back(TMetricWithValues("Top@5", rankMetricsCalculator.TopScore(5)));
        metrics.push_back(TMetricWithValues("Top@10", rankMetricsCalculator.TopScore(10)));

        return metrics;
    }

    TFloatType TransformPrediction(TFloatType prediction) const override {
        return prediction;
    }
};

}
