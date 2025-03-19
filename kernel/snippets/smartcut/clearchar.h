#pragma once

#include <util/generic/string.h>
#include <util/system/types.h>

namespace NSnippets
{
    bool IsPermittedEdgeChar(wchar16 c);
    void ClearChars(TUtf16String& s, bool allowSlash = false, bool allowBreve = false);
}
