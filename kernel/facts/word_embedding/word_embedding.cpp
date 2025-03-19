#include "word_embedding.h"

#include <util/folder/path.h>
#include <util/generic/ymath.h>
#include <util/stream/file.h>
#include <util/string/split.h>

namespace {
    void EnsureParams(const NSc::TDict& config, const TVector<TStringBuf>& params) {
        for (const TStringBuf& param : params) {
            Y_ENSURE(config.contains(param), "Param " + TString(param) + " cannot be found");
        }
    }
    void EnsureFile(const TStringBuf& fileName) {
        Y_ENSURE(TFsPath(fileName).Exists(), "File " + TString(fileName) + " cannot be found");
    }
}

namespace NUnstructuredFeatures {
    namespace NWordEmbedding {

        TWordEmbeddingFeaturesBuilder::TWordEmbeddingFeaturesBuilder(const TString& configJson,
                                                                     const THashMap<TString, TString>& resourceFiles)
            : ResourceFiles(resourceFiles) {
            NSc::TValue config = NSc::TValue::FromJsonThrow(configJson);

            for (const auto& dictionaryName : config["dictionaries"].GetArray()) {
                TString name = TString{dictionaryName.GetString()};
                const TString& wordsFile = ResourceFiles.at(name + ".words");
                EnsureFile(wordsFile);
                const TString& vectorsFile = ResourceFiles.at(name + ".vectors");
                EnsureFile(vectorsFile);
                Dictionaries[name] = MakeHolder<TDictionary>(wordsFile, vectorsFile);
            }

            for (const auto& preprocessorConfig : config["preprocessors"].GetArray()) {
                BuildPreprocessor(preprocessorConfig);
            }

            for (const auto& weighterConfig : config["weighters"].GetArray()) {
                BuildWeighter(weighterConfig);
            }

            for (const auto& featureGroupConfig : config["feature_groups"].GetArray()) {
                EnsureParams(featureGroupConfig, {"left_input", "right_input"});

                TFeatureGroup featureGroup;
                BuildCalculatorInput(featureGroupConfig["left_input"], featureGroup.LeftInput);
                BuildCalculatorInput(featureGroupConfig["right_input"], featureGroup.RightInput);

                for (const auto& calculatorConfig : featureGroupConfig["calculators"].GetArray()) {
                    BuildCalculator(calculatorConfig, featureGroup);
                }

                FeatureGroups.push_back(std::move(featureGroup));
            }
        }

        void TWordEmbeddingFeaturesBuilder::BuildPreprocessor(const NSc::TDict& config) {
            EnsureParams(config, {"type"});

            const TStringBuf& type = config.Get("type").GetString();

            if (type == "subtract_lemmas") {
                EnsureParams(config, {"left_input", "right_input", "output"});
                const TStringBuf& leftInput = config.Get("left_input").GetString();
                const TStringBuf& rightInput = config.Get("right_input").GetString();
                const TStringBuf& output = config.Get("output").GetString();
                Preprocessors.emplace_back(new TSubtractLemmasPreprocessor(TString{leftInput}, TString{rightInput}, TString{output}));
            } else {
                ythrow yexception() << "An unknown type (" << type << ") of preprocessor is used";
            }
        }

        void TWordEmbeddingFeaturesBuilder::BuildCalculatorInput(const NSc::TDict& config, TCalculatorInput& input) const {
            EnsureParams(config, {"annotation", "dictionary"});

            input.InputAnnotation = config.Get("annotation").GetString();
            const TStringBuf& dictionaryName = config.Get("dictionary").GetString();
            Y_ENSURE(Dictionaries.contains(dictionaryName));
            input.Dictionary = Dictionaries.at(dictionaryName).Get();

            if (config.contains("weighter")) {
                const TStringBuf& weighterName = config.Get("weighter").GetString();
                Y_ENSURE(Weighters.contains(weighterName));
                input.Weighter = Weighters.at(weighterName).Get();
            }
        }

        void TWordEmbeddingFeaturesBuilder::BuildCalculator(const NSc::TDict& config,
                                                            TFeatureGroup& featureGroup) const {
            EnsureParams(config, {"type", "names"});

            const TStringBuf& type = config.Get("type").GetString();

            TVector<EFactorId> factorIds;
            for(const auto& name: config.Get("names").GetArray()) {
                EFactorId factorId;
                if (TryFromString<EFactorId>(name.GetString(), factorId)) {
                    factorIds.push_back(factorId);
                } else {
                    ythrow yexception() << "feature name is not in factors_gen.in for calculator type (" << type << ")";
                }
            }

            const size_t embeddingSize = featureGroup.LeftInput.Dictionary->GetEmbeddingSize();

            if (type == "sum_similarity") {
                Y_ENSURE(factorIds.size() == 1);
                featureGroup.Calculators.emplace_back(new TSumSimCalculator(std::move(factorIds), embeddingSize));
            } else if (type == "mean_euclidean_distance") {
                Y_ENSURE(factorIds.size() == 1);
                featureGroup.Calculators.emplace_back(new TMeanEuclideanDistCalculator(std::move(factorIds), embeddingSize));
            } else if (type == "vectors_to_sum_similarity") {
                EnsureParams(config, {"dictionary", "vectors"});
                const TStringBuf& dictionaryName = config.Get("dictionary").GetString();
                const TDictionary* dictionary = Dictionaries.at(dictionaryName).Get();
                TVector<TEmbedding> vectors;
                for (const auto& vectorsItem : config.Get("vectors").GetArray()) {
                    const TStringBuf& name = vectorsItem.GetString();
                    TEmbedding embedding;
                    Y_ENSURE(dictionary->GetEmbedding(UTF8ToWide(name), embedding),
                             "Dictionary " + TString(dictionaryName) + " doesn't contain " + TString(name));
                    vectors.push_back(std::move(embedding));
                }
                Y_ENSURE(factorIds.size() == 2 * vectors.size());
                featureGroup.Calculators.emplace_back(new TVectorsToSumSimCalculator(std::move(factorIds), vectors, embeddingSize));
            } else if (type == "pairwise_similarity_statistics") {
                EnsureParams(config, {"features", "left_prefix_limit", "right_prefix_limit"});

                TVector<EPairwiseStat> features;
                for (const auto& feature : config.Get("features").GetArray()) {
                    features.push_back(FromString<EPairwiseStat>(feature.GetString()));
                }
                Y_ENSURE(factorIds.size() == features.size());

                size_t leftPrefixLimit = static_cast<size_t>(config.Get("left_prefix_limit").GetIntNumber());
                size_t rightPrefixLimit = static_cast<size_t>(config.Get("right_prefix_limit").GetIntNumber());
                size_t windowSize = static_cast<size_t>(config.Get("window_size").GetIntNumber(1));

                featureGroup.Calculators.emplace_back(
                    new TPairwiseSimStatsCalculator(std::move(factorIds), std::move(features), leftPrefixLimit,
                        rightPrefixLimit, windowSize, embeddingSize)
                );
            } else {
                ythrow yexception() << "An unknown type (" << type << ") of calculator is used";
            }
        }

        void TWordEmbeddingFeaturesBuilder::BuildWeighter(const NSc::TDict& config) {
            EnsureParams(config, {"type", "name"});

            const TStringBuf& type = config.Get("type").GetString();
            const TStringBuf& name = config.Get("name").GetString();

            if (type == "sif_weighter") {
                EnsureParams(config, {"alpha"});

                float alpha = static_cast<float>(config.Get("alpha").GetNumber());

                Weighters[name] = MakeHolder<TSIFWeighter>(alpha);
            } else {
                ythrow yexception() << "An unknown type (" << type << ") of weighter is used";
            }
        }

        void TWordEmbeddingFeaturesBuilder::GetWordEmbeddings(TDictionary* dictionary, TWeighter* weighter, const TVector<TAnalyzedWord>& analyzedWords,
                                                              TVector<TEmbedding>& embeddings) const {
            TVector<float> weights;
            if (weighter != nullptr) {
                weighter->Weight(analyzedWords, weights);
            }
            TEmbedding embedding;
            for (size_t i = 0; i < analyzedWords.size(); i++) {
                dictionary->GetEmbedding(analyzedWords[i].Word, embedding);
                if (!weights.empty()) {
                    Multiply(embedding, weights[i]);
                }
                embeddings.push_back(std::move(embedding));
            }
        }

        void TWordEmbeddingFeaturesBuilder::BuildFeatures(THashMap<TString, TVector<TAnalyzedWord>>& input,
                                                          TFactFactorStorage& features) const {
            TVector<float> wordEmbeddingFeatures;
            for (const auto& preprocessor : Preprocessors) {
                preprocessor->Do(input);
            }
            for (const auto& featureGroup : FeatureGroups) {
                const TVector<TAnalyzedWord>& leftInput = input.at(featureGroup.LeftInput.InputAnnotation);
                TVector<TEmbedding> leftEmbeddings;
                GetWordEmbeddings(featureGroup.LeftInput.Dictionary, featureGroup.LeftInput.Weighter, leftInput, leftEmbeddings);

                const TVector<TAnalyzedWord>& rightInput = input.at(featureGroup.RightInput.InputAnnotation);
                TVector<TEmbedding> rightEmbeddings;
                GetWordEmbeddings(featureGroup.RightInput.Dictionary, featureGroup.RightInput.Weighter, rightInput, rightEmbeddings);

                for (const auto& calculator : featureGroup.Calculators) {
                    calculator->Calculate(leftEmbeddings, rightEmbeddings, features);
                }
            }
        }
    }
}
