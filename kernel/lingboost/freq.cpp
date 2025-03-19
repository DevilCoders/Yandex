#include "freq.h"

#include <ysite/yandex/pure/constants.h>

namespace NLingBoost {
    float RevFreqToFreq(i64 reverseFreq) {
        if (!IsValidRevFreq(reverseFreq)) {
            return InvalidFreq;
        }

        if (reverseFreq >= RevFreqCutoff) {
            return FreqCutoff;
        }

        return 1.0f / float(reverseFreq);
    }

    i64 FreqToRevFreq(float freq) {
        if (!IsValidFreq(freq)) {
            return InvalidRevFreq;
        }

        if (freq <= FreqCutoff) {
            return RevFreqCutoff;
        }

        return i64(round(1.0f / freq));
    }
}

