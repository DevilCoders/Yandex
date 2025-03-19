#include "factors.h"

namespace NNeuralNet {
    void SelectFactorsFromStorage(
        const TMultiConstFactorView& view,
        const TFeaturesConfig& config,
        TArrayRef<float> output,
        TSet<TFullFactorIndex>* clampedFactors
    ) {
        size_t inputIndex = 0;
        for (const NSelectFactors::NProto::TFactorsConfig& inputFactorsInfo : config) {
            auto slice = FromString<NFactorSlices::EFactorSlice>(inputFactorsInfo.GetSlice());
            TConstFactorView sliceView = view[slice];

            for (const NSelectFactors::NProto::TFactorConfig& factorInfo : inputFactorsInfo.GetFactors()) {
                if (factorInfo.HasConstValue()) {
                    output[inputIndex] = factorInfo.GetConstValue();
                    ++inputIndex;
                    continue;
                }

                const size_t factorIndex = factorInfo.GetIndex();
                bool isClamped = false;
                if (Y_LIKELY(factorIndex < sliceView.Size())) {
                    if (Y_UNLIKELY(sliceView[factorIndex] < factorInfo.GetMinValue())) {
                        output[inputIndex] = factorInfo.GetMinValue();
                        isClamped = true;
                    } else if (Y_UNLIKELY(sliceView[factorIndex] > factorInfo.GetMaxValue())) {
                        output[inputIndex] = factorInfo.GetMaxValue();
                        isClamped = true;
                    } else {
                        output[inputIndex] = sliceView[factorIndex];
                    }
                }
                if (isClamped && clampedFactors != nullptr) {
                    clampedFactors->emplace(slice, factorIndex);
                }
                ++inputIndex;
            }
        }
    }
}
