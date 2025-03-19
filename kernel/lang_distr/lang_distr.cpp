#include "lang_distr.h"
#include <util/generic/utility.h>
#include <util/generic/ymath.h>

namespace NLangDistr {

    static const TLangMask LanguageMasks[15] = {
        TLangMask(LANG_RUS),
        TLangMask(LANG_ENG),
        TLangMask(LANG_UKR),
        TLangMask(LANG_KAZ),
        TLangMask(LANG_BEL),
        TLangMask(LANG_SPA),
        TLangMask(LANG_ITA),
        TLangMask(LANG_BUL),
        TLangMask(LANG_FRE),
        TLangMask(LANG_RUM),
        TLangMask(LANG_TUR),
        TLangMask(LANG_GER),
        TLangMask(LANG_POL),
        TLangMask(LANG_LIT),
        TLangMask(LANG_TAT)
    };

    ui32 EncodeProbability(float value) {
        return ClampVal<ui32>(value * PROBABILITY_ACCURACY, 0, PROBABILITY_ACCURACY);
    }

    float DecodeProbability(ui32 value) {
        return ClampVal<float>(static_cast<float>(value) / PROBABILITY_ACCURACY, 0.0, 1.0);
    }

    std::pair<ui8, float> GetInfo(ui32 value, ui32 index) {
        Y_ASSERT(index < 4);
        value = (value >> (index * 8)) & 0xFF;
        return std::make_pair(value >> BITS_FOR_PROBABILITY, DecodeProbability(value & PROBABILITY_ACCURACY));
    }

    void LanguagesFilled(ui32 value, size_t& nLangs, float& sumProp) {
        nLangs = 0;
        sumProp = 0.0f;
        for (size_t i = 0; i < 4; ++i) {
            ui8 langInfo = value & 0xFF;
            float probability = DecodeProbability(langInfo & PROBABILITY_ACCURACY);
            sumProp += probability;
            if (probability > 0) {
                ++nLangs;
            }
            value >>= 8;
        }
    }

    ui32 GetPreferredLanguageDistribution(ui32 preferredValue, ui32 otherValue) {
        if (preferredValue == otherValue)
            return preferredValue;

        static const float eps = DecodeProbability(1) / 2; // TODO: make everything up to here constexpr when it's available

        size_t preferredFilledLanguages, otherFilledLanguages;
        float preferredTotalProbability, otherTotalProbability;
        LanguagesFilled(preferredValue, preferredFilledLanguages, preferredTotalProbability);
        LanguagesFilled(otherValue, otherFilledLanguages, otherTotalProbability);
        if ( (otherFilledLanguages > preferredFilledLanguages)
            || (otherFilledLanguages == preferredFilledLanguages && otherTotalProbability - preferredTotalProbability > eps))
        {
            return otherValue;
        } else if (otherFilledLanguages == preferredFilledLanguages && Abs(otherTotalProbability - preferredTotalProbability) < eps) {
            return Max(preferredValue, otherValue);
        }
        return preferredValue;
    }

    float GetLanguageDistribution(const TLangMask& queryLangMask, ui32 value) {
        float sum = 0.0;
        float maxP = 0.0;
        for (size_t i = 0; i < 4; ++i) {
            ui8 langInfo = value & 0xFF;
            ui8 language = (value >> 4) & 0xF;
            if (LanguageMasks[language].HasAny(queryLangMask)) {
                float probability = DecodeProbability(langInfo & PROBABILITY_ACCURACY);
                sum += probability;
                maxP = Max(maxP, probability);
            }
            value >>= 8;
        }

        return maxP / Max(sum, 0.1f);
    }
};
