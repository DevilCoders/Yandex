#include <library/cpp/reverse_geocoder/open_street_map/converter.h>
#include <library/cpp/reverse_geocoder/proto_library/reader.h>
#include <library/cpp/reverse_geocoder/test/unittest/unittest.h>

using namespace NReverseGeocoder;
using namespace NOpenStreetMap;

Y_UNIT_TEST_SUITE(TOpenStreetMapConverter) {
    Y_UNIT_TEST(Convert) {
        UNIT_ASSERT_NO_EXCEPTION(RunPoolConvert("andorra-latest.osm.pbf", "andorra-latest.pbf", 2));

        ::NReverseGeocoder::NProto::TReader reader("andorra-latest.pbf");

        size_t regionsNumber = 0;
        size_t polygonsNumber = 0;

        reader.Each([&](const ::NReverseGeocoder::NProto::TRegion& r) {
            ++regionsNumber;
            polygonsNumber += r.PolygonsSize();
        });

        UNIT_ASSERT_EQUAL(171, regionsNumber);
        UNIT_ASSERT_EQUAL(176, polygonsNumber);
    }
}
