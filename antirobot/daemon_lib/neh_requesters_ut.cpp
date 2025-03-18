#include <library/cpp/testing/unittest/registar.h>

#include "neh_requesters.h"

namespace NAntiRobot {
    Y_UNIT_TEST_SUITE(TNehRequesterImpl) {
        Y_UNIT_TEST(TestInstancesNotEqual) {
            UNIT_ASSERT_VALUES_UNEQUAL(
                dynamic_cast<const TNehRequester*>(&TGeneralNehRequester::Instance()),
                dynamic_cast<const TNehRequester*>(&TCaptchaNehRequester::Instance())
            );
        }
    };
} // namespace NAntiRobot
