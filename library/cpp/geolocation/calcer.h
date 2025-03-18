#pragma once

#include <util/generic/vector.h>

namespace NGeolocationFeatures {
    enum EFeatureNames {
        DistanceToAnkara = 0,
        DistanceToMagadan,
        Latitude,
        Longitude,
        FeaturesCount
    };

    TVector<float> CalcFeatures(float latitude, float longitude);
    float CalcDistance(float latitude1, float longitude1, float latitude2, float longitude2);

    constexpr Y_FORCE_INLINE float FactorToLatitude(float fVal) noexcept {
        return fVal * 180.0f - 90.0f;
    }

    constexpr Y_FORCE_INLINE float FactorToLongitude(float fVal) noexcept {
        return fVal * 360.0f - 180.0f;
    }

    constexpr Y_FORCE_INLINE float LatitudeToFactor(float latitude) noexcept {
        return (latitude + 90.f) / 180.f;
    }

    constexpr Y_FORCE_INLINE float LongitudeToFactor(float longitude) noexcept {
        return (longitude + 180.f) / 360.f;
    }
}
