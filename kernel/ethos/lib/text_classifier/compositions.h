#pragma once

#include "options.h"

#include "classifier_features.h"

#include <util/generic/ptr.h>
#include <util/random/random.h>

namespace NEthos {

class TCombinedTextClassifierModel {
private:
    TTextClassifierFeaturesMaker FeaturesMaker;
    NRegTree::TPredictor Predictor;
public:
    TCombinedTextClassifierModel(const TString& featuresMakerIndexPath,
                                 const TString& predictorPath)
        : Predictor(predictorPath)
    {
        FeaturesMaker.LoadFromFile(featuresMakerIndexPath);
    }

    TCombinedTextClassifierModel(TTextClassifierFeaturesMaker&& featuresMaker,
                                 const NRegTree::TRegressionModel& regressionModel)
        : FeaturesMaker(std::move(featuresMaker))
        , Predictor(regressionModel)
    {
    }

    TCombinedTextClassifierModel(TTextClassifierFeaturesMaker&& featuresMaker,
                                 const NMatrixnet::TMnSseDynamic& mxNetModel)
        : FeaturesMaker(std::move(featuresMaker))
        , Predictor(mxNetModel)
    {
    }

    TBinaryLabelWithFloatPrediction Apply(const TDocument& document, const TApplyOptions* applyOptions = nullptr) const;
    TBinaryLabelWithFloatPrediction Apply(const TStringBuf text, const TApplyOptions* applyOptions = nullptr) const;
    TBinaryLabelWithFloatPrediction Apply(const TVector<ui64>& hashes, const TApplyOptions* applyOptions = nullptr) const;

    template <typename TArgumentType>
    double Probability(const TArgumentType& argument) const {
        return Sigmoid(Apply(argument).Prediction);
    }

    const TDocumentFactory* GetDocumentFactory() const;
};

class TCombinedTextClassifierLearner {
public:
    using TModel = TCombinedTextClassifierModel;

    TTextClassifierOptions TextClassifierOptions;
    NRegTree::TOptions RegTreeOptions;
public:
    TCombinedTextClassifierLearner(const TTextClassifierOptions& textClassifierOptions,
                                   const NRegTree::TOptions& regTreeOptions)
        : TextClassifierOptions(textClassifierOptions)
        , RegTreeOptions(regTreeOptions)
    {
    }

    TModel Learn(TAnyConstIterator<TBinaryLabelDocument> begin,
                 TAnyConstIterator<TBinaryLabelDocument> end) const;

    TModel LearnAndSave(TAnyConstIterator<TBinaryLabelDocument> begin,
                        TAnyConstIterator<TBinaryLabelDocument> end,
                        const TString& featuresMakerIndexPath,
                        const TString& predictorPath) const;
};

class TNewCombinedTextClassifierModel {
private:
    TLemmasMergerFactory LemmasMergerDocumentsFactory;
    THashMap<ui64, ui64> WordHashesRemap;

    size_t FeaturesCount = 0;
    double Offset = 0.;

    NMatrixnet::TMnSseDynamic Model;
public:
    TNewCombinedTextClassifierModel() {
    }

    const TLemmasMergerFactory& GetDocumentsFactory() const {
        return LemmasMergerDocumentsFactory;
    }

    void LoadWithoutMatrixnet(const TString& textModelPath) {
        TFileInput textModelIn(textModelPath);
        Load(&textModelIn);
    }

    void Load(const TString& textModelPath, const TString& matrixnetModelPath) {
        LoadWithoutMatrixnet(textModelPath);
        LoadMatrixnetModel(matrixnetModelPath);
    }

    void Load(IInputStream* in) {
        LemmasMergerDocumentsFactory.LoadLemmasMerger(in);
        ::Load(in, WordHashesRemap);
        ::Load(in, FeaturesCount);
        ::Load(in, Offset);
    }

    void Save(IOutputStream* out) const {
        LemmasMergerDocumentsFactory.GetLemmasMerger().Save(out);
        ::Save(out, WordHashesRemap);
        ::Save(out, FeaturesCount);
        ::Save(out, Offset);
    }

    void LoadMatrixnetModel(const TString& mxInfoPath) {
        TFileInput modelIn(mxInfoPath);
        LoadMatrixnetModel(&modelIn);
    }

    void LoadMatrixnetModel(IInputStream* in) {
        Model.Load(in);
    }

    void SetupOffset(const double offset) {
        Offset = offset;
    }

    void Init(const TLemmerOptions& lemmerOptions, const TCompactSingleLabelFloatWeights& weights, const size_t binsCount, const size_t topWordsCount) {
        TLemmasMergerFactory* documentFactory = dynamic_cast<TLemmasMergerFactory*>(lemmerOptions.GetDocumentFactory());
        if (!documentFactory) {
            return;
        }

        TVector<std::pair<double, ui64>> sortedWords;
        for (auto& indexWithWeight : weights.GetWeightMap()) {
            sortedWords.push_back(std::make_pair(indexWithWeight.second, indexWithWeight.first));
        }
        Sort(sortedWords.begin(), sortedWords.end(), TGreater<>());


        FeaturesCount = binsCount;
        for (size_t i = 0; i < sortedWords.size(); ++i) {
            const size_t bin = i * binsCount / sortedWords.size();
            WordHashesRemap[sortedWords[i].second] = bin;
        }

        for (size_t i = 0; i < topWordsCount && i < sortedWords.size(); ++i) {
            WordHashesRemap[sortedWords[i].second] = FeaturesCount;
            ++FeaturesCount;
        }

        NLemmasMerger::TLemmasMerger& inputLemmasMerger = documentFactory->LemmasMerger;
        TCompactTrieBuilder<wchar16, ui64> minimizedLemmasMergerBuilder;
        for (NLemmasMerger::TLemmasMerger::TConstIterator lmIterator = inputLemmasMerger.Begin(); lmIterator != inputLemmasMerger.End(); ++lmIterator) {
            ui64 wordHash = inputLemmasMerger.Hash(lmIterator.GetValue());
            ui64* existentWordRemap = WordHashesRemap.FindPtr(wordHash);
            if (!existentWordRemap) {
                continue;
            }
            minimizedLemmasMergerBuilder.Add(lmIterator.GetKey(), lmIterator.GetValue());
        }

        TBufferOutput raw;
        minimizedLemmasMergerBuilder.Save(raw);

        TBufferOutput compacted;
        CompactTrieMinimize<TCompactTrie<>::TPacker>(compacted, raw.Buffer().Data(), raw.Buffer().Size(), false);

        LemmasMergerDocumentsFactory.LoadLemmasMerger(TBlob::FromBuffer(compacted.Buffer()));
    }

    TVector<float> CreateFeatures(const TDocument& document) const {
        TVector<float> features(FeaturesCount + 1);
        for (const ui64 wordHash : document.UnigramHashes) {
            const ui64* remap = WordHashesRemap.FindPtr(wordHash);
            if (remap) {
                ++features[*remap];
            } else {
                ++features.back();
            }
        }
        return features;
    }

    TVector<float> CreateFeatures(const TStringBuf text) const {
        return CreateFeatures(LemmasMergerDocumentsFactory.DocumentFromString(text));
    }

    TBinaryLabelWithFloatPrediction Apply(const TStringBuf text) const {
        return Apply(text, 0.);
    }

    TBinaryLabelWithFloatPrediction Apply(const TStringBuf text, const double threshold) const {
        TVector<float> features = CreateFeatures(text);
        double prediction = Model.CalcRelev(features) + Offset;

        EBinaryClassLabel label = BinaryLabelFromPrediction(prediction, threshold);
        return TBinaryLabelWithFloatPrediction(label, prediction);
    }
};

}
