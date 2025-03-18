#include <library/cpp/testing/unittest/registar.h>

#include "night_check.h"

namespace NAntiRobot {
    Y_UNIT_TEST_SUITE(TTestNightCheck) {
        Y_UNIT_TEST(NightCheck) {
            const auto configString = "<Daemon>\n"
                                      "CaptchaApiHost = ::\n"
                                      "CbbApiHost = ::\n"
                                      "NightStartHour = 1\n"
                                      "NightEndHour = 7\n"
                                      "</Daemon>";
            ANTIROBOT_DAEMON_CONFIG_MUTABLE.LoadFromString(configString);

            // +03:00 Потому что Антон так сказал
            UNIT_ASSERT(!IsNightInMoscow(TInstant::ParseIso8601("2016-04-22T07:01+03:00")));
            UNIT_ASSERT(!IsNightInMoscow(TInstant::ParseIso8601("2016-04-22T10:01+03:00")));
            UNIT_ASSERT(!IsNightInMoscow(TInstant::ParseIso8601("2016-04-22T13:01+03:00")));
            UNIT_ASSERT(!IsNightInMoscow(TInstant::ParseIso8601("2016-04-22T17:01+03:00")));
            UNIT_ASSERT(!IsNightInMoscow(TInstant::ParseIso8601("2016-04-22T21:01+03:00")));
            UNIT_ASSERT(!IsNightInMoscow(TInstant::ParseIso8601("2016-04-22T00:01+03:00")));
            UNIT_ASSERT(IsNightInMoscow(TInstant::ParseIso8601("2016-04-22T01:01+03:00")));
            UNIT_ASSERT(IsNightInMoscow(TInstant::ParseIso8601("2016-04-22T03:01+03:00")));
            UNIT_ASSERT(IsNightInMoscow(TInstant::ParseIso8601("2016-04-22T06:59+03:00")));
        }
    }
}
