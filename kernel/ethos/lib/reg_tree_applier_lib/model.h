#pragma once

#include <util/generic/algorithm.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <util/ysaveload.h>

namespace NRegTree {

struct TSimpleCompactStumpInfo {
    size_t LeftChildIdx = 0;
    size_t LinearModelOffset = 0;

    ui32 FeatureNumber = 0;
    float FeatureThreshold = 0.f;

    Y_SAVELOAD_DEFINE(LeftChildIdx, LinearModelOffset, FeatureNumber, FeatureThreshold);
};

struct TCompactModel {
    ui32 FeaturesCount;

    TVector<TSimpleCompactStumpInfo> Stumps;
    TVector<size_t> RootIds;

    TVector<float> LinearModels;

    float Bias;

    Y_SAVELOAD_DEFINE(FeaturesCount, Stumps, RootIds, LinearModels, Bias);

    template <typename T>
    double Prediction(const T* features) const {
        double prediction = Bias;

        for (size_t i = 0; i < RootIds.size(); ++i) {
            const TSimpleCompactStumpInfo* stump = &Stumps[RootIds[i]];
            while (stump->LeftChildIdx) {
                stump = &Stumps[stump->LeftChildIdx + (features[stump->FeatureNumber] > stump->FeatureThreshold)];
            }
            const float* begin = LinearModels.begin() + stump->LinearModelOffset;
            const float* end = begin + FeaturesCount + 1;

            prediction += *begin + std::inner_product(begin + 1, end, features, 0.);
        }

        return prediction;
    }

    template <typename T>
    double Prediction(const TVector<T>& features) const {
        return Prediction(features.begin());
    }
};

}
