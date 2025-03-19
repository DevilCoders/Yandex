#pragma once

#include <kernel/search_types/search_types.h>
#include <kernel/relev_locale/relev_locale.h>
#include <util/system/defaults.h>
#include <util/generic/string.h>
#include <util/generic/set.h>

#include <cstring>

namespace NGroupingAttrs {
class TDocAttrsSwitcher;
class TMetainfo;
}

class TCategSeries;

TCateg GetCountry(const NGroupingAttrs::TMetainfo* pGeoAttr, TCateg relevRegion);

void CalcGeoRegions(ui32 geoAttrNum, ui32 geoForAttrNum, ui32 geoAboutAttrNum, ui32 geoAAttrNum, ui32 geoAdresaNum, NGroupingAttrs::TDocAttrsSwitcher& attrs, TCategSeries* result, TCategSeries* geoaResult = nullptr, TCategSeries* geoResult = nullptr, TCategSeries* adresaResult = nullptr);

TCateg MapRegion(TCateg region);

struct TFormulaGeoRegion {
    NRl::ERelevLocale RelevLocale = NRl::RL_UNIVERSE;
    bool IsSpokRelevRegion = false;
    bool isKUBRRelevRegion = false;
    bool isRussianRelevRegion = false;
    bool isUkrainianRelevRegion = false;
    bool isKazakhRelevRegion = false;
    bool isBelorussianRelevRegion = false;
    bool isTurkishRelevRegion = false;
    bool isMoscowRelevRegion = false;
    bool isPiterRelevRegion = false;
    bool isPolishRelevRegion = false;
    bool isKievRelevRegion = false;
    bool isMinskRelevRegion = false;
    bool isMoscowGeoCityRelevRegion = false;
    bool isPiterGeoCityRelevRegion = false;

    TFormulaGeoRegion() {
    }

    TFormulaGeoRegion(const NGroupingAttrs::TMetainfo* da, TCateg relevRegion, TCateg geoCity,
                      NRl::ERelevLocale relevLocale = NRl::RL_UNIVERSE);
};
