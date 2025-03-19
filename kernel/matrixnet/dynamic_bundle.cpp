#include "dynamic_bundle.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

#include <util/generic/strbuf.h>

namespace {
    const TString ComponentsField = "components";
    const TString TreeIndexFromField = "treeIndexFrom";
    const TString TreeIndexToField  = "treeIndexTo";
    const TString FeatureIndexField = "featureIndex";
    const TString SliceField = "slice";
    const TString IndexField = "index";
    const TString BiasField = "bias";
    const TString ScaleField = "scale";
    const TString FormulaIdField = "formulaId";
}

namespace NMatrixnet {

bool TDynamicBundleComponent::operator==(const TDynamicBundleComponent& other) const {
    return
        TreeIndexFrom == other.TreeIndexFrom &&
        TreeIndexTo == other.TreeIndexTo &&
        FeatureIndex.Slice == other.FeatureIndex.Slice &&
        FeatureIndex.Index == other.FeatureIndex.Index &&
        Bias == other.Bias &&
        Scale == other.Scale &&
        FormulaId == other.FormulaId;
}

void TDynamicBundle::Load(IInputStream* in) {
    auto json = NJson::ReadJsonTree(in);
    const auto& components = json[ComponentsField].GetArray();
    for (const auto& component : components) {
        const auto& index = component[FeatureIndexField];
        Components.push_back({
            static_cast<size_t>(component[TreeIndexFromField].GetInteger()),
            static_cast<size_t>(component[TreeIndexToField].GetInteger()),
            {
                FromString<NFactorSlices::EFactorSlice>(index[SliceField].GetString()),
                static_cast<NFactorSlices::TFactorIndex>(index[IndexField].GetInteger())
            },
            component[BiasField].GetDouble(),
            component.Has(ScaleField) ? component[ScaleField].GetDouble() : 1.0,
            component[FormulaIdField].GetString()
        });
    }
}

void TDynamicBundle::Save(IOutputStream* out) const {
    NJson::TJsonValue components;
    for (const auto& component : Components) {
        auto& componentJson = components.AppendValue(NJson::TJsonValue{});
        componentJson[TreeIndexFromField] = component.TreeIndexFrom;
        componentJson[TreeIndexToField] = component.TreeIndexTo;
        auto& indexJson = componentJson[FeatureIndexField];
        indexJson[SliceField] = ToString<NFactorSlices::EFactorSlice>(component.FeatureIndex.Slice);
        indexJson[IndexField] = component.FeatureIndex.Index;
        componentJson[BiasField] = component.Bias;
        componentJson[FormulaIdField] = component.FormulaId;
        componentJson[ScaleField] = component.Scale;
    }
    NJson::TJsonValue json;
    json[ComponentsField] = components;
    NJson::WriteJson(out, &json);
}

}
