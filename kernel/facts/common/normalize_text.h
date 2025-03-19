#pragma once

#include <util/charset/wide.h>

namespace NUnstructuredFeatures {
    TUtf16String NormalizeText(const TWtringBuf& text);
    TString NormalizeTextUTF8(const TString& text);
}
