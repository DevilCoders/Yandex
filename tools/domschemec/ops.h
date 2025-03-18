#pragma once

#include <util/generic/strbuf.h>

inline bool IsValidationOp(TStringBuf tok) {
    return
        tok == ">=" ||
        tok == "<=" ||
        tok == "!=" ||
        tok == "==" ||
        tok == ">" ||
        tok == "<";
}
