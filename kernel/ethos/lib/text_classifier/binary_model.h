#pragma once

#include <kernel/ethos/lib/data/dataset.h>
#include "document.h"

#include <util/stream/input.h>
#include <util/stream/output.h>

namespace NEthos {

struct TApplyOptions;
class TDocumentFactory;

class IBinaryTextClassifierModel {
public:
    virtual ~IBinaryTextClassifierModel() {}

    virtual TBinaryLabelWithFloatPrediction Apply(const TFloatFeatureVector& features, const TApplyOptions* applyOptions = nullptr) const = 0;
    virtual TBinaryLabelWithFloatPrediction Apply(const TBinaryLabelFloatFeatureVector& featureVector, const TApplyOptions* applyOptions = nullptr) const = 0;
    virtual TBinaryLabelWithFloatPrediction Apply(const TDocument& document, const TApplyOptions* applyOptions = nullptr) const = 0;
    virtual TBinaryLabelWithFloatPrediction Apply(TStringBuf text, const TApplyOptions* applyOptions = nullptr) const = 0;
    virtual TBinaryLabelWithFloatPrediction Apply(const TVector<ui64>& hashes, const TApplyOptions* applyOptions = nullptr) const = 0;
    virtual TBinaryLabelWithFloatPrediction Apply(const TUtf16String& wideText, const TApplyOptions* applyOptions = nullptr) const = 0;

    virtual TBinaryLabelWithFloatPrediction ApplyClean(const TUtf16String& cleanWideText, const TApplyOptions* applyOptions = nullptr) const = 0;

    template <typename TArgumentType>
    double Probability(const TArgumentType& argument) const {
        return Sigmoid(Apply(argument).Prediction);
    }

    virtual void Save(IOutputStream* s) const = 0;
    virtual void Load(IInputStream* s) = 0;

    virtual const TDocumentFactory* GetDocumentFactory() const = 0;

    virtual void MinimizeBeforeSaving() {}

    virtual void PrintDictionary(IOutputStream&, const bool) const {}

    virtual double GetThreshold() {
        return 0.;
    }

    virtual void ResetThreshold() {}
    virtual void ResetDocumentsFactory() {}
};

}
