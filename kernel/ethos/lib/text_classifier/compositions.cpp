#include "compositions.h"

#include <kernel/ethos/lib/metrics/binary_classification_metrics.h>

namespace NEthos {

namespace {
    static inline TBinaryLabelWithFloatPrediction MakeBinaryLabelWithFloatPrediction(const double prediction) {
        NEthos::EBinaryClassLabel label = prediction > 0 ? NEthos::EBinaryClassLabel::BCL_POSITIVE : NEthos::EBinaryClassLabel::BCL_NEGATIVE;
        return TBinaryLabelWithFloatPrediction(label, prediction);
    }
}

TBinaryLabelWithFloatPrediction TCombinedTextClassifierModel::Apply(const TDocument& document, const TApplyOptions* applyOptions) const {
    return Apply(document.UnigramHashes, applyOptions);
}

TBinaryLabelWithFloatPrediction TCombinedTextClassifierModel::Apply(const TStringBuf text, const TApplyOptions* applyOptions) const {
    TDocument document = FeaturesMaker.DocumentFromText(text);
    return Apply(document, applyOptions);
}

TBinaryLabelWithFloatPrediction TCombinedTextClassifierModel::Apply(const TVector<ui64>& hashes, const TApplyOptions* applyOptions) const {
    double prediction = Predictor.CalcRelev(FeaturesMaker.Features<float>(hashes));
    if (applyOptions && applyOptions->NormalizeOnLength && hashes.size()) {
        prediction /= hashes.size();
    }
    return MakeBinaryLabelWithFloatPrediction(prediction);
}

const TDocumentFactory* TCombinedTextClassifierModel::GetDocumentFactory() const {
    return FeaturesMaker.GetDocumentFactory();
}

namespace {
    void SetupBias(NRegTree::TRegressionModel& predictor, const NRegTree::TRegressionModel::TPoolType& pool) {
        TBcMetricsCalculator metricsCalculator;
        for (const NRegTree::TRegressionModel::TInstanceType& instance : pool) {
            const double predictionValue = predictor.NonTransformedPrediction(instance.Features);
            const EBinaryClassLabel predictionClass = predictionValue > 0 ? EBinaryClassLabel::BCL_POSITIVE
                                                                          : EBinaryClassLabel::BCL_NEGATIVE;
            const EBinaryClassLabel targetClass = instance.OriginalGoal > 0 ? EBinaryClassLabel::BCL_POSITIVE
                                                                            : EBinaryClassLabel::BCL_NEGATIVE;
            metricsCalculator.Add(predictionValue, predictionClass, targetClass);
        }
        const double bias = metricsCalculator.BestThreshold();
        predictor.SetupBias(bias);
    }
}

TCombinedTextClassifierLearner::TModel TCombinedTextClassifierLearner::Learn(TAnyConstIterator<TBinaryLabelDocument> begin,
                                                                             TAnyConstIterator<TBinaryLabelDocument> end) const
{
    TTextClassifierFeaturesMaker featuresMaker;
    NRegTree::TRegressionModel::TPoolType pool = featuresMaker.Learn(TextClassifierOptions,
                                                                     begin,
                                                                     end,
                                                                     RegTreeOptions.ThreadsCount);

    NRegTree::TRegressionModel predictor;
    predictor.LearnMutable(pool, NRegTree::BuildFeatureIterators(pool, RegTreeOptions), RegTreeOptions);
    if (TextClassifierOptions.LinearClassifierOptions.ThresholdMode == EThresholdMode::SELECT_BEST_THRESHOLD) {
        SetupBias(predictor, pool);
    }

    return TModel(std::move(featuresMaker), predictor);
}

TCombinedTextClassifierLearner::TModel TCombinedTextClassifierLearner::LearnAndSave(TAnyConstIterator<TBinaryLabelDocument> begin,
                                                                                    TAnyConstIterator<TBinaryLabelDocument> end,
                                                                                    const TString& featuresMakerIndexPath,
                                                                                    const TString& predictorPath) const
{
    TTextClassifierFeaturesMaker featuresMaker;
    NRegTree::TRegressionModel::TPoolType pool = featuresMaker.Learn(TextClassifierOptions,
                                                                     begin,
                                                                     end,
                                                                     RegTreeOptions.ThreadsCount);

    NRegTree::TRegressionModel predictor;
    predictor.LearnMutable(pool, NRegTree::BuildFeatureIterators(pool, RegTreeOptions), RegTreeOptions);
    if (TextClassifierOptions.LinearClassifierOptions.ThresholdMode == EThresholdMode::SELECT_BEST_THRESHOLD) {
        SetupBias(predictor, pool);
    }

    featuresMaker.SaveToFile(featuresMakerIndexPath);
    predictor.SaveToFile(predictorPath);

    return TModel(std::move(featuresMaker), predictor);
}

}
