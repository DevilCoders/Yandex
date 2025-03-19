#pragma once

#include "constants.h"
#include "enum_map.h"

#include <util/generic/vector.h>
#include <util/generic/ymath.h>
#include <util/generic/strbuf.h>

#include <array>

namespace NLingBoost {
    using TRevFreqsByType = TEnumMap<TWordFreq, i64>;
    using TIdfsByType = TEnumMap<TWordFreq, float>;

    constexpr i64 InvalidRevFreq = -1;
    constexpr float InvalidFreq = -1.0f;
    constexpr i64 RevFreqCutoff = i64(1) << 60;
    constexpr float FreqCutoff = 1.0f / float(RevFreqCutoff);

    struct TRevFreq
        : public TWordFreq
    {
        TRevFreqsByType Values;

        TRevFreq()
            : Values(InvalidRevFreq)
        {}
    };

    inline bool IsValidRevFreq(i64 reverseFreq) {
        return reverseFreq >= 1;
    }
    inline bool IsValidFreq(float freq) {
        return 0.0f < freq && freq <= 1.0f;
    }

    inline float ClipFreq(float freq) {
        return Min(1.0f, Max(FreqCutoff, freq));
    }
    inline i64 ClipRevFreq(i64 reverseFreq) {
        return Min(RevFreqCutoff, Max(i64(1), reverseFreq));
    }

    inline float CanonizeFreq(float freq) {
        return IsValidFreq(freq) ? ClipFreq(freq) : InvalidFreq;
    }
    inline i64 CanonizeRevFreq(i64 reverseFreq) {
        return IsValidRevFreq(reverseFreq)
            ? ClipRevFreq(reverseFreq)
            : InvalidRevFreq;
    }

    float RevFreqToFreq(i64 reverseFreq);
    i64 FreqToRevFreq(float freq);
}
