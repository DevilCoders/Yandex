#pragma once

#include <kernel/inflectorlib/fio/core/fio_token.h>

#include <library/cpp/langs/langs.h>
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/dictlib/grambitset.h>

#include <util/generic/string.h>
#include <util/generic/ptr.h>


namespace NFioInflector {
    class TSimpleFioInflector {
        public:
            TSimpleFioInflector(const TString& lang);
            TSimpleFioInflector(ELanguage lang);

            TVector<TUtf16String> Inflect(const TUtf16String& text, const TGramBitSet& hints, const TVector<EGrammar>& cases) const;
            TUtf16String Inflect(const TUtf16String& text, const TGramBitSet& hintGrammars) const;

            TUtf16String Inflect(const TUtf16String& text, const TString& hintGrammars) const {
                return Inflect(text, TGramBitSet::FromString(hintGrammars, ","));
            }

        private:
            void Init(ELanguage lang);

        private:
            THolder<TFioTokenInflector> Infl;
    };
}

