#pragma once

#include "config.h"
#include "gen_geo_data.h"
#include "handler.h"

#include <library/cpp/reverse_geocoder/library/stop_watch.h>

namespace NReverseGeocoder {
    namespace NGenerator {
        class TSlabHandler: public IHandler {
        public:
            TSlabHandler(const TConfig& config, TGenGeoData* geoData)
                : IHandler(config, geoData)
            {
            }

            void Init() override;

            void Update(const NProto::TRegion& region) override;

            void Fini() override;

        private:
            void Update(TGeoId regionId, const NProto::TPolygon& polygon);

            void Update(TGeoId regionId, TGeoId polygonId, const TVector<TPoint>& points,
                        TPolygon::EType type);

            void GenerateAreaBoxes();

            void FinalUpdateRegions();

            TStopWatch StopWatch_;
        };

    }
}
