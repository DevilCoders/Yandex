#pragma once

#include "layers.h"


namespace NNeuralNetApplier {

    void ApplyModel(const TModel& model, const TString& dataset, const TVector<TString>& headerFields,
        const TVector<TString>& outputVariables, TVector<TVector<float>>& result, const ui32 batchSize = 1);

    void ApplyModel(const TString& modelFile, const TString& dataset, const TVector<TString>& headerFields,
        const TVector<TString>& outputVariables, TVector<TVector<float>>& result, const ui32 batchSize = 1);

} // namespace NNeuralNetApplier
