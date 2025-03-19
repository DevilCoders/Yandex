#pragma once

#include <util/generic/strbuf.h>  // for TStringBuf
#include <util/system/defaults.h> // for ui8

// url without scheme and with end slash if present
ui8 CalcUrlLen(const TStringBuf url);
