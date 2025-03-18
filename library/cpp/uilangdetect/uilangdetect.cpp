#include <util/generic/algorithm.h>

#include "bycookie.h"
#include "bytld.h"
#include "byacceptlang.h"
#include "uilangdetect.h"

static const ELanguage good[] = {LANG_RUS, LANG_UKR, LANG_ENG, LANG_KAZ, LANG_BEL, LANG_TAT, LANG_TUR, LANG_CZE};
static const TLangVector goodReportLanguages(good, good + Y_ARRAY_SIZE(good));

ELanguage DetectUserLanguage(const TString& host, const TString& l10n, const TString& myCookie, TStringBuf accLang, const TVector<ELanguage>& goodUserLanguages) {
    const auto language = TryDetectUserLanguage(host, l10n, myCookie, accLang, goodUserLanguages);
    if (language) {
        return *language;
    }

    return LANG_RUS;
}

ELanguage DetectUserLanguage(const TServerRequestData& rd, const TVector<ELanguage>& goodUserLanguages) {
    const auto language = TryDetectUserLanguage(rd, goodUserLanguages);
    if (language) {
        return *language;
    }

    return LANG_RUS;
}

TMaybe<ELanguage> TryDetectUserLanguage(const TString& host, const TString& l10n, const TString& myCookie, TStringBuf accLang, const TVector<ELanguage>& goodUserLanguages) {
    const TLangVector& goodLanguages = goodUserLanguages.size() > 0 ? goodUserLanguages : goodReportLanguages;

    TString tld = TLDByHost(host);

    ELanguage lang = LanguageByName(l10n);
    if (IsIn(goodLanguages, lang))
        return lang;

    if (tld == "com")
        return LANG_ENG;

    if (tld == "tr")
        return LANG_TUR;

    lang = LanguageByMyCookie(myCookie);
    if (lang != LANG_ENG && IsIn(goodLanguages, lang))
        return lang;

    ELanguage domainDefaultLang = LanguageByTLD(tld);
    lang = LanguageByAcceptLanguage(accLang, domainDefaultLang, goodLanguages);
    if (lang != LANG_UNK)
        return lang;

    return {};
}

TMaybe<ELanguage> TryDetectUserLanguage(const TServerRequestData& rd, const TVector<ELanguage>& goodUserLanguages) {
    TString myCookie = GetCookieFromHeader(rd.HeaderInOrEmpty("Cookie"), "my");
    TString domain = rd.ServerName();
    TString l10n = rd.CgiParam.Get("l10n");
    TStringBuf acceptLanguage = rd.HeaderInOrEmpty("Accept-Language");

    return TryDetectUserLanguage(domain, l10n, myCookie, acceptLanguage, goodUserLanguages);
}
