#pragma once

#include <util/generic/string.h>

namespace NSnippets {
    // Encode & escape an UTF-8 string so it can be used in XML 1.0 text and attribute nodes.
    // Characters not allowed in XML 1.0 are replaced with U+FFFD. Entities are used when necessary.
    TString EncodeTextForXml10(const TString& str, bool needEscapeTags = true);
}
