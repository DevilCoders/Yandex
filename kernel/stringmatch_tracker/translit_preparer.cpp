#include "translit_preparer.h"
#include <ysite/yandex/reqanalysis/legacy_normalize.h>
#include <kernel/translit/translit.h>

namespace NStringMatchTracker {

    TString TTranslitPreparer::Prepare(const TTextWithDescription& descr) const {
        TString preparedStr = NormalizeRequestUTF8(descr.GetText());
        return TransliterateBySymbol(preparedStr, descr.GetLang());
    }

}
