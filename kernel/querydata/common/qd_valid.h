#pragma once

#include <util/generic/fwd.h>

namespace NQueryData {
    bool NamespaceIsValid(TStringBuf nspace);
    bool TimestampMicrosecondsIsValid(ui64 tstamp);

    TString FixSourceNameForSaaS(TStringBuf srcName);
}

