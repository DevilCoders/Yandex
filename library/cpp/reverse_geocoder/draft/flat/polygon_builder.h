#pragma once

#include <library/cpp/reverse_geocoder/draft/proto/data_model.pb.h>

#include <util/stream/output.h>

namespace NReverseGeocoder::NDraft::NFlat {
    class TPolygonBuilder {
    public:
        void Build(const NProto::TPolygon& polygon, IOutputStream& outputStream);
    };

}
