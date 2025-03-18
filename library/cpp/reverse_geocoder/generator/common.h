#pragma once

#include "gen_geo_data.h"

#include <library/cpp/reverse_geocoder/core/point.h>
#include <library/cpp/reverse_geocoder/core/edge.h>
#include <library/cpp/reverse_geocoder/core/common.h>

#include <util/generic/vector.h>

namespace NReverseGeocoder {
    namespace NGenerator {
        TSquare GetSquare(const TVector<TPoint>& p);

        TVector<TEdge> MakeEdges(const TVector<TPoint>& points, TGenGeoData* geoData,
                                 bool changeDirection = true);

    }
}
