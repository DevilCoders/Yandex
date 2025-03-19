#include "translocal.h"

#include <kernel/geodb/countries.h>
#include <kernel/geodb/geodb.h>

#include <util/generic/vector.h>
#include <util/system/compiler.h>

// https://geoadmin.yandex-team.ru/#region:977
static constexpr TCateg REPUBLIC_OF_CRIMEA = 977;

static constexpr bool IsRBK(const EYandexSerpTld tld) noexcept {
    return YST_RU == tld || YST_BY == tld || YST_KZ == tld;
}

NGeoDB::TGeoPtr NGeoDB::TranslocalParent(const TGeoPtr& child, const EYandexSerpTld tld,
                                         const TGeoKeeper& geodb, ETranslocalityHack& hack) {
    /* It is expected that regions hierarchy in `geodb` will match Russian point of view, e.g.:
     * Republic of Crimea (977) -> Crimean Federal District (115092) -> Russia (225)
     *
     * While Ukranian point of view is:
     * Autonomous Republic of Crimea (977) -> Ukraine (187)
     */
    if (Y_UNLIKELY(REPUBLIC_OF_CRIMEA == child->GetId())) {
        if (!IsRBK(tld)) {
            hack = CRIMEA_HACK;
            return geodb.Find(UKRAINE_ID);
        }
    }

    hack = NO_HACK;
    return child.Parent();
}

NGeoDB::TGeoPtr NGeoDB::TranslocalParent(const TGeoPtr& child, const EYandexSerpTld tld,
                                         const TGeoKeeper& geodb) {
    ETranslocalityHack hack;
    return TranslocalParent(child, tld, geodb, hack);
}

NGeoDB::TGeoPtr NGeoDB::TranslocalParent(const TCateg child, const EYandexSerpTld tld,
                                         const TGeoKeeper& geodb, ETranslocalityHack& hack) {
    const auto region = geodb.Find(child);
    return TranslocalParent(region, tld, geodb, hack);
}

NGeoDB::TGeoPtr NGeoDB::TranslocalParent(const TCateg child, const EYandexSerpTld tld,
                                         const TGeoKeeper& geodb) {
    ETranslocalityHack hack;
    return TranslocalParent(child, tld, geodb, hack);
}

TCateg NGeoDB::TranslocalParentID(const TGeoPtr& child, const EYandexSerpTld tld,
                                  const TGeoKeeper& geodb, ETranslocalityHack& hack) {
    return TranslocalParent(child, tld, geodb, hack)->GetId();
}

TCateg NGeoDB::TranslocalParentID(const TCateg child, const EYandexSerpTld tld,
                                  const TGeoKeeper& geodb, ETranslocalityHack& hack) {
    return TranslocalParent(child, tld, geodb, hack)->GetId();
}

NGeoDB::TGeoPtr NGeoDB::TranslocalCountry(const TGeoPtr& id, const EYandexSerpTld tld,
                                          const TGeoKeeper& geodb, ETranslocalityHack& hack) {
    auto curHack = NGeoDB::NO_HACK;
    auto hackSet = false;
    auto region = id;
    while (region) {
        if (region->GetType() == COUNTRY) {
            return region;
        }

        region = TranslocalParent(region, tld, geodb, curHack);
        if (!hackSet && NGeoDB::NO_HACK != curHack) {
            hack = curHack;
            hackSet = true;
        }
    }

    return {};
}

NGeoDB::TGeoPtr NGeoDB::TranslocalCountry(const TGeoPtr& id, const EYandexSerpTld tld,
                                          const TGeoKeeper& geodb) {
    ETranslocalityHack hack;
    return TranslocalCountry(id, tld, geodb, hack);
}

NGeoDB::TGeoPtr NGeoDB::TranslocalCountry(const TCateg id, const EYandexSerpTld tld,
                                          const TGeoKeeper& geodb, ETranslocalityHack& hack) {
    const auto region = geodb.Find(id);
    return TranslocalCountry(region, tld, geodb, hack);
}

NGeoDB::TGeoPtr NGeoDB::TranslocalCountry(const TCateg id, const EYandexSerpTld tld,
                                          const TGeoKeeper& geodb) {
    ETranslocalityHack hack;
    return TranslocalCountry(id, tld, geodb, hack);
}

TCateg NGeoDB::TranslocalCountryID(const TGeoPtr& id, const EYandexSerpTld tld,
                                   const TGeoKeeper& geodb, ETranslocalityHack& hack) {
    return TranslocalCountry(id, tld, geodb, hack)->GetId();
}

TCateg NGeoDB::TranslocalCountryID(const TCateg id, const EYandexSerpTld tld,
                                   const TGeoKeeper& geodb, ETranslocalityHack& hack) {
    return TranslocalCountry(id, tld, geodb, hack)->GetId();
}

bool NGeoDB::TranslocalIsIn(const TGeoPtr& child, const TGeoPtr& probableParent,
                            const EYandexSerpTld tld, const TGeoKeeper& geodb,
                            ETranslocalityHack& hack) {
    auto region = child;
    auto curHack = NO_HACK;
    auto hackSet = false;
    while (region && region != probableParent) {
        region = TranslocalParent(region, tld, geodb, curHack);
        if (!hackSet && NO_HACK != curHack) {
            hack = curHack;
            hackSet = true;
        }
    }

    if (region == probableParent) {
        return true;
    }

    return false;
}

bool NGeoDB::TranslocalIsIn(const TCateg child, const TCateg probableParent,
                            const EYandexSerpTld tld, const TGeoKeeper& geodb,
                            ETranslocalityHack& hack) {
    const auto childPtr = geodb.Find(child);
    const auto probableParentPtr = geodb.Find(probableParent);
    return TranslocalIsIn(childPtr, probableParentPtr, tld, geodb, hack);
}

bool NGeoDB::TranslocalIsIn(const TGeoPtr& child, const TCateg probableParent,
                            const EYandexSerpTld tld, const TGeoKeeper& geodb,
                            ETranslocalityHack& hack) {
    const auto probableParentPtr = geodb.Find(probableParent);
    return TranslocalIsIn(child, probableParentPtr, tld, geodb, hack);
}

bool NGeoDB::TranslocalIsIn(const TCateg child, const TGeoPtr& probableParent,
                            const EYandexSerpTld tld, const TGeoKeeper& geodb,
                            ETranslocalityHack& hack) {
    const auto childPtr = geodb.Find(child);
    return TranslocalIsIn(childPtr, probableParent, tld, geodb, hack);
}

NGeoDB::TGeoPtr NGeoDB::TranslocalParentExcludingTypes(
    const TGeoPtr& child, const EYandexSerpTld tld, const TGeoKeeper& geodb,
    const TVector<EType>& regionTypesToExclude, ETranslocalityHack& hack
){
    auto curHack = NO_HACK;
    auto hackSet = false;
    auto region = TranslocalParent(child, tld, geodb, curHack);
    if (NO_HACK != curHack) {
        hackSet = true;
        hack = curHack;
    }

    while (region) {
        auto found = false;
        for (const auto type : regionTypesToExclude) {
            if (type == region->GetType()) {
                found = true;
                break;
            }
        }

        if (!found) {
            break;
        }

        region = TranslocalParent(region, tld, geodb, curHack);
        if (!hackSet && NO_HACK != curHack) {
            hackSet = true;
            hack = curHack;
        }
    }

    return region;
}

NGeoDB::TGeoPtr NGeoDB::TranslocalParentExcludingTypes(
    const TGeoPtr& child, const EYandexSerpTld tld, const TGeoKeeper& geodb,
    const TVector<EType>& regionTypesToExclude
){
    ETranslocalityHack hack;
    return TranslocalParentExcludingTypes(child, tld, geodb, regionTypesToExclude, hack);
}

NGeoDB::TGeoPtr NGeoDB::TranslocalParentExcludingTypes(
    const TCateg child, const EYandexSerpTld tld, const TGeoKeeper& geodb,
    const TVector<EType>& regionTypesToExclude, ETranslocalityHack& hack
){
    const auto childWithData = geodb.Find(child);
    return TranslocalParentExcludingTypes(childWithData, tld, geodb, regionTypesToExclude, hack);
}

NGeoDB::TGeoPtr NGeoDB::TranslocalParentExcludingTypes(
    const TCateg child, const EYandexSerpTld tld, const TGeoKeeper& geodb,
    const TVector<EType>& regionTypesToExclude
){
    ETranslocalityHack hack;
    return TranslocalParentExcludingTypes(child, tld, geodb, regionTypesToExclude, hack);
}

TCateg NGeoDB::TranslocalParentIDExcludingTypes(
    const TGeoPtr& child, const EYandexSerpTld tld, const TGeoKeeper& geodb,
    const TVector<EType>& regionTypesToExclude, ETranslocalityHack& hack
){
    return TranslocalParentExcludingTypes(child, tld, geodb, regionTypesToExclude, hack)->GetId();
}

TCateg NGeoDB::TranslocalParentIDExcludingTypes(
    const TCateg child, const EYandexSerpTld tld, const TGeoKeeper& geodb,
    const TVector<EType>& regionTypesToExclude, ETranslocalityHack& hack
){
    const auto childWithData = geodb.Find(child);
    return TranslocalParentIDExcludingTypes(childWithData, tld, geodb, regionTypesToExclude, hack);
}

NGeoDB::TGeoPtr NGeoDB::TranslocalReduceExcludingTypes(
    const TGeoPtr& region, const EYandexSerpTld tld, const TGeoKeeper& geodb,
    const TVector<EType>& regionTypesToExclude, ETranslocalityHack& hack
){
    auto found = false;
    for (const auto type : regionTypesToExclude) {
        if (type == region->GetType()) {
            found = true;
            break;
        }
    }

    if (found) {
        return TranslocalParentExcludingTypes(region, tld, geodb, regionTypesToExclude, hack);
    }

    return region;
}

NGeoDB::TGeoPtr NGeoDB::TranslocalReduceExcludingTypes(
    const TGeoPtr& region, const EYandexSerpTld tld, const TGeoKeeper& geodb,
    const TVector<EType>& regionTypesToExclude
){
    ETranslocalityHack hack;
    return TranslocalReduceExcludingTypes(region, tld, geodb, regionTypesToExclude, hack);
}

NGeoDB::TGeoPtr NGeoDB::TranslocalReduceExcludingTypes(
    const TCateg region, const EYandexSerpTld tld, const TGeoKeeper& geodb,
    const TVector<EType>& regionTypesToExclude, ETranslocalityHack& hack
){
    const auto regionWithData = geodb.Find(region);
    return TranslocalReduceExcludingTypes(regionWithData, tld, geodb, regionTypesToExclude, hack);
}

NGeoDB::TGeoPtr NGeoDB::TranslocalReduceExcludingTypes(
    const TCateg region, const EYandexSerpTld tld, const TGeoKeeper& geodb,
    const TVector<EType>& regionTypesToExclude
){
    ETranslocalityHack hack;
    return TranslocalReduceExcludingTypes(region, tld, geodb, regionTypesToExclude, hack);
}

TCateg NGeoDB::TranslocalReduceIDExcludingTypes(
    const TGeoPtr& region, const EYandexSerpTld tld, const TGeoKeeper& geodb,
    const TVector<EType>& regionTypesToExclude, ETranslocalityHack& hack
){
    return TranslocalReduceExcludingTypes(region, tld, geodb, regionTypesToExclude, hack)->GetId();
}

TCateg NGeoDB::TranslocalReduceIDExcludingTypes(
    const TCateg region, const EYandexSerpTld tld, const TGeoKeeper& geodb,
    const TVector<EType>& regionTypesToExclude, ETranslocalityHack& hack
){
    return TranslocalReduceExcludingTypes(region, tld, geodb, regionTypesToExclude, hack)->GetId();
}
