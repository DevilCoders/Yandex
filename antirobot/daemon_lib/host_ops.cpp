#include "host_ops.h"

#include "request_params.h"

#include <kernel/hosts/owner/owner.h>

#include <util/generic/utility.h>
#include <util/generic/vector.h>
#include <util/string/split.h>

namespace NAntiRobot {

TStringBuf GetYandexDomainFromHost(const TStringBuf& host) {
    TStringBuf cur = host;

    const size_t MAX_YANDEX_DOMAIN_LEVEL = 3;
    for (size_t i = 0; i < MAX_YANDEX_DOMAIN_LEVEL && !cur.empty(); ++i) {
        TStringBuf curLevelDomain = cur.RNextTok('.');
        if (i > 0 && curLevelDomain == "yandex"sv) {
            // There isn't a typo in the next line. "curLevelDomain" is always a prefix of
            // "host", thus such construction is OK.
            return TStringBuf(curLevelDomain.begin(), host.end());
        }
    }
    return TStringBuf();
}

TStringBuf GetCookiesDomainFromHost(const TStringBuf& host) {
    TStringBuf yandexDomain = GetYandexDomainFromHost(host);
    if (!yandexDomain) {
        return TOwnerCanonizer::WithTrueOwners().GetHostOwner(host);
    }
    return yandexDomain;
}

TStringBuf GetTldFromHost(TStringBuf host) {
    return host.RNextTok('.');
}

ELanguage GetLangFromHost(TStringBuf host) {
    static const TRegexpClassifier<ELanguage> hostToLang(
        {
            {"(.+\\.)?eda\\.yandex", LANG_RUS},
        },
        LANG_UNK);

    auto lang = hostToLang[host];
    if (lang != LANG_UNK) {
        return lang;
    }

    auto tld = GetTldFromHost(host);
    return LanguageByTLD(TString(tld));
}

}
