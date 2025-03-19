#pragma once

#include <util/generic/strbuf.h>


namespace NUta {
    TUtf16String PrepareUrl(TStringBuf url, bool cutWWW, bool cutZeroDomain, bool processWinCode);
}
