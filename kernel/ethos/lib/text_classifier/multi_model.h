#pragma once

#include <kernel/ethos/lib/data/dataset.h>
#include "document.h"
#include "document_factory.h"
#include "options.h"

#include <util/stream/input.h>
#include <util/stream/output.h>

namespace NEthos {

class IMultiTextClassifierModel {
public:
    virtual ~IMultiTextClassifierModel() {}

    virtual TMultiStringFloatPredictions Apply(const TFloatFeatureVector& featureVector) const = 0;
    virtual TMultiStringFloatPredictions Apply(const TMultiBinaryLabelFloatFeatureVector& featureVector) const = 0;
    virtual TMultiStringFloatPredictions Apply(const TDocument& document) const = 0;
    virtual TMultiStringFloatPredictions Apply(TStringBuf text) const = 0;

    virtual TMultiStringFloatPredictions Apply(const TFloatFeatureVector& featureVector, const THashSet<TString>& interestingLabels) const = 0;
    virtual TMultiStringFloatPredictions Apply(const TMultiBinaryLabelFloatFeatureVector& featureVector, const THashSet<TString>& interestingLabels) const = 0;
    virtual TMultiStringFloatPredictions Apply(const TDocument& document, const THashSet<TString>& interestingLabels) const = 0;
    virtual TMultiStringFloatPredictions Apply(TStringBuf text, const THashSet<TString>& interestingLabels) const = 0;

    virtual TMaybe<TStringLabelWithFloatPrediction> ApplyAndGetTheBest(const TFloatFeatureVector& featureVector) const = 0;
    virtual TMaybe<TStringLabelWithFloatPrediction> ApplyAndGetTheBest(const TMultiBinaryLabelFloatFeatureVector& featureVector) const = 0;
    virtual TMaybe<TStringLabelWithFloatPrediction> ApplyAndGetTheBest(const TDocument& document) const = 0;
    virtual TMaybe<TStringLabelWithFloatPrediction> ApplyAndGetTheBest(TStringBuf text) const = 0;

    virtual TStringLabelWithFloatPrediction BestPrediction(const TFloatFeatureVector& featureVector) const = 0;
    virtual TStringLabelWithFloatPrediction BestPrediction(const TMultiBinaryLabelFloatFeatureVector& featureVector) const = 0;
    virtual TStringLabelWithFloatPrediction BestPrediction(const TDocument& document) const = 0;
    virtual TStringLabelWithFloatPrediction BestPrediction(TStringBuf text) const = 0;

    virtual void Save(IOutputStream* s) const = 0;
    virtual void Load(IInputStream* s) = 0;

    virtual const TDocumentFactory* GetDocumentFactory() const = 0;

    virtual void MinimizeBeforeSaving() {}

    virtual void ResetThreshold() {}
};

}
