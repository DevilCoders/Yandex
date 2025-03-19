#include "geodb.h"
#include "countries.h"
#include "entity.h"
#include "fixlist.h"

#include <util/generic/hash.h>
#include <util/generic/vector.h>

namespace NGeoDB {

    using TGeoPtrSet = TSet<TGeoPtr>;

    static const TEntityTypeWeight MINIMUM_WEIGHT = 9;

    static void GetFixBest(const TGeoPtr& userRegion,
                           const TGeoPtrSet& ids,
                           TGeoPtrSet& result) {
        for (const auto& fix: NDetail::FIX_BEST) {
            if (userRegion.IsIn(fix.From)) {
                for (const TGeoPtr& region : ids) {
                    if (region->GetId() == fix.Best) {
                        result.insert(region);
                        return;
                    }
                }
            }
        }
    }

    static void GetFixBetter(const TGeoPtr& userRegion,
                             const TGeoPtrSet& ids,
                             TGeoPtrSet& result) {
        TSet<TCateg> badIds;
        for (const auto& fix: NDetail::FIX_BETTER) {
            if (userRegion.IsIn(fix.From)) {
                if (ids.contains(fix.Better)) {
                    badIds.insert(fix.Worse);
                }
            }
        }
        if (!badIds) {
            return;
        }
        for (const TGeoPtr& region: ids) {
            if (!badIds.contains(region->GetId())) {
                result.insert(region);
            }
        }
    }


    static void GetBestMinWeight(const TGeoPtr& /*userRegion*/,
                                 const TGeoPtrSet& ids,
                                 TGeoPtrSet& result) {
        for (const TGeoPtr& region : ids) {
            const EType type = region->GetType();
            if (type == UNKNOWN)
                continue;

            if (EntityTypeToWeight(type) > MINIMUM_WEIGHT)
                continue;

            result.insert(region);
        }
    }

    static void GetBestGeoSameRegion(const TGeoPtr& userRegion,
                                     const TGeoPtrSet& ids,
                                     TGeoPtrSet& result) {
        if (ids.contains(userRegion))
            result.insert(userRegion);
    }

    static void GetBestGeoSameConstituent(const TGeoPtr& userRegion,
                                          const TGeoPtrSet& geo,
                                          TGeoPtrSet& result) {
        const TGeoPtr& userParent = userRegion.ParentByType(CONSTITUENT_ENTITY);
        if (!userParent)
            return;

        for (const TGeoPtr& region : geo) {
            if (userParent == region.ParentByType(CONSTITUENT_ENTITY))
                result.insert(region);
        }
    }

    static void GetBestGeoCapital(const TGeoPtr& /*userRegion*/,
                                  const TGeoPtrSet& ids,
                                  TGeoPtrSet& result) {
        for (const TGeoPtr& region : ids) {
            if (region->GetCountryCapitalId() == region->GetId())
                result.insert(region);
        }
    }

    static void GetBestGeoMainInEntity(const TGeoPtr& /*userRegion*/,
                                       const TGeoPtrSet& ids,
                                       const EType entityType,
                                       TGeoPtrSet& result) {
        for (const TGeoPtr& region : ids) {
            const TGeoPtr entity = region.ParentByType(entityType);
            if (entity && entity.Chief() && (entity->GetChiefId() == region->GetId())) {
                result.insert(region);
            }
        }
    }

    static void GetBestGeoMainInConstituentEntity(const TGeoPtr& userRegion,
                                                  const TGeoPtrSet& ids,
                                                  TGeoPtrSet& result) {
        GetBestGeoMainInEntity(userRegion, ids, CONSTITUENT_ENTITY, result);
    }

    static void GetBestGeoMainInFederalDistrict(const TGeoPtr& userRegion,
                                                const TGeoPtrSet& ids,
                                                TGeoPtrSet& result) {
        GetBestGeoMainInEntity(userRegion, ids, FEDERAL_DISTRICT, result);
    }

    size_t GetShortestDistance(const TGeoPtr& geoA, const TGeoPtr& geoB) noexcept {
        const auto& pathA = geoA->GetPath();
        const auto& pathB = geoB->GetPath();

        if (pathA.empty() || pathB.empty()) {
            return Max<size_t>();
        }
        auto itA = pathA.rbegin();
        auto itB = pathB.rbegin();
        // descending path should always starts with Earth ID
        Y_ASSERT(*itA == 10000 && *itB == 10000);

        while (itA != pathA.rend() && itB != pathB.rend() && *itA == *itB) {
            ++itA;
            ++itB;
        }
        const size_t distanceFromRootToLCAGeo = itA - pathA.rbegin();

        return pathA.size() + pathB.size() - distanceFromRootToLCAGeo * 2;
    }

    static void GetBestGeoLongestPath(const TGeoPtr& userRegion,
                                      const TGeoPtrSet& geo,
                                      TGeoPtrSet& result) {
        TSet<TCateg> allPath;
        THashMap<TCateg, size_t> commonPathLengths;
        for (size_t i = 0; i < userRegion->PathSize(); ++i) {
            commonPathLengths[userRegion->GetPath(i)] = userRegion->PathSize() - i;
            allPath.insert(userRegion->GetPath(i));
        }

        size_t max = 0;
        for (const TGeoPtr& region : geo) {
            TGeoPtr nearestCommonEntity = region;
            while (nearestCommonEntity && !allPath.contains(nearestCommonEntity->GetId()))
                nearestCommonEntity = nearestCommonEntity.Parent();

            if (!nearestCommonEntity)
                continue;

            size_t commonLen = commonPathLengths[nearestCommonEntity->GetId()];

            if (commonLen < max)
                continue;

            if (commonLen > max) {
                result.clear();
                max = commonLen;
            }
            result.insert(region);
        }
    }

    static void GetBestGeoSameCountry(const TGeoPtr& userRegion,
                                      const TGeoPtrSet& ids,
                                      TGeoPtrSet& result) {
        const TGeoPtr userCountry = userRegion.Country();
        if (!userCountry)
            return;

        for (const TGeoPtr& region : ids) {
            if (region.Country() == userCountry)
                result.insert(region);
        }
    }

    static void GetBestGeoRUBK(const TGeoPtr& userRegion,
                               const TGeoPtrSet& ids,
                               TGeoPtrSet& result) {
        const TCateg userCountryId = userRegion->GetCountryId();
        if (userCountryId == END_CATEG)
            return;

        if (!IsCountryFromKUBR(userCountryId))
            return;

        for (const TCateg countryId : NGeoDB::KUBR_IDS) {
            for (const TGeoPtr& region : ids) {
                if (region->GetCountryId() == countryId)
                    result.insert(region);
            }

            if (result)
                return;
        }
    }

    static void GetBestGeoBiggestEntry(const TGeoPtr& /*userRegion*/,
                                       const TGeoPtrSet& geo,
                                       TGeoPtrSet& result) {
        TEntityTypeWeight min = Max<TEntityTypeWeight>();
        for (const TGeoPtr& region : geo) {
            const EType type = region->GetType();
            if (type == UNKNOWN)
                continue;

            if (EntityTypeToWeight(type) > min)
                continue;

            if (EntityTypeToWeight(type) < min) {
                result.clear();
                min = EntityTypeToWeight(type);
            }

            result.insert(region);
        }
    }

/* XXX unused function
    static void GetBestGeoMain(const TGeoPtr& userRegion,
                               const TGeoPtrSet& geo,
                               TGeoPtrSet& result)
    {
        for (const TGeoPtr& region: geo) {
            if (region->GetIsMain())
                result.insert(region);
        }
    }
    */

/* XXX unused function
    static void GetBestGeoMaxPopulation(const TGeoPtr& userRegion,
                                        const TGeoPtrSet& geo,
                                        TGeoPtrSet& result)
    {
        ui64 max = 0;
        for (const TGeoPtr& region: geo) {
            // можно просто читать дефолтное значение из протобуфа, это и будет 0
            ui64 population = region->GetPopulation();

            if (population < max)
                continue;

            if (population > max) {
                result.clear();
                max = population;
            }
            result.insert(region);
        }
    }
    */

#define BEST_GEO_MAKE_STEP(FUNC)              \
    {                                         \
        FUNC(userRegion, prevResult, result); \
        if (result.size() == 1)               \
            return *result.begin();           \
        if (result)                           \
            prevResult.swap(result);          \
        result.clear();                       \
    }

    TGeoPtr GetBestGeo(const TCateg userRegionId, const TSet<TCateg>& ids, const TGeoKeeper& geoDb) {
        TGeoPtr userRegion = geoDb.Find(userRegionId);

        TGeoPtrSet geos;
        for (const TCateg id : ids) {
            if (id != END_CATEG) {
                TGeoPtr region = geoDb.Find(id);
                if (region->GetId() != END_CATEG) {
                    geos.insert(region);
                }
            }
        }

        return GetBestGeo(userRegion, geos);
    }

    TGeoPtr GetBestGeo(const TGeoPtr& userRegion, const TGeoPtrSet& geos) {
        if (!geos)
            return TGeoPtr();

        TGeoPtrSet prevResult;
        TGeoPtrSet result;
        prevResult = geos;

        if (userRegion) {
            BEST_GEO_MAKE_STEP(GetFixBest);
            BEST_GEO_MAKE_STEP(GetFixBetter);
            BEST_GEO_MAKE_STEP(GetBestMinWeight);
            BEST_GEO_MAKE_STEP(GetBestGeoSameRegion);
            BEST_GEO_MAKE_STEP(GetBestGeoSameConstituent);
            BEST_GEO_MAKE_STEP(GetBestGeoCapital);
            BEST_GEO_MAKE_STEP(GetBestGeoMainInFederalDistrict);
            BEST_GEO_MAKE_STEP(GetBestGeoMainInConstituentEntity);
            //BEST_GEO_MAKE_STEP(GetBestGeoMain);
            //BEST_GEO_MAKE_STEP(GetBestGeoMaxPopulation);
            BEST_GEO_MAKE_STEP(GetBestGeoLongestPath);
            BEST_GEO_MAKE_STEP(GetBestGeoSameCountry);
            BEST_GEO_MAKE_STEP(GetBestGeoRUBK);
            BEST_GEO_MAKE_STEP(GetBestGeoBiggestEntry);
        } else {
            //fallback with out userRegion
            BEST_GEO_MAKE_STEP(GetBestMinWeight);
            BEST_GEO_MAKE_STEP(GetBestGeoCapital);
            BEST_GEO_MAKE_STEP(GetBestGeoMainInFederalDistrict);
            BEST_GEO_MAKE_STEP(GetBestGeoMainInConstituentEntity);
            BEST_GEO_MAKE_STEP(GetBestGeoBiggestEntry);
        }

        // take region with smallest id - for test stability
        const TGeoPtr* min = nullptr;
        for (const TGeoPtr& region : prevResult) {
            if (!min || region->GetId() < min->Get()->GetId())
                min = &region;
        }

        return min ? *min : TGeoPtr();
    }

#undef BEST_GEO_MAKE_STEP

} // namespace NGeoDB
