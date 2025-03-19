#pragma once

#include <kernel/relev_locale/protos/relev_locale.pb.h>
#include <kernel/relev_locale/protos/serptld.pb.h>

#include <kernel/search_types/search_types.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/fwd.h>
#include <util/generic/strbuf.h>

// https://wiki.yandex-team.ru/JandeksPoisk/KachestvoPoiska/GetRelevanceLocale/

/*! Following snippet is here to demonstate how to iterate over elements of ERelevLocale.
 *
 * for (const auto relevLocale: NRl::GetAllLocales()) {
 *     DoSomethingWithRelevLocale(relevLocale);
 *     ....
 * }
 */


const TString& ToString(const NRl::ERelevLocale value);
bool FromString(const TStringBuf& name, NRl::ERelevLocale& ret);
NRl::ERelevLocale ERelevLocaleFromString(const TStringBuf& name);

namespace NRl {
    /*  Parent of 'universe' is 'universe'.
    *  'universe'--- 'tr'
    *           |--- 'eu'--- EU country
    *                   |--- EU country
    *                   |--- ...
    *           |--- 'xcom'--- 'world'
    *           |         |--- 'spok'
    *           |                   |--- SPOK country
    *           |                   |--- SPOK country
    *           |                   |--- ...
    *           |
    *           |
    *           |
    *           |--- 'xussr'--- 'by'
    *                      |--- 'kz'
    *                      |--- 'ua'
    *                      |--- 'ru'
    */
    ERelevLocale GetLocaleParent(const ERelevLocale value);

    // parentLevel - distance from child to parent, for example:
    // child: ru, parent: ru => level: 0
    // child: ru, parent: xussr => level: 1 and so on
    bool IsLocaleDescendantOf(const ERelevLocale child, const ERelevLocale parent,
                              size_t* parentLevel);

    inline bool IsLocaleDescendantOf(const ERelevLocale child, const ERelevLocale parent) {
        return IsLocaleDescendantOf(child, parent, nullptr);
    }

    /*! Return sorted list of locale children (only one level lower).
    * 'unknown' will be missing in the list of 'unknown' children.
    */
    const TVector<ERelevLocale>& GetLocaleChildren(const ERelevLocale parent);

    const TVector<ERelevLocale>& GetAllLocales();

    //! List of those relevance locales who have no children.
    const TVector<ERelevLocale>& GetBasicLocales();

    //! Path from self to universe in the hierarchy tree
    const TVector<ERelevLocale>& GetLocaleWithAllParents(const ERelevLocale self);

    //! Relev locale extended for SPOK
    struct TRelevLocale {
        ERelevLocale RelevLocale{RL_UNIVERSE};
        bool IsSpokCountry{false};

        constexpr TRelevLocale() = default;
        explicit constexpr TRelevLocale(const ERelevLocale locale)
            : TRelevLocale(locale, false) {
        }
        constexpr TRelevLocale(const ERelevLocale locale, const bool isSpokCountry)
            : RelevLocale(locale)
            , IsSpokCountry(isSpokCountry) {
        }
    };

    /** Relev locale evaluation algorithm.
     * @param[in] tld Yandex service top-level domain. E.g. YST_RU for https://yandex.ru
     * @param[in] language User query most probable language. E.g. relev:qmpl in wizard.
     * @param[in] country User country.
     * @param[in] spokCountries List of SPOK countries. (aka "страна по кнопке")
     * @return Corresponding relev locale.
     *
     * See also https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/getrelevancelocale/#algoritm
     */
    TRelevLocale GetLocale(const EYandexSerpTld tld, const ELanguage language,
                           const TCateg country, const THashSet<TCateg>& spokCountries);

    TRelevLocale GetLocale2(const EYandexSerpTld tld, ELanguage language,
                           const TCateg country, const ELanguage uil, const THashSet<TCateg>& spokCountries);

    /** Relev locale evaluation algorithm.
     * @see GetLocale
     */
    ERelevLocale GetLocale(const EYandexSerpTld tld, const ELanguage language,
                           const TCateg country);

    ERelevLocale GetLocale2(const EYandexSerpTld tld, const ELanguage language,
                           const TCateg country, const ELanguage uil);

    /** Search region evaluation algorithm.
     * @param[in] userRegion User region. E.g. &lr for Wizard.
     * @param[in] userCountry User country corresponding to user region. May be evaluated using
     * kernel/geodb or geobase.
     * @param[in] tld Yandex service top-level domain. E.g. YST_RU for https://yandex.ru
     * @param[in] language User query most probable language. E.g. relev:qmpl in wizard.
     * @param[in] relevLocale Relev locale evaluated with GetLocale
     * @param[in] spokCountries List of SPOK countries (aka "страна по кнопке")
     * @return Corresponding search region.
     *
     * See also https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/getrelevancelocale/#algoritm
     */
    TCateg GetSearchRegion(const TCateg userRegion, const TCateg userCountry,
                           const EYandexSerpTld tld, const ELanguage language,
                           const ERelevLocale relevLocale, const THashSet<TCateg>& spokCountries);

    /** Search region evaluation algorithm.
     * @see GetSearchRegion
     */
    TCateg GetSearchRegion(const TCateg userRegion, const TCateg userCountry,
                           const EYandexSerpTld tld, const ELanguage language,
                           const ERelevLocale relevLocale);
}  // NRl

