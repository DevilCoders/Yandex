#include "regions_handler.h"

#include <library/cpp/reverse_geocoder/core/kv.h>
#include <library/cpp/reverse_geocoder/core/polygon.h>
#include <library/cpp/reverse_geocoder/core/region.h>
#include <library/cpp/reverse_geocoder/library/log.h>

#include <util/generic/algorithm.h>

void NReverseGeocoder::NGenerator::TRegionsHandler::Update(const NProto::TRegion& protoRegion) {
    TRegion region;
    memset(&region, 0, sizeof(region));

    region.RegionId = protoRegion.GetRegionId();
    region.KvsOffset = GeoData_->KvsNumber();

    if (Config_.SaveKvs) {
        for (const NProto::TKv& kv : protoRegion.GetKvs()) {
            TKv x;
            x.K = GeoData_->Insert(kv.GetK());
            x.V = GeoData_->Insert(kv.GetV());
            GeoData_->KvsAppend(x);
        }
    }

    region.KvsNumber = GeoData_->KvsNumber() - region.KvsOffset;
    GeoData_->RegionsAppend(region);
}

void NReverseGeocoder::NGenerator::TRegionsHandler::Fini() {
    TRegion* regions = GeoData_->MutRegions();
    TRegion* regionsEnd = GeoData_->RegionsNumber() + regions;

    StableSort(regions, regionsEnd);

    for (size_t i = 0; i < GeoData_->PolygonsNumber(); ++i) {
        const TPolygon& p = GeoData_->Polygons()[i];
        TRegion* r = std::lower_bound(regions, regionsEnd, p.RegionId);
        if (r == regionsEnd || r->RegionId != p.RegionId) {
            LogWarning("Region %lu not exists!", p.RegionId);
            continue;
        }
        r->Square += p.Square;
        r->PolygonsNumber++;
    }
}
