#pragma once

#include "multi_model.h"
#include "options.h"
#include "util.h"

#include <kernel/ethos/lib/logistic_regression/logistic_regression.h>

#include <library/cpp/svnversion/svnversion.h>

#include <util/generic/ptr.h>

namespace NEthos {

class TMultiTextClassifierModel: public IMultiTextClassifierModel {
public:
    int Version = GetProgramSvnRevision();

private:
    TSimpleSharedPtr<IMultiTextClassifierModel> Model;

public:
    TMultiTextClassifierModel() {}

    TMultiTextClassifierModel(THolder<IMultiTextClassifierModel>&& model)
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

    TMultiStringFloatPredictions Apply(const TFloatFeatureVector& featureVector) const override;
    TMultiStringFloatPredictions Apply(const TMultiBinaryLabelFloatFeatureVector& featureVector) const override;
    TMultiStringFloatPredictions Apply(const TDocument& document) const override;
    TMultiStringFloatPredictions Apply(TStringBuf text) const override;

    TMultiStringFloatPredictions Apply(const TFloatFeatureVector& featureVector, const THashSet<TString>& interestingLabels) const override;
    TMultiStringFloatPredictions Apply(const TMultiBinaryLabelFloatFeatureVector& featureVector, const THashSet<TString>& interestingLabels) const override;
    TMultiStringFloatPredictions Apply(const TDocument& document, const THashSet<TString>& interestingLabels) const override;
    TMultiStringFloatPredictions Apply(TStringBuf text, const THashSet<TString>& interestingLabels) const override;

    TMaybe<TStringLabelWithFloatPrediction> ApplyAndGetTheBest(const TFloatFeatureVector& featureVector) const override;
    TMaybe<TStringLabelWithFloatPrediction> ApplyAndGetTheBest(const TMultiBinaryLabelFloatFeatureVector& featureVector) const override;
    TMaybe<TStringLabelWithFloatPrediction> ApplyAndGetTheBest(const TDocument& document) const override;
    TMaybe<TStringLabelWithFloatPrediction> ApplyAndGetTheBest(TStringBuf text) const override;

    TStringLabelWithFloatPrediction BestPrediction(const TFloatFeatureVector& featureVector) const override;
    TStringLabelWithFloatPrediction BestPrediction(const TMultiBinaryLabelFloatFeatureVector& featureVector) const override;
    TStringLabelWithFloatPrediction BestPrediction(const TDocument& document) const override;
    TStringLabelWithFloatPrediction BestPrediction(TStringBuf text) const override;

    const TDocumentFactory* GetDocumentFactory() const override;

    void MinimizeBeforeSaving() override;

    void ResetThreshold() override {
        if (Model) {
            Model->ResetThreshold();
        }
    }
private:
    TSimpleSharedPtr<IMultiTextClassifierModel> CreateModel();
};

class TMultiLogisticRegressionClassifierModel: public IMultiTextClassifierModel {
public:
    TTextClassifierModelOptions ModelOptions;
    TMultiLabelLinearClassifierModel LogisticRegressionModel;

public:
    Y_SAVELOAD_DEFINE_OVERRIDE(ModelOptions, LogisticRegressionModel);

    TMultiLogisticRegressionClassifierModel() {}

    TMultiLogisticRegressionClassifierModel(const TTextClassifierModelOptions& modelOptions,
                                            const TMultiLabelLinearClassifierModel& lrModel)
        : ModelOptions(modelOptions)
        , LogisticRegressionModel(lrModel)
    {
    }

    TMultiStringFloatPredictions Apply(const TFloatFeatureVector& featureVector) const override;
    TMultiStringFloatPredictions Apply(const TMultiBinaryLabelFloatFeatureVector& featureVector) const override;
    TMultiStringFloatPredictions Apply(const TDocument& document) const override;
    TMultiStringFloatPredictions Apply(TStringBuf text) const override;

    TMultiStringFloatPredictions Apply(const TFloatFeatureVector& featureVector, const THashSet<TString>& interestingLabels) const override;
    TMultiStringFloatPredictions Apply(const TMultiBinaryLabelFloatFeatureVector& featureVector, const THashSet<TString>& interestingLabels) const override;
    TMultiStringFloatPredictions Apply(const TDocument& document, const THashSet<TString>& interestingLabels) const override;
    TMultiStringFloatPredictions Apply(TStringBuf text, const THashSet<TString>& interestingLabels) const override;

    TMaybe<TStringLabelWithFloatPrediction> ApplyAndGetTheBest(const TFloatFeatureVector& featureVector) const override;
    TMaybe<TStringLabelWithFloatPrediction> ApplyAndGetTheBest(const TMultiBinaryLabelFloatFeatureVector& featureVector) const override;
    TMaybe<TStringLabelWithFloatPrediction> ApplyAndGetTheBest(const TDocument& document) const override;
    TMaybe<TStringLabelWithFloatPrediction> ApplyAndGetTheBest(TStringBuf text) const override;

    TStringLabelWithFloatPrediction BestPrediction(const TFloatFeatureVector& featureVector) const override;
    TStringLabelWithFloatPrediction BestPrediction(const TMultiBinaryLabelFloatFeatureVector& featureVector) const override;
    TStringLabelWithFloatPrediction BestPrediction(const TDocument& document) const override;
    TStringLabelWithFloatPrediction BestPrediction(TStringBuf text) const override;

    const TDocumentFactory* GetDocumentFactory() const override;

    void MinimizeBeforeSaving() override;

    void ResetThreshold() override {
        LogisticRegressionModel.ResetThreshold();
    }
};

template <typename TItem>
THolder<TMultiLogisticRegressionClassifierModel> LearnMultiLogisticRegressionClassifier(
        TAnyConstIterator<TItem> begin,
        TAnyConstIterator<TItem> end,
        const TVector<TString>& allLabels,
        const TTextClassifierModelOptions& options,
        const TMultiLabelLogisticRegressionLearner& logisticRegressionLearner);

class TMultiLogisticRegressionClassifierLearner {
private:
    TTextClassifierModelOptions Options;
    TMultiLabelLogisticRegressionLearner LogisticRegressionLearner;

public:
    TMultiLogisticRegressionClassifierLearner(TTextClassifierOptions&& options)
        : Options(std::move(options.ModelOptions))
        , LogisticRegressionLearner(std::move(options.LinearClassifierOptions))
    {
    }

    TMultiLogisticRegressionClassifierLearner(const TTextClassifierOptions& options)
        : Options(options.ModelOptions)
        , LogisticRegressionLearner(options.LinearClassifierOptions)
    {
    }

    template <typename TItem>
    THolder<TMultiLogisticRegressionClassifierModel> Learn(
            TAnyConstIterator<TItem> begin,
            TAnyConstIterator<TItem> end,
            const TVector<TString>& allLabels) const
    {
        return LearnMultiLogisticRegressionClassifier<TItem>(begin, end, allLabels, Options, LogisticRegressionLearner);
    }
};


class TMultiTextClassifierLearner {
public:
    using TModel = TMultiTextClassifierModel;

private:
    TMultiLogisticRegressionClassifierLearner Learner;

public:

    TMultiTextClassifierLearner(const TTextClassifierOptions& options)
        : Learner(options)
    {
    }

    TMultiTextClassifierLearner(TTextClassifierOptions&& options)
        : Learner(std::move(options))
    {
    }

    template <typename TItem>
    TMultiTextClassifierModel Learn(TAnyConstIterator<TItem> begin,
                                    TAnyConstIterator<TItem> end,
                                    const TVector<TString>& allLabels) const
    {
        return TMultiTextClassifierModel(Learner.Learn(begin, end, allLabels));
    }
};

}
