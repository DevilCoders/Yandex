#pragma once

#include <library/cpp/charset/doccodes.h>
#include <util/generic/string.h>

namespace NTokenizerVersionsInfo {
    extern const size_t Default;
};

void RemoveOperatorSymbols(TString& s);
void RemoveOperatorSymbols(TUtf16String& s);
bool FixRequest(ECharset encoding, const TString& request, TString* result, size_t tokenizerVersion=NTokenizerVersionsInfo::Default);
