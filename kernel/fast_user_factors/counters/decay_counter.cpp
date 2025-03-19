#include "decay_counter.h"

namespace NFastUserFactors {

    float MoveCounter(
        const float value,
        const ui64 srcTs,
        const ui64 dstTs,
        const float decayDays
    ) noexcept {
        TDecayCounter counter(decayDays);

        counter.Add(srcTs, value);
        if (dstTs > srcTs) {
            counter.Move(dstTs);
        }

        return counter.Accumulate();
    }

}
