#pragma once

#include "token.h"

#include <util/generic/string.h>
#include <util/generic/vector.h>

struct TSimpleTokenizer {
    static TVector<TToken> BuildTokens(const TUtf16String& text);
};
