#include "multi_classifier.h"

#include <library/cpp/containers/dense_hash/dense_hash.h>

namespace NEthos {

TSimpleSharedPtr<IMultiTextClassifierModel> TMultiTextClassifierModel::CreateModel() {
    return new TMultiLogisticRegressionClassifierModel();
}

TMultiStringFloatPredictions TMultiTextClassifierModel::Apply(const TFloatFeatureVector& featureVector) const {
    return Model->Apply(featureVector);
}

TMultiStringFloatPredictions TMultiTextClassifierModel::Apply(const TMultiBinaryLabelFloatFeatureVector& featureVector) const {
    return Model->Apply(featureVector);
}

TMultiStringFloatPredictions TMultiTextClassifierModel::Apply(const TDocument& document) const {
    return Model->Apply(document);
}

TMultiStringFloatPredictions TMultiTextClassifierModel::Apply(TStringBuf text) const {
    return Model->Apply(text);
}

TMultiStringFloatPredictions TMultiTextClassifierModel::Apply(const TFloatFeatureVector& featureVector, const THashSet<TString>& interestingLabels) const {
    return Model->Apply(featureVector, interestingLabels);
}

TMultiStringFloatPredictions TMultiTextClassifierModel::Apply(const TMultiBinaryLabelFloatFeatureVector& featureVector, const THashSet<TString>& interestingLabels) const {
    return Model->Apply(featureVector, interestingLabels);
}

TMultiStringFloatPredictions TMultiTextClassifierModel::Apply(const TDocument& document, const THashSet<TString>& interestingLabels) const {
    return Model->Apply(document, interestingLabels);
}

TMultiStringFloatPredictions TMultiTextClassifierModel::Apply(TStringBuf text, const THashSet<TString>& interestingLabels) const {
    return Model->Apply(text, interestingLabels);
}

TMaybe<TStringLabelWithFloatPrediction> TMultiTextClassifierModel::ApplyAndGetTheBest(const TFloatFeatureVector& featureVector) const {
    return Model->ApplyAndGetTheBest(featureVector);
}

TMaybe<TStringLabelWithFloatPrediction> TMultiTextClassifierModel::ApplyAndGetTheBest(const TMultiBinaryLabelFloatFeatureVector& featureVector) const {
    return Model->ApplyAndGetTheBest(featureVector);
}

TMaybe<TStringLabelWithFloatPrediction> TMultiTextClassifierModel::ApplyAndGetTheBest(const TDocument& document) const {
    return Model->ApplyAndGetTheBest(document);
}

TMaybe<TStringLabelWithFloatPrediction> TMultiTextClassifierModel::ApplyAndGetTheBest(TStringBuf text) const {
    return Model->ApplyAndGetTheBest(text);
}

TStringLabelWithFloatPrediction TMultiTextClassifierModel::BestPrediction(const TFloatFeatureVector& featureVector) const {
    return Model->BestPrediction(featureVector);
}

TStringLabelWithFloatPrediction TMultiTextClassifierModel::BestPrediction(const TMultiBinaryLabelFloatFeatureVector& featureVector) const {
    return Model->BestPrediction(featureVector);
}

TStringLabelWithFloatPrediction TMultiTextClassifierModel::BestPrediction(const TDocument& document) const {
    return Model->BestPrediction(document);
}

TStringLabelWithFloatPrediction TMultiTextClassifierModel::BestPrediction(TStringBuf text) const {
    return Model->BestPrediction(text);

}

const TDocumentFactory* TMultiTextClassifierModel::GetDocumentFactory() const {
    return Model->GetDocumentFactory();
}

void TMultiTextClassifierModel::MinimizeBeforeSaving() {
    Model->MinimizeBeforeSaving();
}

TMultiStringFloatPredictions TMultiLogisticRegressionClassifierModel::Apply(const TFloatFeatureVector& featureVector) const {
    return LogisticRegressionModel.Apply(featureVector);
}

TMultiStringFloatPredictions TMultiLogisticRegressionClassifierModel::Apply(const TMultiBinaryLabelFloatFeatureVector& featureVector) const {
    return LogisticRegressionModel.Apply(featureVector);
}

TMultiStringFloatPredictions TMultiLogisticRegressionClassifierModel::Apply(const TDocument& document) const {
    return Apply(FeatureVectorFromDocument(document, ModelOptions));
}

TMultiStringFloatPredictions TMultiLogisticRegressionClassifierModel::Apply(TStringBuf text) const {
    TDocument document = ModelOptions.LemmerOptions.DocumentFactory->DocumentFromString(text);
    return Apply(document);
}

TMultiStringFloatPredictions TMultiLogisticRegressionClassifierModel::Apply(const TFloatFeatureVector& featureVector, const THashSet<TString>& interestingLabels) const {
    return LogisticRegressionModel.Apply(featureVector, interestingLabels);
}

TMultiStringFloatPredictions TMultiLogisticRegressionClassifierModel::Apply(const TMultiBinaryLabelFloatFeatureVector& featureVector, const THashSet<TString>& interestingLabels) const {
    return LogisticRegressionModel.Apply(featureVector, interestingLabels);
}

TMultiStringFloatPredictions TMultiLogisticRegressionClassifierModel::Apply(const TDocument& document, const THashSet<TString>& interestingLabels) const {
    return Apply(FeatureVectorFromDocument(document, ModelOptions), interestingLabels);
}

TMultiStringFloatPredictions TMultiLogisticRegressionClassifierModel::Apply(TStringBuf text, const THashSet<TString>& interestingLabels) const {
    TDocument document = ModelOptions.LemmerOptions.DocumentFactory->DocumentFromString(text);
    return Apply(document, interestingLabels);
}

TMaybe<TStringLabelWithFloatPrediction> TMultiLogisticRegressionClassifierModel::ApplyAndGetTheBest(const TFloatFeatureVector& featureVector) const {
    return Apply(featureVector).PopBestPositivePrediction();
}

TMaybe<TStringLabelWithFloatPrediction> TMultiLogisticRegressionClassifierModel::ApplyAndGetTheBest(const TMultiBinaryLabelFloatFeatureVector& featureVector) const {
    return Apply(featureVector).PopBestPositivePrediction();
}

TMaybe<TStringLabelWithFloatPrediction> TMultiLogisticRegressionClassifierModel::ApplyAndGetTheBest(const TDocument& document) const {
    return Apply(document).PopBestPositivePrediction();
}
TMaybe<TStringLabelWithFloatPrediction> TMultiLogisticRegressionClassifierModel::ApplyAndGetTheBest(TStringBuf text) const {
    return Apply(text).PopBestPositivePrediction();
}

TStringLabelWithFloatPrediction TMultiLogisticRegressionClassifierModel::BestPrediction(const TFloatFeatureVector& featureVector) const {
    return LogisticRegressionModel.BestPrediction(featureVector);
}

TStringLabelWithFloatPrediction TMultiLogisticRegressionClassifierModel::BestPrediction(const TMultiBinaryLabelFloatFeatureVector& featureVector) const {
    return BestPrediction(featureVector.Features);
}

TStringLabelWithFloatPrediction TMultiLogisticRegressionClassifierModel::BestPrediction(const TDocument& document) const {
    return BestPrediction(FeatureVectorFromDocument(document, ModelOptions));
}

TStringLabelWithFloatPrediction TMultiLogisticRegressionClassifierModel::BestPrediction(TStringBuf text) const {
    TDocument document = ModelOptions.LemmerOptions.DocumentFactory->DocumentFromString(text);
    return BestPrediction(document);
}

const TDocumentFactory* TMultiLogisticRegressionClassifierModel::GetDocumentFactory() const {
    return ModelOptions.LemmerOptions.DocumentFactory.Get();
}

void TMultiLogisticRegressionClassifierModel::MinimizeBeforeSaving() {
    ModelOptions.LemmerOptions.MinimizeLemmasMerger(LogisticRegressionModel.Weights);
}

template <>
THolder<TMultiLogisticRegressionClassifierModel> LearnMultiLogisticRegressionClassifier<TMultiBinaryLabelFloatFeatureVector>(
        TAnyConstIterator<TMultiBinaryLabelFloatFeatureVector> begin,
        TAnyConstIterator<TMultiBinaryLabelFloatFeatureVector> end,
        const TVector<TString>& allLabels,
        const TTextClassifierModelOptions& options,
        const TMultiLabelLogisticRegressionLearner& logisticRegressionLearner)
{
    TMultiLabelLinearClassifierModel lrModel = logisticRegressionLearner.Learn(begin, end, allLabels);

    return THolder<TMultiLogisticRegressionClassifierModel>(new TMultiLogisticRegressionClassifierModel(options, lrModel));
}

template <>
THolder<TMultiLogisticRegressionClassifierModel> LearnMultiLogisticRegressionClassifier<TMultiLabelDocument>(
        TAnyConstIterator<TMultiLabelDocument> begin,
        TAnyConstIterator<TMultiLabelDocument> end,
        const TVector<TString>& allLabels,
        const TTextClassifierModelOptions& options,
        const TMultiLabelLogisticRegressionLearner& logisticRegressionLearner)
{
    TMultiBinaryLabelFloatFeatureVectors learnItems = FeatureVectorsFromMultiLabelDocuments(begin, end, allLabels, options);

    TAnyConstIterator<TMultiBinaryLabelFloatFeatureVector> learnItemsBegin(learnItems.begin());
    TAnyConstIterator<TMultiBinaryLabelFloatFeatureVector> learnItemsEnd(learnItems.end());

    return LearnMultiLogisticRegressionClassifier<TMultiBinaryLabelFloatFeatureVector>(
            learnItemsBegin, learnItemsEnd, allLabels, options, logisticRegressionLearner);
}

}
