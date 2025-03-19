#include "decay_counter.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NFastUserFactors;

Y_UNIT_TEST_SUITE(TCountersTestSuite) {
    Y_UNIT_TEST(TestDecayCounterAdd) {
        THolder<ICounter> counter(new TDecayCounter(1.0));
        UNIT_ASSERT_VALUES_EQUAL(counter->LastTs(), 0);
        UNIT_ASSERT_DOUBLES_EQUAL(counter->Accumulate(), 0.0, 1.0e-6);

        counter->Add(86400, 1.0);
        UNIT_ASSERT_VALUES_EQUAL(counter->LastTs(), 86400);
        UNIT_ASSERT_DOUBLES_EQUAL(counter->Accumulate(), 1.0, 1.0e-6);

        counter->Add(86400 + 43200, 1.0);
        UNIT_ASSERT_VALUES_EQUAL(counter->LastTs(), 86400 + 43200);
        UNIT_ASSERT_DOUBLES_EQUAL(counter->Accumulate(), 1.1, 1.0e-6);

        counter->Add(86400 * 2, 1.0);
        UNIT_ASSERT_VALUES_EQUAL(counter->LastTs(), 86400 * 2);
        UNIT_ASSERT_DOUBLES_EQUAL(counter->Accumulate(), 1.11, 1.0e-6);
    }

    Y_UNIT_TEST(TestDecayCounterMove) {
        THolder<ICounter> counter(new TDecayCounter(1.0));
        counter->Add(86400, 1.0);

        counter->Move(86400 + 43200);
        UNIT_ASSERT_VALUES_EQUAL(counter->LastTs(), 86400 + 43200);
        UNIT_ASSERT_DOUBLES_EQUAL(counter->Accumulate(), 0.1, 1.0e-6);

        counter->Move(86400);
        UNIT_ASSERT_VALUES_EQUAL(counter->LastTs(), 86400);
        UNIT_ASSERT_DOUBLES_EQUAL(counter->Accumulate(), 1.0, 1.0e-6);
    }
}
