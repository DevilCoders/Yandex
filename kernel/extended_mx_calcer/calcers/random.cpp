#include "random.h"

#include <util/digest/murmur.h>
#include <util/generic/algorithm.h>


namespace NExtendedMx {
    TRandom::TRandom(const TString& seed)
        : Gen(MurmurHash<ui32>(seed.c_str(), seed.size()))
    {
    }

    ui32 TRandom::NextInt(const ui32 max) {
        Y_ENSURE(max, "should be positive upper bound");
        std::uniform_int_distribution<ui32> dis(0, max - 1);
        return dis(Gen);
    }

    ui32 TRandom::Choice(const TVector<double>& partSums) {
        Y_ENSURE(partSums, "expected probabilities");

        const auto sum = partSums.back();
        const auto it = LowerBound(partSums.begin(), partSums.end(), NextReal(sum));
        return it - partSums.begin();
    }
}
