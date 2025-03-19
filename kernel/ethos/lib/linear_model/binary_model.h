#pragma once

#include <kernel/ethos/lib/data/dataset.h>
#include <kernel/ethos/lib/data/linear_model.h>

#include <kernel/ethos/lib/linear_classifier_options/options.h>
#include <kernel/ethos/lib/out_of_fold/out_of_fold.h>

#include <library/cpp/containers/dense_hash/dense_hash.h>

#include <util/ysaveload.h>

namespace NEthos {

class TBinaryClassificationLinearModel {
private:
    TCompactSingleLabelFloatWeights Weights;
    double Threshold = 0.;

    bool NormalizeFeatures;
public:
    void Save(IOutputStream* s) const {
        if (NormalizeFeatures) {
            TFloatFeatureWeightMap weightsMap = Weights.GetWeightMap();
            weightsMap[(ui64) -2] = 0.;

            TCompactSingleLabelFloatWeights weights(std::move(weightsMap));
            ::Save(s, weights);
        } else {
            ::Save(s, Weights);
        }

        ::Save(s, Threshold);
    }

    void Load(IInputStream* s) {
        ::Load(s, Weights);
        ::Load(s, Threshold);

        NormalizeFeatures = Weights.Has((ui64) -2);
    }

    TBinaryClassificationLinearModel()
    {
    }

    TBinaryClassificationLinearModel(const TCompactSingleLabelFloatWeights& weights, double threshold, bool normalizeFeatures = false)
        : Weights(weights)
        , Threshold(threshold)
        , NormalizeFeatures(normalizeFeatures)
    {
    }

    TBinaryLabelWithFloatPrediction Apply(const TFloatFeatureVector& features, const TApplyOptions* applyOptions = nullptr) const;
    TBinaryLabelWithFloatPrediction Apply(const TBinaryLabelFloatFeatureVector& features, const TApplyOptions* applyOptions = nullptr) const;

    double ZeroThresholdWeightPrediction(const TFloatFeatureVector& features, const TApplyOptions* applyOptions = nullptr) const;
    double ProbabilityPrediction(const TFloatFeatureVector& features, const TApplyOptions* applyOptions = nullptr) const;

    TBinaryClassificationLinearModel& operator+=(const TBinaryClassificationLinearModel& other);

    double GetThreshold() const;

    const TCompactSingleLabelFloatWeights& GetWeights() const;

    void ResetThreshold() {
        Threshold = 0.;
    }
};

template <typename TLearnerImpl>
TFloatFeatureWeightMap LearnWeights(const TLinearClassifierOptions& options,
                                    TAnyConstIterator<TBinaryLabelFloatFeatureVector> begin,
                                    TAnyConstIterator<TBinaryLabelFloatFeatureVector> end,
                                    double* threshold = nullptr)
{
    TFloatFeatureWeightMap weights;
    TLearnerImpl impl(options);
    impl.Learn(begin, end, 0, &weights, threshold);
    return weights;
}

template <typename TLearnerImpl>
void UpdateWeights(const TLinearClassifierOptions& options,
                   TAnyConstIterator<TBinaryLabelFloatFeatureVector> begin,
                   TAnyConstIterator<TBinaryLabelFloatFeatureVector> end,
                   TFloatFeatureWeightMap& weights,
                   double* threshold = nullptr)
{
    TLearnerImpl impl(options);
    impl.Learn(begin, end, 0, &weights, threshold);
}

template <typename TLearnerImpl>
TFloatFeatureWeightMap LearnWeights(const TLinearClassifierOptions& options,
                                    TAnyIterator<TBinaryLabelFloatFeatureVector> begin,
                                    TAnyIterator<TBinaryLabelFloatFeatureVector> end,
                                    const size_t featureNumber)
{
    switch (options.FeatureGenerationMode) {
    case EFeatureGenerationMode::OUT_OF_GROWING_FOLD: return LearnGrowingFoldFeatures<TLearnerImpl>(options, begin, end, featureNumber);
    case EFeatureGenerationMode::OUT_OF_FOLD: return LearnOutOfFoldFeatures<TLearnerImpl>(options, begin, end, featureNumber);
    }
    Y_ASSERT(0);
    return TFloatFeatureWeightMap();
}

}
