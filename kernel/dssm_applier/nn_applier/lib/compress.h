#pragma once

#include "apply.h"


namespace NNeuralNetApplier {
    struct TCompressionConfig {
        TVector<TString> Variables;
        TString TargetVariable;
        double GridStep = 0;
        TString Dataset;
        size_t NumBins = 0;
        size_t NumBits = 0;
        TVector<TString> HeaderFields;
        bool Verbose = false;
        THashMap<TString, float> SmallValuesToZeroSettings;
        size_t ThreadsCount = 1;
        bool GetOptimizedConfigs = false;
    };

    std::pair<TModelPtr, THashMap<TString, TString>> DoCompress(const TModel& model, const TCompressionConfig& config);

} // namespace NNeuralNetApplier
