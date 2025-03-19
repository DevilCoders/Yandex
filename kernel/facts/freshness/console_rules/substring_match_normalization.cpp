#include "substring_match_normalization.h"

#include <util/charset/wide.h>
#include <util/string/subst.h>

#include <util/generic/string.h>

namespace NFacts {

    TString NormalizeForSubstringFilter(const TString& text) {
        TUtf16String normalized = ToLowerRet(UTF8ToWide(text));
        SubstGlobal(normalized, u'ё', u'е');
        return WideToUTF8(normalized);
    }

}
