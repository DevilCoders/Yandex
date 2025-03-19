#pragma once

#include "optimization.h"
#include "options.h"
#include "pool.h"
#include "splitter.h"

#include <library/cpp/statistics/statistics.h>

#include <util/ysaveload.h>

namespace NRegTree {

struct TTransformerBase {
    ETransformationType TransformationType = ETransformationType::Identity;

    double Factor;
    double Offset;

    double FeatureMean;
    double FeatureFactor;

    TVector<double> Thresholds;

    Y_SAVELOAD_DEFINE(TransformationType, Factor, Offset, FeatureMean, FeatureFactor, Thresholds);

    void ResetTransformation() {
        Factor = 1.;
        Offset = 0.;

        FeatureMean = 0.;

        FeatureFactor = 0.;
        switch (TransformationType) {
        case SmartLogify:
            FeatureFactor = 1.;
            break;
        default:
            break;
        }

        TVector<double>().swap(Thresholds);
    }

    template <typename TFloatType>
    TFloatType Transformation(const TFloatType feature) const {
        switch (TransformationType) {
        case SimpleLogify:
        case SmartLogify:  return feature > FeatureMean ? Factor * log((feature - FeatureMean) * FeatureFactor + 1.) + Offset
                                                        : -Factor * log((FeatureMean - feature) * FeatureFactor + 1.) + Offset;
        case SimpleSigma:
        case SmartSigma:   return (feature - FeatureMean) / (fabs(feature - FeatureMean) + FeatureFactor + 1.) * Factor + Offset;

        case Quantile:     return (UpperBound(Thresholds.begin(), Thresholds.end(), feature) - Thresholds.begin() + 0.5) / (Thresholds.size() + 1);
        case Identity:     return feature;
        }

        Y_ASSERT(0);
        return feature;
    }
};

struct TParametersTuningResult : public TTransformerBase {
    double SumSquaredErrors;

    TParametersTuningResult(const TTransformerBase& transformer, double sse = 0.)
        : TTransformerBase(transformer)
        , SumSquaredErrors(sse)
    {
    }

    template <typename TFloatType>
    void Update(const TPool<TFloatType>& pool, size_t featureNumber, const THashSet<size_t>& testInstanceNumbers = THashSet<size_t>()) {
        TSimpleLinearRegressionSplitterBase<double> slrSolver;
        for (const TInstance<TFloatType>* instance = pool.begin(); instance != pool.end(); ++instance) {
            if (testInstanceNumbers.contains(instance - pool.begin())) {
                continue;
            }
            slrSolver.Add(instance->Goal, 1., this->Transformation(instance->Features[featureNumber]));
        }

        std::pair<double, double> slr = slrSolver.Solve();
        this->Factor *= slr.first;
        this->Offset = this->Offset * slr.first + slr.second;
        SumSquaredErrors = slrSolver.SumSquaredErrors();
    }

    bool operator < (const TParametersTuningResult& other) const {
        return SumSquaredErrors < other.SumSquaredErrors;
    }
};

template <typename TFloatType>
static inline TParametersTuningResult FindMinimum(const TTransformerBase base,
                                                  double start,
                                                  double step,
                                                  double TTransformerBase::* field,
                                                  const TPool<TFloatType>& pool,
                                                  size_t featureNumber,
                                                  const THashSet<size_t>& testInstanceNumbers = THashSet<size_t>())
{
    const double goldenRatio = (sqrt(5.) - 1) / 2;

    TParametersTuningResult preleft(base);
    preleft.*field = start;
    preleft.Update(pool, featureNumber, testInstanceNumbers);

    TParametersTuningResult left = preleft;

    TParametersTuningResult right = preleft;
    right.*field = start + step;
    right.Update(pool, featureNumber, testInstanceNumbers);

    while (right < left) {
        preleft = left;
        left = right;

        step /= goldenRatio;

        right.*field = start + step;
        right.Update(pool, featureNumber, testInstanceNumbers);
    }

    TParametersTuningResult leftMiddle = left;

    TParametersTuningResult rightMiddle = left;
    rightMiddle.*field = left.*field + (right.*field - left.*field) * goldenRatio;
    rightMiddle.Update(pool, featureNumber, testInstanceNumbers);

    left = preleft;

    while (right.*field - left.*field > 1e-5 && right.*field > left.*field * 1.0001) {
        if (leftMiddle < rightMiddle) {
            right = rightMiddle;
            rightMiddle = leftMiddle;

            leftMiddle.*field = right.*field - (right.*field - left.*field) * goldenRatio;
            leftMiddle.Update(pool, featureNumber, testInstanceNumbers);
        } else {
            left = leftMiddle;
            leftMiddle = rightMiddle;

            rightMiddle.*field = left.*field + (right.*field - left.*field) * goldenRatio;
            rightMiddle.Update(pool, featureNumber, testInstanceNumbers);
        }
    }

    return left;
}

class TTransformer : public TTransformerBase {
public:
    TTransformer(const TTransformerBase& transformer = TTransformerBase())
        : TTransformerBase(transformer)
    {
    }

    template <typename TFloatType>
    void Learn(const TPool<TFloatType>& pool,
               const TOptions& options,
               size_t featureNumber,
               const THashSet<size_t>& testInstanceNumbers = THashSet<size_t>())
    {
        this->TransformationType = options.TransformationType;
        this->ResetTransformation();

        NStatistics::TStatisticsCalculator<double> statisticsCalculator;
        TVector<double> featureValues;

        switch (TransformationType) {
        case SimpleSigma:
        case SimpleLogify:
            for (const TInstance<TFloatType>* instance = pool.begin(); instance != pool.end(); ++instance) {
                if (testInstanceNumbers.contains(instance - pool.begin())) {
                    continue;
                }
                statisticsCalculator.Push(instance->Features[featureNumber]);
            }
            FeatureMean = statisticsCalculator.Mean();
            FeatureFactor = TransformationType == SimpleSigma
                            ? statisticsCalculator.StandardDeviation()
                            : 1. / (statisticsCalculator.StandardDeviation() + 1.);
            break;
        case SmartLogify:
        case SmartSigma:
            for (size_t i = 0; i < options.TransformationIterations; ++i) {
                IterateMean(pool, featureNumber, testInstanceNumbers);
                IterateFeatureFactor(pool, featureNumber, testInstanceNumbers);
            }
            break;
        case Quantile:
            for (const TInstance<TFloatType>* instance = pool.begin(); instance != pool.end(); ++instance) {
                if (testInstanceNumbers.contains(instance - pool.begin())) {
                    continue;
                }
                featureValues.push_back(instance->Features[featureNumber]);
            }
            Sort(featureValues.begin(), featureValues.end());

            {
                size_t passed = 0;
                for (size_t i = 0; i < featureValues.size(); ++i, ++passed) {
                    if (passed * options.DiscretizationLevelsCount < featureValues.size()) {
                        continue;
                    }
                    if (featureValues[i] > featureValues[i - 1] + 1e-6) {
                        this->Thresholds.push_back((featureValues[i] + featureValues[i - 1]) / 2);
                        passed = 0;
                    }
                }
                if (passed * options.DiscretizationLevelsCount * 4 < featureValues.size()) {
                    Thresholds.pop_back();
                }
            }

            break;
        case Identity:
            break;
        }
    }
private:
    template <typename TFloatType>
    void IterateFeatureFactor(const TPool<TFloatType>& pool,
                              size_t featureNumber,
                              const THashSet<size_t>& testInstanceNumbers = THashSet<size_t>())
    {
        *this = (TTransformerBase) FindMinimum((TTransformerBase) *this,
                                               0.,
                                               1.,
                                               &TTransformerBase::FeatureFactor,
                                               pool,
                                               featureNumber,
                                               testInstanceNumbers);
    }

    template <typename TFloatType>
    void IterateMean(const TPool<TFloatType>& pool,
                     size_t featureNumber,
                     const THashSet<size_t>& testInstanceNumbers = THashSet<size_t>())
    {
        TParametersTuningResult lhs = FindMinimum((TTransformerBase) *this,
                                                  TransformationType == SmartSigma ? FeatureMean : 0.,
                                                 -1.,
                                                  &TTransformerBase::FeatureMean,
                                                  pool,
                                                  featureNumber,
                                                  testInstanceNumbers);
        TParametersTuningResult rhs = FindMinimum((TTransformerBase) *this,
                                                  TransformationType == SmartSigma ? FeatureMean : 0.,
                                                 +1.,
                                                  &TTransformerBase::FeatureMean,
                                                  pool,
                                                  featureNumber,
                                                  testInstanceNumbers);
        *this = (TTransformerBase) Min(lhs, rhs);
    }
};

class TFeaturesTransformer {
private:
    TVector<TTransformerBase> Transformers;
public:
    Y_SAVELOAD_DEFINE(Transformers);

    size_t Size() const {
        return Transformers.size();
    }

    bool IsFake() const {
        return Transformers.empty() || Transformers.front().TransformationType == ETransformationType::Identity;
    }

    const TTransformerBase& operator [] (const size_t featureNumber) const {
        return Transformers[featureNumber];
    }

    template <typename TFloatType>
    void Learn(const TPool<TFloatType>& pool, const TOptions& options, const THashSet<size_t>& testInstanceNumbers = THashSet<size_t>()) {
        Transformers.resize(pool.GetFeaturesCount());

        const size_t firstFeatureNumberToTransform = options.LossFunctionName == "SURPLUS" ? 1 : 0;
        THolder<IThreadPool> queue = CreateThreadPool(options.ThreadsCount);
        for (size_t featureNumber = firstFeatureNumberToTransform; featureNumber < Transformers.size(); ++featureNumber) {
            Transformers[featureNumber].TransformationType = options.TransformationType;
            Transformers[featureNumber].ResetTransformation();
            queue->SafeAddFunc([&, this, featureNumber](){
                TTransformer transformer;
                transformer.Learn(pool, options, featureNumber, testInstanceNumbers);
                this->Transformers[featureNumber] = (TTransformerBase) transformer;
            });
        }
        Delete(std::move(queue));
    }

    void Fix(const size_t originalFeaturesCount, const TSet<size_t> ignoredFeatures) {
        TVector<TTransformerBase> fixedTransformers(originalFeaturesCount);

        for (TTransformerBase& transformer : fixedTransformers) {
            transformer.ResetTransformation();
            transformer.TransformationType = ETransformationType::Identity;
        }

        const TTransformerBase* learnedTransformer = Transformers.begin();
        for (size_t featureNumber = 0; featureNumber < originalFeaturesCount; ++featureNumber) {
            if (ignoredFeatures.contains(featureNumber)) {
                continue;
            }
            fixedTransformers[featureNumber] = *learnedTransformer;
            ++learnedTransformer;
        }

        fixedTransformers.swap(Transformers);
    }

    template <typename TFloatType>
    TFloatType TransformFeature(const TFloatType feature, const size_t featureNumber) const {
        return Transformers[featureNumber].Transformation(feature);
    }

    template <typename TFloatType>
    void TransformFeatures(TVector<TFloatType>& features) const {
        Y_ASSERT(features.size() == Transformers.size());

        TFloatType* feature = features.begin();
        const TTransformerBase* transformer = Transformers.begin();
        for (; feature != features.end(); ++feature, ++transformer) {
            *feature = transformer->Transformation(*feature);
        }
    }

    template <typename TFloatType>
    void TransformPool(TPool<TFloatType>& pool) const {
        if (IsFake()) {
            return;
        }

        for (TInstance<TFloatType>& instance : pool) {
            TransformFeatures(instance.Features);
        }
    }
};

}
