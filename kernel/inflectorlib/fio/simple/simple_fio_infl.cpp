#include "simple_fio_infl.h"

#include <kernel/inflectorlib/fio/core/fio_token.h>
#include <kernel/inflectorlib/fio/rus_fio/rus_fio_language.h>


EGrammar GetCase(const TGramBitSet& gramms) {
    for (EGrammar gr : gramms & NSpike::AllMajorCases) {
        return gr;
    }
    return gNominative;
}

EGrammar GetGender(const TGramBitSet& gramms) {
    static const TGramBitSet genderGr(gMasculine, gFeminine);
    for (EGrammar gr : gramms & genderGr) {
        return gr;
    }
    return gMasFem;
}

EGrammar GetProper(const TGramBitSet& gramms) {
    static const TGramBitSet properGr(gFirstName, gSurname, gPatr);
    for (EGrammar gr : gramms & properGr) {
        return gr;
    }

    return gInvalid;
}

namespace NFioInflector {
    TSimpleFioInflector::TSimpleFioInflector(const TString& lang) {
        Init(LanguageByName(lang));
    }

    TSimpleFioInflector::TSimpleFioInflector(ELanguage lang) {
        Init(lang);
    }

    void TSimpleFioInflector::Init(ELanguage lang) {
        if (lang == LANG_RUS) {
            Infl.Reset(new TFioTokenInflector(TRusFIOLanguage::GetLang()));
            return;
        }

        ythrow yexception() << "FIO inflection in " << NameByLanguage(lang) << " isn't supported";
    }

    TVector<TUtf16String> TSimpleFioInflector::Inflect(const TUtf16String& text, const TGramBitSet& hintGrams, const TVector<EGrammar>& cases) const {
        TVector<TFioToken> fioToks;
        TokenizeText(text, &fioToks);

        EGrammar gProper = GetProper(hintGrams);
        if (gProper != gInvalid) {
            for (auto& tok : fioToks) {
                tok.Mark = gProper;
            }
        } else {
            Infl->SetUnsetProper(&fioToks, TGramBitSet());
        }

        EGrammar gGender = GetGender(hintGrams);
        if (gGender == gMasFem) {
            gGender = Infl->GuessGender(fioToks);
        }

        Infl->LemmatizeFio(&fioToks, gGender);

        TVector<TUtf16String> result;
        result.reserve(cases.size());
        for (auto gCase : cases) {
            result.push_back(Infl->InflectInCase(fioToks, gCase));
        }
        return result;
    }

    TUtf16String TSimpleFioInflector::Inflect(const TUtf16String& text, const TGramBitSet& hintGrams) const {
        return Inflect(text, hintGrams, {GetCase(hintGrams)})[0];
    }
}

