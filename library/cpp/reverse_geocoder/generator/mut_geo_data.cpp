#include "mut_geo_data.h"

using namespace NReverseGeocoder;
using namespace NGenerator;

NReverseGeocoder::NGenerator::TMutGeoData::TMutGeoData() {
#define GEO_BASE_DEF_VAR(TVar, Var) \
    Var##_ = TVar();

#define GEO_BASE_DEF_ARR(TArr, Arr) \
    // undef

    GEO_BASE_DEF_GEO_DATA

#undef GEO_BASE_DEF_VAR
#undef GEO_BASE_DEF_ARR
}
