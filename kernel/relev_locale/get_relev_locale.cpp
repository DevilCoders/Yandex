#include "relev_locale.h"

#include <kernel/search_types/search_types.h>
#include <kernel/country_data/countries.h> // RelevLocale2Country, Lang2SPOKCountry

#include <library/cpp/langs/langs.h>

#include <util/generic/hash_set.h>
#include <util/generic/vector.h>
#include <util/system/yassert.h>

using namespace NRl;

static constexpr ERelevLocale GetRelevLocaleRuTld(const EYandexSerpTld /*tld*/,
                                                  const ELanguage /*language*/) {
    return RL_RU;
}

static ERelevLocale GetRelevLocaleTrTld(const EYandexSerpTld /*tld*/,
                                        const ELanguage language) {
    if (LANG_RUS == language) {
        return RL_RU;
    } else {
        return RL_TR;
    }
}

static ERelevLocale GetRelevLocaleUaByKzTld(const EYandexSerpTld tld,
                                            const ELanguage /*language*/) {
    switch (tld) {
        case YST_UA:
            return RL_UA;
        case YST_BY:
            return RL_BY;
        case YST_KZ:
            return RL_KZ;
        default:
            Y_ASSERT(!"Unexpected top level domain (aka tld).");
    }
    return RL_WORLD; // make compiler happy
}

static ERelevLocale GetRelevLocaleComTld(const EYandexSerpTld /*tld*/, const ELanguage language) {
    switch (language) {
        case LANG_RUS:
            return NRl::RL_RU;
        case LANG_UKR:
            return NRl::RL_UA;
        case LANG_BEL:
            return NRl::RL_BY;
        case LANG_KAZ:
            return NRl::RL_KZ;
        case LANG_TUR:
            return NRl::RL_TR;
        default:
            return NRl::RL_WORLD;
    }
}

static ERelevLocale GetRelevLocaleEU(const EYandexSerpTld /*tld*/, const TCateg country) {
    const static THashSet<TCateg> euCountries = {
            120 /*pl*/, 10077 /*ro*/, 96 /*de*/, 124 /*fr*/, 102 /*uk*/, 116 /*hu*/, 115 /*bg*/, 125 /*cz*/,
            205 /*it*/, 203 /*dk*/, 10063 /*ie*/, 204 /*es*/, 118 /*nl*/, 119 /*no*/, 10074 /*pt*/, 114 /*be*/,
            113 /*at*/, 10083 /*hr*/, 20574 /*cy*/, 123 /*fi*/, 246 /*gr*/, 10064 /*is*/, 10067 /*li*/,
            21203 /*lu*/, 10069 /*mt*/, 121 /*sk*/, 122 /*si*/, 127 /*se*/, 117 /*lt*/, 206 /*lv*/, 179 /*ee*/
    };
    const TCountryData* countryPtr = GetCountryData(country);
    if (euCountries.contains(country) && countryPtr) {
        return countryPtr->RelevLocale;
    }
    return NRl::RL_WORLD;
}

static ERelevLocale TrySpokRelevLocale(const ELanguage language, const TCateg country,
                                       const THashSet<TCateg>& spokCountries) {
    const auto* countryData = GetCountryData(country);
    if (countryData && spokCountries.contains(countryData->CountryId)) {
        return countryData->RelevLocale;
    } else {
        const auto* countryByQmpl = Lang2SPOKCountry(language);
        if (countryByQmpl && spokCountries.contains(countryByQmpl->CountryId)) {
            return countryByQmpl->RelevLocale;
        }
    }
    return RL_UNIVERSE;
}

TRelevLocale NRl::GetLocale2(const EYandexSerpTld tld, ELanguage language,
                                      const TCateg country, const ELanguage uil, const THashSet<TCateg>& spokCountries) {
    static constexpr TCateg RUSSIA = 225;

    const TCountryData* countryPtr = GetCountryData(country);
    const TCountryData* uilCountryPtr = Lang2SPOKCountry(uil);
    const TCountryData* langCountryPtr = Lang2SPOKCountry(language);

    switch (tld) {
        case YST_TR: {
            if (language == LANG_RUS) {
                return TRelevLocale{RL_RU};
            } else {
                return TRelevLocale{RL_TR};
            }
        }
        case YST_COM: {
            if (language == LANG_UNK || language == LANG_ENG) {
                language = uil;
                langCountryPtr = uilCountryPtr;
            }
            switch (language) {
                case LANG_RUS:
                case LANG_UKR:
                case LANG_BEL:
                case LANG_KAZ:
                case LANG_TUR:
                    return TRelevLocale{langCountryPtr ? langCountryPtr->RelevLocale : RL_WORLD};
                default: {
                    if (spokCountries.contains(country) && countryPtr && !UnknownLanguage(language) && language != LANG_MAX &&
                        (language == LANG_ENG || countryPtr->NationalLanguage.Test(language))) {
                        return TRelevLocale{countryPtr->RelevLocale, true};
                    } else if (langCountryPtr && spokCountries.contains(langCountryPtr->CountryId)) {
                        return TRelevLocale{langCountryPtr->RelevLocale, true};
                    } else {
                        return TRelevLocale{RL_WORLD};
                    }
                    break;
                }
            }
        }
        case YST_RU:
            switch (country) {
                case RUSSIA:
                    if (uilCountryPtr && IsLocaleDescendantOf(uilCountryPtr->RelevLocale, RL_XUSSR)) {
                        return TRelevLocale{uilCountryPtr->RelevLocale};
                    } else {
                        return TRelevLocale{RL_RU};
                    }
                default:
                    if (countryPtr && IsLocaleDescendantOf(countryPtr->RelevLocale, RL_XUSSR)) {
                        return TRelevLocale{countryPtr->RelevLocale};
                    } else {
                        return TRelevLocale{RL_RU};
                    }
                    break;
            }
        case YST_EU:
            return TRelevLocale{GetRelevLocaleEU(tld, country)};
        default: {
            const TCountryData* tldCountryPtr = GetCountryData(GetCountryByTld(tld));
            if (tldCountryPtr) {
                return TRelevLocale{tldCountryPtr->RelevLocale};
            } else {
                return TRelevLocale{RL_WORLD};
            }
            break;
        }
    }
}

TRelevLocale NRl::GetLocale(const EYandexSerpTld tld, const ELanguage language,
                                     const TCateg country, const THashSet<TCateg>& spokCountries) {
    static constexpr TCateg UKRAINE_ID = 187;
    static constexpr TCateg BELARUS_ID = 149;
    static constexpr TCateg KAZAKHSTAN_ID = 159;

    switch (tld) {
        case YST_RU: {
            // redirect didn't work in mobile, xml: SERP-27421
            switch (country) {
                case UKRAINE_ID:
                    return TRelevLocale{RL_UA};
                case BELARUS_ID:
                    return TRelevLocale{RL_BY};
                case KAZAKHSTAN_ID:
                    return TRelevLocale{RL_KZ};
                default:
                    return TRelevLocale{GetRelevLocaleRuTld(tld, language)};
            }
        }
        case YST_UA:
        case YST_BY:
        case YST_KZ:
            return TRelevLocale{GetRelevLocaleUaByKzTld(tld, language)};
        case YST_FI:
            return TRelevLocale{RL_FI};
        case YST_PL:
            return TRelevLocale{RL_PL};
        case YST_LV:
            return TRelevLocale{RL_LV};
        case YST_LT:
            return TRelevLocale{RL_LT};
        case YST_EE:
            return TRelevLocale{RL_EE};
        case YST_EU:
            return TRelevLocale{GetRelevLocaleEU(tld, country)};
        case YST_TR:
            return TRelevLocale{GetRelevLocaleTrTld(tld, language)};
        case YST_COM: {
            const auto spokLocale = TrySpokRelevLocale(language, country, spokCountries);
            if (RL_UNIVERSE == spokLocale) {
                return TRelevLocale{GetRelevLocaleComTld(tld, language)};
            } else {
                return TRelevLocale{spokLocale, true};
            }
        } case YST_UNKNOWN:
            return TRelevLocale{RL_UNIVERSE};
        default:
            return TRelevLocale{RL_WORLD};
    }
    return {}; // make compiler happy
}

static TCateg MinRegion(const TCateg relevCountry, const TCateg userCountry,
                        const TCateg userRegion) {
    if (END_CATEG == userCountry || relevCountry != userCountry) {
        return relevCountry;
    } else {
        return userRegion;
    }
}

TCateg NRl::GetSearchRegion(const TCateg userRegion, const TCateg userCountry,
                                     const EYandexSerpTld tld, const ELanguage language,
                                     const ERelevLocale relevLocale,
                                     const THashSet<TCateg>& spokCountries) {
    // tld, language and spokCountries are already taken into account in relevLocale
    Y_UNUSED(tld);
    Y_UNUSED(language);
    Y_UNUSED(spokCountries);

    static constexpr TCateg USA = 84;

    if (relevLocale == RL_WORLD) { //"international" search expects USA as a country due to historical (yandex.com) reasons
        return USA;
    }
    if (relevLocale == RL_UNIVERSE) { //unknown relev locale or relev locale out of hierarchy
        return END_CATEG;
    }

    const auto countryByLocale = RelevLocale2Country(relevLocale);
    return MinRegion(countryByLocale, userCountry, userRegion);
}

TCateg NRl::GetSearchRegion(const TCateg userRegion, const TCateg userCountry,
                                     const EYandexSerpTld tld, const ELanguage language,
                                     const ERelevLocale relevLocale) {
    return GetSearchRegion(userRegion, userCountry, tld, language, relevLocale, {});
}


ERelevLocale NRl::GetLocale(const EYandexSerpTld tld, const ELanguage language,
                                     const TCateg country) {
    return GetLocale(tld, language, country, {}).RelevLocale;
}

ERelevLocale NRl::GetLocale2(const EYandexSerpTld tld, const ELanguage language,
                                     const TCateg country, const ELanguage uil) {
    return GetLocale2(tld, language, country, uil, {}).RelevLocale;
}
