#include "binary_model.h"

namespace NEthos {

double TBinaryClassificationLinearModel::ZeroThresholdWeightPrediction(
        const TFloatFeatureVector& features, const TApplyOptions* applyOptions) const
{
    double result = NormalizeFeatures ? Weights.NormalizedRawPrediction(features, applyOptions) : Weights.RawPrediction(features, applyOptions);
    return result - Threshold;
}

double TBinaryClassificationLinearModel::ProbabilityPrediction(const TFloatFeatureVector& features, const TApplyOptions* applyOptions) const {
    return Sigmoid(ZeroThresholdWeightPrediction(features, applyOptions));
}

TBinaryLabelWithFloatPrediction TBinaryClassificationLinearModel::Apply(
        const TFloatFeatureVector& features, const TApplyOptions* applyOptions) const
{
    double nonTransformedPrediction = ZeroThresholdWeightPrediction(features, applyOptions);
    EBinaryClassLabel label = BinaryLabelFromPrediction(nonTransformedPrediction, 0.);
    return TBinaryLabelWithFloatPrediction(label, nonTransformedPrediction);
}

TBinaryLabelWithFloatPrediction TBinaryClassificationLinearModel::Apply(
        const TBinaryLabelFloatFeatureVector& features, const TApplyOptions* applyOptions) const
{
    return Apply(features.Features, applyOptions);
}

TBinaryClassificationLinearModel& TBinaryClassificationLinearModel::operator+=(
        const TBinaryClassificationLinearModel& rhs)
{
    Weights += rhs.Weights;
    Threshold += rhs.Threshold;

    return *this;
}

const TCompactSingleLabelFloatWeights& TBinaryClassificationLinearModel::GetWeights() const {
    return Weights;
}

double TBinaryClassificationLinearModel::GetThreshold() const {
    return Threshold;
}

}
