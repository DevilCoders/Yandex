#pragma once

#include <library/cpp/reverse_geocoder/core/geo_data/geo_data.h>

#include <util/generic/string.h>

namespace NReverseGeocoder {
    namespace NGenerator {
        class TGenGeoData: public IGeoData {
#define GEO_BASE_DEF_VAR(TVar, Var) \
    virtual void Set##Var(const TVar& var) = 0;

#define GEO_BASE_DEF_ARR(TArr, Arr) \
    virtual TArr* Mut##Arr() = 0;   \
    virtual void Arr##Append(const TArr& arr) = 0;

        public:
            GEO_BASE_DEF_GEO_DATA

#undef GEO_BASE_DEF_VAR
#undef GEO_BASE_DEF_ARR

            // Insert unique point into points array.
            virtual TRef Insert(const TPoint& p) = 0;

            // Insert unique edge into edges array.
            virtual TRef Insert(const TEdge& e) = 0;

            // Inser unique blob into blobs array.
            virtual TRef Insert(const TString& s) = 0;
        };

    }
}
