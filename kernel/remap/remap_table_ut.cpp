#include "remap_table.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/ymath.h>

Y_UNIT_TEST_SUITE(TRemapTableTest) {
    void DoTestRemap(const TVector<float>& remapData, const TVector<float>& points, const TVector<float>& values) {
        const TRemapTable table(remapData.data(), remapData.size());
        UNIT_ASSERT_VALUES_EQUAL(points.size(), values.size());
        for (size_t i = 0; i < points.size(); i++) {
            UNIT_ASSERT_DOUBLES_EQUAL(table.Remap(points[i]), values[i], 1e-8);
        }
    }

    Y_UNIT_TEST(SimpleTest) {
        DoTestRemap(
            {1, 3, 5},
            {1, 2, 3, 4, 5},
            {0.0, 0.25, 0.5, 0.75, 1.0});
    }
}
