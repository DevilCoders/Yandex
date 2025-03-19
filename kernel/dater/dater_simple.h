#pragma once

#include "dater.h"

namespace ND2 {

struct TDater {
    // set these only if you want date coordinates
    NSegm::TPosCoords InputTokenPositions;

    TWtringBuf InputText;
    TDates     OutputDates;

    TDater(bool lookForDigits = true, bool acceptNoYear = false)
        : Ctx(lookForDigits, acceptNoYear)
    {}

    void Localize(ELanguage lang, ELanguage auxlang = LANG_UNK, ECountryType ct = CT_DMY);

    void Scan();
    void Clear();

private:
    TDaterScanContext Ctx;
};

}
