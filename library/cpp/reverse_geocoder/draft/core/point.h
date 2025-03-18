#pragma once

#include "location.h"

#include <util/system/types.h>

namespace NReverseGeocoder::NDraft {
    struct TPoint {
        i32 X = 0;
        i32 Y = 0;
    };

    inline TPoint operator-(const TPoint& a, const TPoint& b) {
        return {a.X - b.X, a.Y - b.Y};
    }

    inline bool operator==(const TPoint& a, const TPoint& b) {
        return a.X == b.X && a.Y == b.Y;
    }

    inline bool operator!=(const TPoint& a, const TPoint& b) {
        return !(a == b);
    }

    inline ui64 Cross(const TPoint& a, const TPoint& b) {
        return 1LL * a.X * b.Y - 1LL * a.Y * b.X;
    }

    inline bool operator<(const TPoint& a, const TPoint& b) {
        return a.X < b.X || (a.X == b.X && a.Y < b.Y);
    }

    inline TPoint LocationToPoint(const TLocation& location) {
        return {
            static_cast<i32>(location.Lon * 1e6),
            static_cast<i32>(location.Lat * 1e6)};
    }

}
