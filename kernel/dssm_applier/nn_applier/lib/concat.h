#pragma once

#include "layers.h"


namespace NNeuralNetApplier {
    static constexpr TStringBuf RequiredInputName = "batch";

    void AddSuffix(TModel& model, const TString& suffix, const TVector<TString>& keep);

    TModelPtr MergeModels(const TVector<TModel>& models, bool ignoreVersionsDiff = false);

} // namespace NNeuralNetApplier
