#include "os_family.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(OSFamilyTests) {
    Y_UNIT_TEST(TestThereAndBackAgain) {
        for (size_t index = 0; index < EOSFamily_ARRAYSIZE; ++index) {
            const auto osFamily = static_cast<EOSFamily>(index);
            UNIT_ASSERT_VALUES_EQUAL(osFamily, FromString<EOSFamily>(ToString(osFamily)));

            auto osFamilyOther = EOSFamily{};
            UNIT_ASSERT(TryFromString(ToString(osFamily), osFamilyOther));
            UNIT_ASSERT_VALUES_EQUAL(osFamily, osFamilyOther);
        }
    }

    Y_UNIT_TEST(TestExceptionOnInvalidValue) {
        UNIT_ASSERT_EXCEPTION(FromString<EOSFamily>("ololol.ololol"), TFromStringException);
        UNIT_ASSERT_EXCEPTION(ToString(static_cast<EOSFamily>(-1)), yexception);
        UNIT_ASSERT_EXCEPTION(ToString(static_cast<EOSFamily>(100500)), yexception);
    }
}
