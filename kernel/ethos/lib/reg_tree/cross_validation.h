#pragma once

#include "compositions.h"
#include "options.h"

#include <util/random/mersenne.h>
#include <util/random/shuffle.h>

#include <util/string/vector.h>

namespace NRegTree {

inline void AddMetrics(TVector<TMetricWithValues>& target, TVector<TMetricWithValues>& source) {
    if (target.empty()) {
        target = source;
    } else {
        TMetricWithValues* sourceMetric = source.begin();
        for (TMetricWithValues* targetMetric = target.begin(); targetMetric != target.end(); ++targetMetric, ++sourceMetric) {
            *targetMetric += *sourceMetric;
        }
    }
}

inline void AddMetrics(TVector<TVector<TMetricWithValues> >& target, TVector<TVector<TMetricWithValues> >& source) {
    if (target.empty()) {
        target = source;
    } else {
        TVector<TMetricWithValues>* sourceMetrics = source.begin();
        for (TVector<TMetricWithValues>* targetMetrics = target.begin(); targetMetrics != target.end(); ++targetMetrics, ++sourceMetrics) {
            AddMetrics(*targetMetrics, *sourceMetrics);
        }
    }
}

template <typename TFloatType>
inline TVector<TVector<TMetricWithValues> > QrossValidation(const TPool<TFloatType>& pool,
                                                            const TVector<TFeatureIterator<TFloatType> >& featureIterators,
                                                            const TOptions& options,
                                                            bool quiet = false)
{
    TVector<size_t> queries(pool.GetQueriesCount());
    for (size_t i = 0; i < queries.size(); ++i) {
        queries[i] = i;
    }

    TMersenne<size_t> mersenne;
    for (size_t shuffleRun = 0; shuffleRun < options.CVSeed % 1023; ++shuffleRun) {
        for (size_t i = 0; i < pool.GetInstancesCount(); ++i) {
            mersenne.GenRand();
        }
    }

    TVector<TVector<TMetricWithValues> > runsSumMetrics;

    for (size_t cvRun = 1; cvRun <= options.CVRuns; ++cvRun) {
        Shuffle(queries.begin(), queries.end(), mersenne);

        THashMap<size_t, size_t> queryIdToFoldNumber;
        for (size_t i = 0; i < queries.size(); ++i) {
            queryIdToFoldNumber[queries[i]] = i % options.CVFoldsCount;
        }

        TVector<THashSet<size_t> > folds(options.CVFoldsCount);
        for (size_t i = 0; i < pool.GetInstancesCount(); ++i) {
            if (pool[i].PoolId == 0) {
                folds[queryIdToFoldNumber[pool[i].QueryId]].insert(i);
            }
        }

        TVector<TVector<TMetricWithValues> > sumMetrics;
        for (const THashSet<size_t>* fold = folds.begin(); fold != folds.end(); ++fold) {
            TVector<TVector<TMetricWithValues> > metrics = TBoosting<TFloatType>().Learn(pool, featureIterators, options, "", *fold);
            AddMetrics(sumMetrics, metrics);
        }
        AddMetrics(runsSumMetrics, sumMetrics);

        if (!quiet) {
            Cout << "cross-validation metrics over " << cvRun << " runs:\n";
            PrintMetrics(Cout, runsSumMetrics, options.CVFoldsCount * cvRun);
            Cout << "\n";
        }
    }

    for (TVector<TMetricWithValues>& sumMetrics : runsSumMetrics) {
        for (TMetricWithValues& sumMetric : sumMetrics) {
            sumMetric.LearnValue /= options.CVFoldsCount * options.CVRuns;
            sumMetric.TestValue /= options.CVFoldsCount * options.CVRuns;
        }
    }

    return runsSumMetrics;
}

template <typename TFloatType>
inline TVector<TVector<TMetricWithValues> > CrossValidation(const TPool<TFloatType>& pool,
                                                            const TVector<TFeatureIterator<TFloatType> >& featureIterators,
                                                            const TOptions& options,
                                                            bool quiet = false)
{
    TVector<size_t> positiveInstanceNumbers;
    TVector<size_t> negativeInstanceNumbers;
    for (size_t instanceNumber = 0; instanceNumber < pool.GetInstancesCount(); ++instanceNumber) {
        if (options.LossFunctionName != "LOGISTIC" || pool[instanceNumber].OriginalGoal > options.ClassificationThreshold) {
             positiveInstanceNumbers.push_back(instanceNumber);
        } else {
             negativeInstanceNumbers.push_back(instanceNumber);
        }
    }

    TMersenne<size_t> mersenne;
    for (size_t shuffleRun = 0; shuffleRun < options.CVSeed % 1023; ++shuffleRun) {
        for (size_t i = 0; i < pool.GetInstancesCount(); ++i) {
            mersenne.GenRand();
        }
    }

    TVector<TVector<TMetricWithValues> > runsSumMetrics;

    for (size_t cvRun = 1; cvRun <= options.CVRuns; ++cvRun) {
        Shuffle(positiveInstanceNumbers.begin(), positiveInstanceNumbers.end(), mersenne);
        Shuffle(negativeInstanceNumbers.begin(), negativeInstanceNumbers.end(), mersenne);
        TVector<THashSet<size_t> > folds(options.CVFoldsCount);
        for (size_t i = 0; i < positiveInstanceNumbers.size(); ++i) {
            if (pool[positiveInstanceNumbers[i]].PoolId == 0) {
                folds[i % options.CVFoldsCount].insert(positiveInstanceNumbers[i]);
            }
        }
        for (size_t i = 0; i < negativeInstanceNumbers.size(); ++i) {
            if (pool[negativeInstanceNumbers[i]].PoolId == 0) {
                folds[i % options.CVFoldsCount].insert(negativeInstanceNumbers[i]);
            }
        }

        TVector<TVector<TMetricWithValues> > sumMetrics;
        for (const THashSet<size_t>& fold : folds) {
            TVector<TVector<TMetricWithValues> > metrics = TBoosting<TFloatType>().Learn(pool, featureIterators, options, "", fold);
            AddMetrics(sumMetrics, metrics);
        }
        AddMetrics(runsSumMetrics, sumMetrics);

        if (!quiet) {
            Cout << "cross-validation metrics over " << cvRun << " runs:\n";
            PrintMetrics(Cout, runsSumMetrics, options.CVFoldsCount * cvRun);
            Cout << "\n";
        }
    }

    for (TVector<TMetricWithValues>& sumMetrics : runsSumMetrics) {
        for (TMetricWithValues& sumMetric : sumMetrics) {
            sumMetric.LearnValue /= options.CVFoldsCount * options.CVRuns;
            sumMetric.TestValue /= options.CVFoldsCount * options.CVRuns;
        }
    }

    return runsSumMetrics;
}

namespace NPrivate {

struct TOptionsWithMetrics {
    TOptions Options;
    TVector<TVector<TMetricWithValues> > Metrics;

    TOptionsWithMetrics(const TOptions& options)
        : Options(options)
    {
    }

    bool operator < (const TOptionsWithMetrics& other) const {
        return Metrics.front().front().TestValue > other.Metrics.front().front().TestValue;
    }

    bool operator > (const TOptionsWithMetrics& other) const {
        return other < *this;
    }
};

}

template <typename TFloatType>
inline void SelectFeatures(const TPool<TFloatType>& pool,
                           const TVector<TFeatureIterator<TFloatType> >& featureIterators,
                           TOptions baseOptions,
                           TVector<size_t> featuresForEval,
                           const TVector<size_t>& deletedFeatures)
{
    TOptions currentOptions(baseOptions);

    currentOptions.IgnoredFeatures.clear();
    for (const size_t featureToDelete : deletedFeatures) {
        if (featureToDelete < pool.GetFeaturesCount()) {
            currentOptions.IgnoredFeatures.insert(featureToDelete);
        }
    }

    if (featuresForEval.empty()) {
        for (size_t featureNumber = 0; featureNumber < pool.GetFeaturesCount(); ++featureNumber) {
            if (!currentOptions.IgnoredFeatures.contains(featureNumber)) {
                featuresForEval.push_back(featureNumber);
            }
        }
    }

    for (const size_t featureToEval : featuresForEval) {
        if (featureToEval < pool.GetFeaturesCount()) {
            currentOptions.IgnoredFeatures.insert(featureToEval);
        }
    }

    NPrivate::TOptionsWithMetrics bestOptions(currentOptions);

    TVector<size_t> selectedFeatures;
    for (size_t featureNumber = 0; featureNumber < pool.GetFeaturesCount(); ++featureNumber) {
        if (!currentOptions.IgnoredFeatures.contains(featureNumber)) {
            selectedFeatures.push_back(featureNumber);
        }
    }
    TVector<size_t> bestSelectedFeatures(selectedFeatures);

    for (size_t featuresToAddCount = 0; featuresToAddCount < featuresForEval.size(); ++featuresToAddCount) {
        TVector<NPrivate::TOptionsWithMetrics> optionsToEval;
        TVector<size_t> usedFeatures;
        for (const size_t featureToEval : featuresForEval) {
            if (!currentOptions.IgnoredFeatures.contains(featureToEval)) {
                continue;
            }
            optionsToEval.push_back(NPrivate::TOptionsWithMetrics(currentOptions));
            optionsToEval.back().Options.IgnoredFeatures.erase(featureToEval);
            usedFeatures.push_back(featureToEval);
        }

        THolder<IThreadPool> queue = CreateThreadPool(baseOptions.SelectFeaturesThreads);
        for (size_t i = 0; i < optionsToEval.size(); ++i) {
            queue->SafeAddFunc([&, i](){
                NPrivate::TOptionsWithMetrics& optionsWithMetrics = optionsToEval[i];
                optionsWithMetrics.Metrics = optionsWithMetrics.Options.UseQueries
                    ? QrossValidation(pool, featureIterators, optionsWithMetrics.Options, true)
                    : CrossValidation(pool, featureIterators, optionsWithMetrics.Options, true);

                static TMutex lock;
                TGuard<TMutex> guard(lock);

                Cout << "    added " + ToString(usedFeatures[i]) << "\n";
                PrintMetrics(Cout, optionsWithMetrics.Metrics);
                Cout << Endl;
            });
        }
        queue->Stop();

        NPrivate::TOptionsWithMetrics* newOptions = baseOptions.LossFunctionName == "RANK"
            ? MinElement(optionsToEval.begin(), optionsToEval.end())
            : MaxElement(optionsToEval.begin(), optionsToEval.end());

        if (!newOptions) {
            break;
        }

        selectedFeatures.push_back(usedFeatures[newOptions - optionsToEval.begin()]);
        currentOptions = newOptions->Options;

        if (featuresToAddCount == 0 || (baseOptions.LossFunctionName == "RANK" ? *newOptions < bestOptions : *newOptions > bestOptions)) {
            bestOptions = *newOptions;
            bestSelectedFeatures = selectedFeatures;
        }

        Cout << JoinVectorIntoString(selectedFeatures, ":") << "\n";
        PrintMetrics(Cout, newOptions->Metrics);
        Cout << "\n===================\n";
        Cout << Endl;
    }

    Cout << "best features set:\n";
    Cout << JoinVectorIntoString(bestSelectedFeatures, ":") << "\n";
    PrintMetrics(Cout, bestOptions.Metrics);
    Cout << Endl;
}

}
