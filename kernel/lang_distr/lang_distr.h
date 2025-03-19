#pragma once
#include <library/cpp/langmask/langmask.h>
#include <util/generic/vector.h>
#include <library/cpp/langs/langs.h>
#include <util/generic/yexception.h>
#include <util/system/defaults.h>

//Namespace which contain class and functions using to operate with language distribution data
namespace NLangDistr {

    const size_t BITS_FOR_LANGUAGE = 4;
    const size_t BITS_FOR_PROBABILITY = 8 - BITS_FOR_LANGUAGE;

    //accuracy step will be (1 / PROBABILITY_ACCURACY)
    const size_t PROBABILITY_ACCURACY = (1 << BITS_FOR_PROBABILITY) - 1;

    // Class used to remap normal document language into number from range 0-15.
    // This remapped language id is going to be used in stored language distribution data
    class TDocLangRemap {
    private:
        TVector<int> RemapTable;
    public:
        TDocLangRemap()
            : RemapTable(LANG_MAX, -1) {

            ELanguage allowedLang[15] = {
                LANG_RUS,
                LANG_ENG,
                LANG_UKR,
                LANG_KAZ,
                LANG_BEL,
                LANG_SPA,
                LANG_ITA,
                LANG_BUL,
                LANG_FRE,
                LANG_RUM,
                LANG_TUR,
                LANG_GER,
                LANG_POL,
                LANG_LIT,
                LANG_TAT
            };

            for (size_t i = 0; i < Y_ARRAY_SIZE(allowedLang); ++i) {
                RemapTable[allowedLang[i]] = i;
            }
        }

        //return 0-15 for languages which is stored in language distribution data
        //return negative value otherwise
        int operator() (const ELanguage& docLang) const {
            return RemapTable[docLang];
        }
    };

    ui32 EncodeProbability(float value);
    float DecodeProbability(ui32 value);
    std::pair<ui8, float> GetInfo(ui32 value, ui32 index);

    ui32 GetPreferredLanguageDistribution(ui32 preferredValue, ui32 otherValue);

    float GetLanguageDistribution(const TLangMask& queryLangMask, ui32 value);
}
