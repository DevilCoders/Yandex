#include "classifier_features.h"

#include <library/cpp/string_utils/url/url.h>

#include <library/cpp/string_utils/old_url_normalize/url.h>

#include <util/generic/hash.h>

namespace NEthos {

NRegTree::TRegressionModel::TInstanceType TTextClassifierFeaturesMaker::ToInstance(const TBinaryLabelDocument& document) const {
    NRegTree::TRegressionModel::TInstanceType instance;

    instance.QueryId = ComputeHash(GetOnlyHost(StrongNormalizeUrl(document.Id))) % Max<int>();
    instance.Goal = instance.OriginalGoal = document.Label == EBinaryClassLabel::BCL_POSITIVE ? +1.0 : -1.0;
    instance.Weight = document.Weight;
    instance.Features = Features<double>(document);
    instance.PoolId = 0;

    return instance;
}

NRegTree::TRegressionModel::TPoolType TTextClassifierFeaturesMaker::Learn(TTextClassifierOptions&& options,
                                                                          TAnyConstIterator<TBinaryLabelDocument> begin,
                                                                          TAnyConstIterator<TBinaryLabelDocument> end,
                                                                          const size_t threadsCount)
{
    TBinaryLabelFloatFeatureVectors learnItems = FeatureVectorsFromBinaryLabelDocuments(begin, end, options.ModelOptions);

    if (!options.LinearClassifierOptions.FeaturesToUseCount) {
        return LearnWithoutFeatureSelection(std::move(options), begin, learnItems.begin(), learnItems.end(), threadsCount);
    }

    TFeaturesSelector<TBinaryLabelFloatFeatureVector> featuresSelector;
    for (const TBinaryLabelFloatFeatureVector& featureVector : learnItems) {
        featuresSelector.Add(featureVector);
    }
    featuresSelector.Finish(options.LinearClassifierOptions.FeaturesToUseCount);

    TVector<TBinaryLabelFloatFeatureVector> modifiedItems;
    for (const TBinaryLabelFloatFeatureVector& featureVector : learnItems) {
        modifiedItems.push_back(featuresSelector.SelectFeatures(featureVector));
    }

    return LearnWithoutFeatureSelection(std::move(options), begin, modifiedItems.begin(), modifiedItems.end(), threadsCount);
}

NRegTree::TRegressionModel::TPoolType TTextClassifierFeaturesMaker::LearnWithoutFeatureSelection(TTextClassifierOptions&& options,
                                                                                                 TAnyConstIterator<TBinaryLabelDocument> documentsBegin,
                                                                                                 TAnyIterator<TBinaryLabelFloatFeatureVector> begin,
                                                                                                 TAnyIterator<TBinaryLabelFloatFeatureVector> end,
                                                                                                 const size_t threadsCount)
{
    TTextClassifierModelOptions modelOptions = options.ModelOptions;
    TLinearClassifierOptions baseLogisticRegressionOptions = std::move(options.LinearClassifierOptions);

    ModelOptions = std::move(options.ModelOptions);

    TVector<TVariableOptions> variableOptions = GenerateVariableOptions();
    ModelsCount = variableOptions.size();

    for (TAnyIterator<TBinaryLabelFloatFeatureVector> it = begin; it != end; ++it) {
        TVector<double>(ModelsCount).swap(it->ExternalFeatures);
    }

    TConcurrentMutableFloatWeights weights(ModelsCount);
    THolder<IThreadPool> queue = CreateThreadPool(threadsCount);
    for (size_t modelNumber = 0; modelNumber < ModelsCount; ++modelNumber) {
        queue->SafeAddFunc([&, modelNumber, begin, end]() {
            TLinearClassifierOptions logisticRegressionOptions = variableOptions[modelNumber].Modify(baseLogisticRegressionOptions);

            TBinaryTextWeightsLearner learner(logisticRegressionOptions);
            weights.Weights[modelNumber] = learner.LearnWeights(begin, end, modelNumber);
        });
    }
    queue->Stop();

    Weights = weights.ToCompactMultiLabelWeights(baseLogisticRegressionOptions.WeightsLowerBound);

    NRegTree::TRegressionModel::TPoolType pool;

    TAnyConstIterator<TBinaryLabelDocument> documentIt = documentsBegin;
    TAnyIterator<TBinaryLabelFloatFeatureVector> featuresIt = begin;
    for (; featuresIt != end; ++featuresIt, ++documentIt) {
        if (!featuresIt->HasKnownMark()) {
            continue;
        }

        NRegTree::TRegressionModel::TInstanceType instance;

        instance.QueryId = ComputeHash(GetOnlyHost(StrongNormalizeUrl(documentIt->Id))) % 1000000000;
        instance.Goal = instance.OriginalGoal = documentIt->Label == EBinaryClassLabel::BCL_POSITIVE ? +1.0 : -1.0;
        instance.Weight = documentIt->Weight;
        instance.Features = Features<double>(documentIt->UnigramHashes, featuresIt->ExternalFeatures);
        instance.PoolId = 0;

        pool.AddInstance(instance);
    }

    return pool;
}


}
