#include "options_1562208.h"
#include "model_1562208.h"

namespace NEthos_1562208 {

template<typename T>
double Factor(const TBinaryLabelFeatureVector<T>& item, const TLinearClassifierOptions& options) {
    double factor = item.IsPositive() ? options.PositivesFactor : -options.NegativesFactor;
    factor *= item.Weight;
    return factor;
}

double RawPrediction(const TFloatFeatureVector& features,
                     const TLogisticRegressionModel::TWeightsMap& weights)
{
    double result = 0.;
    for (const TIndexedFloatFeature& feature : features) {
        if (const float* weight = weights.FindPtr(feature.Index)) {
            result += feature.Value * *weight;
        }
    }
    return result;
}

double TLogisticRegressionModel::ZeroThresholdPrediction(const TFloatFeatureVector& features) const {
    double result = RawPrediction(features, Weights);
    return result - Threshold;
}

double TLogisticRegressionModel::ProbabilityPrediction(const TFloatFeatureVector& features) const {
    return Sigmoid(ZeroThresholdPrediction(features));
}

TBinaryLabelWithFloatPrediction TLogisticRegressionModel::Apply(
        const TFloatFeatureVector& features, const NEthos::TApplyOptions*) const
{
    double nonTransformedPrediction = ZeroThresholdPrediction(features);
    EBinaryClassLabel label = BinaryLabelFromPrediction(nonTransformedPrediction, 0.);
    return TBinaryLabelWithFloatPrediction(label, nonTransformedPrediction);
}

TBinaryLabelWithFloatPrediction TLogisticRegressionModel::Apply(
        const TBinaryLabelFloatFeatureVector& features, const NEthos::TApplyOptions*) const
{
    return Apply(features.Features);
}

TLogisticRegressionModel& TLogisticRegressionModel::operator += (const TLogisticRegressionModel& other) {
    for (auto& indexWithWeight : other.Weights) {
        Weights[indexWithWeight.first] += indexWithWeight.second;
    }
    Threshold += other.Threshold;
    return *this;
}

const TLogisticRegressionModel::TWeightsMap& TLogisticRegressionModel::GetWeights() const {
    return Weights;
}

double TLogisticRegressionModel::GetThreshold() const {
    return Threshold;
}

namespace {

void AddHashes(const TVector<ui64>& hashes, const TIndexWeighter& indexWeighter,
               TVector<TIndexedFloatFeature>& hashWeights)
{
    for (size_t position = 0; position < hashes.size(); ++position) {
        ui64 hash = hashes[position];
        double positionWeight = indexWeighter(position);
        if (positionWeight < 0.1) {
            break;
        }
        hashWeights.push_back(TIndexedFloatFeature(hash, positionWeight));
    }
}

double TransformWordWeight(double value, EWordWeightsTransformation wwTransformation) {
    switch (wwTransformation) {
        case EWordWeightsTransformation::NO_TRANSFORMATION:
            return value;
        case EWordWeightsTransformation::LOG:
            return log(2. + value);
        case EWordWeightsTransformation::SQUARED_ROOT:
            return sqrt(value);
        case EWordWeightsTransformation::CONSTANT:
            return 1.;
    }
    Y_ASSERT(0);
    return 0.;
}

TFloatFeatureVector FeatureVectorFromHashes(const TVector<ui64>& unigramHashes,
                                            const TVector<ui64>& bigramHashes,
                                            const TTextClassifierModelOptions& options)
{
    TVector<TIndexedFloatFeature> features;
    AddHashes(unigramHashes, options.IndexWeighter, features);
    if (options.UseBigrams) {
        AddHashes(bigramHashes, options.IndexWeighter, features);
    }

    if (features.empty()) {
        return features;
    }

    Sort(features.begin(), features.end());

    TIndexedFloatFeature* write = features.begin();
    TIndexedFloatFeature* read = write + 1;

    for (; read != features.end(); ++read) {
        if (read->Index == write->Index) {
            write->Value += read->Value;
            continue;
        }
        write->Value = TransformWordWeight(write->Value, options.WordWeightsTransformation);

        ++write;
        *write = *read;
    }
    write->Value = TransformWordWeight(write->Value, options.WordWeightsTransformation);

    features.erase(write + 1, features.end());

    return features;
}

TFloatFeatureVector FeatureVectorFromDocument(const TDocument& document,
                                              const TTextClassifierModelOptions& options)
{
    TVector<ui64> bigramHashes;
    for (size_t i = 0; i + 1 < document.UnigramHashes.size(); ++i) {
        bigramHashes.push_back(CombineHashes<ui64>(document.UnigramHashes[i], document.UnigramHashes[i + 1]));
    }
    return FeatureVectorFromHashes(document.UnigramHashes, bigramHashes, options);
}

}

TBinaryLabelWithFloatPrediction TDefaultTextClassifierModel::Apply(const TFloatFeatureVector& features, const NEthos::TApplyOptions*) const {
    return LogisticRegressionModel.Apply(features);
}

TBinaryLabelWithFloatPrediction TDefaultTextClassifierModel::Apply(const TDocument& document, const NEthos::TApplyOptions*) const {
    return Apply(FeatureVectorFromDocument(document, ModelOptions));
}

TBinaryLabelWithFloatPrediction TDefaultTextClassifierModel::Apply(const TBinaryLabelFloatFeatureVector& featureVector, const NEthos::TApplyOptions*) const {
    return Apply(featureVector.Features);
}

TBinaryLabelWithFloatPrediction TDefaultTextClassifierModel::Apply(TStringBuf text, const NEthos::TApplyOptions*) const {
    TDocument document = ModelOptions.LemmerOptions.DocumentFactory->DocumentFromString(text);
    return Apply(document);
}

TBinaryLabelWithFloatPrediction TDefaultTextClassifierModel::Apply(const TVector<ui64>& hashes, const NEthos::TApplyOptions*) const {
    TVector<ui64> bigramHashes;
    if (ModelOptions.UseBigrams) {
        for (size_t i = 0; i + 1 < hashes.size(); ++i) {
            bigramHashes.push_back(CombineHashes<ui64>(hashes[i], hashes[i + 1]));
        }
    }
    return Apply(FeatureVectorFromHashes(hashes, bigramHashes, ModelOptions));
}

TBinaryLabelWithFloatPrediction TDefaultTextClassifierModel::Apply(const TUtf16String& wideText, const TApplyOptions* applyOptions) const {
    TDocument document = ModelOptions.LemmerOptions.DocumentFactory->DocumentFromWtring(wideText);
    return Apply(document, applyOptions);
}

TBinaryLabelWithFloatPrediction TDefaultTextClassifierModel::ApplyClean(const TUtf16String& cleanWideText, const TApplyOptions* applyOptions) const {
    TDocument document = ModelOptions.LemmerOptions.DocumentFactory->DocumentFromCleanWtring(cleanWideText);
    return Apply(document, applyOptions);
}

const TLogisticRegressionModel::TWeightsMap& TDefaultTextClassifierModel::GetWeights() const {
    return LogisticRegressionModel.GetWeights();
}

const TDocumentFactory* TDefaultTextClassifierModel::GetDocumentFactory() const {
    return ModelOptions.LemmerOptions.DocumentFactory.Get();
}

void TDefaultTextClassifierModel::MinimizeWeights() {
    if (fabs(ModelOptions.WeightsLowerBound - 0.f) > 1.0e-6f) {
        TLogisticRegressionModel::TWeightsMap minimizedWeights;
        double threshold = LogisticRegressionModel.GetThreshold();

        for (const auto& weight : GetWeights()) {
            if (fabs(weight.second) > ModelOptions.WeightsLowerBound) {
                minimizedWeights.emplace(weight);
            }
        }

        LogisticRegressionModel = TLogisticRegressionModel(minimizedWeights, threshold);
    }
}

// XXX: think of another design
void TDefaultTextClassifierModel::MinimizeLemmasMerger() {
    if(TLemmasMergerFactory* documentFactory =
            dynamic_cast<TLemmasMergerFactory*>(ModelOptions.LemmerOptions.DocumentFactory.Get()))
    {
        TCompactTrieBuilder<wchar16, ui64> minimizedLemmasMergerBuilder;
        {
            const TLogisticRegressionModel::TWeightsMap& weights = GetWeights();
            const NLemmasMerger::TLemmasMerger& lemmasMerger = documentFactory->LemmasMerger;
            for (auto&& word : lemmasMerger) {
                ui64 wordHash = lemmasMerger.Hash(word.second);
                if (weights.Has(wordHash)) {
                    minimizedLemmasMergerBuilder.Add(word.first, word.second);
                }
            }
        }

        TBufferOutput raw;
        minimizedLemmasMergerBuilder.Save(raw);

        TBufferOutput compacted;
        CompactTrieMinimize<TCompactTrie<>::TPacker>(compacted, raw.Buffer().Data(), raw.Buffer().Size(), false);

        documentFactory->LemmasMerger = NLemmasMerger::TLemmasMerger();
        documentFactory->LemmasMerger.Init(TBlob::FromBuffer(compacted.Buffer()));
    }
}

}
