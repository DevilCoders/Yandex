#include "qd_valid.h"

#include "qd_source_names.h"

#include <util/string/util.h>

namespace NQueryData {
    bool NamespaceIsValid(TStringBuf nspace) {
        static str_spn allowed{"a-zA-Z0-9_:-", true};
        return nspace && allowed.cbrk(nspace.begin(), nspace.end()) == nspace.end();
    }

    bool TimestampMicrosecondsIsValid(ui64 tstamp) {
        return tstamp > 1'000'000'000'000'000ull && tstamp < 4'000'000'000'000'000ull;
    }

    TString FixSourceNameForSaaS(TStringBuf srcName) {
        TStringBuf mainSrc, auxSrc;
        mainSrc = GetMainSourceName(srcName, auxSrc);
        return ComposeSourceName(mainSrc, auxSrc);
    }
}

