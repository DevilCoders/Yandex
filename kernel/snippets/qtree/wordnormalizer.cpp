#include "wordnormalizer.h"

#include <kernel/lemmer/alpha/abc.h>

namespace NSnippets {

static bool IsNormalized(TWtringBuf word, ELanguage langId) {
    return NLemmer::GetAlphaRules(langId)->IsNormalized(word.data(), word.size());
}

static bool Normalize(TWtringBuf word, ELanguage langId, TUtf16String& normalizedWord) {
    size_t bufSize = Max<size_t>(word.size() * 2, 64);
    normalizedWord.resize(bufSize);
    NLemmer::TConvertRet r = NLemmer::GetAlphaRules(langId)->Normalize(word.data(), word.size(), normalizedWord.begin(), bufSize);
    if (!r.Valid) {
        return false;
    }
    normalizedWord.resize(r.Length);
    return true;
}

TWordNormalizer::TWordNormalizer(const TLangMask& langs)
    : Langs(langs)
{
}

void TWordNormalizer::GetNormalizedWords(TWtringBuf word, TLangMask wordLangs, TVector<TUtf16String>& normalizedWords) const {
    normalizedWords.clear();
    TUtf16String normalizedWord;

    if (Normalize(word, LANG_UNK, normalizedWord)) {
        normalizedWords.push_back(normalizedWord);
    }

    // Rare cases, for example Russian words with Latin "C" instead of Cyrillic one
    TLangMask langs = Langs & wordLangs;
    for (ELanguage langId : langs) {
        if (IsNormalized(word, langId)) {
            continue;
        }
        if (Normalize(word, langId, normalizedWord)) {
            normalizedWords.push_back(normalizedWord);
        }
    }
}

} // namespace NSnippets
