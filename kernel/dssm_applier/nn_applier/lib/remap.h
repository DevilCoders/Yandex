#pragma once

#include "apply.h"


namespace NNeuralNetApplier {

    TModelPtr DoRemap(const TModel& fromModel, const TModel& toModel, const TString& dataset, const TString& fromOutput,
        const TString& toOutput, const TVector<TString>& fromHeaderFields, const TVector<TString>& toHeaderFields,
        const ui64 numPoints);

} // namespace NNeuralNetApplier
