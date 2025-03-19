#pragma once

#include <util/generic/strbuf.h>
#include <util/system/defaults.h>

namespace NReMorph {

enum EColorCode {
    CC_BLACK = 0,
    CC_RED,
    CC_GREEN,
    CC_YELLOW,
    CC_BLUE,
    CC_MAGENTA,
    CC_CYAN,
    CC_WHITE
};

TStringBuf AnsiSgrColor(EColorCode color, bool bold = false);
TStringBuf AnsiSgrColorReset();

} // NReMorph
