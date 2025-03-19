#pragma once

#include "fio_inflector_core.h"

#include <kernel/lemmer/dictlib/grammar_enum.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>


namespace NFioInflector {

    struct TFioToken {
        TUtf16String Text;
        EGrammar Mark = gInvalid;
        bool IsWord = false;
        bool StartsFromUpperCase = false;
        TCharCategory CharCategory = CC_EMPTY;
        TWLemmaArray Lemmas;

        TFioToken(const TUtf16String& text, EGrammar proper)
            : Text(text)
            , Mark(proper)
        {
        }
    };

    class TFioTokenInflector {
    private:
        TFioInflectorCore Inflector;

    private:
        EGrammar GuessGender(const TVector<TFioToken>& tokens, EGrammar proper) const;

    public:
        TFioTokenInflector(const TLanguage* lang, const TFioInflectorOptions& options = {})
            : Inflector(lang, options)
        {
        }

        void SetUnsetProper(TVector<TFioToken>* tokens, const TGramBitSet& hint) const;

        EGrammar GuessGender(const TVector<TFioToken>& tokens) const;
        void LemmatizeFio(TVector<TFioToken>* tokens, EGrammar gender) const;
        TUtf16String InflectInCase(const TVector<TFioToken>& tokens, EGrammar gCase) const;
    };

    void TokenizeText(const TUtf16String& text, TVector<TFioToken>* tokens);
}
