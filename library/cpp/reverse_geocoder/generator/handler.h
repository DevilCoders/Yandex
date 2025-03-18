#pragma once

#include "config.h"
#include "gen_geo_data.h"

#include <library/cpp/reverse_geocoder/core/geo_data/geo_data.h>
#include <library/cpp/reverse_geocoder/proto/region.pb.h>

#include <util/generic/ptr.h>

namespace NReverseGeocoder {
    namespace NGenerator {
        class IHandler {
        public:
            IHandler(const TConfig& config, TGenGeoData* geoData)
                : GeoData_(geoData)
                , Config_(config)
            {
            }

            virtual void Init() = 0;

            virtual void Update(const NProto::TRegion& region) = 0;

            virtual void Fini() = 0;

            virtual ~IHandler() {
            }

        protected:
            TGenGeoData* GeoData_;
            TConfig Config_;
        };

        using THandlerPtr = TSimpleSharedPtr<IHandler>;
        using THandlerPtrs = TVector<THandlerPtr>;

    }
}
