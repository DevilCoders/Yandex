#include <library/cpp/reverse_geocoder/open_street_map/open_street_map.h>
#include <library/cpp/reverse_geocoder/open_street_map/parser.h>
#include <library/cpp/reverse_geocoder/open_street_map/reader.h>
#include <library/cpp/reverse_geocoder/open_street_map/simple_counter.h>
#include <library/cpp/reverse_geocoder/library/memory.h>
#include <library/cpp/reverse_geocoder/test/unittest/unittest.h>

using namespace NReverseGeocoder;
using namespace NOpenStreetMap;

static const char* OSM_PBF_TEST_FILE = "andorra-latest.osm.pbf";

Y_UNIT_TEST_SUITE(TOpenStreetMapParser) {
    Y_UNIT_TEST(Parse) {
        TFileInput inputStream(OSM_PBF_TEST_FILE);

        TReader reader(&inputStream);
        NReverseGeocoder::NOpenStreetMap::TSimpleCounter count;

        count.Parse(&reader);

        UNIT_ASSERT_EQUAL(123736ull, count.NodesNumber());
        UNIT_ASSERT_EQUAL(142ull, count.RelationsNumber());
        UNIT_ASSERT_EQUAL(5750ull, count.WaysNumber());

        UNIT_ASSERT_EQUAL(count.NodesProcessed() + count.DenseNodesProcessed(), count.NodesNumber());
        UNIT_ASSERT_EQUAL(count.WaysProcessed(), count.WaysNumber());
        UNIT_ASSERT_EQUAL(count.RelationsProcessed(), count.RelationsNumber());

        LogInfo("Parsed %lu relations, %lu ways and %lu nodes",
                count.RelationsNumber(), count.WaysNumber(), count.NodesNumber());
    }

    Y_UNIT_TEST(PoolParser) {
        TVector<NReverseGeocoder::NOpenStreetMap::TSimpleCounter> simpleCounters;

        simpleCounters.emplace_back();
        simpleCounters.emplace_back();

        TPoolParser parser;
        parser.Parse(OSM_PBF_TEST_FILE, simpleCounters);

        UNIT_ASSERT_EQUAL(123736ull, simpleCounters[0].NodesNumber() + simpleCounters[1].NodesNumber());
        UNIT_ASSERT_EQUAL(142ull, simpleCounters[0].RelationsNumber() + simpleCounters[1].RelationsNumber());
        UNIT_ASSERT_EQUAL(5750ull, simpleCounters[0].WaysNumber() + simpleCounters[1].WaysNumber());

        LogInfo("Blocks processed 1 = %lu", simpleCounters[0].BlocksProcessed());
        LogInfo("Blobks processed 2 = %lu", simpleCounters[1].BlocksProcessed());
    }
}
