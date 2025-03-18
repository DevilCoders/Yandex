#pragma once

#include "handler.h"

namespace NReverseGeocoder {
    namespace NGenerator {
        class TRegionsHandler: public IHandler {
        public:
            TRegionsHandler(const TConfig& config, TGenGeoData* geoData)
                : IHandler(config, geoData)
            {
            }

            void Init() override {
            }

            void Update(const NProto::TRegion& region) override;

            void Fini() override;
        };

    }
}
