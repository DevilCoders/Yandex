#include <library/cpp/eventlog/dumper/common.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TParseTimeTest){
    Y_UNIT_TEST(TestExplicitUTC) {
        UNIT_ASSERT_VALUES_EQUAL(ParseTime("2018-11-06T02:24:10+00", 0),
                                 TInstant::ParseIso8601("2018-11-06T02:24:10+00").MicroSeconds());

        UNIT_ASSERT_VALUES_EQUAL(ParseTime("2018-11-06T02:24:10Z", 0, 0),
                                 TInstant::ParseIso8601("2018-11-06T02:24:10+00").MicroSeconds());

        UNIT_ASSERT_VALUES_EQUAL(ParseTime("2018-11-06T02:24:10Z", 0, +3),
                                 TInstant::ParseIso8601("2018-11-06T02:24:10+00").MicroSeconds());
    }

    Y_UNIT_TEST(TestExplicitMSK) {
        UNIT_ASSERT_VALUES_EQUAL(ParseTime("2018-11-06T02:24:10+03", 0),
                                 TInstant::ParseIso8601("2018-11-06T02:24:10+03").MicroSeconds());

        UNIT_ASSERT_VALUES_EQUAL(ParseTime("2018-11-06T02:24:10+03", 0, 0),
                                 TInstant::ParseIso8601("2018-11-06T02:24:10+03").MicroSeconds());
    }

    Y_UNIT_TEST(TestDefaultOffset) {
        UNIT_ASSERT_VALUES_EQUAL(ParseTime("2018-11-06T02:24:10", 0),
                                 TInstant::ParseIso8601("2018-11-06T02:24:10+03").MicroSeconds());

        UNIT_ASSERT_VALUES_EQUAL(ParseTime("2018-11-06T02:24:10", 0, 0),
                                 TInstant::ParseIso8601("2018-11-06T02:24:10+00").MicroSeconds());

        UNIT_ASSERT_VALUES_EQUAL(ParseTime("2018-11-06T02:24:10", 0, +4),
                                 TInstant::ParseIso8601("2018-11-06T02:24:10+04").MicroSeconds());

        UNIT_ASSERT_VALUES_EQUAL(ParseTime("2018-11-06T02:24:10", 0, +2),
                                 TInstant::ParseIso8601("2018-11-06T02:24:10+02").MicroSeconds());
    }

    Y_UNIT_TEST(TestToday) {
        const ui64 def = 1541461811123456ULL;
        const ui64 res = ParseTime("10:00:57", def);
        UNIT_ASSERT(res != def);
        // Real result depends on current time on the machine and its timezone settings.
        // Check only seconds.
        UNIT_ASSERT_VALUES_EQUAL((res / 1000000) % 60, 57);
    }

    Y_UNIT_TEST(TestEmptyString) {
        const ui64 def = 1541445850000000ULL;
        UNIT_ASSERT_VALUES_EQUAL(ParseTime("", def), def);
    }

    Y_UNIT_TEST(TestTrash) {
        const ui64 def = 1541461811123456ULL;
        UNIT_ASSERT_VALUES_EQUAL(ParseTime("not_a_valid_time_string", def, +3), def);
    }

    Y_UNIT_TEST(TestUnixMicroseconds) {
        UNIT_ASSERT_VALUES_EQUAL(ParseTime("1540228660634530", 0), 1540228660634530ULL);
    }
}
