#include "naive_bayes.h"

#include <kernel/ethos/lib/features_selector/features_selector.h>
#include <kernel/ethos/lib/metrics/binary_classification_metrics.h>

#include <library/cpp/linear_regression/welford.h>

#include <util/datetime/cputimer.h>
#include <util/generic/hash.h>
#include <util/generic/array_ref.h>
#include <util/random/mersenne.h>
#include <util/random/shuffle.h>
#include <util/string/printf.h>
#include <util/system/mutex.h>
#include <util/thread/pool.h>

namespace NEthos {

namespace {
    class TLearnerImpl {
    private:
        const TLinearClassifierOptions& Options;

    public:
        TLearnerImpl(const TLinearClassifierOptions& options)
            : Options(options)
        {
        }

        void Learn(TAnyConstIterator<TBinaryLabelFloatFeatureVector> begin,
                   TAnyConstIterator<TBinaryLabelFloatFeatureVector> end,
                   size_t labelIndex,
                   TFloatFeatureWeightMap* weights,
                   double* threshold = nullptr)
        {
            if (!Options.FeaturesToUseCount) {
                LearnWithoutFeatureSelection(begin, end, labelIndex, weights);
            } else {
                TFeaturesSelector<TBinaryLabelFloatFeatureVector> featuresSelector;
                for (TAnyConstIterator<TBinaryLabelFloatFeatureVector> it = begin; it != end; ++it) {
                    featuresSelector.Add(*it, labelIndex);
                }
                featuresSelector.Finish(Options.FeaturesToUseCount);

                TVector<TBinaryLabelFloatFeatureVector> modifiedFeatures;
                for (TAnyConstIterator<TBinaryLabelFloatFeatureVector> it = begin; it != end; ++it) {
                    modifiedFeatures.push_back(featuresSelector.SelectFeatures(*it));
                }

                LearnWithoutFeatureSelection(modifiedFeatures.begin(),
                                             modifiedFeatures.end(),
                                             labelIndex,
                                             weights);
            }

            if (threshold) {
                *threshold = FindBestThreshold(begin, end, labelIndex, weights);
            }
        }
    private:
        void LearnWithoutFeatureSelection(TAnyConstIterator<TBinaryLabelFloatFeatureVector> begin,
                                          TAnyConstIterator<TBinaryLabelFloatFeatureVector> end,
                                          size_t labelIndex,
                                          TFloatFeatureWeightMap* weights)
        {
            TVector<const TBinaryLabelFloatFeatureVector*> learnItems;

            for (TAnyConstIterator<TBinaryLabelFloatFeatureVector> it = begin; it != end; ++it) {
                const TBinaryLabelFloatFeatureVector& item = *it;
                if (item.Features.empty()) {
                    continue;
                }

                if (!item.HasKnownMark()) {
                    continue;
                }

                if (item.IsPositive(labelIndex) == Options.LearnBayesOnNegatives) {
                    continue;
                }

                learnItems.push_back(&item);
            }

            if (learnItems.empty()) {
                return;
            }

            TVector<double> itemWeights(learnItems.size(), 1.);

            TDenseHash<ui64, double> featureWeights;
            for (const TBinaryLabelFloatFeatureVector* item : learnItems) {
                for (const TIndexedFloatFeature& feature : item->Features) {
                    featureWeights[feature.Index] = 1.;
                }
            }

            double sumFeatureWeights = featureWeights.Size();
            for (size_t iterationNumber = 0; iterationNumber <= Options.BayesIterationsCount; ++iterationNumber) {
                TDenseHash<ui64, double> newFeatureWeights;
                sumFeatureWeights = 0;

                for (size_t itemNumber = 0; itemNumber < learnItems.size(); ++itemNumber) {
                    const TBinaryLabelFloatFeatureVector& item = *learnItems[itemNumber];
                    const double itemWeight = itemWeights[itemNumber];

                    for (const TIndexedFloatFeature& feature : item.Features) {
                        newFeatureWeights[feature.Index] += feature.Value * itemWeight;
                        sumFeatureWeights += feature.Value * itemWeight;
                    }
                }

                for (auto& featureWithNewWeight : newFeatureWeights) {
                    const ui64 featureIdx = featureWithNewWeight.first;
                    const double newWeight = featureWithNewWeight.second;
                    featureWeights[featureIdx] = newWeight + Options.BayesRegularizationParameter;
                }

                for (size_t itemNumber = 0; itemNumber < learnItems.size(); ++itemNumber) {
                    const TBinaryLabelFloatFeatureVector& item = *learnItems[itemNumber];
                    double& itemWeight = itemWeights[itemNumber];

                    itemWeight = 0.;
                    double normalizer = 0.;
                    for (const TIndexedFloatFeature& feature : item.Features) {
                        itemWeight -= log(featureWeights.Value(feature.Index, 0)
                            / sumFeatureWeights) * feature.Value;
                        normalizer += feature.Value;
                    }
                    if (normalizer) {
                        itemWeight /= normalizer;
                    }
                }
            }

            for (auto& featureWithWeight : featureWeights) {
                (*weights)[featureWithWeight.first] = log(featureWithWeight.second / sumFeatureWeights);
            }
            weights->SetupDefaultFeatureWeight(log(Options.BayesRegularizationParameter / sumFeatureWeights));
        }

        double FindBestThreshold(TAnyConstIterator<TBinaryLabelFloatFeatureVector> begin,
                                 TAnyConstIterator<TBinaryLabelFloatFeatureVector> end,
                                 size_t labelIndex,
                                 TFloatFeatureWeightMap* weights)
        {
            TBcMetricsCalculator binaryClassificationMetricsCalculator;

            for (; begin != end; ++begin) {
                const TBinaryLabelFloatFeatureVector& item = *begin;
                if (!item.HasKnownMark(labelIndex)) {
                    continue;
                }

                double rawPrediction = NormalizedLinearPrediction(*weights, item.Features);
                EBinaryClassLabel targetLabel = item.IsPositive(labelIndex) == Options.LearnBayesOnNegatives ? EBinaryClassLabel::BCL_NEGATIVE
                                                                                                             : EBinaryClassLabel::BCL_POSITIVE;

                binaryClassificationMetricsCalculator.Add(rawPrediction, EBinaryClassLabel::BCL_NEGATIVE, targetLabel, item.Weight);
            }

            return binaryClassificationMetricsCalculator.BestThreshold();
        }
    };
}

TBinaryClassificationLinearModel TBinaryLabelNaiveBayesLearner::Learn(
        TAnyConstIterator<TBinaryLabelFloatFeatureVector> begin,
        TAnyConstIterator<TBinaryLabelFloatFeatureVector> end) const
{
    TConcurrentMutableFloatWeights weights;
    double threshold;

    TLearnerImpl impl(Options);
    impl.Learn(begin, end, 0, &weights.Weights.at(0), &threshold);

    return TBinaryClassificationLinearModel(weights.ToCompactSingleLabelWeights(Options.WeightsLowerBound), threshold, true);
}

TFloatFeatureWeightMap TBinaryLabelNaiveBayesLearner::LearnWeights(TAnyConstIterator<TBinaryLabelFloatFeatureVector> begin,
                                                                   TAnyConstIterator<TBinaryLabelFloatFeatureVector> end,
                                                                   double* threshold) const
{
    return NEthos::LearnWeights<TLearnerImpl>(Options, begin, end, threshold);
}

TFloatFeatureWeightMap TBinaryLabelNaiveBayesLearner::LearnWeights(TAnyIterator<TBinaryLabelFloatFeatureVector> begin,
                                                                   TAnyIterator<TBinaryLabelFloatFeatureVector> end,
                                                                   const size_t featureNumber) const
{
    return NEthos::LearnWeights<TLearnerImpl>(Options, begin, end, featureNumber);
}

}
