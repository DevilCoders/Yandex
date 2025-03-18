#pragma once

#include "handler.h"
#include "gen_geo_data.h"

namespace NReverseGeocoder {
    namespace NGenerator {
        class TRawHandler: public IHandler {
        public:
            TRawHandler(const TConfig& config, TGenGeoData* geoData)
                : IHandler(config, geoData)
            {
            }

            void Init() override;

            void Update(const NProto::TRegion& region) override;

            void Fini() override;

        private:
            void Update(TGeoId regionId, TGeoId polygonId, const TVector<TPoint>& rawPoints,
                        TRawPolygon::EType type);

            void Update(TGeoId regionId, const NProto::TPolygon& polygon);
        };

    }
}
