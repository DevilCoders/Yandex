#include "optimizations.h"
#include "tokenizer_builder.h"

#include <util/generic/hash_set.h>
#include <util/generic/map.h>
#include <util/generic/set.h>


namespace NNeuralNetApplier {

    void DoAddTrigramIndex(TModel& model) {
        auto trieToTermIndex = [](const auto& trie) {
            TTermToIndex mapping;
            for (const auto& item : trie) {
                mapping[item.first] = item.second;
            }
            return mapping;
        };

        const TString tokenizerType = "TTrigramTokenizer";

        for (const TString& var : model.Parameters.GetVariables()) {
            if (dynamic_cast<const TSparsifier*>(model.Parameters.at(var).Get())) {
                const TSparsifier& sparsifier = dynamic_cast<const TSparsifier&>(*model.Parameters.at(var).Get());
                TVector<TAtomicSharedPtr<ITokenizer>> tokenizers = sparsifier.GetTokenizers();
                for (TAtomicSharedPtr<ITokenizer>& tokenizer : tokenizers) {
                    if (tokenizer->GetTypeName() == tokenizerType) {
                        tokenizer = new TCachedTrigramTokenizer(
                            trieToTermIndex(tokenizer->GetTrie()),
                            tokenizer->NeedLowercaseInput(),
                            tokenizer->DoUseUnknownWord(),
                            tokenizer->GetUnknownWordId()
                        );
                    }
                }
                model.Parameters[var] = new TSparsifier(tokenizers);
            } else if (dynamic_cast<const TGlobalSparsifier*>(model.Parameters.at(var).Get())) {
                const TGlobalSparsifier& sparsifier = dynamic_cast<const TGlobalSparsifier&>(*model.Parameters.at(var).Get());
                TVector<TAtomicSharedPtr<ITokenizer>> tokenizers = sparsifier.GetTokenizers();
                auto remap = sparsifier.DoGetRemap();
                for (TAtomicSharedPtr<ITokenizer>& tokenizer : tokenizers) {
                    const TTokenizerUid oldName = tokenizer->GetUid();
                    if (tokenizer->GetTypeName() == tokenizerType) {
                        tokenizer = new TCachedTrigramTokenizer(
                            trieToTermIndex(tokenizer->GetTrie()),
                            tokenizer->NeedLowercaseInput(),
                            tokenizer->DoUseUnknownWord(),
                            tokenizer->GetUnknownWordId()
                        );
                    }
                    for (auto& i : remap) {
                        for (auto& j : i.second) {
                            if (j.first == oldName) {
                                j.first = tokenizer->GetUid();
                            }
                        }
                    }
                }
                model.Parameters[var] = new TGlobalSparsifier(tokenizers, remap);
            }
        }
    }

    void DoMergeTries(TModel& model) {
        TVector<ILayerPtr>& layers = model.Layers;

        // move all field extractor layers, so that they're calculated first
        // && collect all possible inputs
        THashSet<TString> inputSet;
        TVector<ILayerPtr> moveToStartLayers;
        THashSet<ILayer*> moveToStartLayersSet;
        for (const ILayerPtr& layer : layers) {
            if (dynamic_cast<const TStringToSparseMatrixLayer*>(layer.Get())) {
                TSet<TString> outputs;
                for (const TString& output : layer->GetOutputs()) {
                    outputs.insert(output);
                }
                const auto layerDependencies = GetOutputDependencies(model, outputs);
                for (const ILayerPtr& depLayer : layerDependencies.first) {
                    if (!moveToStartLayersSet.contains(depLayer.Get())) {
                        moveToStartLayersSet.insert(depLayer.Get());
                        moveToStartLayers.push_back(depLayer);
                    }
                }

                Y_ENSURE(layer->GetInputs().size() == 2, "Input with sparsifier should have size() == 2");
                const bool hasSparsifier0 = model.Parameters.has(layer->GetInputs()[0]);
                const bool hasSparsifier1 = model.Parameters.has(layer->GetInputs()[1]);
                if (!hasSparsifier0 && hasSparsifier1) {
                    inputSet.insert(layer->GetInputs()[0]);
                } else if (hasSparsifier0 && !hasSparsifier1) {
                    inputSet.insert(layer->GetInputs()[1]);
                } else {
                    Y_ENSURE(false);
                }
            }
        }

        layers.erase(std::remove_if(layers.begin(), layers.end(), [&moveToStartLayers](const auto& item) {
            return std::find(moveToStartLayers.begin(), moveToStartLayers.end(), item) != moveToStartLayers.end();
        }), layers.end());
        layers.insert(layers.begin(), moveToStartLayers.begin(), moveToStartLayers.end());

        // lambda for TStringToSparseMatrixLayer inputs and outputs propper extraction
        auto tryExtractLayerData = [&inputSet](const auto& layer, TString* inputName, TString* sparsifierName, TString* output = nullptr) {
            const TStringToSparseMatrixLayer* ptr = dynamic_cast<const TStringToSparseMatrixLayer*>(layer.Get());
            if (!ptr) {
                return false;
            }
            Y_ENSURE(ptr->GetInputs().size() == 2, "Input with sparsifier should have size() == 2");
            if (inputSet.contains(ptr->GetInputs()[0])) {
                *inputName = ptr->GetInputs()[0];
                *sparsifierName = ptr->GetInputs()[1];
            } else if (inputSet.contains(ptr->GetInputs()[1])) {
                *inputName = ptr->GetInputs()[1];
                *sparsifierName = ptr->GetInputs()[0];
            } else {
                return false;
            }

            if (output) {
                *output = ptr->GetOutputs()[0];
            }
            return true;
        };

        // create a mapping from the input to the set of its tokenizers
        TMap<TString, TSet<TString>> inputToTokenizerSet;
        for (const ILayerPtr& layer : layers) {
            TString input;
            TString sparsifierName;
            if (!tryExtractLayerData(layer, &input, &sparsifierName)) {
                continue;
            }

            const TSparsifier* sparsifier = dynamic_cast<TSparsifier*>(model.Parameters[sparsifierName].Get());
            Y_ENSURE(sparsifier, sparsifierName + " is not a sparsifier name");

            for (const TAtomicSharedPtr<ITokenizer>& tokenizer : sparsifier->GetTokenizers()) {
                inputToTokenizerSet[input].insert(tokenizer->GetUid());
            }
        }

        // mapping of inputs, groupped by same tokenizer sets
        TMap<TSet<TString>, TSet<TString>> groups2inputs;
        for (const auto& item : inputToTokenizerSet) {
            groups2inputs[item.second].insert(item.first);
        }

        // tokenizerSet -> list ( < output, tokenizer > )
        TMap<TSet<TString>, TVector<std::pair<TString, TAtomicSharedPtr<ITokenizer>>>> groups2outputs;

        // input -> outputlList
        THashMap<TString, TVector<TString>> inputs2outputs;

        // collect tokenizers, group them by inputGroup they belong to
        // erase all TStringToSparseMatrixLayer, except the first one for each input
        TVector<ILayerPtr> layersToRemove;
        for (auto layerPtr = layers.begin(); layerPtr != layers.end(); ++layerPtr) {
            TString input;
            TString sparsifierName;
            TString outputName;
            if (!tryExtractLayerData(*layerPtr, &input, &sparsifierName, &outputName)) {
                continue;
            }

            const TSparsifier* sparsifier = dynamic_cast<TSparsifier*>(model.Parameters.at(sparsifierName).Get());
            Y_ENSURE(sparsifier, sparsifierName + " is not a sparsifier name");

            const TVector<TAtomicSharedPtr<ITokenizer>>& tokenizers = sparsifier->GetTokenizers();
            TVector<std::pair<TString, TAtomicSharedPtr<ITokenizer>>>& destTokenizerVector = groups2outputs[inputToTokenizerSet.at(input)];
            for (const TAtomicSharedPtr<ITokenizer>& tokenizer : tokenizers) {
                destTokenizerVector.emplace_back(outputName, tokenizer);
            }
            inputs2outputs[input].push_back(outputName);

            layersToRemove.push_back(*layerPtr);
        }

        layers.erase(std::remove_if(layers.begin(), layers.end(), [&layersToRemove](const auto& item) {
            return std::find(layersToRemove.begin(), layersToRemove.end(), item) != layersToRemove.end();
        }), layers.end());

        if (layersToRemove.empty()) {
            return;
        }

        // for each group sort tokenizers, so that they're merged from smallest to biggest
        for (auto& item : groups2outputs) {
            Sort(item.second.begin(), item.second.end(), [](const auto& lhs, const auto& rhs) {
                return lhs.second->GetTrie().Size() < rhs.second->GetTrie().Size();
            });
        }

        // merge tokenizers, create TGlobalSparsifiers & TGlobalStringToSparseMatrixLayers
        TVector<ILayerPtr> globalSparseLayers;
        size_t globalSparsifierIndex = 0;
        for (const auto& inputGroup : groups2inputs) {
            THashMap<TString, TVector<std::pair<TTokenizerUid, TVector<ui32>>>> resultRemap;

            const TVector<std::pair<TString, TAtomicSharedPtr<ITokenizer>>>& tokList = groups2outputs.at(inputGroup.first);
            THashMap<TTokenizerUid, TTokenizerBuilder> uniqueTokenizers;

            for (const std::pair<TString, TAtomicSharedPtr<ITokenizer>>& tokPair : tokList) {
                const TString& out = tokPair.first;
                const TAtomicSharedPtr<ITokenizer>& tokenizer = tokPair.second;

                if (!uniqueTokenizers.contains(tokenizer->GetUid())) {
                    uniqueTokenizers[tokenizer->GetUid()] = TTokenizerBuilder(
                        tokenizer->GetTypeName(),
                        tokenizer->NeedLowercaseInput(),
                        tokenizer->DoUseUnknownWord(),
                        tokenizer->GetUnknownWordId(),
                        tokenizer->GetVersion()
                    );
                }

                auto& tokBuilder = uniqueTokenizers[tokenizer->GetUid()];

                // remap for specific pair <trie, output>
                TVector<ui32> remap;
                for (const std::pair<TUtf16String, size_t>& item : tokenizer->GetTrie()) {
                    const TUtf16String token = item.first;
                    const size_t oldId = item.second;
                    const size_t newId = tokBuilder.AddToken(token);
                    if (newId >= remap.size()) {
                        remap.resize(newId + 1, tokenizer->DoUseUnknownWord() ? tokenizer->GetUnknownWordId() : Max<ui32>());
                    }
                    remap[newId] = oldId;
                }
                remap.push_back(tokenizer->DoUseUnknownWord() ? tokenizer->GetUnknownWordId() : Max<ui32>());

                resultRemap[out].emplace_back(tokenizer->GetUid(), std::move(remap));
            }

            TVector<TAtomicSharedPtr<ITokenizer>> resultTokenizers;
            for (const auto& tokenizer : uniqueTokenizers) {
                resultTokenizers.push_back(tokenizer.second.GetTokenizer());
            }

            const TString globalSparsifierName = "GlobalSparsifier" + ToString(globalSparsifierIndex);
            ++globalSparsifierIndex;

            model.Parameters[globalSparsifierName] = new TGlobalSparsifier(resultTokenizers, resultRemap);

            for (const TString& input : inputGroup.second) {
                layers.push_back(
                    new TGlobalStringToSparseMatrixLayer(input, globalSparsifierName, inputs2outputs.at(input))
                );
                globalSparseLayers.push_back(layers.back());
            }
        }

        // remove unused variables from model
        model.Init();
        const TVector<TString> allVars = model.AllVariables();
        THashSet<TString> usedVariables(allVars.begin(), allVars.end());
        for (const TString& var : model.Parameters.GetVariables()) {
            if (!usedVariables.contains(var)) {
                model.RemoveVariable(var);
            }
        }

        // sort layers
        model.Init();
        for (const ILayerPtr& globalSparseLayer : globalSparseLayers) {
            TSet<TString> outputs;
            for (const TString& output : globalSparseLayer->GetOutputs()) {
                outputs.insert(output);
            }
            const auto layerDependencies = GetOutputDependencies(model, outputs);
            size_t lastDepLayerIndex = 0;
            for (const ILayerPtr& depLayer : layerDependencies.first) {
                if (depLayer == globalSparseLayer) {
                    continue;
                }
                const auto foundIt = Find(layers.begin(), layers.end(), depLayer);
                const size_t delta = foundIt - layers.begin();
                if (lastDepLayerIndex < delta) {
                    lastDepLayerIndex = delta;
                }
            }
            if (lastDepLayerIndex + 1 < layers.size()) {
                ++lastDepLayerIndex;
            }
            layers.erase(Find(layers.begin(), layers.end(), globalSparseLayer));
            layers.insert(layers.begin() + lastDepLayerIndex, globalSparseLayer);
        }
    }

} // namespace NNeuralNetApplier
