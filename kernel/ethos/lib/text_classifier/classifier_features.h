#pragma once

#include "options.h"
#include "binary_classifier.h"
#include "util.h"

#include <kernel/ethos/lib/logistic_regression/logistic_regression.h>
#include <kernel/ethos/lib/features_selector/features_selector.h>
#include <kernel/ethos/lib/reg_tree/compositions.h>

#include <library/cpp/svnversion/svnversion.h>

#include <util/generic/ptr.h>

namespace NEthos {
class TTextClassifierFeaturesMaker {
public:
    int Version = GetProgramSvnRevision();

private:
    TTextClassifierModelOptions ModelOptions;

    size_t ModelsCount;
    TCompactMultiLabelFloatWeights Weights;
public:
    Y_SAVELOAD_DEFINE(ModelOptions, ModelsCount, Weights);

    void LoadFromFile(const TString& filename) {
        TFileInput in(filename);
        Load(&in);
    }

    void SaveToFile(const TString& filename) const {
        TFixedBufferFileOutput out(filename);
        Save(&out);
    }

    template<typename TFloatType>
    TVector<TFloatType> Features(const TVector<ui64>& hashes) const {
        TFloatFeatureVector features = FeatureVectorFromHashes(hashes, ModelOptions);
        TVector<double> predictions = Weights.Predictions(features, ModelsCount);

        return Features<TFloatType>(hashes, predictions);
    }

    template<typename TFloatType>
    TVector<TFloatType> Features(const TDocument& document) const {
        return Features<TFloatType>(document.UnigramHashes);
    }

    const TDocument DocumentFromText(const TStringBuf text) const {
        return ModelOptions.LemmerOptions.DocumentFactory->DocumentFromString(text);
    }

    template<typename TFloatType>
    TVector<TFloatType> Features(const TStringBuf text) const {
        return Features<TFloatType>(DocumentFromText(text));
    }

    NRegTree::TRegressionModel::TInstanceType ToInstance(const TBinaryLabelDocument& document) const;

    NRegTree::TRegressionModel::TPoolType Learn(TTextClassifierOptions&& options,
                                                TAnyConstIterator<TBinaryLabelDocument> begin,
                                                TAnyConstIterator<TBinaryLabelDocument> end,
                                                const size_t threadsCount);

    NRegTree::TRegressionModel::TPoolType Learn(const TTextClassifierOptions& options,
                                                TAnyConstIterator<TBinaryLabelDocument> begin,
                                                TAnyConstIterator<TBinaryLabelDocument> end,
                                                const size_t threadsCount)
    {
        TTextClassifierOptions optionsCopy(options);
        return Learn(std::move(optionsCopy), begin, end, threadsCount);
    }

    const TDocumentFactory* GetDocumentFactory() const {
        return ModelOptions.LemmerOptions.DocumentFactory.Get();
    }
private:
    struct TVariableOptions {
        const double PositivesFactor;
        const double Offset;
        const size_t IterationsCountMultiplier;

        ELearningMethod LearningMethod;
        bool LearnBayesOnNegatives;

        size_t BayesIterations;

        TLinearClassifierOptions Modify(const TLinearClassifierOptions& logisticRegressionOptions) const {
            TLinearClassifierOptions modifiedOptions(logisticRegressionOptions);
            modifiedOptions.NegativesFactor = 1.;
            modifiedOptions.PositivesFactor = PositivesFactor;
            modifiedOptions.PositivesOffset = Offset;
            modifiedOptions.NegativesOffset = Offset;
            modifiedOptions.IterationsCountMultiplier = IterationsCountMultiplier;
            modifiedOptions.LearningMethod = LearningMethod;
            modifiedOptions.LearnBayesOnNegatives = LearnBayesOnNegatives;
            modifiedOptions.BayesIterationsCount = BayesIterations;
            return modifiedOptions;
        }
    };

    static TVector<TVariableOptions> GenerateVariableOptions() {
        TVector<TVariableOptions> variableOptions;

        static double positivesFactors[] = {1., 10.};
        static double offsets[] = {0., 100.};
        static size_t iterationCountMultipliers[] = {5, 2, 1};

        variableOptions.push_back(TVariableOptions{1., 100., 10, ELearningMethod::LM_LOGISTIC_REGRESSION, false, 0});
        for (const size_t iterationsCountMultiplier : iterationCountMultipliers) {
            for (const double positivesFactor : positivesFactors) {
                for (const double offset : offsets) {
                    variableOptions.push_back(TVariableOptions{positivesFactor, offset, iterationsCountMultiplier, ELearningMethod::LM_LOGISTIC_REGRESSION, false, 0});
                }
            }
        }

        variableOptions.push_back(TVariableOptions{1., 0., 0, ELearningMethod::LM_NAIVE_BAYES, false, 0});
        variableOptions.push_back(TVariableOptions{1., 0., 0, ELearningMethod::LM_NAIVE_BAYES, false, 1});
        variableOptions.push_back(TVariableOptions{1., 0., 0, ELearningMethod::LM_NAIVE_BAYES, true, 0});
        variableOptions.push_back(TVariableOptions{1., 0., 0, ELearningMethod::LM_NAIVE_BAYES, true, 1});

        return variableOptions;
    }

    template <typename TFloatType>
    static void AddHashFeatures(const TVector<ui64>& hashes, TVector<TFloatType>& features) {
        ui32 maxHashCount = 0;
        THashMap<ui64, ui32> hashCount;
        for (const ui64 hash : hashes) {
            maxHashCount = Max(maxHashCount, ++hashCount[hash]);
        }

        double entropy = 0.;
        TVector<double> freqs;

        for (auto&& hashWithCount : hashCount) {
            const double freq = (double) hashWithCount.second / hashes.size();
            entropy += log(freq) * freq;
            freqs.push_back(freq);
        }
        Sort(freqs.begin(), freqs.end(), TGreater<>());

        for (size_t i = 0; i + 1 < freqs.size(); ++i) {
            freqs[i + 1] += freqs[i];
        }

        features.push_back(Logify(hashes.size()));
        features.push_back(Logify(hashCount.size()));
        features.push_back(Normalize(hashCount.size(), hashes.size()));
        features.push_back(Normalize(maxHashCount, hashes.size()));

        features.push_back(entropy);
        features.push_back(Accumulate(freqs, 0.0));
    }

    NRegTree::TRegressionModel::TPoolType LearnWithoutFeatureSelection(TTextClassifierOptions&& options,
                                                                       TAnyConstIterator<TBinaryLabelDocument> documentsBegin,
                                                                       TAnyIterator<TBinaryLabelFloatFeatureVector> begin,
                                                                       TAnyIterator<TBinaryLabelFloatFeatureVector> end,
                                                                       const size_t threadsCount);

    template <typename TFloatType>
    TVector<TFloatType> Features(const TVector<ui64>& hashes, const TVector<double>& predictions) const {
        TVector<TFloatType> features;

        AddHashFeatures(hashes, features);

        for (const double prediction : predictions) {
            const double logifiedPrediction = Logify(prediction);
            features.push_back(logifiedPrediction);
        }

        return features;
    }

    static double Logify(const double value) {
        return value > 0 ? log(1. + value) : -log(1. - value);
    }

    static double Normalize(const double value, const size_t normalizer) {
        return value / (normalizer + 1);
    }
};

}
