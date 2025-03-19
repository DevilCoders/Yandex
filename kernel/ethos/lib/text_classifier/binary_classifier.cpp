#include "binary_classifier.h"

#include "tagged_models/model_1562208.h"

#include <library/cpp/containers/dense_hash/dense_hash.h>

namespace NEthos {

TSimpleSharedPtr<IBinaryTextClassifierModel> TBinaryTextClassifierModel::CreateModel() {
    if (Version > 0 && Version < 1567316) {
        return new NEthos_1562208::TDefaultTextClassifierModel();
    } else {
        return new TLinearBinaryClassifierModel();
    }
}

TBinaryLabelWithFloatPrediction TBinaryTextClassifierModel::Apply(const TFloatFeatureVector& features, const TApplyOptions* applyOptions) const {
    return Model->Apply(features, applyOptions);
}

TBinaryLabelWithFloatPrediction TBinaryTextClassifierModel::Apply(const TDocument& document, const TApplyOptions* applyOptions) const {
    return Model->Apply(document, applyOptions);
}

TBinaryLabelWithFloatPrediction TBinaryTextClassifierModel::Apply(const TBinaryLabelFloatFeatureVector& featureVector, const TApplyOptions* applyOptions) const {
    return Model->Apply(featureVector, applyOptions);
}

TBinaryLabelWithFloatPrediction TBinaryTextClassifierModel::Apply(TStringBuf text, const TApplyOptions* applyOptions) const {
    return Model->Apply(text, applyOptions);
}

TBinaryLabelWithFloatPrediction TBinaryTextClassifierModel::Apply(const TVector<ui64>& hashes, const TApplyOptions* applyOptions) const {
    return Model->Apply(hashes, applyOptions);
}

TBinaryLabelWithFloatPrediction TBinaryTextClassifierModel::Apply(const TUtf16String& wideText, const TApplyOptions* applyOptions) const {
    return Model->Apply(wideText, applyOptions);
}

TBinaryLabelWithFloatPrediction TBinaryTextClassifierModel::ApplyClean(const TUtf16String& cleanWideText, const TApplyOptions* applyOptions) const {
    return Model->ApplyClean(cleanWideText, applyOptions);
}

const TDocumentFactory* TBinaryTextClassifierModel::GetDocumentFactory() const {
    return Model->GetDocumentFactory();
}

TBinaryLabelWithFloatPrediction TLinearBinaryClassifierModel::Apply(const TFloatFeatureVector& features, const TApplyOptions* applyOptions) const {
    return LinearClassifierModel.Apply(features, applyOptions);
}

TBinaryLabelWithFloatPrediction TLinearBinaryClassifierModel::Apply(const TDocument& document, const TApplyOptions* applyOptions) const {
    return Apply(FeatureVectorFromDocument(document, ModelOptions), applyOptions);
}

TBinaryLabelWithFloatPrediction TLinearBinaryClassifierModel::Apply(const TBinaryLabelFloatFeatureVector& featureVector, const TApplyOptions* applyOptions) const {
    return Apply(featureVector.Features, applyOptions);
}

TBinaryLabelWithFloatPrediction TLinearBinaryClassifierModel::Apply(TStringBuf text, const TApplyOptions* applyOptions) const {
    TDocument document = ModelOptions.LemmerOptions.DocumentFactory->DocumentFromString(text);
    return Apply(document, applyOptions);
}

TBinaryLabelWithFloatPrediction TLinearBinaryClassifierModel::Apply(const TVector<ui64>& hashes, const TApplyOptions* applyOptions) const {
    return Apply(FeatureVectorFromHashes(hashes, ModelOptions), applyOptions);
}

TBinaryLabelWithFloatPrediction TLinearBinaryClassifierModel::Apply(const TUtf16String& wideText, const TApplyOptions* applyOptions) const {
    TDocument document = ModelOptions.LemmerOptions.DocumentFactory->DocumentFromWtring(wideText);
    return Apply(document, applyOptions);
}

TBinaryLabelWithFloatPrediction TLinearBinaryClassifierModel::ApplyClean(const TUtf16String& cleanWideText, const TApplyOptions* applyOptions) const {
    TDocument document = ModelOptions.LemmerOptions.DocumentFactory->DocumentFromCleanWtring(cleanWideText);
    return Apply(document, applyOptions);
}

const TCompactSingleLabelFloatWeights& TLinearBinaryClassifierModel::GetWeights() const {
    return LinearClassifierModel.GetWeights();
}

double TLinearBinaryClassifierModel::GetThreshold() const {
    return LinearClassifierModel.GetThreshold();
}

const TDocumentFactory* TLinearBinaryClassifierModel::GetDocumentFactory() const {
    return ModelOptions.LemmerOptions.DocumentFactory.Get();
}

}

void TLinearBinaryClassifierModel::PrintDictionary(IOutputStream& out, const bool shortExport) const {
    TLemmasMergerFactory* documentFactory = dynamic_cast<TLemmasMergerFactory*>(ModelOptions.LemmerOptions.DocumentFactory.Get());
    if (!documentFactory) {
        return;
    }

    const NLemmasMerger::TLemmasMerger& lemmasMerger = documentFactory->LemmasMerger;

    TVector<std::pair<double, TUtf16String> > weightedWords;
    THashSet<ui32> usedWords;

    const TCompactSingleLabelFloatWeights& weights = GetWeights();
    for (const std::pair<TUtf16String, ui32>& lemmasMergerEntry : lemmasMerger) {
        ui64 wordHash = lemmasMerger.Hash(lemmasMergerEntry.second);
        if (const float* weight = weights.FindPtrToWeights(wordHash)) {
            if (!shortExport) {
                out << lemmasMergerEntry.first << "\t" << *weight << "\n";
            }

            TMaybe<ui32> wordNumber = lemmasMerger.GetWordNumber(lemmasMergerEntry.first);
            if (wordNumber && !usedWords.contains(*wordNumber)) {
                weightedWords.push_back(std::make_pair(*weight, lemmasMerger.GetWordByNumber(*wordNumber)));
                if (shortExport) {
                    usedWords.insert(*wordNumber);
                }
            }
        }
    }

    if (shortExport) {
        Sort(weightedWords.begin(), weightedWords.end(), TGreater<>());
        for (const std::pair<double, TUtf16String>& weightWithWeight : weightedWords) {
            out << weightWithWeight.second << "\t" << weightWithWeight.first << "\n";
        }
    }
}
