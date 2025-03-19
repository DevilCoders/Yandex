#pragma once

#include "options.h"
#include "binary_model.h"
#include "util.h"

#include <kernel/ethos/lib/logistic_regression/logistic_regression.h>
#include <kernel/ethos/lib/naive_bayes/naive_bayes.h>

#include <library/cpp/svnversion/svnversion.h>

#include <util/generic/ptr.h>

namespace NEthos {

class TBinaryTextClassifierModel: public IBinaryTextClassifierModel {
public:
    int Version = GetProgramSvnRevision();

private:
    TSimpleSharedPtr<IBinaryTextClassifierModel> Model;

public:
    TBinaryTextClassifierModel() {}

    TBinaryTextClassifierModel(THolder<IBinaryTextClassifierModel>&& model)
        : Model(model.Release())
    {
    }

    void Save(IOutputStream* s) const override {
        ::Save(s, Version);
        Model->Save(s);
    }

    void Load(IInputStream* s) override {
        ::Load(s, Version);
        Model = CreateModel();
        Model->Load(s);
    }

    void LoadFromFile(const TString& filename) {
        TFileInput in(filename);
        Load(&in);
    }

    void SaveToFile(const TString& filename) const {
        TFixedBufferFileOutput out(filename);
        Save(&out);
    }

    bool Empty() const {
        return !Model;
    }

    TBinaryLabelWithFloatPrediction Apply(const TFloatFeatureVector& features, const TApplyOptions* applyOptions = nullptr) const override;
    TBinaryLabelWithFloatPrediction Apply(const TBinaryLabelFloatFeatureVector& featureVector, const TApplyOptions* applyOptions = nullptr) const override;
    TBinaryLabelWithFloatPrediction Apply(const TDocument& document, const TApplyOptions* applyOptions = nullptr) const override;
    TBinaryLabelWithFloatPrediction Apply(TStringBuf text, const TApplyOptions* applyOptions = nullptr) const override;
    TBinaryLabelWithFloatPrediction Apply(const TVector<ui64>& hashes, const TApplyOptions* applyOptions = nullptr) const override;
    TBinaryLabelWithFloatPrediction Apply(const TUtf16String& wideText, const TApplyOptions* applyOptions = nullptr) const override;

    TBinaryLabelWithFloatPrediction ApplyClean(const TUtf16String& cleanWideText, const TApplyOptions* applyOptions = nullptr) const override;

    const TDocumentFactory* GetDocumentFactory() const override;

    void MinimizeBeforeSaving() override {
        if (Model) {
            Model->MinimizeBeforeSaving();
        }
    }

    void PrintDictionary(IOutputStream& out, const bool shortExport) const override {
        if (Model) {
            Model->PrintDictionary(out, shortExport);
        }
    }

    void ResetThreshold() override {
        if (Model) {
            Model->ResetThreshold();
        }
    }

    double GetThreshold() override {
        return Model ? Model->GetThreshold() : 0.;
    }

    void ResetDocumentsFactory() override {
        if (Model) {
            Model->ResetDocumentsFactory();
        }
    }
private:
    TSimpleSharedPtr<IBinaryTextClassifierModel> CreateModel();
};

class TLinearBinaryClassifierModel: public IBinaryTextClassifierModel {
public:
    TTextClassifierModelOptions ModelOptions;
    TBinaryClassificationLinearModel LinearClassifierModel;

public:
    Y_SAVELOAD_DEFINE_OVERRIDE(ModelOptions, LinearClassifierModel);

    void SaveToFile(const TString& path) const {
        TFixedBufferFileOutput modelOut(path);
        Save(&modelOut);
    }

    void LoadFromFile(const TString& path) {
        TFileInput modelIn(path);
        Load(&modelIn);
    }

    TLinearBinaryClassifierModel() {}

    TLinearBinaryClassifierModel(const TTextClassifierModelOptions& modelOptions,
                                 const TBinaryClassificationLinearModel& lrModel)
        : ModelOptions(modelOptions)
        , LinearClassifierModel(lrModel)
    {
    }

    const TTextClassifierModelOptions& GetModelOptions() const {
        return ModelOptions;
    }

    void MinimizeBeforeSaving() override {
        ModelOptions.LemmerOptions.MinimizeLemmasMerger(LinearClassifierModel.GetWeights());
    }

    TBinaryLabelWithFloatPrediction Apply(const TFloatFeatureVector& features, const TApplyOptions* applyOptions = nullptr) const override;
    TBinaryLabelWithFloatPrediction Apply(const TBinaryLabelFloatFeatureVector& featureVector, const TApplyOptions* applyOptions = nullptr) const override;
    TBinaryLabelWithFloatPrediction Apply(const TDocument& document, const TApplyOptions* applyOptions = nullptr) const override;
    TBinaryLabelWithFloatPrediction Apply(TStringBuf text, const TApplyOptions* applyOptions = nullptr) const override;
    TBinaryLabelWithFloatPrediction Apply(const TVector<ui64>& hashes, const TApplyOptions* applyOptions = nullptr) const override;
    TBinaryLabelWithFloatPrediction Apply(const TUtf16String& wideText, const TApplyOptions* applyOptions = nullptr) const override;

    TBinaryLabelWithFloatPrediction ApplyClean(const TUtf16String& cleanWideText, const TApplyOptions* applyOptions = nullptr) const override;

    const TDocumentFactory* GetDocumentFactory() const override;

    const TCompactSingleLabelFloatWeights& GetWeights() const;
    double GetThreshold() const;

    void PrintDictionary(IOutputStream& out, const bool shortExport) const override;

    double GetThreshold() override {
        return LinearClassifierModel.GetThreshold();
    }

    void ResetThreshold() override {
        LinearClassifierModel.ResetThreshold();
    }

    void ResetDocumentsFactory() override {
        ModelOptions.LemmerOptions.DocumentFactory.Reset();
    }
};

class TBinaryTextClassifierLearner {
public:
    using TModel = TBinaryTextClassifierModel;

    TTextClassifierModelOptions ModelOptions;
    TLinearClassifierOptions LinearClassifierOptions;
public:
    TBinaryTextClassifierLearner(const TTextClassifierModelOptions& modelOptions,
                                 const TLinearClassifierOptions& linearClassifierOptions)
        : ModelOptions(modelOptions)
        , LinearClassifierOptions(linearClassifierOptions)
    {
    }

    TBinaryTextClassifierLearner(const TTextClassifierOptions& options)
        : ModelOptions(options.ModelOptions)
        , LinearClassifierOptions(options.LinearClassifierOptions)
    {
    }

    TBinaryTextClassifierLearner(TTextClassifierOptions&& options)
        : ModelOptions(std::move(options.ModelOptions))
        , LinearClassifierOptions(std::move(options.LinearClassifierOptions))
    {
    }

    TBinaryTextClassifierModel Learn(TAnyConstIterator<TBinaryLabelFloatFeatureVector> begin,
                                     TAnyConstIterator<TBinaryLabelFloatFeatureVector> end) const
    {
        switch (LinearClassifierOptions.LearningMethod) {
        case ELearningMethod::LM_LOGISTIC_REGRESSION : return Learn<TBinaryLabelLogisticRegressionLearner>(begin, end);
        case ELearningMethod::LM_NAIVE_BAYES : return Learn<TBinaryLabelNaiveBayesLearner>(begin, end);
        }
        Y_ASSERT(0);
        return TBinaryTextClassifierModel();
    }
private:
    template <typename TBinaryLabelLinearClassifierLearner>
    TBinaryTextClassifierModel Learn(TAnyConstIterator<TBinaryLabelFloatFeatureVector> begin,
                                     TAnyConstIterator<TBinaryLabelFloatFeatureVector> end) const
    {
        TBinaryLabelLinearClassifierLearner learner(LinearClassifierOptions);
        TBinaryClassificationLinearModel lrModel = learner.Learn(begin, end);
        return TBinaryTextClassifierModel(MakeHolder<TLinearBinaryClassifierModel>(ModelOptions, lrModel));
    }
};

class TBinaryTextWeightsLearner {
public:
    using TModel = TBinaryTextClassifierModel;

    TLinearClassifierOptions LinearClassifierOptions;
public:
    TBinaryTextWeightsLearner(const TLinearClassifierOptions& linearClassifierOptions)
        : LinearClassifierOptions(linearClassifierOptions)
    {
    }

    TFloatFeatureWeightMap LearnWeights(TAnyConstIterator<TBinaryLabelFloatFeatureVector> begin,
                                        TAnyConstIterator<TBinaryLabelFloatFeatureVector> end,
                                        double* threshold = nullptr) const
    {
        switch (LinearClassifierOptions.LearningMethod) {
        case ELearningMethod::LM_LOGISTIC_REGRESSION : return LearnWeights<TBinaryLabelLogisticRegressionLearner>(begin, end, threshold);
        case ELearningMethod::LM_NAIVE_BAYES : return LearnWeights<TBinaryLabelNaiveBayesLearner>(begin, end, threshold);
        }
        Y_ASSERT(0);
        return TFloatFeatureWeightMap();
    }

    TFloatFeatureWeightMap LearnWeights(TAnyIterator<TBinaryLabelFloatFeatureVector> begin,
                                        TAnyIterator<TBinaryLabelFloatFeatureVector> end,
                                        const size_t featureNumber) const
    {
        switch (LinearClassifierOptions.LearningMethod) {
        case ELearningMethod::LM_LOGISTIC_REGRESSION : return LearnWeights<TBinaryLabelLogisticRegressionLearner>(begin, end, featureNumber);
        case ELearningMethod::LM_NAIVE_BAYES : return LearnWeights<TBinaryLabelNaiveBayesLearner>(begin, end, featureNumber);
        }
        Y_ASSERT(0);
        return TFloatFeatureWeightMap();
    }
private:
    template <typename TBinaryLabelLinearClassifierLearner>
    TFloatFeatureWeightMap LearnWeights(TAnyConstIterator<TBinaryLabelFloatFeatureVector> begin,
                                        TAnyConstIterator<TBinaryLabelFloatFeatureVector> end,
                                        double* threshold = nullptr) const
    {
        TBinaryLabelLinearClassifierLearner learner(LinearClassifierOptions);
        return learner.LearnWeights(begin, end, threshold);
    }

    template <typename TBinaryLabelLinearClassifierLearner>
    TFloatFeatureWeightMap LearnWeights(TAnyIterator<TBinaryLabelFloatFeatureVector> begin,
                                        TAnyIterator<TBinaryLabelFloatFeatureVector> end,
                                        const size_t featureNumber) const
    {
        TBinaryLabelLinearClassifierLearner learner(LinearClassifierOptions);
        return learner.LearnWeights(begin, end, featureNumber);
    }
};

class TBinaryTextClassifierModelsStorage {
private:
    TVector<ELanguage> Languages;
    TVector<TBinaryTextClassifierModel> Models;
public:
    Y_SAVELOAD_DEFINE(Languages, Models);

    void LoadFromFile(const TString& filename) {
        TFileInput in(filename);
        Load(&in);
    }
    void LoadFromBlob(const TBlob& blob) {
        TMemoryInput in(blob.Data(), blob.Size());
        Load(&in);
    }

    void SaveToFile(const TString& filename) const {
        TFixedBufferFileOutput out(filename);
        Save(&out);
    }

    TBinaryTextClassifierModelsStorage() {
    }

    TBinaryTextClassifierModelsStorage(const TVector<std::pair<TString, TString> >& languagesWithModels, const TString& defaultModelPath) {
        Languages.reserve(languagesWithModels.size() + 1);
        Models.reserve(languagesWithModels.size() + 1);

        for (const std::pair<TString, TString>& languageWithModel : languagesWithModels) {
            const TString& languageName = languageWithModel.first;
            const TString& modelPath = languageWithModel.second;

            const ELanguage language = LanguageByName(languageName);

            TBinaryTextClassifierModel model;
            model.LoadFromFile(modelPath);

            Languages.push_back(language);
            Models.push_back(model);
        }

        TBinaryTextClassifierModel defaultModel;
        defaultModel.LoadFromFile(defaultModelPath);

        Languages.push_back(LANG_UNK);
        Models.push_back(defaultModel);
    }

    const TVector<ELanguage>& GetLanguages() const {
        return Languages;
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

    template <typename TArgumentType>
    double Probability(const ELanguage language, const TArgumentType& argument) const {
        return Sigmoid(Apply(language, argument).Prediction);
    }

    const TBinaryTextClassifierModel* ModelByLanguage(const ELanguage language) const {
        const TBinaryTextClassifierModel* defaultModel = nullptr;

        for (size_t modelNumber = 0; modelNumber < Models.size(); ++modelNumber) {
            if (Languages[modelNumber] == language) {
                return &Models[modelNumber];
            }
            if (Languages[modelNumber] == LANG_UNK) {
                defaultModel = &Models[modelNumber];
            }
        }

        return defaultModel;
    }

    void ResetThreshold() {
        for (TBinaryTextClassifierModel& model : Models) {
            model.ResetThreshold();
        }
    }
private:
    template <typename T>
    TBinaryLabelWithFloatPrediction ApplyAdapter(const ELanguage language, const T& arg) const {
        if (const TBinaryTextClassifierModel* model = ModelByLanguage(language)) {
            return model->Apply(arg);
        }
        return TBinaryLabelWithFloatPrediction(EBinaryClassLabel::BCL_UNKNOWN, 0.f);
    }
};

}
