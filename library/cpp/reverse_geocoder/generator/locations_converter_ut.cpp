#include <library/cpp/testing/unittest/registar.h>

#include "locations_converter.h"

using namespace NReverseGeocoder;
using namespace NGenerator;

Y_UNIT_TEST_SUITE(TLocationsConverter) {
    Y_UNIT_TEST(ConvertLocations) {
        const TVector<TLocation> rawLocations = {
            {0, 0},
            {1, 1},
            {2, 2},
            {3, 3},
            {3, 0},
            {0, 0},
            {5, 5},
            {6, 6},
            {7, 7},
            {7, 5},
            {5, 5},
            {10, 10},
            {11, 11},
            {12, 12},
            {12, 10},
        };

        const TVector<TLocation> ansLocations = {
            {0, 0},
            {3, 3},
            {3, 0},
            {5, 5},
            {7, 7},
            {7, 5},
            {10, 10},
            {12, 12},
            {12, 10}};

        const TVector<size_t> offsets = {0, 3, 6};
        const TVector<size_t> sizes = {3, 3, 3};

        TLocationsConverter converter;

        size_t callbacksCount = 0;

        converter.Each(rawLocations, [&](TVector<TLocation> const& locations) {
            UNIT_ASSERT_EQUAL(sizes[callbacksCount], locations.size());

            size_t offset = offsets[callbacksCount];
            size_t size = sizes[callbacksCount];

            for (size_t i = offset; i < offset + size; ++i)
                UNIT_ASSERT(IsEqualLocations(ansLocations[i], locations[i - offset]));

            ++callbacksCount;
        });

        UNIT_ASSERT_EQUAL(3ULL, callbacksCount);
    }
}
