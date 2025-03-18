#pragma once

#include <library/cpp/dot_product/dot_product.h>
#include <library/cpp/l1_distance/l1_distance.h>
#include <library/cpp/l2_distance/l2_distance.h>

namespace NKNNIndex {
    enum class EDistanceType {
        L1Distance /* "l1_distance" */,
        L2Distance /* "l2_distance" */,
        L2SqrDistance /* "l2_sqr_distance" */,
        DotProduct /* "dot_product" */,
    };

    template <class FeatureType, EDistanceType distanceType>
    struct TDistanceCalculator {};

    template <class FeatureType>
    struct TDistanceCalculator<FeatureType, EDistanceType::L1Distance>: public NL1Distance::TL1Distance<FeatureType> {};

    template <class FeatureType>
    struct TDistanceCalculator<FeatureType, EDistanceType::L2Distance>: public NL2Distance::TL2Distance<FeatureType> {};

    template <class FeatureType>
    struct TDistanceCalculator<FeatureType, EDistanceType::L2SqrDistance>: public NL2Distance::TL2SqrDistance<FeatureType> {};

    template <class FeatureType>
    struct TDistanceCalculator<FeatureType, EDistanceType::DotProduct> {
        using TResult = decltype(DotProduct((const FeatureType*)nullptr, (const FeatureType*)nullptr, 0));
        inline TResult operator()(const FeatureType* l, const FeatureType* r, int length) const {
            return DotProduct(l, r, length);
        }
    };

    template <class FeatureType, EDistanceType distanceType>
    struct TDistanceLess {
        using TDistanceResult = typename TDistanceCalculator<FeatureType, distanceType>::TResult;
        inline bool operator()(TDistanceResult l, TDistanceResult r) const {
            return l < r;
        }
    };

    template <class FeatureType>
    struct TDistanceLess<FeatureType, EDistanceType::DotProduct> {
        using TDistanceResult = typename TDistanceCalculator<FeatureType, EDistanceType::DotProduct>::TResult;
        inline bool operator()(TDistanceResult l, TDistanceResult r) const {
            return l > r;
        }
    };

}
