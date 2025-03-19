#include "countries.h"

#include <kernel/search_types/search_types.h>
#include <kernel/relev_locale/protos/relev_locale.pb.h>

#include <library/cpp/langmask/langmask.h>

#include <util/generic/map.h>
#include <util/generic/hash_set.h>
#include <util/generic/singleton.h>
#include <library/cpp/deprecated/split/split_iterator.h>
#include <library/cpp/string_utils/url/url.h>

// static data
namespace {

class TCountriesStaticData {

public:

    typedef TMap<TString, TRelevCountryInfo::TNationalDomainId> TZone2NationalIdMap;
    typedef TMap<TCateg, size_t> TCountry2DataArrNum;
    typedef TVector<TVector<TCateg> > TData2CountriesArr;

    THashSet<TCateg> Countries;
    TVector<bool> IsCountry;
    TVector<ui8> ErfCodes;
    TRelevCountryInfo::TCountryDataArr              CountryDataArr;
    TCountry2DataArrNum                             Country2DataArrNum;
    TZone2NationalIdMap                             NationalDomain2Id;
    TVector<TRelevCountryInfo::TNationalDomainId>   IsNational;
    TVector<TLangMask>                              CountryLanguage;
    TVector<TCateg>                                 CountriesByErfCode;
    TData2CountriesArr                              CountriesByLang;
    TVector<TCateg>                                 CountriesByRelevLocale;
    THashMap<EYandexSerpTld, TCateg>               SerpTld2Country;

    void Check(const TCountryData&) const;
    void RegisterCountryData(const TCountryData&);

    static inline const TCountriesStaticData& Instance() {
        return *Singleton<TCountriesStaticData>();
    }
};

template <typename T> void EnsureSize(TVector<T>& vec, const size_t size, const T &defaultValue)
{
    if ( vec.size() <= size ) {
        vec.resize(2 * size + 1, defaultValue);
    }
}

void TCountriesStaticData::Check(const TCountryData& countryData) const
{
    if ( 0 == countryData.ErfCode ) {
        Cerr << "TCountryData " << countryData.CountryId << " has zero ErfCode" << Endl;
        exit(1);
    }
    if ( NationalDomain2Id.find(countryData.NationalDomain) != NationalDomain2Id.end() ) {
        Cerr << "TCountryData with NationalDomain " << countryData.NationalDomain << " already registered" << Endl;exit(1);
    }
}

void TCountriesStaticData::RegisterCountryData(const TCountryData& countryData)
{
    Check(countryData);

    size_t arrNum = CountryDataArr.size();
    CountryDataArr.push_back(&countryData);

    const TCateg countryId  = countryData.CountryId;
    const size_t index      = static_cast<const size_t>(countryId);
    Country2DataArrNum[countryId] = arrNum;

    Countries.insert(countryId);

    EnsureSize(IsCountry, index, false);
    IsCountry[index] = true;

    NationalDomain2Id[countryData.NationalDomain] = static_cast<TRelevCountryInfo::TNationalDomainId>(countryData.ErfCode);

    EnsureSize(IsNational, index, static_cast<TRelevCountryInfo::TNationalDomainId>(0));
    IsNational[index] = static_cast<TRelevCountryInfo::TNationalDomainId>(countryData.ErfCode);

    EnsureSize(CountryLanguage, index, TLangMask());
    CountryLanguage[index] = countryData.NationalLanguage;

    EnsureSize(ErfCodes, index, static_cast<ui8>(0));
    ErfCodes[index] = countryData.ErfCode;

    EnsureSize(CountriesByErfCode, countryData.ErfCode, COUNTRY_INVALID);
    CountriesByErfCode[countryData.ErfCode] = countryId;

    EnsureSize(CountriesByRelevLocale, countryData.RelevLocale, COUNTRY_INVALID);
    CountriesByRelevLocale[countryData.RelevLocale] = countryId;

    TVector<TCateg> defLangArr;
    const TLangMask& langMask = countryData.NationalLanguage;
    for (ELanguage lang : langMask) {
        EnsureSize(CountriesByLang, lang, defLangArr);
        CountriesByLang[lang].push_back(countryId);
    }

    if (countryData.SerpTld != YST_UNKNOWN) {
        SerpTld2Country[countryData.SerpTld] = countryData.CountryId;
    }
}

} // namespace

#define STATIC_DATA (TCountriesStaticData::Instance())

const THashSet<TCateg>& GetRelevCountries() {
    return STATIC_DATA.Countries;
}

bool IsCountry(TCateg region) {
    if (region >= 0 && region < (TCateg)STATIC_DATA.IsCountry.size())
        return STATIC_DATA.IsCountry[(size_t)region];
    return false;
}

const TCountryData* GetCountryData(TCateg country) {
    TCountriesStaticData::TCountry2DataArrNum::const_iterator i = STATIC_DATA.Country2DataArrNum.find(country);
    if (i != STATIC_DATA.Country2DataArrNum.end() && i->second < STATIC_DATA.CountryDataArr.size())
        return STATIC_DATA.CountryDataArr[i->second];
    return nullptr;
}

TCateg GetCountryByTld(EYandexSerpTld tld) {
    const auto i = STATIC_DATA.SerpTld2Country.find(tld);
    if (i != STATIC_DATA.SerpTld2Country.end()) {
        return i->second;
    } else {
        return END_CATEG;
    }
}

TRelevCountryInfo::TNationalDomainId TRelevCountryInfo::GetNationalDomainId(const TString& host)
{
    TCountriesStaticData::TZone2NationalIdMap::const_iterator it = STATIC_DATA.NationalDomain2Id.find(GetZone(host));
    if ( STATIC_DATA.NationalDomain2Id.end() == it ) {
        return 0;
    }
    return it->second;
}

bool TRelevCountryInfo::IsDomainNational(const TCateg countryId, const TNationalDomainId domainId)
{
    if ( domainId == 0 ) {
        return false;
    }
    if ( countryId >= 0 && countryId < static_cast<const TCateg>(STATIC_DATA.IsNational.size()) ) {
        return domainId == STATIC_DATA.IsNational[static_cast<size_t>(countryId)];
    }
    return false;
}

bool TRelevCountryInfo::IsLanguageNational(const TCateg countryId, const ELanguage language)
{
    if (language == LANG_UNK) {
        return false;
    }
    if (countryId >= 0 && static_cast<size_t>(countryId) < STATIC_DATA.CountryLanguage.size()) {
        return STATIC_DATA.CountryLanguage[static_cast<size_t>(countryId)].Test(language);
    } else {
        return false;
    }
}

const TRelevCountryInfo::TCountryDataArr& TRelevCountryInfo::GetCountryDataArr()
{
    return STATIC_DATA.CountryDataArr;
}

ui8 Country2ErfCode(TCateg country)
{
    if (country >= 0 && static_cast<size_t>(country) < STATIC_DATA.ErfCodes.size())
        return STATIC_DATA.ErfCodes[static_cast<size_t>(country)];
    else
        return 0;
}

TCateg RelevLocale2Country(NRl::ERelevLocale relevLocale) {
    if (static_cast<size_t>(relevLocale) < STATIC_DATA.CountriesByRelevLocale.size())
        return STATIC_DATA.CountriesByRelevLocale[relevLocale];
    else
        return COUNTRY_INVALID;
}

TCateg ErfCode2Country(ui8 erfCode)
{
    if (erfCode < STATIC_DATA.CountriesByErfCode.size())
        return STATIC_DATA.CountriesByErfCode[erfCode];
    else
        return COUNTRY_INVALID;
}

void Lang2Country(ELanguage lang, TVector<TCateg>& countries)
{
    if ((size_t)lang < STATIC_DATA.CountriesByLang.size()) {
        countries = STATIC_DATA.CountriesByLang[lang];
    }
}

TCateg NationalDomain2Country(const TString& nationalDomain)
{
    TCountriesStaticData::TZone2NationalIdMap::const_iterator it = STATIC_DATA.NationalDomain2Id.find(nationalDomain);
    if (it == STATIC_DATA.NationalDomain2Id.end()) {
        return COUNTRY_INVALID;
    }
    return ErfCode2Country(it->second);
}

#undef STATIC_DATA

TCountryData::TCountryData(const TCateg countryId, const TString& nationalDomain, const ui8 erfCode,
                           const TString& wikipediaHosts, const TLangMask nationalLanguage,
                           NRl::ERelevLocale relevLocale, bool isRelevCountry,
                           bool isLocalizable, const EYandexSerpTld tld)
    : CountryId(countryId)
    , NationalDomain(nationalDomain)
    , ErfCode(erfCode)
    , NationalLanguage(nationalLanguage)
    , RelevLocale(relevLocale)
    , IsRelevCountry(isRelevCountry)
    , IsLocalizable(isLocalizable)
    , SerpTld(tld)
{
    const TSplitDelimiters delims(",");
    const TDelimitersStrictSplit split(wikipediaHosts, delims);
    Split(split, &WikipediaHosts);

    Singleton<TCountriesStaticData>()->RegisterCountryData(*this);
}

const TCountryData* Lang2SPOKCountry(ELanguage lang) {
    switch (lang) {
        case LANG_ARA:
            return &COUNTRY_SAUDI_ARABIA;
        case LANG_HUN:
            return &COUNTRY_HUNGARY;
        case LANG_VIE:
            return &COUNTRY_VIETNAM;
        case LANG_DAN:
            return &COUNTRY_DENMARK;
        case LANG_IND:
            return &COUNTRY_INDONESIA;
        case LANG_SPA:
            return &COUNTRY_SPAIN;
        case LANG_ITA:
            return &COUNTRY_ITALY;
        case LANG_CHI:
            return &COUNTRY_CHINA;
        case LANG_KOR:
            return &COUNTRY_SOUTH_KOREA;
        case LANG_GER:
            return &COUNTRY_GERMANY;
        case LANG_DUT:
            return &COUNTRY_NETHERLANDS;
        case LANG_NOR:
            return &COUNTRY_NORWAY;
        case LANG_POL:
            return &COUNTRY_POLAND;
        case LANG_POR:
            return &COUNTRY_PORTUGAL;
        case LANG_RUS:
            return &COUNTRY_RUSSIA;
        case LANG_THA:
            return &COUNTRY_THAILAND;
        case LANG_FRE:
            return &COUNTRY_FRANCE;
        case LANG_JPN:
            return &COUNTRY_JAPAN;
        case LANG_MAY:
            return &COUNTRY_MALAYSIA;
        case LANG_UKR:
            return &COUNTRY_UKRAINE;
        case LANG_BEL:
            return &COUNTRY_BELARUS;
        case LANG_KAZ:
            return &COUNTRY_KAZAKHSTAN;
        case LANG_AZE:
            return &COUNTRY_AZERBAIJAN;
        case LANG_ARM:
            return &COUNTRY_ARMENIA;
        case LANG_GEO:
            return &COUNTRY_GEORGIA;
        case LANG_KIR:
            return &COUNTRY_KYRGYZSTAN;
        case LANG_LAV:
            return &COUNTRY_LATVIA;
        case LANG_LIT:
            return &COUNTRY_LITHUANIA;
        case LANG_TGK:
            return &COUNTRY_TAJIKISTAN;
        case LANG_TUK:
            return &COUNTRY_TURKMENISTAN;
        case LANG_UZB:
            return &COUNTRY_UZBEKISTAN;
        case LANG_EST:
            return &COUNTRY_ESTONIYA;
        case LANG_HEB:
            return &COUNTRY_ISRAEL;
        case LANG_TUR:
            return &COUNTRY_TURKEY;
        case LANG_FIN:
            return &COUNTRY_FINLAND;
        case LANG_HRV:
            return &COUNTRY_CROATIA;
        case LANG_SLO:
            return &COUNTRY_SLOVAKIA;
        case LANG_SLV:
            return &COUNTRY_SLOVENIA;
        case LANG_RUM:
            return &COUNTRY_ROMANIA;
        case LANG_MLT:
            return &COUNTRY_MALTA;
        case LANG_LTZ:
            return &COUNTRY_LUXEMBOURG;
        case LANG_GRE:
            return &COUNTRY_GREECE;
        case LANG_CZE:
            return &COUNTRY_CZECH;
        case LANG_BUL:
            return &COUNTRY_BULGARIA;
        default:
            return nullptr;
    }
}

using namespace NRl;

const TCountryData COUNTRY_RUSSIA         (225, "ru", 1, "ru.wikipedia.org", LANG_RUS, RL_RU, true, true, YST_RU);
const TCountryData COUNTRY_UKRAINE        (187, "ua", 2, "uk.wikipedia.org", LANG_UKR, RL_UA, true, true, YST_UA);
const TCountryData COUNTRY_KAZAKHSTAN     (159, "kz", 3, "kk.wikipedia.org", LANG_KAZ, RL_KZ, true, true, YST_KZ);
const TCountryData COUNTRY_BELARUS        (149, "by", 4, "be.wikipedia.org,be-x-old.wikipedia.org", LANG_BEL, RL_BY, true, true, YST_BY);
const TCountryData COUNTRY_TURKEY         (983, "tr", 5, "tr.wikipedia.org", LANG_TUR, RL_TR, true, true, YST_TR);
const TCountryData COUNTRY_POLAND         (120, "pl", 6, "pl.wikipedia.org", LANG_POL, RL_PL, true, true, YST_PL);
const TCountryData COUNTRY_LITHUANIA      (117, "lt", 7, "lt.wikipedia.org", LANG_LIT, RL_LT, true, true, YST_LT);
const TCountryData COUNTRY_LATVIA         (206, "lv", 8, "lv.wikipedia.org", LANG_LAV, RL_LV, true, true, YST_LV);
const TCountryData COUNTRY_ESTONIYA       (179, "ee", 9, "et.wikipedia.org", LANG_EST, RL_EE, true, true, YST_EE);
const TCountryData COUNTRY_MOLDOVA        (208, "md", 10, "mo.wikipedia.org", LANG_RUM, RL_MD, true, true, YST_MD);
const TCountryData COUNTRY_ROMANIA        (10077, "ro", 11, "ro.wikipedia.org", LANG_RUM, RL_RO, true, true, YST_EU);
const TCountryData COUNTRY_GERMANY        (96, "de", 12, "de.wikipedia.org", LANG_GER, RL_DE, true, true, YST_EU);
const TCountryData COUNTRY_USA            (84, "us", 13, "en.wikipedia.org", LANG_ENG, RL_US, true);
const TCountryData COUNTRY_GEORGIA        (169, "ge", 14, "ka.wikipedia.org", LANG_GEO, RL_GE, true, true, YST_GE);
const TCountryData COUNTRY_UZBEKISTAN     (171, "uz", 15, "uz.wikipedia.org", LANG_UZB, RL_UZ, true, true, YST_UZ);
const TCountryData COUNTRY_ISRAEL         (181, "il", 16, "he.wikipedia.org", LANG_HEB, RL_IL, true, true, YST_IL);
const TCountryData COUNTRY_FRANCE         (124, "fr", 17, "fr.wikipedia.org", LANG_FRE, RL_FR, true, true, YST_EU);
const TCountryData COUNTRY_CANADA         (95, "ca", 18,  "en.wikipedia.org,fr.wikipedia.org", TLangMask(LANG_ENG, LANG_FRE), RL_CA);
const TCountryData COUNTRY_UNITED_KINGDOM (102, "uk", 19, "en.wikipedia.org", LANG_ENG, RL_GB, true, true, YST_EU);
const TCountryData COUNTRY_HUNGARY        (116, "hu", 20, "hu.wikipedia.org", LANG_HUN, RL_HU, true, true, YST_EU);
const TCountryData COUNTRY_AZERBAIJAN     (167, "az", 21, "az.wikipedia.org", LANG_AZE, RL_AZ, true, true, YST_AZ);
const TCountryData COUNTRY_BULGARIA       (115, "bg", 22, "bg.wikipedia.org", LANG_BUL, RL_BG, true, true, YST_EU);
const TCountryData COUNTRY_INDIA          (994, "in", 23, "ta.wikipedia.org,hi.wikipedia.org", LANG_ENG, RL_IN);
const TCountryData COUNTRY_CZECH          (125, "cz", 24, "cs.wikipedia.org", LANG_CZE, RL_CZ, true, true, YST_EU);
const TCountryData COUNTRY_ITALY          (205, "it", 25, "it.wikipedia.org", LANG_ITA, RL_IT, true, true, YST_EU);
const TCountryData COUNTRY_SAUDI_ARABIA   (10540, "sa", 26, "ar.wikipedia.org", LANG_ARA, RL_SA);
const TCountryData COUNTRY_VIETNAM        (10093, "vn", 27, "vi.wikipedia.org", LANG_VIE, RL_VN);
const TCountryData COUNTRY_DENMARK        (203, "dk", 28, "da.wikipedia.org", LANG_DAN, RL_DK, true, true, YST_EU);
const TCountryData COUNTRY_INDONESIA      (10095, "id", 29, "id.wikipedia.org", LANG_IND, RL_ID);
const TCountryData COUNTRY_IRELAND        (10063, "ie", 30, "ga.wikipedia.org", LANG_ENG, RL_IE, true, true, YST_EU);
const TCountryData COUNTRY_SPAIN          (204, "es", 31, "es.wikipedia.org", LANG_SPA, RL_ES, true, true, YST_EU);
const TCountryData COUNTRY_CHINA          (134, "cn", 32, "zh.wikipedia.org", LANG_CHI, RL_CN);
const TCountryData COUNTRY_SOUTH_KOREA    (135, "kr", 33, "ko.wikipedia.org", LANG_KOR, RL_KR);
const TCountryData COUNTRY_NETHERLANDS    (118, "nl", 34, "nl.wikipedia.org", LANG_DUT, RL_NL, true, true, YST_EU);
const TCountryData COUNTRY_NORWAY         (119, "no", 35, "no.wikipedia.org", LANG_NOR, RL_NO, true, true, YST_EU);
const TCountryData COUNTRY_PORTUGAL       (10074, "pt", 36, "pt.wikipedia.org", LANG_POR, RL_PT, true, true, YST_EU);
const TCountryData COUNTRY_THAILAND       (995, "th", 37, "th.wikipedia.org", LANG_THA, RL_TH);
const TCountryData COUNTRY_PHILIPPINES    (10108, "ph", 38, "tl.wikipedia.org", LANG_ENG, RL_PH);
const TCountryData COUNTRY_JAPAN          (137, "jp", 39, "ja.wikipedia.org", LANG_JPN, RL_JP);
const TCountryData COUNTRY_AUSTRALIA      (211, "au", 40, "en.wikipedia.org", LANG_ENG, RL_AU);
const TCountryData COUNTRY_BELGIUM        (114, "be", 41, "nl.wikipedia.org,fr.wikipedia.org", TLangMask(LANG_DUT, LANG_FRE), RL_BE, true, true, YST_EU);
const TCountryData COUNTRY_BRAZIL         (94, "br", 42, "pt.wikipedia.org", LANG_POR, RL_BR);
const TCountryData COUNTRY_EGYPT          (1056, "eg", 43, "ar.wikipedia.org", LANG_ARA, RL_EG);
const TCountryData COUNTRY_MEXICO         (20271, "mx", 44, "es.wikipedia.org", LANG_SPA, RL_MX);
const TCountryData COUNTRY_NEW_ZEALAND    (139, "nz", 45, "en.wikipedia.org", LANG_ENG, RL_NZ);
const TCountryData COUNTRY_PUERTO_RICO    (20764, "pr", 46, "es.wikipedia.org", LANG_SPA, RL_PR);
const TCountryData COUNTRY_SWITZERLAND    (126, "ch", 47, "de.wikipedia.org,fr.wikipedia.org", TLangMask(LANG_GER, LANG_FRE, LANG_ITA), RL_CH);
const TCountryData COUNTRY_MALAYSIA       (10097, "my", 48, "ms.wikipedia.org", LANG_MAY, RL_MY);
const TCountryData COUNTRY_ARMENIA        (168, "am", 49, "am.wikipedia.org", LANG_ARM, RL_AM, true, true, YST_AM);
const TCountryData COUNTRY_KYRGYZSTAN     (207, "kg", 50, "ky.wikipedia.org", LANG_KIR, RL_KG, true, true, YST_KG);
const TCountryData COUNTRY_TAJIKISTAN     (209, "tj", 51, "tg.wikipedia.org", LANG_TGK, RL_TJ, true, true, YST_TJ);
const TCountryData COUNTRY_TURKMENISTAN   (170, "tm", 52, "tk.wikipedia.org", LANG_TUK, RL_TM, true, true, YST_TM);
const TCountryData COUNTRY_SERBIA         (180, "rs", 53, "sr.wikipedia.org", TLangMask(LANG_SRP, LANG_HRV), RL_RS);
const TCountryData COUNTRY_COTE_D_IVOIRE  (20733, "ci", 54, "fr.wikipedia.org", LANG_FRE, RL_CI);
const TCountryData COUNTRY_AUSTRIA        (113, "at", 55, "de.wikipedia.org", LANG_GER, RL_AT, true, true, YST_EU);
const TCountryData COUNTRY_CROATIA        (10083, "hr", 56, "hr.wikipedia.org", LANG_HRV, RL_HR, true, true, YST_EU);
const TCountryData COUNTRY_CYPRUS         (20574, "cy", 57, "gr.wikipedia.org", LANG_GRE, RL_CY, true, true, YST_EU);
const TCountryData COUNTRY_FINLAND        (123, "fi", 58, "fi.wikipedia.org", TLangMask(LANG_FIN, LANG_SWE), RL_FI, true, true, YST_FI);
const TCountryData COUNTRY_GREECE         (246, "gr", 59, "el.wikipedia.org", LANG_GRE, RL_GR, true, true, YST_EU);
const TCountryData COUNTRY_ICELAND        (10064, "is", 60, "is.wikipedia.org", LANG_ICE, RL_IS, true, true, YST_EU);
const TCountryData COUNTRY_LIECHTENSTEIN  (10067, "li", 61, "de.wikipedia.org", LANG_GER, RL_LI, true, true, YST_EU);
const TCountryData COUNTRY_LUXEMBOURG     (21203, "lu", 62, "de.wikipedia.org", TLangMask(LANG_GER, LANG_FRE, LANG_LTZ), RL_LU, true, true, YST_EU);
const TCountryData COUNTRY_MALTA          (10069, "mt", 63, "mt.wikipedia.org", LANG_MLT, RL_MT, true, true, YST_EU);
const TCountryData COUNTRY_SLOVAKIA       (121, "sk", 64, "sk.wikipedia.org", LANG_SLO, RL_SK, true, true, YST_EU);
const TCountryData COUNTRY_SLOVENIA       (122, "si", 65, "sl.wikipedia.org", LANG_SLV, RL_SI, true, true, YST_EU);
const TCountryData COUNTRY_SWEDEN         (127, "se", 66, "sv.wikipedia.org", LANG_SWE, RL_SE, true, true, YST_EU);
const TCountryData COUNTRY_CAMEROON       (20736, "cm", 67, "cm.wikipedia.org", LANG_FRE, RL_CM);
const TCountryData COUNTRY_SENEGAL        (21441, "sn", 68, "sn.wikipedia.org", LANG_FRE, RL_SN);
const TCountryData COUNTRY_ANGOLA         (21182, "ao", 69, "pt.wikipedia.org", LANG_POR, RL_AO);
