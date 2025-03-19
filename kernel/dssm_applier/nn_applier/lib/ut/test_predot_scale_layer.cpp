#include <kernel/dssm_applier/nn_applier/lib/layers.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>

#include <util/folder/path.h>
#include <util/folder/dirut.h>

#include <util/stream/file.h>
#include <util/stream/str.h>

#include <util/memory/blob.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/algorithm.h>

#include <util/system/types.h>

using namespace NNeuralNetApplier;

namespace {
    const TString INPUT{"input"};
    const TString OUTPUT{"output"};
    const TString MULTIPLIER{"multiplier"};
    const TString BIAS{"bias"};

    struct TContextFixture : public NUnitTest::TBaseFixture {
        TContextFixture()
            : Context{}
            , EvalContext{&Context}
            , LayerWithoutScaling{INPUT, "", "", OUTPUT}
            , ScaleLayer{INPUT, MULTIPLIER, BIAS, OUTPUT}
        {
            Input = new TMatrix{};
            EvalContext[INPUT].Reset(Input);

            Multiplier = new TMatrix{};
            Multiplier->Resize(1, 1);
            Context[MULTIPLIER].Reset(Multiplier);

            Bias = new TMatrix{};
            Bias->Resize(1, 1);
            Context[BIAS].Reset(Bias);

            LayerWithoutScaling.Init(Context);
        }

        const TMatrix& GetOutput() {
            return *VerifyDynamicCast<TMatrix*>(EvalContext[OUTPUT].Get());
        }

        TContext Context;
        TEvalContext EvalContext;
        TMatrix* Input;
        TMatrix* Multiplier;
        TMatrix* Bias;
        TPreDotScaleLayer LayerWithoutScaling;
        TPreDotScaleLayer ScaleLayer;
    };


} // unnamed namespace

Y_UNIT_TEST_SUITE_F(PreDotScaleLayer, TContextFixture) {
    Y_UNIT_TEST(CreatesOutputMatrix) {
        LayerWithoutScaling.Apply(EvalContext);

        UNIT_ASSERT(EvalContext.has(OUTPUT));
    }
    Y_UNIT_TEST(ReturnsEmptyOnEmptyInput) {
        LayerWithoutScaling.Apply(EvalContext);

        UNIT_ASSERT(GetOutput().GetSize() == 0);
    }
    Y_UNIT_TEST(PrependsOneElementToInput) {
        Input->Resize(1, 1);

        LayerWithoutScaling.Apply(EvalContext);

        UNIT_ASSERT(GetOutput().GetSize() == Input->GetSize() + 1);
    }
    Y_UNIT_TEST(PrependsOneElementToEachRow) {
        auto rowCount = 123;
        Input->Resize(rowCount, 3);

        LayerWithoutScaling.Apply(EvalContext);

        UNIT_ASSERT(GetOutput().GetSize() == Input->GetSize() + rowCount);
    }
    Y_UNIT_TEST(ByDefaultStoresInputAtEndOfOutput) {
        Input->Resize(123, 11);
        Iota(Input->GetData(), Input->GetData() + Input->GetSize(), 1234);

        LayerWithoutScaling.Apply(EvalContext);

        for (auto i = 0u; i != Input->GetNumRows(); ++i) {
            auto* inRow = (*Input)[i];
            UNIT_ASSERT(
                Equal(inRow, inRow + Input->GetNumColumns(), GetOutput()[i] + 1));
        }
    }
    Y_UNIT_TEST(PrependsBiasValue) {
        float biasValue = 3.125;

        (*Bias)[0][0] = biasValue;
        Input->Resize(1, 123);

        ScaleLayer.Apply(EvalContext);

        UNIT_ASSERT(GetOutput()[0][0] == biasValue);
    }
    Y_UNIT_TEST(ScalesEachElement) {
        auto columnCount = 123;
        float scale = 13;

        Input->Resize(1, columnCount);
        Iota(Input->GetData(), Input->GetData() + Input->GetSize(), 77);

        (*Multiplier)[0][0] = scale;

        ScaleLayer.Apply(EvalContext);

        UNIT_ASSERT(
            Equal(Input->GetData(), Input->GetData() + Input->GetNumColumns(),
                  GetOutput().GetData() + 1,
                  [scale](float in, float res) {
                      return res == scale * in;
                  }));
    }
}

namespace {
    TVector<ILayerPtr>::iterator FindWithOutput(TVector<ILayerPtr>& layers,
                                              const TString& output) {
        auto layer = FindIf(layers, [&output](const ILayerPtr& l) {
            return l->GetOutputs().size() == 1 && l->GetOutputs()[0] == output;
        });
        UNIT_ASSERT(layer != layers.end());
        return layer;
    }

    void AddPreDotScaling(TVector<ILayerPtr>& layers, const TString& output,
                          const TString& multiplier, const TString& bias) {
        auto modifiedLayer = FindWithOutput(layers, output);
        UNIT_ASSERT(modifiedLayer + 1 != layers.end());

        TString intermediateOutput{output + "_predot_scaling"};
        (*modifiedLayer)->RenameVariable(output, intermediateOutput);
        ILayerPtr scaling{new TPreDotScaleLayer{intermediateOutput, multiplier,
                                                bias, output}};
        layers.insert(modifiedLayer + 1, scaling);
    }

    void CompareOutputs(const TString& originalFilename,
                        const TString& originalOutput,
                        const TString& modifiedFilename,
                        const TString& modifiedOutput) {
        TVector<TString> annotations{
            "query",   "region_id",        "domain_id", "goodurl_uta",
            "snippet", "stripped_goodurl", "title"};
        auto originalModel = TModel::FromFile(originalFilename);
        originalModel.Init();
        auto modifiedModel = TModel::FromFile(modifiedFilename);
        modifiedModel.Init();

        for (const auto value : {"value", "another_value", "onemore"}) {
            TVector<TString> values{annotations.size(), value};
            auto sample = MakeAtomicShared<TSample>(annotations, values);
            TVector<float> originalResult;
            originalModel.Apply(sample, {originalOutput}, originalResult);
            UNIT_ASSERT(originalResult.size() == 1);

            TVector<float> modifiedResult;
            modifiedModel.Apply(sample, {modifiedOutput}, modifiedResult);
            UNIT_ASSERT(modifiedResult.size() == 1);

            UNIT_ASSERT_DOUBLES_EQUAL(originalResult[0], modifiedResult[0], 0.001);
        }
    }
}

Y_UNIT_TEST_SUITE(MoveScaleBeforeDot) {
    Y_UNIT_TEST(AssessorModel) {
        TString originalFilename{"assessor.appl"};
        TString modifiedFilename{"assessor.appl.scale_before_dot"};

        TString bannerOutput = "$doc_embedder$before_quant";
        TString queryOutput = "query_embedding";
        TString dotOutput = "joint_afterdot";
        TString scaleOutput = "joint_output";
        TString finalOutput = "output";

        auto model = TModel::FromFile(originalFilename);
        // Cerr << model.ModelGraphString() << Endl;

        AddPreDotScaling(model.Layers, bannerOutput, "", "");

        auto outerScaler = FindWithOutput(model.Layers, scaleOutput);
        auto scalerInputs = (*outerScaler)->GetInputs();
        UNIT_ASSERT(scalerInputs.size() == 3);
        TString multiplier = scalerInputs[1];
        TString bias = scalerInputs[2];
        AddPreDotScaling(model.Layers, queryOutput, multiplier, bias);

        auto dotLayer = FindWithOutput(model.Layers, dotOutput);
        (*dotLayer)->RenameVariable(dotOutput, finalOutput);
        model.Layers.erase(dotLayer + 1, model.Layers.end());
        // Cerr << model.ModelGraphString() << Endl;

        // save model
        {
            TFileOutput outputFile(modifiedFilename);
            model.Save(&outputFile);
        }

        CompareOutputs(originalFilename, scaleOutput, modifiedFilename, finalOutput);
    }
}
