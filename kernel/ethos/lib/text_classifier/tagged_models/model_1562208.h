#pragma once

#include "options_1562208.h"

#include <kernel/ethos/lib/data/dataset.h>
#include <kernel/ethos/lib/text_classifier/binary_model.h>
#include <kernel/ethos/lib/text_classifier/options.h>
#include <kernel/ethos/lib/util/any_iterator.h>

#include <library/cpp/containers/dense_hash/dense_hash.h>

#include <util/ysaveload.h>

using namespace NEthos;

namespace NEthos_1562208 {

class TLogisticRegressionModel {
public:
    using TWeightsMap = TDenseHash<ui64, float>;

private:
    TWeightsMap Weights;
    double Threshold = 0.;

public:
    TLogisticRegressionModel()
        : Weights((ui64) -1)
    {
    }

    TLogisticRegressionModel(const TWeightsMap& weights, double threshold)
        : Weights(weights)
        , Threshold(threshold)
    {
    }

    Y_SAVELOAD_DEFINE(Weights, Threshold);

    TBinaryLabelWithFloatPrediction Apply(const TFloatFeatureVector& features, const NEthos::TApplyOptions* applyOptions = nullptr) const;
    TBinaryLabelWithFloatPrediction Apply(const TBinaryLabelFloatFeatureVector& features, const NEthos::TApplyOptions* applyOptions = nullptr) const;

    double ZeroThresholdPrediction(const TFloatFeatureVector& features) const;
    double ProbabilityPrediction(const TFloatFeatureVector& features) const;

    TLogisticRegressionModel& operator += (const TLogisticRegressionModel& other);

    double GetThreshold() const;
    const TWeightsMap& GetWeights() const;
};

class TDefaultTextClassifierModel: public IBinaryTextClassifierModel {
public:
    NEthos_1562208::TTextClassifierModelOptions ModelOptions;
    TLogisticRegressionModel LogisticRegressionModel;

public:
    Y_SAVELOAD_DEFINE_OVERRIDE(ModelOptions, LogisticRegressionModel);

    TDefaultTextClassifierModel() {}

    TDefaultTextClassifierModel(const NEthos_1562208::TTextClassifierModelOptions& modelOptions,
                                const TLogisticRegressionModel& lrModel)
        : ModelOptions(modelOptions)
        , LogisticRegressionModel(lrModel)
    {
        //XXX: remove from here
        MinimizeWeights();
        MinimizeLemmasMerger();
    }

    TBinaryLabelWithFloatPrediction Apply(const TFloatFeatureVector& features, const NEthos::TApplyOptions* applyOptions = nullptr) const override;
    TBinaryLabelWithFloatPrediction Apply(const TBinaryLabelFloatFeatureVector& featureVector, const NEthos::TApplyOptions* applyOptions = nullptr) const override;
    TBinaryLabelWithFloatPrediction Apply(const TDocument& document, const NEthos::TApplyOptions* applyOptions = nullptr) const override;
    TBinaryLabelWithFloatPrediction Apply(TStringBuf text, const NEthos::TApplyOptions* applyOptions = nullptr) const override;
    TBinaryLabelWithFloatPrediction Apply(const TVector<ui64>& hashes, const NEthos::TApplyOptions* applyOptions = nullptr) const override;
    TBinaryLabelWithFloatPrediction Apply(const TUtf16String& wideText, const TApplyOptions* applyOptions = nullptr) const override;

    TBinaryLabelWithFloatPrediction ApplyClean(const TUtf16String& cleanWideText, const TApplyOptions* applyOptions = nullptr) const override;

    const TDocumentFactory* GetDocumentFactory() const override;

private:
    const TLogisticRegressionModel::TWeightsMap& GetWeights() const;
    void MinimizeWeights();
    void MinimizeLemmasMerger();
};

class TDefaultTextClassifierModelStorage {
private:
    TVector<ELanguage> Languages;
    TVector<TDefaultTextClassifierModel> Models;
public:
    Y_SAVELOAD_DEFINE(Languages, Models);

    void LoadFromFile(const TString& filename) {
        TFileInput in(filename);
        Load(&in);
    }

    void SaveToFile(const TString& filename) const {
        TFixedBufferFileOutput out(filename);
        Save(&out);
    }

    TDefaultTextClassifierModelStorage() {
    }

    TDefaultTextClassifierModelStorage(TVector<std::pair<TString, TString> > languagesWithModels) {
        Sort(languagesWithModels.begin(), languagesWithModels.end());

        Languages.reserve(languagesWithModels.size());
        Models.reserve(languagesWithModels.size());

        for (const std::pair<TString, TString>& languageWithModel : languagesWithModels) {
            const TString& languageName = languageWithModel.first;
            const TString& modelPath = languageWithModel.second;

            const ELanguage language = LanguageByName(languageName);
            if (language == LANG_UNK) {
                Cerr << "unknown language: " << languageName << " for model " << modelPath << "; skipping" << Endl;
                continue;
            }

            TDefaultTextClassifierModel model;
            {
                TFileInput modelIn(modelPath);
                model.Load(&modelIn);
            }

            Languages.push_back(language);
            Models.push_back(model);
        }
    }

    TBinaryLabelWithFloatPrediction Apply(const ELanguage language, const TFloatFeatureVector& features) const {
        return ApplyAdapter(language, features);
    }

    TBinaryLabelWithFloatPrediction Apply(const ELanguage language, const TBinaryLabelFloatFeatureVector& featureVector) const {
        return ApplyAdapter(language, featureVector);
    }

    TBinaryLabelWithFloatPrediction Apply(const ELanguage language, const TDocument& document) const {
        return ApplyAdapter(language, document);
    }

    TBinaryLabelWithFloatPrediction Apply(const ELanguage language, TStringBuf text) const {
        return ApplyAdapter(language, text);
    }
private:
    const TDefaultTextClassifierModel* ModelByLanguage(const ELanguage language) const {
        const ELanguage* foundLanguage = LowerBound(Languages.begin(), Languages.end(), language);
        if (foundLanguage == Languages.end() || *foundLanguage != language) {
            return nullptr;
        }
        return &Models[foundLanguage - Languages.data()];
    }

    template <typename T>
    TBinaryLabelWithFloatPrediction ApplyAdapter(const ELanguage language, const T& arg) const {
        if (const TDefaultTextClassifierModel* model = ModelByLanguage(language)) {
            return model->Apply(arg);
        }
        return TBinaryLabelWithFloatPrediction(EBinaryClassLabel::BCL_UNKNOWN, 0.f);
    }
};

}
