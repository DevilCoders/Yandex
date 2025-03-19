#pragma once

namespace {
    constexpr i64 InvalidRevFreq = -1;
    constexpr float InvalidFreq = -1.0f;
    constexpr i64 RevFreqCutoff = i64(1) << 60;
    constexpr float FreqCutoff = 1.0f / float(RevFreqCutoff);

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
}
