#pragma once
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NUtil {
    TString ToCamelCase(const TStringBuf sb);
    TString ToCamelCase(const TString& s);
}
