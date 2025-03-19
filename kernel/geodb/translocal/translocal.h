#pragma once

#include <kernel/search_types/search_types.h>
#include <kernel/geodb/protos/geodb.pb.h>
#include <kernel/geodb/translocal/protos/hack.pb.h>
#include <kernel/relev_locale/protos/serptld.pb.h>

#include <util/generic/fwd.h>

namespace NGeoDB {
    class TGeoPtr;
    class TGeoKeeper;
}

namespace NGeoDB {
    /* Return parent of `child`, taking translocality into account. E.g. for Republic of Crimea (aka
     * Automous Republic of Crimea) parent is Crimean Federal District if we are in Russia and
     * Ukraine if we are in Ukraine.
     *
     * @child               Region, whose parent is requested.
     * @tld                 Service top level domain (e.g. country whose point of view we going to
     *                      consider).
     * @geodb               Regions hierarchy, each region must have at least `Id` and
     *                      `ParentId` fields.
     * @hack [out]          First hack that was applied.
     *
     * @return              Translocal parent of `child`, if no hacks were applied will return
     *                      `ParentId` of `child`. If `child` has no parents will return default
     *                      initialized `TGeoPtr` (whose `Id` is `END_CATEG`).
     *
     * NOTE: Regions hierarchy must be russian, otherwise algorithm will work incorrectly.
     */
    TGeoPtr TranslocalParent(const TGeoPtr& child, const EYandexSerpTld tld ,
                             const TGeoKeeper& geodb, ETranslocalityHack& hack);
    TGeoPtr TranslocalParent(const TGeoPtr& child, const EYandexSerpTld tld ,
                             const TGeoKeeper& geodb);

    TGeoPtr TranslocalParent(const TCateg child, const EYandexSerpTld tld ,
                             const TGeoKeeper& geodb, ETranslocalityHack& hack);
    TGeoPtr TranslocalParent(const TCateg child, const EYandexSerpTld tld ,
                             const TGeoKeeper& geodb);

    TCateg TranslocalParentID(const TGeoPtr& child, const EYandexSerpTld tld,
                              const TGeoKeeper& geodb, ETranslocalityHack& hack);
    inline TCateg TranslocalParentID(const TGeoPtr& child, const EYandexSerpTld tld,
                                     const TGeoKeeper& geodb) {
        ETranslocalityHack hack;
        return TranslocalParentID(child, tld, geodb, hack);
    }

    TCateg TranslocalParentID(const TCateg child, const EYandexSerpTld tld,
                              const TGeoKeeper& geodb, ETranslocalityHack& hack);
    inline TCateg TranslocalParentID(const TCateg child, const EYandexSerpTld tld,
                                     const TGeoKeeper& geodb) {
        ETranslocalityHack hack;
        return TranslocalParentID(child, tld, geodb, hack);
    }

    /* Return nearest parent of `child`, whose type is not into `regionTypesToExclude`. E.g. for
     * Moscow Metro station Park Kultury (id=20490, type=`SUBWAY_STATION`) parents are (in
     * ascending order): Khamovniki District (id=120542, type=`CITY_SUBAREA`), Central Administrative
     * Okrug (id=20279, type=`CITY_AREA`), Moscow (id=213, type=`CITY`), etc; if
     * `regionTypesToExclude={CITY_SUBAREA, CITY_AREA}` then function will return Moscow.
     *
     * @child               Region, whose parent is requested.
     * @tld                 Service top level domain (e.g. country whose point of view we are going
     *                      to consider)
     * @geodb               Regions hierarchy, each region must have at least `Id`, `ParentId` and
     *                      `Type` fields.
     * @hack[out]           First hack that was applied.
     *
     * @return              First translocal parent of `child`, whose type is not in
     *                      `regionTypesToExclude`. If there are no such parents will return default
     *                      default initialized `TGeoPtr` (whose `Id` is `END_CATEG`).
     *
     * NOTE: Regions hierarchy must be russian, otherwise algorithm will work incorrectly.
     */
    TGeoPtr TranslocalParentExcludingTypes(const TGeoPtr& child, const EYandexSerpTld tld,
                                           const TGeoKeeper& geodb,
                                           const TVector<EType>& regionTypesToExclude,
                                           ETranslocalityHack& hack);
    TGeoPtr TranslocalParentExcludingTypes(const TGeoPtr& child, const EYandexSerpTld tld,
                                           const TGeoKeeper& geodb,
                                           const TVector<EType>& regionTypesToExclude);

    TGeoPtr TranslocalParentExcludingTypes(const TCateg child, const EYandexSerpTld tld,
                                           const TGeoKeeper& geodb,
                                           const TVector<EType>& regionTypesToExclude,
                                           ETranslocalityHack& hack);
    TGeoPtr TranslocalParentExcludingTypes(const TCateg child, const EYandexSerpTld tld,
                                           const TGeoKeeper& geodb,
                                           const TVector<EType>& regionTypesToExclude);

    TCateg TranslocalParentIDExcludingTypes(const TGeoPtr& child, const EYandexSerpTld tld,
                                            const TGeoKeeper& geodb,
                                            const TVector<EType>& regionTypesToExclude,
                                            ETranslocalityHack& hack);
    inline TCateg TranslocalParentIDExcludingTypes(const TGeoPtr& child, const EYandexSerpTld tld,
                                                   const TGeoKeeper& geodb,
                                                   const TVector<EType>& regionTypesToExclude) {
        ETranslocalityHack hack;
        return TranslocalParentIDExcludingTypes(child, tld, geodb, regionTypesToExclude, hack);
    }

    TCateg TranslocalParentIDExcludingTypes(const TCateg child, const EYandexSerpTld tld,
                                            const TGeoKeeper& geodb,
                                            const TVector<EType>& regionTypesToExclude,
                                            ETranslocalityHack& hack);
    inline TCateg TranslocalParentIDExcludingTypes(const TCateg child, const EYandexSerpTld tld,
                                                   const TGeoKeeper& geodb,
                                                   const TVector<EType>& regionTypesToExclude) {
        ETranslocalityHack hack;
        return TranslocalParentIDExcludingTypes(child, tld, geodb, regionTypesToExclude, hack);
    }

    /* Almost the same as `TranslocalParentExcludingTypes`, but if `region` type is not in
     * `regionTypesToExclude` will return `region`.
     */
    TGeoPtr TranslocalReduceExcludingTypes(const TGeoPtr& region, const EYandexSerpTld tld,
                                           const TGeoKeeper& geodb,
                                           const TVector<EType>& regionTypesToExclude,
                                           ETranslocalityHack& hack);
    TGeoPtr TranslocalReduceExcludingTypes(const TGeoPtr& region, const EYandexSerpTld tld,
                                           const TGeoKeeper& geodb,
                                           const TVector<EType>& regionTypesToExclude);

    TGeoPtr TranslocalReduceExcludingTypes(const TCateg region, const EYandexSerpTld tld,
                                           const TGeoKeeper& geodb,
                                           const TVector<EType>& regionTypesToExclude,
                                           ETranslocalityHack& hack);
    TGeoPtr TranslocalReduceExcludingTypes(const TCateg region, const EYandexSerpTld tld,
                                           const TGeoKeeper& geodb,
                                           const TVector<EType>& regionTypesToExclude);

    TCateg TranslocalReduceIDExcludingTypes(const TGeoPtr& region, const EYandexSerpTld tld,
                                            const TGeoKeeper& geodb,
                                            const TVector<EType>& regionTypesToExclude,
                                            ETranslocalityHack& hack);
    inline TCateg TranslocalReduceIDExcludingTypes(const TGeoPtr& region, const EYandexSerpTld tld,
                                                   const TGeoKeeper& geodb,
                                                   const TVector<EType>& regionTypesToExclude) {
        ETranslocalityHack hack;
        return TranslocalReduceIDExcludingTypes(region, tld, geodb, regionTypesToExclude, hack);
    }

    TCateg TranslocalReduceIDExcludingTypes(const TCateg region, const EYandexSerpTld tld,
                                            const TGeoKeeper& geodb,
                                            const TVector<EType>& regionTypesToExclude,
                                            ETranslocalityHack& hack);
    inline TCateg TranslocalReduceIDExcludingTypes(const TCateg region, const EYandexSerpTld tld,
                                                   const TGeoKeeper& geodb,
                                                   const TVector<EType>& regionTypesToExclude) {
        ETranslocalityHack hack;
        return TranslocalReduceIDExcludingTypes(region, tld, geodb, regionTypesToExclude, hack);
    }

    /* Return country of `id`, taking translocality into account. E.g. for Republic of Crimea (aka
     * Automous Republic of Crimea) country is Russia if we are in Russia and Ukraine if we are in
     * Ukraine.
     *
     * @id                  Region, whose country is requested.
     * @tld                 Service top level domain (e.g. country whose point of view we going to
     *                      consider).
     * @geodb               Regions hierarchy, each region must have at least `Id` and
     *                      `ParentId` fields.
     * @hack [out]          First hack was applied.
     *
     * @return              Translocal parent of `id`, if no hacks were applied will return
     *                      `CountryId` of `id`. If there are no countries above `id` will return
     *                      default initialized `TGeoPtr` (whose `Id` is `END_CATEG`).
     *
     * NOTE: Regions hierarchy must be russian, otherwise algorithm will work incorrectly.
     */
    TGeoPtr TranslocalCountry(const TGeoPtr& id, const EYandexSerpTld tld, const TGeoKeeper& geodb,
                              ETranslocalityHack& hack);
    TGeoPtr TranslocalCountry(const TGeoPtr& id, const EYandexSerpTld tld, const TGeoKeeper& geodb);

    TGeoPtr TranslocalCountry(const TCateg id, const EYandexSerpTld tld, const TGeoKeeper& geodb,
                              ETranslocalityHack& hack);
    TGeoPtr TranslocalCountry(const TCateg id, const EYandexSerpTld tld, const TGeoKeeper& geodb);

    TCateg TranslocalCountryID(const TGeoPtr& id, const EYandexSerpTld tld,
                               const TGeoKeeper& geodb, ETranslocalityHack& hack);
    inline TCateg TranslocalCountryID(const TGeoPtr& id, const EYandexSerpTld tld,
                                      const TGeoKeeper& geodb) {
        ETranslocalityHack hack;
        return TranslocalCountryID(id, tld, geodb, hack);
    }

    TCateg TranslocalCountryID(const TCateg id, const EYandexSerpTld tld, const TGeoKeeper& geodb,
                               ETranslocalityHack& hack);
    inline TCateg TranslocalCountryID(const TCateg id, const EYandexSerpTld tld,
                                      const TGeoKeeper& geodb) {
        ETranslocalityHack hack;
        return TranslocalCountryID(id, tld, geodb, hack);
    }


    /* Check if `child` lies in `probableParent` subtree, taking translocality into account.
     *
     * @child               Child identifier
     * @probableParent      Probable parent identifier
     * @tld                 Service top level domain (e.g. country whose point of view we going to
     *                      consider).
     * @geodb               Regions hierarchy, each region must have at least `Id` and
     *                      `ParentId` fields.
     * @hack[out]           First hack that was applied.
     *
     * @return              `true` if `child` lies in `probableParent` subtree, will return `true`
     *                      if `child == probableParent`, will return `false` in other cases.
     *
     * NOTE: Regions hierarchy must be russian, otherwise algorithm will work incorrectly.
     */
    bool TranslocalIsIn(const TGeoPtr& child, const TGeoPtr& probableParent,
                        const EYandexSerpTld tld, const TGeoKeeper& geodb,
                        ETranslocalityHack& hack);
    inline bool TranslocalIsIn(const TGeoPtr& child, const TGeoPtr& probableParent,
                               const EYandexSerpTld tld, const TGeoKeeper& geodb) {
        ETranslocalityHack hack;
        return TranslocalIsIn(child, probableParent, tld, geodb, hack);
    }

    bool TranslocalIsIn(const TCateg child, const TCateg probableParent,
                        const EYandexSerpTld tld, const TGeoKeeper& geodb,
                        ETranslocalityHack& hack);
    inline bool TranslocalIsIn(const TCateg child, const TCateg probableParent,
                               const EYandexSerpTld tld, const TGeoKeeper& geodb) {
        ETranslocalityHack hack;
        return TranslocalIsIn(child, probableParent, tld, geodb, hack);
    }

    bool TranslocalIsIn(const TGeoPtr& child, const TCateg probableParent,
                        const EYandexSerpTld tld, const TGeoKeeper& geodb,
                        ETranslocalityHack& hack);
    inline bool TranslocalIsIn(const TGeoPtr& child, const TCateg probableParent,
                               const EYandexSerpTld tld, const TGeoKeeper& geodb) {
        ETranslocalityHack hack;
        return TranslocalIsIn(child, probableParent, tld, geodb, hack);
    }

    bool TranslocalIsIn(const TCateg child, const TGeoPtr& probableParent,
                        const EYandexSerpTld tld, const TGeoKeeper& geodb,
                        ETranslocalityHack& hack);
    inline bool TranslocalIsIn(const TCateg child, const TGeoPtr& probableParent,
                               const EYandexSerpTld tld, const TGeoKeeper& geodb) {
        ETranslocalityHack hack;
        return TranslocalIsIn(child, probableParent, tld, geodb, hack);
    }
}  // namespace NGeoDB
