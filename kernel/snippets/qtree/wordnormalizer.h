#pragma once

#include <library/cpp/langmask/langmask.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NSnippets {

class TWordNormalizer {
    TLangMask Langs;
public:
    TWordNormalizer(const TLangMask& langs);
    void GetNormalizedWords(TWtringBuf word, TLangMask wordLangs, TVector<TUtf16String>& normalizedWords) const;
};

} // namespace NSnippets
