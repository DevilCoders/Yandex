#pragma once

#include <kernel/ethos/lib/linear_classifier_options/options.h>

#include <library/cpp/containers/dense_hash/dense_hash.h>
#include <library/cpp/linear_regression/welford.h>

#include <utility>
#include <util/generic/ymath.h>
#include <util/ysaveload.h>

namespace NEthos {

template <typename TWeight>
class TFeatureWeightMap : public TDenseHash<ui64, TWeight> {
private:
    using TBase = TDenseHash<ui64, TWeight>;
public:
    TFeatureWeightMap()
        : TBase((ui64) -1)
    {
    }

    TFeatureWeightMap Minimize(const double lowerBound) const {
        TFeatureWeightMap<TWeight> compactMap;

        for (auto& weight : *this) {
            if (fabs(weight.second) > lowerBound) {
                compactMap.emplace(weight);
            }
        }

        return compactMap;
    }

    void SetupDefaultFeatureWeight(const double weight) {
        (*this)[(ui64) -3] = weight;
    }

    double GetDefaultFeatureWeight() const {
        return this->Value((ui64) -3, 0);
    }
};

using TFloatFeatureWeightMap = TFeatureWeightMap<float>;

template <typename TWeight>
double LinearPrediction(const TFeatureWeightMap<TWeight>& weights,
                        const TFloatFeatureVector& features,
                        const TApplyOptions* applyOptions = nullptr)
{
    double result = 0.;
    size_t length = features.size();

    if (!applyOptions || (!applyOptions->MaxTokens && !applyOptions->MinTokens)) {
        for (const TIndexedFloatFeature& feature : features) {
            if (const TWeight* weight = weights.FindPtr(feature.Index)) {
                result += feature.Value * *weight;
            }
        }
    } else {
        TVector<double> tokenWeights;
        tokenWeights.reserve(length);

        for (const TIndexedFloatFeature& feature : features) {
            if (const TWeight* weight = weights.FindPtr(feature.Index)) {
                tokenWeights.push_back(feature.Value * *weight);
            } else {
                tokenWeights.push_back(0.);
            }
        }

        Sort(tokenWeights.begin(), tokenWeights.end());

        size_t newLength = 0;
        for (size_t i = 0; i < length; ++i) {
            if (i < applyOptions->MinTokens || i + applyOptions->MaxTokens >= length) {
                result += tokenWeights[i];
                ++newLength;
            }
        }
        length = newLength;
    }

    if (applyOptions && applyOptions->NormalizeOnLength && length) {
        result /= length;
    }

    return result;
}

template <typename TWeight>
double NormalizedLinearPrediction(const TFeatureWeightMap<TWeight>& weights,
                                  const TFloatFeatureVector& features,
                                  const TApplyOptions* applyOptions = nullptr)
{
    TMeanCalculator meanCalculator;
    for (const TIndexedFloatFeature& feature : features) {
        if (const TWeight* weight = weights.FindPtr(feature.Index)) {
            meanCalculator.Add(*weight, feature.Value);
        } else {
            meanCalculator.Add(weights.GetDefaultFeatureWeight(), feature.Value);
        }
    }

    double result = meanCalculator.GetMean();
    if (applyOptions && applyOptions->NormalizeOnLength && features.size()) {
        result /= features.size();
    }

    return result;
}

template <typename TWeight>
class TCompactSingleLabelWeights {
private:
    TFeatureWeightMap<TWeight> Weights;

public:
    Y_SAVELOAD_DEFINE(Weights);

    TCompactSingleLabelWeights() {
    }

    TCompactSingleLabelWeights(const TFeatureWeightMap<TWeight>& weights)
        : Weights(weights)
    {
    }

    TCompactSingleLabelWeights(TFeatureWeightMap<TWeight>&& weights)
        : Weights(std::move(weights))
    {
    }

    TCompactSingleLabelWeights& operator+=(const TCompactSingleLabelWeights& rhs) {
        for (auto& indexWithWeight : rhs.Weights) {
            Weights[indexWithWeight.first] += indexWithWeight.second;
        }

        return *this;
    }

    double RawPrediction(const TFloatFeatureVector& features, const TApplyOptions* applyOptions = nullptr) const {
        return LinearPrediction(Weights, features, applyOptions);
    }

    double NormalizedRawPrediction(const TFloatFeatureVector& features, const TApplyOptions* applyOptions = nullptr) const {
        return NormalizedLinearPrediction(Weights, features, applyOptions);
    }

    const TWeight* FindPtrToWeights(ui64 featureIndex) const {
        return Weights.FindPtr(featureIndex);
    }

    bool Has(ui64 featureIndex) const {
        return Weights.Has(featureIndex);
    }

    const TFeatureWeightMap<TWeight>& GetWeightMap() const {
        return Weights;
    }
};

using TCompactSingleLabelFloatWeights = TCompactSingleLabelWeights<float>;

template <typename TWeight>
class TCompactMultiLabelWeights {
private:
    TVector<TWeight> Weights;
    TDenseHash<ui64, size_t> Offsets;

public:
    Y_SAVELOAD_DEFINE(Weights, Offsets);

    TCompactMultiLabelWeights()
    {
    }

    TCompactMultiLabelWeights(const TVector<TWeight>& weights, const TDenseHash<ui64, size_t>& offsets)
        : Weights(weights)
        , Offsets(offsets)
    {
    }

    TCompactMultiLabelWeights(TVector<TWeight>&& weights, TDenseHash<ui64, size_t>&& offsets)
        : Weights(weights)
        , Offsets(offsets)
    {
    }

    TVector<double> Predictions(const TFloatFeatureVector& featuresVector, const size_t length) const {
        TVector<double> predictions(length);

        for (const TIndexedFloatFeature& feature : featuresVector) {
            if (const float* offset = FindPtrToWeights(feature.Index)) {
                const float featureValue = feature.Value;
                for (size_t labelNumber = 0; labelNumber < length; ++labelNumber) {
                    predictions[labelNumber] += featureValue * *(offset + labelNumber);
                }
            }
        }

        return predictions;
    }

    const TWeight* FindPtrToWeights(ui64 featureIndex) const {
        if (const size_t* offset = Offsets.FindPtr(featureIndex)) {
            return &(Weights.at(*offset));
        }

        return nullptr;
    }

    const TDenseHash<ui64, size_t>& GetWeightOffsets() const {
        return Offsets;
    }
};

using TCompactMultiLabelFloatWeights = TCompactMultiLabelWeights<float>;

template <typename TWeight>
class TConcurrentMutableWeights {
public:
    TVector<TFeatureWeightMap<TWeight>> Weights;

public:
    TConcurrentMutableWeights(size_t labelCount = 1)
        : Weights(labelCount)
    {
    }

    TConcurrentMutableWeights(TVector<TFeatureWeightMap<TWeight>>&& source)
        : Weights(std::move(source))
    {
    }

    // Each thread has its own labelNumber
    TWeight& GetMutableWeight(ui64 featureIndex, size_t labelIndex = 0) {
        return Weights.at(labelIndex).GetMutable(featureIndex);
    }

    double RawPrediction(const TFloatFeatureVector& features, size_t labelIndex = 0) const {
        return LinearPrediction(Weights.at(labelIndex), features);
    }

    TCompactSingleLabelWeights<TWeight> ToCompactSingleLabelWeights(TWeight lowerBound, size_t labelIndex = 0) {
        return TCompactSingleLabelWeights<TWeight>(Weights[labelIndex].Minimize(lowerBound));
    }

    TCompactMultiLabelWeights<TWeight> ToCompactMultiLabelWeights(TWeight lowerBound) const {
        size_t labelCount = Weights.size();
        size_t nextFeatureOffset = 0;

        TVector<TWeight> weights;
        TDenseHash<ui64, size_t> offsets;

        for (size_t labelIndex = 0; labelIndex < Weights.size(); ++labelIndex) {
            for (auto& weight : Weights.at(labelIndex)) {
                if (fabs(weight.second) <= lowerBound) {
                    continue;
                }

                if (!offsets.Has(weight.first)) {
                    offsets.emplace(weight.first, nextFeatureOffset * labelCount);
                    ++nextFeatureOffset;
                    weights.resize(nextFeatureOffset * labelCount);
                }

                size_t offset = offsets.Value(weight.first, 0) + labelIndex;
                weights.at(offset) = weight.second;
            }
        }

        return TCompactMultiLabelWeights<TWeight>(std::move(weights), std::move(offsets));
    }
};

using TConcurrentMutableFloatWeights = TConcurrentMutableWeights<float>;

}
