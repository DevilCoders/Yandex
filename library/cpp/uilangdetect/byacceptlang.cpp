#include <util/generic/algorithm.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>
#include <util/string/strip.h>

#include "byacceptlang.h"

struct TLanguageWithWeight {
    ELanguage Language;
    double Weight;
};

static ELanguage ReadLanguage(TStringBuf langName) {
    TString langISO = to_lower(StripString(ToString(langName)));
    langISO.remove(2);
    return LanguageByName(langISO);
}

TLangVector ReadLanguages(TStringBuf langString, const TString& delimiter) {
    TLangVector langs;
    while (langString.length()) {
        TStringBuf token = langString.NextTok(delimiter);
        langs.push_back(ReadLanguage(token));
    }
    return langs;
}

static double ReadWeight(TStringBuf langWeight) {
    if (langWeight.length() == 0)
        return 1.0;

    TStringBuf q = langWeight.NextTok('=');

    if (to_lower(StripString(ToString(q))) == "q") {
        try {
            return FromString<double>(StripString(ToString(langWeight)));
        } catch (const yexception&) {
        }
    }

    return 0.0;
}

static TLanguageWithWeight ReadLanguageWithWeight(TStringBuf token) {
    TStringBuf langName, langWeight;
    token.Split(';', langName, langWeight);

    TLanguageWithWeight res = {ReadLanguage(langName), ReadWeight(langWeight)};

    if (res.Language == LANG_UNK)
        res.Weight = 0.0;

    return res;
}

ELanguage LanguageByAcceptLanguage(TStringBuf acceptLanguages, ELanguage defaultLanguage, const TLangVector& goodLanguages) {
    TLanguageWithWeight bestLang = {LANG_UNK, -1.0};

    while (acceptLanguages.length()) {
        TStringBuf token = acceptLanguages.NextTok(','); TLanguageWithWeight lang = ReadLanguageWithWeight(token);
        if (lang.Weight > bestLang.Weight || (lang.Weight == bestLang.Weight && lang.Language == defaultLanguage))
            if (IsIn(goodLanguages, lang.Language))
                bestLang = lang;
    }

    return bestLang.Language == LANG_UNK ? defaultLanguage : bestLang.Language;
}
