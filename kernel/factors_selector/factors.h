#pragma once

#include <kernel/factors_selector/proto/factors_config.pb.h>
#include <kernel/factor_storage/factor_view.h>

namespace NNeuralNet {
    using TFeaturesConfig = google::protobuf::RepeatedPtrField<NSelectFactors::NProto::TFactorsConfig>;

    void SelectFactorsFromStorage(
        const TMultiConstFactorView& view,
        const TFeaturesConfig& config,
        TArrayRef<float> output,
        TSet<TFullFactorIndex>* clampedFactors = nullptr
    );
}
