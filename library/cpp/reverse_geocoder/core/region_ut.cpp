#include <library/cpp/testing/unittest/registar.h>

#include "region.h"

using namespace NReverseGeocoder;

static TRegion GarbageRegion(int init) {
    TRegion region;
    for (int x = 0; x < (int)sizeof(region); ++x)
        *(((char*)&region) + x) = (char)(x + init);
    return region;
}

Y_UNIT_TEST_SUITE(TRegion) {
    Y_UNIT_TEST(OperatorEquals) {
        TRegion a = GarbageRegion(17);
        TRegion b = GarbageRegion(37);
        TRegion c = GarbageRegion(11);

        a.RegionId = 71;
        b.RegionId = 71;
        c.RegionId = 0;

        UNIT_ASSERT(a == b);
        UNIT_ASSERT(!(a == c));
    }

    Y_UNIT_TEST(OperatorLess) {
        TRegion a = GarbageRegion(77);
        TRegion b = GarbageRegion(11);
        TRegion c = GarbageRegion(99);

        a.RegionId = 7;
        b.RegionId = 9;
        c.RegionId = 7;

        UNIT_ASSERT(a < b);
        UNIT_ASSERT(!(b < a));
        UNIT_ASSERT(!(a < c));
        UNIT_ASSERT(!(c < a));
    }

    Y_UNIT_TEST(OperatorNumericLess) {
        TRegion a = GarbageRegion(31);
        a.RegionId = 11;

        UNIT_ASSERT(a < static_cast<TGeoId>(12));
        UNIT_ASSERT(!(a < static_cast<TGeoId>(10)));
        UNIT_ASSERT(!(a < static_cast<TGeoId>(11)));
        UNIT_ASSERT(!(a < static_cast<TGeoId>(0)));

        UNIT_ASSERT(!(static_cast<TGeoId>(12) < a));
        UNIT_ASSERT(static_cast<TGeoId>(10) < a);
        UNIT_ASSERT(!(static_cast<TGeoId>(11) < a));
    }

    Y_UNIT_TEST(Better) {
        TRegion a = GarbageRegion(31);
        TRegion b = GarbageRegion(77);
        TRegion c = GarbageRegion(3);

        a.Square = 17;
        b.Square = 18;
        c.Square = 16;

        UNIT_ASSERT(a.Better(b));
        UNIT_ASSERT(c.Better(b));
        UNIT_ASSERT(!a.Better(c));
        UNIT_ASSERT(!b.Better(a));
    }
}
