#pragma once

#include <random>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

namespace NExtendedMx {
    class TRandom {
    public:
        TRandom(const TString& seed);
        ui32 NextInt(const ui32 max);

        template <typename TReal>
        TReal NextReal(const TReal max) {
            static_assert(std::is_floating_point<TReal>::value, "should be floating point type");
            Y_ENSURE(max > 0, "should be positive upper bound");
            std::uniform_real_distribution<TReal> dis(0, max);
            return dis(Gen);
        }

        ui32 Choice(const TVector<double>& partSums);

    private:
        std::mt19937 Gen;
    };
}
