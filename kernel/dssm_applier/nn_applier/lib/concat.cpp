#include "concat.h"


namespace NNeuralNetApplier {
    void AddSuffix(TModel& model, const TString& suffix, const TVector<TString>& keep) {
        TVector<TString> allVariables = model.AllVariables();
        for (const TString& var : allVariables) {
            if (var != RequiredInputName && Find(keep.begin(), keep.end(), var) == keep.end()) {
                model.RenameVariable(var, var + suffix);
            }
        }

        TStringStream stringStream;
        model.Save(&stringStream);
        TBlob blob = TBlob::FromStream(stringStream);
        model.Load(blob);
    }

    TModelPtr MergeModels(const TVector<TModel>& models, bool ignoreVersionsDiff) {
        Y_ENSURE(!models.empty(), "merging empty list of models is unsupported operation");
        const TVersionRange& supportedVersions = models.front().GetSupportedVersions();
        if (!ignoreVersionsDiff) {
            for (size_t i = 1; i < models.size(); ++i) {
                Y_ENSURE(models[i].GetSupportedVersions() == supportedVersions, "Trying to merge models which support different versions");
            }
        }

        TModelPtr result = new TModel();
        result->SetSupportedVersions(supportedVersions);

        for (const TModel& other : models) {
            for (const TString& var : other.Parameters.GetVariables()) {
                if (result->Parameters.has(var)) {
                    if (result->Parameters.at(var)->GetTypeName() != other.Parameters.at(var)->GetTypeName()) {
                        throw yexception() << "Incompatible types: " << result->Parameters.at(var)->GetTypeName()
                            << " and " << other.Parameters.at(var)->GetTypeName();
                    }
                    continue;
                }
                result->Parameters[var] = other.Parameters.at(var);
            }
            for (const TString& input : other.Inputs) {
                if (result->Parameters.has(input)) {
                    result->Parameters[input] = CreateState(result->Parameters.at(input)->GetTypeName());
                }
            }

            for (auto& layer : other.Layers) {
                TVector<TString> inputs = layer->GetInputs();
                TVector<TString> outputs = layer->GetOutputs();
                bool haveIdenticalLayer = false;
                for (ILayerPtr& curLayer : result->Layers) {
                    TVector<TString> curInputs = curLayer->GetInputs();
                    TVector<TString> curOutputs = curLayer->GetOutputs();
                    if (inputs == curInputs && outputs == curOutputs) {
                        haveIdenticalLayer = true;
                        break;
                    }
                }
                if (haveIdenticalLayer) {
                    continue;
                }
                result->Layers.push_back(layer->Clone());
            }

            result->Inputs.insert(result->Inputs.end(), other.Inputs.begin(), other.Inputs.end());
            Sort(result->Inputs.begin(), result->Inputs.end());
            result->Inputs.erase(Unique(result->Inputs.begin(), result->Inputs.end()), result->Inputs.end());
            result->Init();
        }

        return result;
    }

}
