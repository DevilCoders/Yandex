#include <kernel/extended_mx_calcer/interface/common.h>
#include <kernel/dssm_applier/nn_applier/lib/states.h>
#include <kernel/dssm_applier/nn_applier/lib/layers.h>

#include "bundle.h"

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/string_utils/base64/base64.h>


using namespace NExtendedMx;

class TNeuralNet : public TBundleBase<TNeuralNetConstProto> {
public:
    TNeuralNet(const NSc::TValue& scheme)
            : TBundleBase(scheme) {}
    void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const formulasStoragePtr) override {
        Y_UNUSED(formulasStoragePtr);
        Model.Load(TBlob::FromString(Base64Decode(TString(Scheme().Model().Bin().AsString()))));
        NumFeats = Scheme().NumFeats();
        Output = Scheme().Output();
    }

    double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override {
        EnsureFeatureCount(featuresCount);
        context.DbgLog() << "TNeuralNet calculation started\n";
        TVector<TString> inputsValues, inputsNames;
        TVector<TIntrusivePtr<NNeuralNetApplier::TMatrix>> variables;
        for (const auto& inputInfo : Scheme().InputsInfo()) {
            auto featuresData = GetFeaturesSubvector(features, inputInfo);
            inputsValues.push_back(JoinSeq(" ", std::move(featuresData)));
            inputsNames.push_back(TString(inputInfo.Name().Get()));
        }

        context.DbgLog() << "TNeuralNet set input features\n";
        NNeuralNetApplier::TEvalContext modelContext;
        if (!inputsNames.empty()) {
            TAtomicSharedPtr<NNeuralNetApplier::TSample> sample = new NNeuralNetApplier::TSample(inputsNames, inputsValues);
            Model.FillEvalContextFromSample(sample, modelContext);
        }
        for (const auto& variableInfo : Scheme().VariablesInfo()) {
            auto featuresData = GetFeaturesSubvector(features, variableInfo);
            const size_t featuresSize = featuresData.size();
            modelContext[TString(variableInfo.Name())] = new NNeuralNetApplier::TMatrix(1, featuresSize, std::move(featuresData));
        }
        context.DbgLog() << "TNeuralNet set input variables\n";
        TVector<float> result;
        Model.Apply(modelContext, {Output}, result);
        context.DbgLog() << "TNeuralNet calculated model\n";
        context.GetResult().Embed().Assign(result);
        return 0;
    }

    size_t GetNumFeats() const override {
        return NumFeats;
    }

private:
    NNeuralNetApplier::TModel Model;
    size_t NumFeats = 0;
    TString Output;

    TVector<float> GetFeaturesSubvector(const float* features, const TNeuralNetInputInfoConstProto& inputInfo) const {
        Y_ENSURE(inputInfo.EndPos() <= NumFeats);
        return TVector<float>(&features[inputInfo.BeginPos()], &features[inputInfo.EndPos()]);
    }
};

TExtendedCalculatorRegistrator<TNeuralNet> ExtendedNeuralNetRegistrator("neural_net");
