#include "features_selector.h"

#include <util/generic/ymath.h>

namespace NEthos {

void TFeatureStats::Add(const EBinaryClassLabel label, const TIndexedFloatFeature& feature) {
    switch (label) {
    case EBinaryClassLabel::BCL_POSITIVE:
        SumPositiveValues += fabs(feature.Value);
        break;
    case EBinaryClassLabel::BCL_NEGATIVE:
        SumNegativeValues += fabs(feature.Value);
        break;
    default:
        break;
    }
    FeatureIndex = feature.Index;
}

void TFeatureStats::SetupDiscriminationFactors(const size_t positivesCount, const size_t negativesCount) {
    double positivesFrequency = (SumPositiveValues + 1.) / positivesCount;
    double negativesFrequency = (SumNegativeValues + 1.) / negativesCount;

    PositiveDiscriminationFactor = positivesFrequency * log(positivesFrequency / negativesFrequency);
    NegativeDiscriminationFactor = negativesFrequency * log(negativesFrequency / positivesFrequency);
}

}
