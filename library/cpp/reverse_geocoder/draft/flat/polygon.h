#pragma once

#include <library/cpp/reverse_geocoder/draft/core/point.h>

#include <util/generic/strbuf.h>
#include <util/system/types.h>

namespace NReverseGeocoder::NDraft {
    enum class EPolygonType {
        Unknown = 0,
        Inner = 1,
        Outer = 2,
    };

    class TPolygon {
    public:
        TPolygon(TStringBuf flatMemory)
            : FlatMemory_(flatMemory)
        {
        }

        bool Contains(TPoint) const {
            return false;
        }

    private:
        TStringBuf FlatMemory_;
    };

}
