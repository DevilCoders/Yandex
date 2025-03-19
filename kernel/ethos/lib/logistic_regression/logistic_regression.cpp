#include "logistic_regression.h"

#include <kernel/ethos/lib/features_selector/features_selector.h>
#include <kernel/ethos/lib/linear_model/binary_model.h>
#include <kernel/ethos/lib/metrics/binary_classification_metrics.h>

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

    template <typename TItem>
    double Factor(const TItem& item, const TLinearClassifierOptions& options, size_t labelIndex, const double positivesReWeightFactor) {
        double factor = item.IsPositive(labelIndex) ? options.PositivesFactor * positivesReWeightFactor : -options.NegativesFactor;
        factor *= item.Weight;
        return factor;
    }

    template <typename TItem>
    class TLearnerImpl {
    private:
        const TLinearClassifierOptions& Options;

    public:
        TLearnerImpl(const TLinearClassifierOptions& options)
            : Options(options)
        {
        }

        void Learn(TAnyConstIterator<TItem> begin,
                   TAnyConstIterator<TItem> end,
                   size_t labelIndex,
                   TFloatFeatureWeightMap* weights,
                   double* threshold = nullptr)
        {
            if (!Options.FeaturesToUseCount) {
                LearnWithoutFeatureSelection(begin, end, labelIndex, weights, threshold);
                return;
            }

            TFeaturesSelector<TItem> featuresSelector;
            for (TAnyConstIterator<TItem> it = begin; it != end; ++it) {
                featuresSelector.Add(*it, labelIndex);
            }
            featuresSelector.Finish(Options.FeaturesToUseCount);

            TVector<TItem> modifiedFeatures;
            for (TAnyConstIterator<TItem> it = begin; it != end; ++it) {
                modifiedFeatures.push_back(featuresSelector.SelectFeatures(*it));
            }

            LearnWithoutFeatureSelection(modifiedFeatures.begin(),
                                         modifiedFeatures.end(),
                                         labelIndex,
                                         weights,
                                         threshold);
        }

    private:
        void LearnWithoutFeatureSelection(TAnyConstIterator<TItem> begin,
                                          TAnyConstIterator<TItem> end,
                                          size_t labelIndex,
                                          TFloatFeatureWeightMap* weights,
                                          double* threshold)
        {
            switch (Options.StratificationMode) {
                case NO_STRATIFICATION:
                    LearnWithoutStratification(begin, end, labelIndex, weights);
                    break;
                case LABEL_STRATIFICATION:
                    LearnWithStratification(begin, end, labelIndex, weights);
                    break;
            }

            if (threshold) {
                *threshold = Options.ThresholdMode == SELECT_BEST_THRESHOLD
                        ? FindBestThreshold(begin, end, labelIndex, weights)
                        : 0.;
            }
        }

        void LearnWithoutStratification(TAnyConstIterator<TItem> begin,
                                        TAnyConstIterator<TItem> end,
                                        size_t labelIndex,
                                        TFloatFeatureWeightMap* weights)

        {
            TVector<const TItem*> markedItems;
            TVector<const TItem*> unmarkedItems;

            for (; begin != end; ++begin) {
                const TItem& item = *begin;
                if (item.Features.empty()) {
                    continue;
                }

                (item.HasKnownMark(labelIndex) ? markedItems : unmarkedItems).push_back(&item);
            }

            if (markedItems.empty()) {
                return;
            }

            TMersenne<ui64> mersenne;

            const size_t iterationsCount = Options.GetIterationsCount(markedItems.size() + unmarkedItems.size());
            size_t unmarkedInstanceNumber = 0;
            for (size_t iterationNumber = 0; iterationNumber < iterationsCount; ++iterationNumber) {
                Step(*markedItems[mersenne.GenRand() % markedItems.size()], labelIndex, weights, 1.);
                Step(*markedItems[mersenne.GenRand() % markedItems.size()], labelIndex, weights, 1.);

                if (Options.TransductiveMode != NO_TRANSDUCTIVE_LEARNING && !unmarkedItems.empty()) {
                    for (size_t unmarkedNumber = 0; unmarkedNumber < Options.UnmarkedCount; ++unmarkedNumber, ++unmarkedInstanceNumber) {
                        Step(*unmarkedItems[unmarkedInstanceNumber % unmarkedItems.size()], labelIndex, weights, 1.);
                    }
                }
            }
        }

        void LearnWithStratification(TAnyConstIterator<TItem> begin,
                                     TAnyConstIterator<TItem> end,
                                     size_t labelIndex,
                                     TFloatFeatureWeightMap* weights)
        {
            TVector<const TItem*> positiveItems;
            TVector<const TItem*> negativeItems;
            TVector<const TItem*> unmarkedItems;

            for (; begin != end; ++begin) {
                const TItem& item = *begin;
                if (item.Features.empty()) {
                    continue;
                }

                if (!item.HasKnownMark(labelIndex)) {
                    unmarkedItems.push_back(&item);
                } else if (item.IsPositive(labelIndex)) {
                    positiveItems.push_back(&item);
                } else {
                    negativeItems.push_back(&item);
                }
            }

            if (positiveItems.empty() || negativeItems.empty()) {
                return;
            }

            const double positivesReWeightFactor = Options.ReWeightStratification ? (double) positiveItems.size() / negativeItems.size() : 1.;

            TMersenne<ui64> mersenne;
            Shuffle(positiveItems.begin(), positiveItems.end(), mersenne);
            Shuffle(negativeItems.begin(), negativeItems.end(), mersenne);
            Shuffle(unmarkedItems.begin(), unmarkedItems.end(), mersenne);

            const size_t iterationsCount = Options.GetIterationsCount(positiveItems.size() + negativeItems.size() + unmarkedItems.size());
            size_t unmarkedInstanceNumber = 0;
            for (size_t iterationNumber = 0; iterationNumber < iterationsCount; ++iterationNumber) {
                Step(*positiveItems[iterationNumber % positiveItems.size()], labelIndex, weights, positivesReWeightFactor);
                Step(*negativeItems[iterationNumber % negativeItems.size()], labelIndex, weights, positivesReWeightFactor);

                if (Options.TransductiveMode != NO_TRANSDUCTIVE_LEARNING && !unmarkedItems.empty()) {
                    for (size_t unmarkedNumber = 0; unmarkedNumber < Options.UnmarkedCount; ++unmarkedNumber, ++unmarkedInstanceNumber) {
                        Step(*unmarkedItems[unmarkedInstanceNumber % unmarkedItems.size()], labelIndex, weights, positivesReWeightFactor);
                    }
                }
            }
        }

        void Step(const TItem& item, size_t labelIndex, TFloatFeatureWeightMap* weights, const double positivesReWeightFactor) {
            double factor = GetFactor(item, labelIndex, weights, positivesReWeightFactor);
            for (const TIndexedFloatFeature& feature : item.Features) {
                (*weights)[feature.Index] += factor * feature.Value;
            }
        }

        double GetFactor(const TItem& item, size_t labelIndex, TFloatFeatureWeightMap* weights, const double positivesReWeightFactor) {
            double nonTransformedPrediction = LinearPrediction(*weights, item.Features);

            if (item.HasKnownMark(labelIndex)) {
                double antiMargin = item.IsPositive(labelIndex)
                    ? Options.PositivesOffset - nonTransformedPrediction
                    : Options.NegativesOffset + nonTransformedPrediction;

                double mistakeProbability = Sigmoid(antiMargin);
                return mistakeProbability * Factor(item, Options, labelIndex, positivesReWeightFactor) * Options.Shrinkage;
            }

            switch (Options.TransductiveMode) {
                case TRANSDUCTIVE_RECALL:
                    return Sigmoid(-nonTransformedPrediction) * Options.Shrinkage * Options.UmarkedFactor;
                case TRANSDUCTIVE_PRECISION:
                    return Sigmoid(+nonTransformedPrediction) * Options.Shrinkage * Options.UmarkedFactor;
                case NO_TRANSDUCTIVE_LEARNING:
                default:
                    Y_ASSERT(0);
                    return 0.;
            }
        }

        double FindBestThreshold(TAnyConstIterator<TItem> begin,
                                 TAnyConstIterator<TItem> end,
                                 size_t labelIndex,
                                 TFloatFeatureWeightMap* weights)
        {
            TBcMetricsCalculator binaryClassificationMetricsCalculator;

            for (; begin != end; ++begin) {
                const TItem& item = *begin;
                if (item.HasKnownMark(labelIndex)) {
                    double rawPrediction = LinearPrediction(*weights, item.Features);

                    binaryClassificationMetricsCalculator.Add(rawPrediction, BinaryLabelFromPrediction(rawPrediction, 0.),
                                                              item.GetLabel(labelIndex),
                                                              item.Weight);
                }
            }

            return binaryClassificationMetricsCalculator.BestThreshold();
        }
    };
}

TBinaryClassificationLinearModel TBinaryLabelLogisticRegressionLearner::Learn(
        TAnyConstIterator<TBinaryLabelFloatFeatureVector> begin,
        TAnyConstIterator<TBinaryLabelFloatFeatureVector> end) const
{
    TConcurrentMutableFloatWeights weights;
    double threshold;

    TLearnerImpl<TBinaryLabelFloatFeatureVector> impl(Options);
    impl.Learn(begin, end, 0, &weights.Weights.at(0), &threshold);

    return TBinaryClassificationLinearModel(weights.ToCompactSingleLabelWeights(Options.WeightsLowerBound), threshold);
}

TFloatFeatureWeightMap TBinaryLabelLogisticRegressionLearner::LearnWeights(TAnyConstIterator<TBinaryLabelFloatFeatureVector> begin,
                                                                           TAnyConstIterator<TBinaryLabelFloatFeatureVector> end,
                                                                           double* threshold) const
{
    return NEthos::LearnWeights<TLearnerImpl<TBinaryLabelFloatFeatureVector> >(Options, begin, end, threshold);
}

void TBinaryLabelLogisticRegressionLearner::UpdateWeights(TAnyConstIterator<TBinaryLabelFloatFeatureVector> begin,
                                                          TAnyConstIterator<TBinaryLabelFloatFeatureVector> end,
                                                          TFloatFeatureWeightMap& weights,
                                                          double* threshold) const
{
    NEthos::UpdateWeights<TLearnerImpl<TBinaryLabelFloatFeatureVector> >(Options, begin, end, weights, threshold);
}

TFloatFeatureWeightMap TBinaryLabelLogisticRegressionLearner::LearnWeights(TAnyIterator<TBinaryLabelFloatFeatureVector> begin,
                                                                           TAnyIterator<TBinaryLabelFloatFeatureVector> end,
                                                                           const size_t featureNumber) const
{
    return NEthos::LearnWeights<TLearnerImpl<TBinaryLabelFloatFeatureVector> >(Options, begin, end, featureNumber);
}

TMultiLabelLinearClassifierModel TMultiLabelLogisticRegressionLearner::Learn(
    TAnyConstIterator<TMultiBinaryLabelFloatFeatureVector> begin,
    TAnyConstIterator<TMultiBinaryLabelFloatFeatureVector> end,
    const TVector<TString>& allLabels) const
{
    TConcurrentMutableFloatWeights weights(allLabels.size());
    TVector<double> thresholds(allLabels.size(), 0.);

    TLearnerImpl<TMultiBinaryLabelFloatFeatureVector> impl(Options);

    THolder<IThreadPool> queue(CreateThreadPool(Options.ThreadCount));

    ui64 learnedModelsCount = 0;
    for (size_t labelIndex = 0; labelIndex < allLabels.size(); ++labelIndex) {
        queue->SafeAddFunc([&, labelIndex]() {
            TSimpleTimer timer;
            impl.Learn(begin, end, labelIndex, &weights.Weights.at(labelIndex), &thresholds.at(labelIndex));

            {
                static TMutex lock;
                TGuard<TMutex> guard(lock);

                ++learnedModelsCount;
                double learnedPart = (double) learnedModelsCount / allLabels.size();
                Cerr << Sprintf("(%.2lf%%)\t", learnedPart * 100) << "learned model for " << allLabels[labelIndex] << " in " << timer.Get() << Endl;
            }
        });
    }

    queue->Stop();

    return TMultiLabelLinearClassifierModel(weights.ToCompactMultiLabelWeights(Options.WeightsLowerBound),
                                              thresholds,
                                              allLabels);
}

}
