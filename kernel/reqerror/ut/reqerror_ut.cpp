#include <library/cpp/testing/unittest/registar.h>

#include <kernel/reqerror/reqerror.h>

Y_UNIT_TEST_SUITE(ReqErrorTest) {

    Y_UNIT_TEST(TErrorTest) {
        // Simple exception with code
        try {
            throw TError(yxSERVICE_UNAVAILABLE);
            UNIT_ASSERT(false);
        } catch (yexception& exc) {
            UNIT_ASSERT_VALUES_EQUAL(exc.what(), "Service unavailable. ");
        }

        // Test message concatenation, written after SEARCH-16xx
        try {
            throw TError(yxSERVICE_UNAVAILABLE) << "Worker threads queue exhausted. ";
            UNIT_ASSERT(false);
        } catch (yexception& exc) {
            UNIT_ASSERT_VALUES_EQUAL(exc.what(), "Service unavailable. Worker threads queue exhausted. ");
        }

    }
}
