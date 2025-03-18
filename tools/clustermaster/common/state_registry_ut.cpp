#include "state_registry.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/ptr.h>

Y_UNIT_TEST_SUITE(TStateRegistryTest) {

    Y_UNIT_TEST(FindByStateId) {
        UNIT_ASSERT_VALUES_EQUAL(TStateRegistry::findByState(TS_RUNNING)->SmallName, "running");
    }

    Y_UNIT_TEST(FindBySmallName) {
        UNIT_ASSERT_VALUES_EQUAL(TStateRegistry::findBySmallName("running")->State, TS_RUNNING);
    }
}
