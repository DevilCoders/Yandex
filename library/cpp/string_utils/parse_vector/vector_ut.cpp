#include "vector_parser.h"
#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(VectorParser) {
    Y_UNIT_TEST(TestParseVector) {
        TVector<TDuration> durations;
        UNIT_ASSERT_EQUAL(TryParseStringToVector("1s, 32m,   32  , 23 ", durations), true);
        UNIT_ASSERT_VALUES_EQUAL(durations.size(), 4);
        UNIT_ASSERT_VALUES_EQUAL(durations[0], TDuration::Seconds(1));
        UNIT_ASSERT_VALUES_EQUAL(durations[1], TDuration::Minutes(32));
        UNIT_ASSERT_VALUES_EQUAL(durations[2], TDuration::Seconds(32));
        UNIT_ASSERT_VALUES_EQUAL(durations[3], TDuration::Seconds(23));
        UNIT_ASSERT_EQUAL(TryParseStringToVector("1s, 32m,   32  23 ", durations), false);
    };
};
