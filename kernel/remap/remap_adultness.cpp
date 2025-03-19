#include "remap_adultness.h"

#include <math.h>
#include <kernel/factor_storage/factor_storage.h>

ui8 AdultnessToErfVal(float adultness, ui32 version) {
    Y_ASSERT(adultness >= 0.0f);
    float floatErfVal;
    if (0 == version)
        floatErfVal = adultness;
    else floatErfVal = 255.0f * logf(adultness + 1) / logf(255.0f + 1);

    return (ui8)ClampVal(floatErfVal, 0.0f, 255.0f);
}

float ErfValToAdultnessFactor(ui8 erfVal, ui32 version) {
    if (0 == version)
        return Ui82Float(erfVal);
    return (expf(Ui82Float(erfVal) * logf(255.0f + 1)) - 1) / 255.0f;
}

TAdultnessFactorCache::TAdultnessFactorCache() {
    resize(2);
    for (ui32 version = 0; version < 2; ++version)
        for (size_t erfVal = 0; erfVal <= 255; ++erfVal)
            (*this)[version].push_back(::ErfValToAdultnessFactor((ui8)erfVal, version));
}
