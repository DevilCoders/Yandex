#pragma once

#include <kernel/facts/features_calculator/calculator_data.h>
#include <kernel/facts/features_calculator/calculator_config.h>
#include <kernel/matrixnet/mn_dynamic.h>

namespace NFactClassifiers {
    struct TClassifierData {
        TClassifierData(const NUnstructuredFeatures::TConfig& config);

        const float Threshold;
        const size_t TopKNNCandidates;
        THolder<NMatrixnet::TMnSseDynamic> ClassifierModel;
        const NUnstructuredFeatures::TCalculatorData FeaturesData;
    };
}
