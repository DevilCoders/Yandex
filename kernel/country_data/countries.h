#pragma once

#include <kernel/search_types/search_types.h> // TCateg
#include <kernel/relev_locale/protos/relev_locale.pb.h>
#include <kernel/relev_locale/protos/serptld.pb.h>

#include <library/cpp/langmask/langmask.h>

#include <util/generic/fwd.h>
#include <util/generic/vector.h>

/**
 * Country related information.
 */
struct TCountryData {
    TCateg CountryId;                       /// Country id from geoa.c2p
    TString NationalDomain;                  /// National domain for country. Like "ru" for Russia.
    ui8 ErfCode;                            /// Code to store country id in Erf. NOTE: zero means UNKNOWN country!
    TVector<TString> WikipediaHosts;         /// Wikipedia host for country. Like "ru.wikipedia.org" for Russia.
    TLangMask NationalLanguage;             /// National language for country. Like LANG_RUS for Russia
    NRl::ERelevLocale RelevLocale; /// Relev locale of country
    bool IsRelevCountry;                    /// Pass country identifier to cgi
    bool IsLocalizable;                     /// Determine query localiztion in the country
    EYandexSerpTld SerpTld;

    TCountryData(const TCateg, const TString&, const ui8, const TString&, const TLangMask,
                 NRl::ERelevLocale relevLocale, bool isRelevCountry = false,
                 bool isLocalizable = false, EYandexSerpTld tld = YST_UNKNOWN);

    operator TCateg() const;
};


// Put below countries.
extern const TCountryData   COUNTRY_RUSSIA;
extern const TCountryData   COUNTRY_UKRAINE;
extern const TCountryData   COUNTRY_KAZAKHSTAN;
extern const TCountryData   COUNTRY_BELARUS;
extern const TCountryData   COUNTRY_TURKEY;
extern const TCountryData   COUNTRY_POLAND;
extern const TCountryData   COUNTRY_LITHUANIA;
extern const TCountryData   COUNTRY_LATVIA;
extern const TCountryData   COUNTRY_ESTONIYA;
extern const TCountryData   COUNTRY_MOLDOVA;
extern const TCountryData   COUNTRY_ROMANIA;
extern const TCountryData   COUNTRY_GERMANY;
extern const TCountryData   COUNTRY_USA;
extern const TCountryData   COUNTRY_GEORGIA;
extern const TCountryData   COUNTRY_UZBEKISTAN;
extern const TCountryData   COUNTRY_ISRAEL;
extern const TCountryData   COUNTRY_FRANCE;
extern const TCountryData   COUNTRY_CANADA;
extern const TCountryData   COUNTRY_UNITED_KINGDOM;
extern const TCountryData   COUNTRY_HUNGARY;
extern const TCountryData   COUNTRY_AZERBAIJAN;
extern const TCountryData   COUNTRY_BULGARIA;
extern const TCountryData   COUNTRY_INDIA;
extern const TCountryData   COUNTRY_CZECH;
extern const TCountryData   COUNTRY_ITALY;
extern const TCountryData   COUNTRY_SAUDI_ARABIA;
extern const TCountryData   COUNTRY_VIETNAM;
extern const TCountryData   COUNTRY_DENMARK;
extern const TCountryData   COUNTRY_INDONESIA;
extern const TCountryData   COUNTRY_IRELAND;
extern const TCountryData   COUNTRY_SPAIN;
extern const TCountryData   COUNTRY_CHINA;
extern const TCountryData   COUNTRY_SOUTH_KOREA;
extern const TCountryData   COUNTRY_NETHERLANDS;
extern const TCountryData   COUNTRY_NORWAY;
extern const TCountryData   COUNTRY_PORTUGAL;
extern const TCountryData   COUNTRY_THAILAND;
extern const TCountryData   COUNTRY_PHILIPPINES;
extern const TCountryData   COUNTRY_JAPAN;
extern const TCountryData   COUNTRY_AUSTRALIA;
extern const TCountryData   COUNTRY_BELGIUM;
extern const TCountryData   COUNTRY_BRAZIL;
extern const TCountryData   COUNTRY_EGYPT;
extern const TCountryData   COUNTRY_MEXICO;
extern const TCountryData   COUNTRY_NEW_ZEALAND;
extern const TCountryData   COUNTRY_PUERTO_RICO;
extern const TCountryData   COUNTRY_SWITZERLAND;
extern const TCountryData   COUNTRY_MALAYSIA;
extern const TCountryData   COUNTRY_ARMENIA;
extern const TCountryData   COUNTRY_KYRGYZSTAN;
extern const TCountryData   COUNTRY_TAJIKISTAN;
extern const TCountryData   COUNTRY_TURKMENISTAN;
extern const TCountryData   COUNTRY_SERBIA;
extern const TCountryData   COUNTRY_COTE_D_IVOIRE;
extern const TCountryData   COUNTRY_FINLAND;
extern const TCountryData   COUNTRY_AUSTRIA;
extern const TCountryData   COUNTRY_CROATIA;
extern const TCountryData   COUNTRY_CYPRUS;
extern const TCountryData   COUNTRY_GREECE;
extern const TCountryData   COUNTRY_ICELAND;
extern const TCountryData   COUNTRY_LIECHTENSTEIN;
extern const TCountryData   COUNTRY_LUXEMBOURG;
extern const TCountryData   COUNTRY_MALTA;
extern const TCountryData   COUNTRY_SLOVAKIA;
extern const TCountryData   COUNTRY_SLOVENIA;
extern const TCountryData   COUNTRY_SWEDEN;
extern const TCountryData   COUNTRY_CAMEROON;
extern const TCountryData   COUNTRY_SENEGAL;
extern const TCountryData   COUNTRY_ANGOLA;

/* static */ const TCateg         COUNTRY_INVALID = END_CATEG;


const THashSet<TCateg>& GetRelevCountries();
bool IsCountry(TCateg country);
const TCountryData* GetCountryData(TCateg country);
ui8 Country2ErfCode(TCateg country);
TCateg ErfCode2Country(ui8 erfCode);
void Lang2Country(ELanguage, TVector<TCateg>&);
TCateg NationalDomain2Country(const TString& nationalDomain);
const TCountryData* Lang2SPOKCountry(ELanguage);
TCateg RelevLocale2Country(NRl::ERelevLocale relevLocale);
TCateg GetCountryByTld(EYandexSerpTld tld);

/**
 * Put to this class global country-related methods.
 */
struct TRelevCountryInfo
{
    typedef ui8 TNationalDomainId; /// National domain identifier to store in Erf.
    typedef TVector<const TCountryData*> TCountryDataArr;


    /**
     * @return non zero national domain id if host domain zone is marked as national.
     * See realization of TCountriesStaticData::InitNationalData() in cpp file to see which domain zones are national.
     */
    static TNationalDomainId GetNationalDomainId(const TString& host);

    /**
     * @param[in] countryId user's country id.
     * @param[in] domainId host domain id from Erf.
     *
     * @return true if countryId corresponds to specified national domain.
     */
    static bool IsDomainNational(const TCateg countryId, const TNationalDomainId domainId);

    /**
     * @param[in] countryId user's country id.
     * @param[in] language document's language from Erf.
     *
     * @return true if countryId corresponds to specified language.
     */
    static bool IsLanguageNational(const TCateg countryId, const ELanguage language);

    /**
     * @return array with all countries which we are note in search. See const TCountryData COUNTRY_* global variables.
     */
    static const TCountryDataArr& GetCountryDataArr();
};

inline TCountryData::operator TCateg() const
{
    return CountryId;
}
