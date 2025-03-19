#include "serptld.h"

#include <google/protobuf/descriptor.h>
#include <library/cpp/containers/dictionary/dictionary.h>

#include <util/generic/singleton.h>
#include <util/generic/strbuf.h>
#include <util/stream/output.h>
#include <util/string/cast.h>

#include <utility>

namespace {
    const static std::pair<const char*, EYandexSerpTld> E_YANDEX_SERP_TLDS[] = {
        std::make_pair("UNKNOWN", YST_UNKNOWN),
        std::make_pair("ru", YST_RU),
        std::make_pair("ua", YST_UA),
        std::make_pair("tr", YST_TR),
        std::make_pair("by", YST_BY),
        std::make_pair("kz", YST_KZ),
        std::make_pair("uz", YST_UZ),
        std::make_pair("az", YST_AZ),
        std::make_pair("am", YST_AM),
        std::make_pair("ge", YST_GE),
        std::make_pair("kg", YST_KG),
        std::make_pair("lv", YST_LV),
        std::make_pair("lt", YST_LT),
        std::make_pair("md", YST_MD),
        std::make_pair("tj", YST_TJ),
        std::make_pair("tm", YST_TM),
        std::make_pair("ee", YST_EE),
        std::make_pair("il", YST_IL),
        std::make_pair("fr", YST_FR),
        std::make_pair("de", YST_DE),
        std::make_pair("com", YST_COM),
        std::make_pair("pl", YST_PL),
        std::make_pair("fi", YST_FI),
        std::make_pair("eu", YST_EU)};

    using TSparseId2Str = NNameIdDictionary::TSparseId2Str<EYandexSerpTld, TStringBuf>;
    using TYandexSerpTld2String = TEnum2String<EYandexSerpTld, TStringBuf, TSparseId2Str>;

    struct TYandexSerpTldNames: public TYandexSerpTld2String {
        TYandexSerpTldNames()
            : TYandexSerpTld2String(E_YANDEX_SERP_TLDS)
        {
            Y_VERIFY(NProtoBuf::GetEnumDescriptor<EYandexSerpTld>()->value_count() == Y_ARRAY_SIZE(E_YANDEX_SERP_TLDS));
        }
    };

    const static std::pair<const char*, EYandexSerpTld> E_YANDEX_SECOND_LEVEL_SERP_TLDS[] = {
        std::make_pair("co.il", YST_IL),
        std::make_pair("com.tr", YST_TR),
        std::make_pair("com.am", YST_AM),
        std::make_pair("com.ge", YST_GE)};

    struct TYandexSecondLevelSerpTldNames: public TYandexSerpTld2String {
        TYandexSecondLevelSerpTldNames()
            : TYandexSerpTld2String(E_YANDEX_SECOND_LEVEL_SERP_TLDS)
        {
        }
    };
}

template <>
bool TryFromStringImpl<EYandexSerpTld>(const char* data, size_t len, EYandexSerpTld& tld) {
    if (!data || '\0' == data[0]) {
        return false;
    }

    const auto res = EYandexSerpTldFromString(TStringBuf{data, len});
    if (YST_UNKNOWN == res) {
        return false;
    }

    tld = res;
    return true;
}

template <>
EYandexSerpTld FromStringImpl<EYandexSerpTld>(const char* data, size_t len) {
    TStringBuf tld(data, len);
    return EYandexSerpTldFromString(tld);
}

EYandexSerpTld EYandexSerpTldFromString(const TStringBuf& tldStr) {
    if (!tldStr) {
        return YST_RU;
    }
    if (TStringBuf("xn--p1ai") == tldStr) {
        return YST_KZ;
    }
    EYandexSerpTld tld;
    if (Default<TYandexSecondLevelSerpTldNames>().TryName2Id(tldStr, tld))
        return tld;
    if (Default<TYandexSerpTldNames>().TryName2Id(tldStr, tld))
        return tld;
    return YST_UNKNOWN;
}

// This function prints string representation of EYandexSerpTld
template <>
void Out<EYandexSerpTld>(IOutputStream& output, TTypeTraits<EYandexSerpTld>::TFuncParam x) {
    output << Default<TYandexSerpTldNames>().Id2Name(x);
}

TString GetYandexDomainBySerpTld(EYandexSerpTld tld) {
    TStringBuf tldStr;
    if (Default<TYandexSecondLevelSerpTldNames>().TryId2Name(tld, tldStr))
        return TString(tldStr);
    return TString(Default<TYandexSerpTldNames>().Id2Name(tld));
}
