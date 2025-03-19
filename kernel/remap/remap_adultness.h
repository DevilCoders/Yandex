#pragma once

#include <util/system/defaults.h>
#include <util/generic/singleton.h>
#include <util/generic/utility.h>
#include <util/generic/vector.h>

ui8 AdultnessToErfVal(float adultness, ui32 version);

class TAdultnessFactorCache: public TVector< TVector<float> > {
public:
    TAdultnessFactorCache();

    inline float ErfValToAdultnessFactor(ui8 erfVal) const {
        return (*this)[1][erfVal];
    }
};

static inline float CachedErfValToAdultnessFactor(ui8 erfVal) {
    return Singleton<TAdultnessFactorCache>()->ErfValToAdultnessFactor(erfVal);
}
