#pragma once

#include <kernel/ethos/lib/linear_classifier_options/options.h>
#include <kernel/ethos/lib/linear_model/binary_model.h>
#include <kernel/ethos/lib/linear_model/multi_model.h>

#include <kernel/ethos/lib/util/any_iterator.h>
#include <kernel/ethos/lib/util/shuffle_iterator.h>

#include <library/cpp/containers/dense_hash/dense_hash.h>

#include <util/generic/array_ref.h>
#include <util/ysaveload.h>

namespace NEthos {

class TBinaryLabelLogisticRegressionLearner {
private:
    TLinearClassifierOptions Options;

public:
    using TModel = TBinaryClassificationLinearModel;

public:
    TBinaryLabelLogisticRegressionLearner(const TLinearClassifierOptions& options)
        : Options(options)
    {
    }

    TBinaryLabelLogisticRegressionLearner(TLinearClassifierOptions&& options)
        : Options(std::move(options))
    {
    }

    TBinaryClassificationLinearModel Learn(TAnyConstIterator<TBinaryLabelFloatFeatureVector> begin,
                                           TAnyConstIterator<TBinaryLabelFloatFeatureVector> end) const;

    TFloatFeatureWeightMap LearnWeights(TAnyConstIterator<TBinaryLabelFloatFeatureVector> begin,
                                        TAnyConstIterator<TBinaryLabelFloatFeatureVector> end,
                                        double* threshold = nullptr) const;

    void UpdateWeights(TAnyConstIterator<TBinaryLabelFloatFeatureVector> begin,
                       TAnyConstIterator<TBinaryLabelFloatFeatureVector> end,
                       TFloatFeatureWeightMap& weights,
                       double* threshold = nullptr) const;

    TFloatFeatureWeightMap LearnWeights(TAnyIterator<TBinaryLabelFloatFeatureVector> begin,
                                        TAnyIterator<TBinaryLabelFloatFeatureVector> end,
                                        const size_t featureNumber) const;
};

class TMultiLabelLogisticRegressionLearner {
private:
    TLinearClassifierOptions Options;

public:
    using TModel = TMultiLabelLinearClassifierModel;

public:
    TMultiLabelLogisticRegressionLearner(const TLinearClassifierOptions& options)
        : Options(options)
    {
    }

    TMultiLabelLogisticRegressionLearner(TLinearClassifierOptions&& options)
        : Options(std::move(options))
    {
    }

    TMultiLabelLinearClassifierModel Learn(TAnyConstIterator<TMultiBinaryLabelFloatFeatureVector> begin,
                                             TAnyConstIterator<TMultiBinaryLabelFloatFeatureVector> end,
                                             const TVector<TString>& allLabels) const;
};

}
