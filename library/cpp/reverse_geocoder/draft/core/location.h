#pragma once

#include <library/cpp/reverse_geocoder/draft/proto/data_model.pb.h>

namespace NReverseGeocoder::NDraft {
    struct TLocation {
        double Lon = 0;
        double Lat = 0;
    };

    inline bool IsValidLocation(const TLocation& loc) {
        return loc.Lat >= -90. && loc.Lat <= 90. && loc.Lon >= 0. && loc.Lon <= 180.;
    }

    inline TLocation CreateLocation(const NProto::TLocation& loc) {
        return {loc.GetLon(), loc.GetLat()};
    }
}
