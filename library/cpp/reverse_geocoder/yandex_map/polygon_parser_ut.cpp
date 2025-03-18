#include "polygon_parser.h"

#include <library/cpp/reverse_geocoder/core/location.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NReverseGeocoder;
using namespace NYandexMap;

Y_UNIT_TEST_SUITE(TPolygonParser) {
    static void check_polygon(const NProto::TRegion& region, const TVector<TLocation>& locations) {
        UNIT_ASSERT_EQUAL(1, region.PolygonsSize());

        const NProto::TPolygon& p = region.GetPolygons(0);
        UNIT_ASSERT_EQUAL(locations.size(), p.LocationsSize());

        for (size_t i = 0; i < locations.size(); ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(locations[i].Lat, p.GetLocations(i).GetLat(), 1e-9);
            UNIT_ASSERT_DOUBLES_EQUAL(locations[i].Lon, p.GetLocations(i).GetLon(), 1e-9);
        }
    }

    Y_UNIT_TEST(CorrectParse) {
        static const TString str = "((17.2234 28.111019, 89.323232 11.717189, 17.5543 -100.12))";

        NProto::TRegion region;

        TPolygonParser parser(region);
        parser.Parse(str);

        check_polygon(region, {{17.2234, 28.111019}, {89.323232, 11.717189}, {17.5543, -100.12}});
    }

    Y_UNIT_TEST(IncorrectParse) {
        NProto::TRegion fake;
        TPolygonParser parser(fake);

        UNIT_ASSERT_EXCEPTION(parser.Parse("(17.22 11))"), yexception);
        UNIT_ASSERT_EXCEPTION(parser.Parse("(((17.22 11))"), yexception);
        UNIT_ASSERT_EXCEPTION(parser.Parse("((17.22 11,,8 10))"), yexception);
        UNIT_ASSERT_EXCEPTION(parser.Parse("((18.22 11"), yexception);
        UNIT_ASSERT_EXCEPTION(parser.Parse("123"), yexception);
    }
}
