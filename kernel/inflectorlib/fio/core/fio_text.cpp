#include "fio_text.h"

#include "fio_token.h"

#include <util/string/vector.h>

namespace NFioInflector {

// Visible functions impl
EGrammar CutGender(TUtf16String& w) {
    size_t i = 0;
    while ((i < w.size()) && (w[i] != ':'))
        ++i;
    size_t j = i;
    if (w[i] == ':') {
        ++i;
        while (IsSpace(w[i]))
            ++i;
        if (w[i] == 'm') {
            w.resize(j);
            return gMasculine;
        }
        if (w[i] == 'f') {
            w.resize(j);
            return gFeminine;
        }
    }
    w.resize(j);
    return gMasFem;
}

TVector<TUtf16String> InflectText(const TLanguage* lang, const TUtf16String& text, const EGrammar cases[], EGrammar gender) {
    TVector<TFioToken> fioToks;
    TokenizeText(text, &fioToks);

    TFioTokenInflector fioTokenInflector(lang);
    if (gender == gMasFem) {
        fioTokenInflector.SetUnsetProper(&fioToks, TGramBitSet());
        gender = fioTokenInflector.GuessGender(fioToks);
    } else {
        fioTokenInflector.SetUnsetProper(&fioToks, TGramBitSet(gender));
    }

    fioTokenInflector.LemmatizeFio(&fioToks, gender);

    TVector<TUtf16String> inflections;
    for (size_t i = 0; cases[i] != gInvalid; ++i) {
        TUtf16String inflection = fioTokenInflector.InflectInCase(fioToks, cases[i]);
        inflections.push_back(inflection);
    }

    return inflections;
}

TVector<TUtf16String> InflectText(const TLanguage* lang, const TUtf16String& text, const EGrammar cases[]) {
    return InflectText(lang, text, cases, gMasFem);
}

EGrammar GuessGender(const TLanguage* lang, const TUtf16String& text) {
    TVector<TFioToken> fioToks;
    TokenizeText(text, &fioToks);

    TFioTokenInflector fioTokenInflector(lang);
    fioTokenInflector.SetUnsetProper(&fioToks, TGramBitSet());
    EGrammar gender = fioTokenInflector.GuessGender(fioToks);

    return gender;
}

} // namespace NFioInflector
