#include "environment.h"

#include <antirobot/daemon_lib/ut/utils.h>

using namespace NAntiRobot;


Y_UNIT_TEST_SUITE_IMPL(TestEnvironment, TTestAntirobotMediumBase) {
    Y_UNIT_TEST(CustomHashingRules) {
        {
            ANTIROBOT_DAEMON_CONFIG_MUTABLE.CustomHashingRules = "";

            TEnv env;

            UNIT_ASSERT(env.CustomHashingMap.Find(TAddr("1.1.1.1")).Empty());
        }

        auto badValues = {"kdfvhbdsf", "1.1.1.1/16=10,1.1.1.1/16=11", "2.2.2.2/16=10,2.2.2.4/17=11", "=", "2.2.2.2/16", "1/1=1", "=1", "2.2.2.2/=12"};

        for (auto badValue : badValues) {
            ANTIROBOT_DAEMON_CONFIG_MUTABLE.CustomHashingRules = badValue;

            UNIT_ASSERT_EXCEPTION(TEnv(), yexception);
        }

        {
            ANTIROBOT_DAEMON_CONFIG_MUTABLE.CustomHashingRules = "1.1.1.1/16=10,2.2.2.2/24=20";

            TEnv env;

            UNIT_ASSERT(env.CustomHashingMap.Find(TAddr("1.1.1.1")).Defined());
            UNIT_ASSERT_VALUES_EQUAL(env.CustomHashingMap.Find(TAddr("1.1.1.1")).GetRef(), 10);
            UNIT_ASSERT(env.CustomHashingMap.Find(TAddr("1.1.1.0")).Defined());
            UNIT_ASSERT_VALUES_EQUAL(env.CustomHashingMap.Find(TAddr("1.1.1.0")).GetRef(), 10);
            UNIT_ASSERT(env.CustomHashingMap.Find(TAddr("1.1.1.2")).Defined());
            UNIT_ASSERT_VALUES_EQUAL(env.CustomHashingMap.Find(TAddr("1.1.1.2")).GetRef(), 10);
            UNIT_ASSERT(env.CustomHashingMap.Find(TAddr("1.1.0.0")).Defined());
            UNIT_ASSERT_VALUES_EQUAL(env.CustomHashingMap.Find(TAddr("1.1.0.0")).GetRef(), 10);
            UNIT_ASSERT(env.CustomHashingMap.Find(TAddr("1.1.2.3")).Defined());
            UNIT_ASSERT_VALUES_EQUAL(env.CustomHashingMap.Find(TAddr("1.1.2.3")).GetRef(), 10);

            UNIT_ASSERT(env.CustomHashingMap.Find(TAddr("1.0.0.0")).Empty());
            UNIT_ASSERT(env.CustomHashingMap.Find(TAddr("1.2.0.0")).Empty());
            UNIT_ASSERT(env.CustomHashingMap.Find(TAddr("1.255.0.0")).Empty());

            UNIT_ASSERT(env.CustomHashingMap.Find(TAddr("2.2.2.2")).Defined());
            UNIT_ASSERT_VALUES_EQUAL(env.CustomHashingMap.Find(TAddr("2.2.2.2")).GetRef(), 20);
            UNIT_ASSERT(env.CustomHashingMap.Find(TAddr("2.2.2.0")).Defined());
            UNIT_ASSERT_VALUES_EQUAL(env.CustomHashingMap.Find(TAddr("2.2.2.0")).GetRef(), 20);
            UNIT_ASSERT(env.CustomHashingMap.Find(TAddr("2.2.2.3")).Defined());
            UNIT_ASSERT_VALUES_EQUAL(env.CustomHashingMap.Find(TAddr("2.2.2.3")).GetRef(), 20);

            UNIT_ASSERT(env.CustomHashingMap.Find(TAddr("2.2.0.0")).Empty());

            UNIT_ASSERT(env.CustomHashingMap.Find(TAddr("3.1.1.1")).Empty());
            UNIT_ASSERT(env.CustomHashingMap.Find(TAddr("3.2.2.2")).Empty());
        }
    }
}
