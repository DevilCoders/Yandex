#include <kernel/dssm_applier/nn_applier/lib/layers.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>

#include <util/stream/file.h>

#include <util/memory/blob.h>

#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/generic/algorithm.h>

#include <util/system/types.h>

#include <iterator>

using namespace NNeuralNetApplier;

namespace {

    const TString MODEL_FILENAME{"dssm_sample.bin"};

    bool HasName(const TVector<TString>& strings, const TString& name) {
        auto pos = Find(strings.begin(), strings.end(), name);
        return pos != strings.end();
    }

    // free variables must be in context or has name input, free
    // variables must not be in a layer output
    void ValidateVariablesAreFree(const TModel& model, const TString& input,
                                  const TVector<TString>& variables) {
        auto contextNames = model.Parameters.GetVariables();
        for (const auto& name : variables) {
            if (name == input) {
                continue;
            }
            UNIT_ASSERT_C(HasName(contextNames, name),
                          "Free variable name is not in context,"
                          "there is no way to evaluate it");
        }
    }

    // there must be some layers, layer input must be either a free
    // variable or an output of a layer with lower index
    void ValidateLayerOrder(const TVector<ILayerPtr>& layers,
                            const TVector<TString>& freeVariables) {
        UNIT_ASSERT_C(layers.size() > 0,
                      "Layers must contain at least output layer");
        UNIT_ASSERT_C(freeVariables.size() > 0,
                      "At least input name must be free");
        auto allowedInputs =
            TSet<TString>{freeVariables.begin(), freeVariables.end()};
        auto isAllowed = [&allowedInputs](const TString& name) {
            return allowedInputs.count(name) > 0;
        };
        for (const auto& layer : layers) {
            auto inputs = layer->GetInputs();
            UNIT_ASSERT_C(AllOf(inputs.begin(), inputs.end(), isAllowed),
                          "Layer has an input that can not be evaluated");
            auto outputs = layer->GetOutputs();
            Copy(outputs.begin(), outputs.end(),
                 std::inserter(allowedInputs, allowedInputs.end()));
        }
    }

    // first layer must have just one input with specified name
    void ValidateFirstLayer(const TString& name,
                            const TVector<ILayerPtr>& layers) {
        auto inputs = layers.front()->GetInputs();
        UNIT_ASSERT_EQUAL_C(1, inputs.size(),
                            "First layer must have one input");
        UNIT_ASSERT_EQUAL_C(name, inputs.front(),
                            "First layer have unexpected input name");
    }

    // last layer must have just one output with specified name
    void ValidateLastLayer(const TString& name,
                           const TVector<ILayerPtr>& layers) {
        auto outputs = layers.back()->GetOutputs();
        UNIT_ASSERT_EQUAL_C(1, outputs.size(),
                            "Last layer must have one output");
        UNIT_ASSERT_EQUAL_C(name, outputs.front(),
                            "Last layer have unexpected output name");
    }
} // unnamed namespace

Y_UNIT_TEST_SUITE(GetOutputDependenciesTest) {
    Y_UNIT_TEST(SingleLayerModel) {
        auto annotationToOutput =
            THashMap<TString, TString>{{"URL", "url"}, {"TEXT", "text"}};
        auto inputLayer =
            MakeAtomicShared<TFieldExtractorLayer>("input", annotationToOutput);
        auto model = TModel();
        model.Layers.push_back(inputLayer);
        for (const auto& annotationOutput : annotationToOutput) {
            auto layersAndVariables =
                GetOutputDependencies(model, annotationOutput.second);
            UNIT_ASSERT_EQUAL(1, layersAndVariables.first.size());
            UNIT_ASSERT_EQUAL(inputLayer, layersAndVariables.first[0]);
            UNIT_ASSERT_EQUAL(1, layersAndVariables.second.size());
            UNIT_ASSERT_EQUAL("input", layersAndVariables.second[0]);
        }
    } // SingleLayerModel
    Y_UNIT_TEST(SampleModel) {
        auto model = NNeuralNetApplier::TModel{};
        auto blob = TBlob{};
        {
            TFileInput inputFile(MODEL_FILENAME);
            blob = TBlob::FromStream(inputFile);
            model.Load(blob);
        }
        model.Init();

        auto input = GetModelInputName(model);
        auto outputs = TVector<TString>{"doc_embedding", "query_embedding"};
        for (const auto& output : outputs) {
            auto result = GetOutputDependencies(model, output);
            UNIT_ASSERT_C(HasName(result.second, input),
                          "Input name is not among free variables");
            ValidateVariablesAreFree(model, input, result.second);
            ValidateLayerOrder(result.first, result.second);
            ValidateFirstLayer(input, result.first);
            ValidateLastLayer(output, result.first);
        }
    } // SampleModel
} // GetOutputDependenciesTest
