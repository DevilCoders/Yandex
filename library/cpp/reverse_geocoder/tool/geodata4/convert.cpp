#include <library/cpp/geobase/lookup.hpp>
#include <library/cpp/reverse_geocoder/proto_library/writer.h>

using namespace NReverseGeocoder;

template <typename TRegionId>
static void WriteBorders(const NGeobase::TLookup& geobase, NProto::TWriter& writer, const TRegionId& regionId) {
    const std::vector<std::vector<NGeobase::TGeoPoint>> borders; // was: borders = geobase.get_border_points(regionId);
    if (borders.empty())
        return;

    const auto geobase_region = geobase.GetRegionById(regionId);

    NProto::TRegion region;
    region.SetRegionId(regionId);

    NProto::TKv* kv = region.AddKvs();
    kv->SetK("name:en");
    kv->SetV(geobase_region.GetEnName().c_str());

    for (const auto& border : borders) {
        NProto::TPolygon* polygon = region.AddPolygons();
        for (const auto& location : border) {
            NProto::TLocation* l = polygon->AddLocations();
            l->SetLat(location.Lat);
            l->SetLon(location.Lon);
        }
        polygon->SetType(NProto::TPolygon::TYPE_OUTER);
        polygon->SetPolygonId(0);
    }

    writer.Write(region);
}

void ConvertGeoData4(const char* inputPath, const char* outputPath) {
    NProto::TWriter writer(outputPath);
    NGeobase::TLookup geobase(inputPath);
    const auto regionIds = geobase.GetTree(NGeobase::EARTH_REGION_ID);
    for (const auto& regionId : regionIds)
        WriteBorders(geobase, writer, regionId);
}
