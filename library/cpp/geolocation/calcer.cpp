#include "calcer.h"

#include <cmath>
#include <util/generic/utility.h>

namespace NGeolocationFeatures {
    constexpr float ToRad(float f) {
        return f / 180.f * M_PI;
    }

    struct TCoord {
        float Lat;
        float Lon;

        constexpr TCoord(float lat, float lon)
            : Lat(ToRad(lat))
            , Lon(ToRad(lon))
        {
        }
    };

    float CalcEarthDistance(const TCoord& lhs, const TCoord& rhs) {
        static const float EarthRadius = 6371.f;
        const float u = sin((rhs.Lat - lhs.Lat) / 2.f);
        const float v = sin((rhs.Lon - lhs.Lon) / 2.f);
        return 2.f * EarthRadius * asin(sqrt(u * u + cos(lhs.Lat) * cos(rhs.Lat) * v * v));
    }

    float Normalize(float f) {
        return 10000.f / (f * f + 10000.f);
    }

    TVector<float> CalcFeatures(float latitude, float longitude) {
        const TCoord location{latitude, longitude};

        TVector<float> features(FeaturesCount);

        features[DistanceToAnkara] = Normalize(
            CalcEarthDistance(
                {39.9403f, 32.8625f}, // Ankara
                location));
        features[DistanceToMagadan] = Normalize(
            CalcEarthDistance(
                {59.5682f, 150.809f}, // Magadan
                location));
        features[Latitude] = LatitudeToFactor(latitude);
        features[Longitude] = LongitudeToFactor(longitude);

        return features;
    }

    float CalcDistance(float latitude1, float longitude1, float latitude2, float longitude2) {
        return CalcEarthDistance({latitude1, longitude1}, {latitude2, longitude2});
    }
}
