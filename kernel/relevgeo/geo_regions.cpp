#include "geo_regions.h"

#include <kernel/groupattrs/config.h>
#include <kernel/groupattrs/metainfo.h>
#include <kernel/groupattrs/switcher.h>
#include <kernel/region2country/countries.h>

TCateg MapRegion(TCateg region) {
    if (region == 382)
        return 225;
    if (region == 862)
        return 187;
    if (region == 958)
        return 166;
    if (region == 894)
        return 149;
    if (region == 926)
        return 159;
    if (region == 350)
        return 213;
    if (region == 21892)
        return 2;
    return region;
}

TCateg GetCountry(const NGroupingAttrs::TMetainfo* pGeoAttr, TCateg relevRegion) {
    if (END_CATEG != relevRegion) {
        //const NGroupingAttrs::TMetainfo* pGeoAttr = da.Metainfos().Metainfo("geoa");
        if (IsCountry(relevRegion)) {
            return relevRegion;
        }
        if (pGeoAttr) {
            TCateg now = relevRegion;
            while (now != END_CATEG && !IsCountry(now))
                now = pGeoAttr->Categ2Parent(now);
            return now;
        } else {
            return COUNTRY_INVALID;
        }
    } else {
        return COUNTRY_INVALID;
    }
}

inline void FillGeoRegions29(ui32 attrNum, NGroupingAttrs::TDocAttrsSwitcher& attrs, TCategSeries* result, bool add) {
    if ((result->Empty() || add) && (attrNum != NGroupingAttrs::TConfig::NotFound)) {
        attrs.SwitchToAttr(attrNum);
        for (NGroupingAttrs::TDocAttrsSwitcher::TIterator it = attrs.Begin(); attrs.End() != it; ++it) {
            const TCateg currGeo = MapRegion(*it);
            if (29 != currGeo && 0 != currGeo)
                result->AddCateg(currGeo);
        }
    }
}

inline void FillGeoRegions(ui32 attrNum, NGroupingAttrs::TDocAttrsSwitcher& attrs, TCategSeries* result) {
    if (result->Empty() && attrNum != NGroupingAttrs::TConfig::NotFound) {
        attrs.SwitchToAttr(attrNum);
        for (NGroupingAttrs::TDocAttrsSwitcher::TIterator it = attrs.Begin(); attrs.End() != it; ++it) {
            const TCateg currGeo = MapRegion(*it);
            result->AddCateg(currGeo);
        }
    }
}

void CalcGeoRegions(ui32 geoAttrNum, ui32 geoForAttrNum, ui32 geoAboutAttrNum, ui32 geoAAttrNum, ui32 geoAdresaAttrNum, NGroupingAttrs::TDocAttrsSwitcher& attrs, TCategSeries* result, TCategSeries* geoaResult, TCategSeries* geoResult, TCategSeries* adresaResult) {
    result->Clear();
    FillGeoRegions29(geoForAttrNum, attrs, result, true);
    FillGeoRegions29(geoAboutAttrNum, attrs, result, true);
    FillGeoRegions29(geoAttrNum, attrs, result, true);
    FillGeoRegions29(geoAdresaAttrNum, attrs, result, true);
    FillGeoRegions(geoAAttrNum, attrs, result);
    if (geoaResult) {
        geoaResult->Clear();
        FillGeoRegions(geoAAttrNum, attrs, geoaResult);
    }
    if (geoResult) {
        geoResult->Clear();
        FillGeoRegions29(geoAttrNum, attrs, geoResult, true);
    }
    if (adresaResult) {
        adresaResult->Clear();
        FillGeoRegions29(geoAdresaAttrNum, attrs, adresaResult, true);
    }
}

static bool IsRelevRegion(const NGroupingAttrs::TMetainfo* geo, TCateg relevRegion, TCateg region) {
    if (END_CATEG == relevRegion || geo == nullptr) {
        return false;
    }
    return geo->HasValueInCategPath(region, relevRegion);
}

TFormulaGeoRegion::TFormulaGeoRegion(const NGroupingAttrs::TMetainfo* geo, TCateg relevRegion, TCateg geoCity, NRl::ERelevLocale relevLocale):
    RelevLocale(relevLocale)
{
    switch (RelevLocale) {
        case NRl::RL_RU:
            isRussianRelevRegion = true;
            break;
        case NRl::RL_UA:
            isUkrainianRelevRegion = true;
            break;
        case NRl::RL_KZ:
            isKazakhRelevRegion = true;
            break;
        case NRl::RL_BY:
            isBelorussianRelevRegion = true;
            break;
        case NRl::RL_TR:
            isTurkishRelevRegion = true;
            break;
        default:
            break;
    }
    IsSpokRelevRegion = NRl::IsLocaleDescendantOf(RelevLocale, NRl::RL_SPOK);
    isKUBRRelevRegion = isKazakhRelevRegion || isUkrainianRelevRegion || isBelorussianRelevRegion || isRussianRelevRegion;
    isMoscowGeoCityRelevRegion = isRussianRelevRegion && IsRelevRegion(geo, geoCity, 1);
    isPiterGeoCityRelevRegion = isRussianRelevRegion && IsRelevRegion(geo, geoCity, 10174);

    isPolishRelevRegion = IsRelevRegion(geo, relevRegion, COUNTRY_POLAND.CountryId);
    isMoscowRelevRegion = isRussianRelevRegion && IsRelevRegion(geo, relevRegion, 1);
    isPiterRelevRegion = isRussianRelevRegion && IsRelevRegion(geo, relevRegion, 10174);
    isKievRelevRegion = isUkrainianRelevRegion && IsRelevRegion(geo, relevRegion, 20544);
    isMinskRelevRegion = isBelorussianRelevRegion && IsRelevRegion(geo, relevRegion, 29630);
}
