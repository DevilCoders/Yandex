#include "grads.h"

#include <kernel/dssm_applier/nn_applier/lib/layers.h>

#include <util/generic/cast.h>
#include <util/generic/hash.h>
#include <util/generic/queue.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/vector.h>

using namespace NNeuralNetApplier;

TString GetGradDataName(const TString& target, const TString& oldName) {
    return "grad__" + target + "__by__" + oldName;
}

template<class T>
T CheckInputAndCast(const TContext& context, const TString& input) {
    return VerifyDynamicCast<T>(context.at(input).Get());
}

const TString ZERO_BIAS_INPUT = "NNAPPLIER$GRAD$ZERO_SCALAR";

using TEdge = std::pair<TString, size_t>;
using TModelGraph = THashMap<TString, TVector<TEdge>>;

TSet<TString> LayersDfs(const TModelGraph& graph, const TVector<TString>& from) {
    TSet<TString> visited;
    TDeque<TString> elements(from.begin(), from.end());
    while (!elements.empty()) {
        auto vertex = elements.front();
        elements.pop_front();
        if (!visited.contains(vertex)) {
            visited.insert(vertex);
            if (graph.contains(vertex)) {
                for (const auto& v2 : graph.at(vertex)) {
                    elements.push_back(v2.first);
                }
            }
        }
    }
    return visited;
}

TSet<TString> GetIntermediateLayers(TModel& model, const TString& target, const TVector<TString>& from) {
    TModelGraph graph, backGraph;
    for (size_t i = 0; i < model.Layers.size(); ++i) {
        const auto& layer = model.Layers[i];
        for (const TString& input : layer->GetInputs()) {
            for (const TString& output : layer->GetOutputs()) {
                graph[input].emplace_back(output, i);
                backGraph[output].emplace_back(input, i);
            }
        }
    }
    TSet<TString> usedForward = LayersDfs(graph, from);
    TSet<TString> usedBackward = LayersDfs(backGraph, {target});
    TSet<TString> result;
    std::set_intersection(usedForward.begin(), usedForward.end(), usedBackward.begin(), usedBackward.end(), std::inserter(result, result.begin()));
    return result;
}

TString GetSparseOutPositionsName(const TString& output) {
    return output + "_positions";
}

void AddBackProp(TModel& model, const TString& target, const TVector<TString>& from, const TWeightsJoinOptions& joinOptions) {
    if (model.Parameters.has(ZERO_BIAS_INPUT)) {
        TMatrix* matrix = VerifyDynamicCast<TMatrix*>(model.Parameters.at(ZERO_BIAS_INPUT).Get());
        Y_VERIFY(matrix->GetNumRows() == 1);
        Y_VERIFY(matrix->GetNumColumns() == 1);
        Y_VERIFY((*matrix)[0][0] == 0.0);
    } else {
        model.Parameters[ZERO_BIAS_INPUT] = new TMatrix(1, 1, 0.0);
    }

    TSet<TString> neededParams = GetIntermediateLayers(model, target, from);

    TSet<TString> fromNotCovered(from.begin(), from.end());
    TSet<TString> fromCovered;
    THashMap<TString, TVector<TString>> sparsifiersByInput;
    for (auto& layer : model.Layers) {
        if (layer->GetTypeName() == "TStringToSparseMatrixLayer" || layer->GetTypeName() == "TStringToSparseMatrixAndPosLayer") {
            if (fromNotCovered.contains(layer->GetInputs()[0]) && neededParams.contains(layer->GetOutputs()[0])) {
                layer.Reset(new TStringToSparseMatrixAndPosLayer(layer->GetInputs()[0], layer->GetInputs()[1], layer->GetOutputs()[0], GetSparseOutPositionsName(layer->GetOutputs()[0])));
                fromCovered.insert(layer->GetInputs()[0]);
                sparsifiersByInput[layer->GetInputs()[0]].push_back(layer->GetOutputs()[0]);
            }
        }
    }
    if (fromNotCovered != fromCovered) {
        for (const auto& s : fromCovered) {
            fromNotCovered.erase(s);
        }
        ythrow yexception() << "inputs not covered: " << JoinStrings(TVector<TString>(fromNotCovered.begin(), fromNotCovered.end()), ", ") << " for " << target;
    }

    THashMap<TString, size_t> backInputs;

    TVector<ILayerPtr> gradLayers;

    gradLayers.push_back(new TElementwiseTransform<TConst1>(target, GetGradDataName(target, target)));
    backInputs[GetGradDataName(target, target)] = 1;

    for (auto layerRaw = model.Layers.rbegin(); layerRaw != model.Layers.rend(); ++layerRaw) {
        const auto& inputs = layerRaw->Get()->GetInputs();
        const auto& outputs = layerRaw->Get()->GetOutputs();
        const TString& typeName = layerRaw->Get()->GetTypeName();
        TString gradOutput = GetGradDataName(target, outputs[0]);
        TString gradInput = GetGradDataName(target, inputs[0]);
        if (typeName == "TDotLayer") {
            if (!neededParams.contains(outputs[0])) {
                continue;
            }
            TString gradOutput = GetGradDataName(target, outputs[0]);
            for (size_t i = 0; i < 2; ++i) {
                if (!neededParams.contains(inputs[i])) {
                    continue;
                }
                TString gradInput = GetGradDataName(target, inputs[i]);
                if (++backInputs[gradInput] > 1) {
                    ythrow yexception() << "gradient as " << gradInput << " has too many outputs (not supported yet)";
                }
                auto gradLayer = new THadamarlikeProductLayer(inputs[1 - i], gradOutput, gradInput);
                gradLayers.push_back(gradLayer);
            }
        } else if (typeName == "TConcatLayer") {
            if (!neededParams.contains(outputs[0])) {
                continue;
            }
            TString gradOutput = GetGradDataName(target, outputs[0]);
            TVector<TString> gradInputs;
            gradInputs.reserve(inputs.size());
            for (const TString& s : inputs) {
                TString gradInput = GetGradDataName(target, s);
                if (++backInputs[gradInput] > 1) {
                    ythrow yexception() << "gradient as " << gradInput << " has too many outputs (not supported yet)";
                }
                gradInputs.push_back(gradInput);
            }
            auto gradLayer = new TSplitLayer(gradOutput, inputs, gradInputs);
            gradLayers.push_back(gradLayer);
        } else {//one input - one output layer
            if (inputs.empty() || outputs.empty() || !neededParams.contains(outputs[0]) || !neededParams.contains(inputs[0])) {
                continue;
            }
            if (typeName == "TStringToSparseMatrixAndPosLayer") {//sparsifier gradients will be added and joined later
                continue;
                /*auto gradLayer = new TJoinSparseGradsLayer(gradOutput, outputs[1], gradInput, joinOptions);
                gradLayers.push_back(gradLayer);*/
            }
            if (++backInputs[gradInput] > 1) {
                ythrow yexception() << "gradient as " << gradInput << " has too many outputs (not supported yet)";
            }
            if (typeName == "TScaleLayer") {
                auto gradLayer = new TScaleLayer(gradOutput, inputs[1], ZERO_BIAS_INPUT, gradInput);
                gradLayers.push_back(gradLayer);
            } else if (typeName == "TMulRowLayer") {
                auto gradLayer = new TMulRowLayer(gradOutput, inputs[1], gradInput);
                gradLayers.push_back(gradLayer);
            } else if (typeName == "TShallowCopyLayer") {
                auto gradLayer = new TShallowCopyLayer(gradOutput, gradInput);
                gradLayers.push_back(gradLayer);
            } else if (typeName == "TNormalizeRowsLayer") {
                TNormalizeRowsLayer* layer = static_cast<TNormalizeRowsLayer*>(layerRaw->Get());
                auto gradLayer = new TNormalizeRowsGradientLayer(inputs[0], outputs[0], gradOutput, gradInput, layer->GetNormBase());
                gradLayers.push_back(gradLayer);
            } else if (typeName == "TAffineLayer") {
                auto gradLayer = new TLinearTransposedLayer(gradOutput, inputs[1], gradInput);
                gradLayers.push_back(gradLayer);
            } else if (typeName == "TElementwiseTransform<rlu>") {
                auto gradLayer = new TElementwiseGradientLayer<TRlu>(inputs[0], outputs[0], gradOutput, gradInput);
                gradLayers.push_back(gradLayer);
            } else if (typeName == "TAddRowLayer") {
                auto gradLayer = new TShallowCopyLayer(gradOutput, gradInput);
                gradLayers.push_back(gradLayer);
            } else if (typeName == "TLayerNorm2Fixed") {
                auto gradLayer = new TNorm2GradientLayer(inputs[0], gradOutput, gradInput, layerRaw->Get()->GetName() == "TLayerNorm2<buggy>");
                gradLayers.push_back(gradLayer);
            } else if (typeName == "TSparseMatrixToEmbeddingLayer") {
                auto gradLayer = new TSparseMatrixToEmbeddingGradientLayer(gradOutput, inputs[0], inputs[1], gradInput);
                gradLayers.push_back(gradLayer);
            } else {
                ythrow yexception()
                    << "no back prop for " << layerRaw->Get()->GetTypeName() << " from " << JoinStrings(outputs, ", ") << " to " << JoinStrings(inputs, ", ") << Endl;
            }
        }
    }
    for (const auto& input : sparsifiersByInput) {
        TVector<TString> sparseGradsOutputs;
        TVector<TString> sparsePositions;
        for (const TString& sparsifierOut : input.second) {
            sparseGradsOutputs.push_back(GetGradDataName(target, sparsifierOut));
            sparsePositions.push_back(GetSparseOutPositionsName(sparsifierOut));
        }
        TString joinedGrads = JoinStrings(sparseGradsOutputs, "$") + "_joined";
        TString joinedPositions = JoinStrings(sparsePositions, "$") + "_joined";
        gradLayers.push_back(
            new TJoinMultiplySparsesAndPositionsLayer(sparseGradsOutputs, sparsePositions, joinedGrads, joinedPositions)
        );
        gradLayers.push_back(
            new TJoinSparseGradsLayer(joinedGrads, joinedPositions, GetGradDataName(target, input.first), joinOptions)
        );
    }
    model.Layers.insert(model.Layers.end(), gradLayers.begin(), gradLayers.end());
}

void ApplyModelModifiedValue(TModel& model, TEvalContext& context, const TString& modifiedValue, size_t index, float value) {
    for (auto& layer : model.Layers) {
        layer->Apply(context);
        if (!layer->GetOutputs().empty() && layer->GetOutputs()[0] == modifiedValue) {
            TSparseMatrix& matrix = *VerifyDynamicCast<TSparseMatrix*>(context.at(modifiedValue).Get());
            for (auto& row : matrix.Vectors) {
                bool modified = false;
                for (size_t i = 0; i < row.Indexes.size(); ++i) {
                    if (row.Indexes[i] == index) {
                        row.Values[i] += value;
                        modified = true;
                        break;
                    }
                }
                if (!modified) {
                    row.Indexes.push_back(index);
                    row.Values.push_back(value);
                }
            }
        }
    }
}

bool CheckGrads(TModel& model, const TString& target, const TString& from, const TVector<TSample>& samples) {
    const auto& intermediateLayers = GetIntermediateLayers(model, target, {from});
    struct TSparseGradFlow {
        TString InputName;
        TString GradName;
        size_t Layer;
    };
    /*TString sparseInputName;
    TString sparseGradName;
    size_t sparsifierLayer = -1;*/
    TVector<TSparseGradFlow> flows;
    for (size_t i = 0; i < model.Layers.size(); ++i) {
        const auto& layer = *model.Layers[i];
        if (layer.GetTypeName() == "TStringToSparseMatrixAndPosLayer" && layer.GetInputs()[0] == from && intermediateLayers.contains(layer.GetOutputs()[0])) {
            flows.emplace_back(TSparseGradFlow{layer.GetOutputs()[0], GetGradDataName(target, layer.GetOutputs()[0]), i});
        }
    }
    if (flows.empty()) {
        return false;
    }

    TEvalContext context;
    context["input"].Reset(new TSamplesVector());

    TSamplesVector* samplesVector = VerifyDynamicCast<TSamplesVector*>(
        context.at("input").Get());
    TVector<TAtomicSharedPtr<ISample>> samplesP(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        samplesP[i].Reset(new TSample(samples[i]));
    }
    samplesVector->SetSamples(samplesP);

    for (const auto& flow : flows) {
        const TString& sparseGradName = flow.GradName;
        const TString& sparseInputName = flow.InputName;
        for (size_t sample = 0; sample < samples.size(); ++sample) {
            for (auto& layer : model.Layers) {
                layer->Apply(context);
            }

            TSparseVector grads = VerifyDynamicCast<TSparseMatrix*>(context.at(sparseGradName).Get())->Vectors[sample];
            TSparseVector sparseInput = VerifyDynamicCast<TSparseMatrix*>(context.at(sparseInputName).Get())->Vectors[sample];
            Y_VERIFY(grads.Indexes.size() == sparseInput.Indexes.size());
            const float STEP = 1.0 / 256;

            for (size_t i = 0; i < grads.Indexes.size(); ++i) {
                ApplyModelModifiedValue(model, context, sparseInputName, grads.Indexes[i], -STEP);
                float value_m1 = (*VerifyDynamicCast<TMatrix*>(context.at(target).Get()))[sample][0];
                ApplyModelModifiedValue(model, context, sparseInputName, grads.Indexes[i], STEP);
                float value_1 = (*VerifyDynamicCast<TMatrix*>(context.at(target).Get()))[sample][0];

                float derivative = (value_1 - value_m1) / 2.0 / STEP;
                float res = fabs(derivative - grads.Values[i]);

                if (res > 0.01 * sqrt(derivative * derivative + grads.Values[i] * grads.Values[i]) && res > 0.01) {
                    return false;
                }
            }
        }
    }
    return true;
}
