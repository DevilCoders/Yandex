#pragma once

#include <kernel/lemmer/core/lemmer.h>
#include <kernel/lemmer/dictlib/grammar_index.h>
#include <kernel/lemmer/core/formgenerator.h>
#include <kernel/lemmer/core/language.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NLanguageRubbishDump {
    NLemmer::TLllLevel EasternSlavicLooksLikeLemma(const TYandexLemma& lemma, char* gramForGenerator = nullptr, size_t bufsLen = 0);
    NLemmer::TLllLevel DefaultLooksLikeLemma(const TYandexLemma& lemma);
    inline TAutoPtr<NLemmer::TGrammarFiltr> EasternSlavicLooksLikeLemmaExt(const TLanguage* lang, const TYandexLemma& lemma) {
        char gram[255];
        if (!EasternSlavicLooksLikeLemma(lemma, gram, 255))
            return nullptr;
        TAutoPtr<NLemmer::TGrammarFiltr> p = lang->GetGrammFiltr(gram);
        p->SetLemma(lemma);
        return p;
    }

    template <typename T, size_t N>
    size_t SizeOfArray(T (&)[N]) {
        return N;
    }

    wchar16 ToLowerTurkish(wchar16 ch);
    wchar16 ToUpperTurkish(wchar16 ch);

    bool EnglishCanBreak(const wchar16* word, size_t pos1, size_t len1, size_t pos2, size_t len2);
}
