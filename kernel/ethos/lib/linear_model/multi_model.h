#pragma once

#include <kernel/ethos/lib/data/dataset.h>
#include <kernel/ethos/lib/data/linear_model.h>

#include <util/generic/hash_set.h>

#include <util/ysaveload.h>

namespace NEthos {

class TMultiLabelLinearClassifierModel {
public:
    TCompactMultiLabelFloatWeights Weights;
    TVector<double> Thresholds;
    TVector<TString> Labels;

public:
    Y_SAVELOAD_DEFINE(Weights, Thresholds, Labels);

    TMultiLabelLinearClassifierModel()
    {
    }

    TMultiLabelLinearClassifierModel(const TCompactMultiLabelFloatWeights& weights,
                                       const TVector<double>& thresholds,
                                       const TVector<TString>& labels)
        : Weights(weights)
        , Thresholds(thresholds)
        , Labels(labels)
    {
    }

    TStringLabelWithFloatPrediction BestPrediction(const TFloatFeatureVector& features) const;

    TMultiStringFloatPredictions Apply(const TFloatFeatureVector& features) const;
    TMultiStringFloatPredictions Apply(const TMultiBinaryLabelFloatFeatureVector& features) const;

    TMultiStringFloatPredictions Apply(const TFloatFeatureVector& features, const THashSet<TString>& interestingLabels) const;
    TMultiStringFloatPredictions Apply(const TMultiBinaryLabelFloatFeatureVector& features, const THashSet<TString>& interestingLabels) const;

    void ResetThreshold() {
        TVector<double>(Thresholds.size(), 0.).swap(Thresholds);
    }
};

}
