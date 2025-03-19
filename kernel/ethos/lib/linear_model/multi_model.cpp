#include "multi_model.h"

namespace NEthos {

TStringLabelWithFloatPrediction TMultiLabelLinearClassifierModel::BestPrediction(const TFloatFeatureVector& features) const {
    TVector<double> resultWeights(Labels.size());
    for (const TIndexedFloatFeature& feature : features) {
        if (const float* offset = Weights.FindPtrToWeights(feature.Index)) {
            for (size_t labelNumber = 0; labelNumber < Labels.size(); ++labelNumber) {
                resultWeights[labelNumber] += feature.Value * *(offset + labelNumber);
            }
        }
    }
    for (size_t i = 0; i < Labels.size(); ++i) {
        resultWeights[i] -= Thresholds[i];
    }
    size_t maxPredictionPosition = MaxElement(resultWeights.begin(), resultWeights.end()) - resultWeights.begin();
    return TStringLabelWithFloatPrediction(Labels[maxPredictionPosition], resultWeights[maxPredictionPosition]);
}

TMultiStringFloatPredictions TMultiLabelLinearClassifierModel::Apply(const TFloatFeatureVector& features) const {
    TVector<double> resultWeights = Weights.Predictions(features, Labels.size());

    TMultiStringFloatPredictions predictions(Labels.size());

    for (size_t labelNumber = 0; labelNumber < Labels.size(); ++labelNumber) {
        resultWeights[labelNumber] -= Thresholds.at(labelNumber);

        const TStringLabelWithFloatPrediction prediction(Labels[labelNumber], resultWeights[labelNumber]);
        predictions.AddLabelWithPrediction(prediction);
    }

    return predictions;
}

TMultiStringFloatPredictions TMultiLabelLinearClassifierModel::Apply(const TMultiBinaryLabelFloatFeatureVector& features) const {
    return Apply(features.Features);
}


TMultiStringFloatPredictions TMultiLabelLinearClassifierModel::Apply(const TFloatFeatureVector& features, const THashSet<TString>& interestingLabels) const {
    TVector<size_t> indexes;
    for (size_t labelIndex = 0; labelIndex < Labels.size(); ++labelIndex) {
        if (interestingLabels.contains(Labels[labelIndex])) {
            indexes.push_back(labelIndex);
        }
    }

    TVector<double> resultWeights(indexes.size());

    for (const TIndexedFloatFeature& feature : features) {
        if (const float* offset = Weights.FindPtrToWeights(feature.Index)) {
            for (size_t indexNumber = 0; indexNumber < indexes.size(); ++indexNumber) {
                resultWeights.at(indexNumber) += feature.Value * *(offset + indexes[indexNumber]);
            }
        }
    }

    TMultiStringFloatPredictions predictions;
    for (size_t indexNumber = 0; indexNumber < indexes.size(); ++indexNumber) {
        resultWeights.at(indexNumber) -= Thresholds.at(indexes[indexNumber]);
        TStringLabelWithFloatPrediction prediction(Labels.at(indexes[indexNumber]),
                                                   resultWeights.at(indexNumber));
        predictions.AddLabelWithPrediction(prediction);
    }

    return predictions;
}

TMultiStringFloatPredictions TMultiLabelLinearClassifierModel::Apply(const TMultiBinaryLabelFloatFeatureVector& features, const THashSet<TString>& interestingLabels) const {
    return Apply(features.Features, interestingLabels);
}

}
