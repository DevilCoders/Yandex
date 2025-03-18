#pragma once

#include "gen_geo_data.h"
#include "handler.h"

#include <library/cpp/reverse_geocoder/proto/region.pb.h>
#include <library/cpp/reverse_geocoder/library/stop_watch.h>

#include <util/generic/vector.h>

namespace NReverseGeocoder {
    namespace NGenerator {
        struct TConfig;

        class TGenerator {
        public:
            TGenerator(const TConfig& config, TGenGeoData* geoData);

            void Init();
            void Update(const NProto::TRegion& region);
            void Fini();

        private:
            TConfig Config;
            TGenGeoData* GeoData;
            THandlerPtrs Handlers;
        };

        void Generate(const TConfig& config);
    }
}
