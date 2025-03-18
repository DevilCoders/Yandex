#include <util/string/cast.h>
#include <util/string/strip.h>

#include "mycookie.h"
#include "bycookie.h"

ELanguage LanguageByMyCookie(const TString& myCookie) {
    // https://wiki.yandex-team.ru/MyCookie/NomerBloka/
    // https://proxy.sandbox.yandex-team.ru/730244850
    static const ELanguage langIdByValue[] = {
        /* 0  */ LANG_UNK,
        /* 1  */ LANG_RUS,
        /* 2  */ LANG_UKR,
        /* 3  */ LANG_ENG,
        /* 4  */ LANG_KAZ,
        /* 5  */ LANG_BEL,
        /* 6  */ LANG_TAT,
        /* 7  */ LANG_AZE,
        /* 8  */ LANG_TUR,
        /* 9  */ LANG_ARM,
        /* 10 */ LANG_GEO,
        /* 11 */ LANG_RUM,
        /* 12 */ LANG_GER,
        /* 13 */ LANG_IND,
        /* 14 */ LANG_CHI,
        /* 15 */ LANG_SPA,
        /* 16 */ LANG_POR,
        /* 17 */ LANG_FRE,
        /* 18 */ LANG_ITA,
        /* 19 */ LANG_JPN,
        /* 20 */ LANG_BRE,
        /* 21 */ LANG_CZE,
        /* 22 */ LANG_FIN,
        /* 23 */ LANG_UZB,
        /* 24 */ LANG_SRP,
        /* 25 */ LANG_MAC,
        /* 26 */ LANG_PER};

    const int MC_INTL = 39;

    if (myCookie.empty())
        return LANG_UNK;

    int value = -1;

    TMyCookie cookie(myCookie);
    if (cookie.GetValuesCount(MC_INTL) > 1)
        value = cookie.GetValue(MC_INTL, 1);

    if (value >= 0 && value < (int)Y_ARRAY_SIZE(langIdByValue))
        return langIdByValue[value];

    return LANG_UNK;
}

TString GetCookieFromHeader(TStringBuf header, TStringBuf cookieName) {
    TString cookieNameLower(cookieName);
    cookieNameLower.to_lower();
    while (header.length()) {
        TString name = StripString(ToString(header.NextTok('=')));
        TString val = ToString(header.NextTok(';'));
        if (to_lower(name) == cookieNameLower)
            return val;
    }
    return TString();
}
