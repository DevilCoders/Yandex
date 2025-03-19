#pragma once

#include <kernel/lemmer/dictlib/grammar_enum.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/lemmer.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>


namespace NFioInflector {
    struct TFioInflectorOptions {
        bool SkipAdvancedNormaliation = true; // disables aggresive normalization;
                                              // e.g. без -> бес, see BEGEMOT-949
    };

    class TFioInflectorCore {
    private:
        const TLanguage* Lang = nullptr;
        TFioInflectorOptions Options;

    private:
        EGrammar GuessGender(const TUtf16String& name, const TGramBitSet& hint) const;

        TYandexLemma AnalyzeChunk(const TUtf16String& s,
            const char * requiredGrammar,
            NLemmer::EAccept acc = NLemmer::AccBastard) const;

        TWLemmaArray MyAnalyze(const TUtf16String& s, const char * requiredGrammar) const;
        TUtf16String Generate(const TYandexLemma& lemma, EGrammar gCase) const;

    public:
        TFioInflectorCore(
            const TLanguage* lang,
            const TFioInflectorOptions& options = {})
            : Lang(lang)
            , Options(options)
        {
        }

        EGrammar GuessGender(const TUtf16String& name, const TUtf16String& surname) const;
        EGrammar GuessGender(const TUtf16String& name, EGrammar proper) const;
        EGrammar GuessGender(const TUtf16String& name) const;

        EGrammar GuessProper(const TUtf16String& name, const TGramBitSet& hint) const;

        // gender - gMasculine, gFeminine
        // proper - gSurname, gFirstName, gPatr
        TWLemmaArray Analyze(const TUtf16String& text, EGrammar gender, EGrammar proper) const;
        TWLemmaArray Analyze(const TUtf16String& text, EGrammar gender) const;
        TWLemmaArray Analyze(const TUtf16String& name, const TGramBitSet& hint) const;

        TUtf16String Generate(const TWLemmaArray& lemmas, EGrammar gCase) const;
    };


} // namespace NFioInflector

