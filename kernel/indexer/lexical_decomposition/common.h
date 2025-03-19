#pragma once

#include <util/system/defaults.h>

namespace NLexicalDecomposition {
    enum EVocabularyWordType {
        VWT_ORFO = 0,
        VWT_FREQ,
        VWT_STOP,
        VWT_UNKNOWN,
    };

    const ui32 INFTY = (ui32)(-1);
    const ui32 VOCABULARY_VERSION = 1;

#pragma pack(push, 4)
    struct TWordAdditionalInfo {
        ui32 Length;
        ui32 Frequency;
        ui32 Type;

        TWordAdditionalInfo(ui32 length = 0, ui32 frequency = 0, ui32 type = VWT_UNKNOWN)
            : Length(length)
            , Frequency(frequency)
            , Type(type)
        {
        }
    };
    static_assert(12 == sizeof(TWordAdditionalInfo), "expect 12 == sizeof(TWordAdditionalInfo)");
#pragma pack(pop)

    typedef TVector<const TWordAdditionalInfo*> TWordAdditionalInfoArr;

    enum EDecompositionOptions {
        DO_ORFO = 1,       /// words returning as a result are entirely from the vocabulary
        DO_MANUAL = 2,     /// try to do the manual decomposition before the general one
        DO_GENERAL = 4,    /// do the general decompositioin
        DO_COMPLETE = 8,   /// consider full token covers only
        DO_4WORDSMAX = 16, /// decompositions with 5 or more words will be assumed incorrect

        DO_DEFAULT = DO_ORFO | DO_MANUAL | DO_GENERAL | DO_COMPLETE | DO_4WORDSMAX, /// default options
        DO_ALL = DO_ORFO | DO_MANUAL | DO_GENERAL | DO_COMPLETE | DO_4WORDSMAX,
    };

}
