#include "relev_locale.h"

#include <util/generic/array_size.h>
#include <util/generic/hash.h>
#include <util/generic/mapfindptr.h>
#include <util/generic/singleton.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>

#include <array>

using namespace NRl;

namespace {
    // Locale names according to http://en.wikipedia.org/wiki/ISO_3166-1
    static constexpr char const* const RELEV_LOCALE_TO_STR[] {
        "universe", // RL_UNIVERSE
        "ru",      // RL_RU
        "ua",      // RL_UA
        "by",      // RL_BY
        "kz",      // RL_KZ
        "tr",      // RL_TR
        "de",      // RL_DE
        "it",      // RL_IT
        "pl",      // RL_PL
        "lt",      // RL_LT
        "lv",      // RL_LV
        "ee",      // RL_EE
        "md",      // RL_MD
        "ro",      // RL_RO
        "us",      // RL_US
        "ge",      // RL_GE
        "uz",      // RL_UZ
        "il",      // RL_IL
        "fr",      // RL_FR
        "ca",      // RL_CA
        "gb",      // RL_GB
        "hu",      // RL_HU
        "az",      // RL_AZ
        "bg",      // RL_BG
        "in",      // RL_IN
        "cz",      // RL_CZ
        "sa",      // RL_SA
        "vn",      // RL_VN
        "dk",      // RL_DK
        "id",      // RL_ID
        "ie",      // RL_IE
        "es",      // RL_ES
        "cn",      // RL_CN
        "kr",      // RL_KR
        "nl",      // RL_NL
        "no",      // RL_NO
        "pt",      // RL_PT
        "th",      // RL_TH
        "ph",      // RL_PH
        "jp",      // RL_JP
        "au",      // RL_AU
        "be",      // RL_BE
        "br",      // RL_BR
        "eg",      // RL_EG
        "mx",      // RL_MX
        "nz",      // RL_NZ
        "pr",      // RL_PR
        "ch",      // RL_CH
        "world",   // RL_WORLD
        "xussr",   // RL_XUSSR
        "xcom",    // RL_XCOM
        "spok",    // RL_SPOK
        "my",      // RL_MY
        "am",      // RL_AM
        "kg",      // RL_KG
        "tj",      // RL_TJ
        "tm",      // RL_TM
        "rs",      // RL_RS
        "ci",      // RL_CI
        "fi",      // RL_FI = Finland
        "eu",      // RL_EU = common EU locale
        "at",      // RL_AT = Austria
        "hr",      // RL_HR = Croatia
        "cy",      // RL_CY = Cyprus
        "gr",      // RL_GR = Greece
        "is",      // RL_IS = Iceland
        "li",      // RL_LI = Liechtenstein
        "lu",      // RL_LU = Luxembourg
        "mt",      // RL_MT = Malta
        "sk",      // RL_SK = Slovakia
        "si",      // RL_SI = Slovenia
        "se",      // RL_SE = Spain
        "cm",      // RL_CM = Cameroon
        "sn",      // RL_SN = Senegal
        "ao"       // RL_AO = Angola 
    };


    static constexpr size_t RELEV_LOCALE_TO_STR_SIZE = Y_ARRAY_SIZE(RELEV_LOCALE_TO_STR);


    static_assert(
        ERelevLocale_ARRAYSIZE == RELEV_LOCALE_TO_STR_SIZE,
        "You must define string representation for each member of ERelevLocale"
    );

    static_assert(
        ERelevLocale_MIN == 0,
        "Smallest value of enum must be zero"
    );


    class THelper {
    public:
        THelper() {
            for (int index = 0; index < ERelevLocale_ARRAYSIZE; ++index) {
                EnumToName_[index] = RELEV_LOCALE_TO_STR[index];
                if (NameToEnum_.contains(EnumToName_[index])) {
                    ythrow yexception() << '"' << EnumToName_[index] << '"'
                                        << " was mentioned at least twice";
                }
                NameToEnum_[EnumToName_[index]] = static_cast<ERelevLocale>(index);
            }

            // Backward compatibility for unknown. It was renamed to universe
            NameToEnum_["unknown"] = RL_UNIVERSE;
        }

        inline const TString& Name(const ERelevLocale& locale) const {
            return EnumToName_.at(locale);
        }

        inline ERelevLocale Enum(const TStringBuf& name) const {
            ERelevLocale relevLocale;
            if (!Enum(name, relevLocale))
                ythrow TFromStringException() << "\"" << name << "\" is not a valid relev_locale value";
            return relevLocale;
        }

        inline bool Enum(const TStringBuf& name, ERelevLocale& locale) const {
            if (auto ptr = MapFindPtr(NameToEnum_, name)) {
                locale = *ptr;
                return true;
            }
            return false;
        }

    private:
        std::array<TString, ERelevLocale_ARRAYSIZE> EnumToName_;
        THashMap<TString, ERelevLocale> NameToEnum_;
    };

}  // namespace


const TString& ToString(const ERelevLocale value) {
    return Default<::THelper>().Name(value);
}


bool FromString(const TStringBuf& name, ERelevLocale& ret) {
    return Default<::THelper>().Enum(name, ret);
}

ERelevLocale ERelevLocaleFromString(const TStringBuf& name) {
    return Default<::THelper>().Enum(name);
}

// FromString() support
template <>
ERelevLocale FromStringImpl<ERelevLocale>(const char* s, size_t len) {
    return ERelevLocaleFromString(TStringBuf(s, len));
}

template <>
bool TryFromStringImpl<ERelevLocale>(const char* s, size_t len, ERelevLocale& res) {
    return FromString(TStringBuf{s, len}, res);
}
