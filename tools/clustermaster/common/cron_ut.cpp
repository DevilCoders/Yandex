#include "cron.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/ptr.h>
#include <util/system/env.h>

namespace {
    constexpr auto TZ_VARIABLE_NAME = "TZ";

    class TMoscowZoneGuard {
    private:
        TString TZ;
    public:
        TMoscowZoneGuard() : TZ(GetEnv(TZ_VARIABLE_NAME)) {
            SetEnv(TZ_VARIABLE_NAME, ":Europe/Moscow");
            tzset();
        }
        ~TMoscowZoneGuard() {
            if (TZ) {
                SetEnv(TZ_VARIABLE_NAME, TZ);
            } else {
                unsetenv(TZ_VARIABLE_NAME);
            }
            tzset();
        }
    };
}

Y_UNIT_TEST_SUITE(TCronEntryTest) {

    const TDuration MOSCOW_TIME_OFFSET = TDuration::Hours(4);

    // extended format: s m h dom m dow
    // cron format: m h dom m dow

    Y_UNIT_TEST(SmokeTest) {
        TCronEntry cron("0-49/3,2,5,*/7 1-10 * * *");
    }

    Y_UNIT_TEST(InvalidEntries) {
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * * * * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("abcd"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("1 1 1 1"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("1 1 1 1 1 1 1"), TCronException);
    }

    Y_UNIT_TEST(OutOfRangeEntries) {
        // first group without seconds
        UNIT_ASSERT_EXCEPTION(TCronEntry("60 * * * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("0-60 * * * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("0-60/2 * * * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("*/31 * * * *"), TCronException);

        //second group with seconds
        UNIT_ASSERT_EXCEPTION(TCronEntry("60 * * * * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("0-60 * * * * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("0-60/2 * * * * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("*/31 * * * * *"), TCronException);

        UNIT_ASSERT_EXCEPTION(TCronEntry("* 24 * * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* 0-24 * * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* 0-24/2 * * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* */13 * * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* 0-22/13 * * *"), TCronException);

        UNIT_ASSERT_EXCEPTION(TCronEntry("* * 24 * * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * 0-24 * * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * 0-24/2 * * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * */13 * * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * 0-22/13 * * *"), TCronException);

        UNIT_ASSERT_EXCEPTION(TCronEntry("* * 0 * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * 32 * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * 0-15 * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * 15-32 * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * */17 * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * 2-22/17 * *"), TCronException);

        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * 0 * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * 32 * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * 0-15 * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * 15-32 * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * */17 * *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * 2-22/17 * *"), TCronException);

        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * 0 *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * 13 *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * 0-5 *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * 5-13 *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * */7 *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * 2-10/7 *"), TCronException);

        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * * 0 *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * * 13 *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * * 0-5 *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * * 5-13 *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * * */7 *"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * * 2-10/7 *"), TCronException);

        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * * 8"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * * 3-8"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * * */5"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * * 3-8/5"), TCronException);

        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * * * 8"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * * * 3-8"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * * * */5"), TCronException);
        UNIT_ASSERT_EXCEPTION(TCronEntry("* * * * * 3-8/5"), TCronException);
    }

    Y_UNIT_TEST(Triggering) {
        TMoscowZoneGuard guard;
        UNIT_ASSERT(TCronEntry("* * * * *")(TInstant::ParseIso8601("2013-01-01T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(TCronEntry("* * * * *")(TInstant::ParseIso8601("2013-01-01T00:01") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(TCronEntry("* * * * *")(TInstant::ParseIso8601("2012-07-23T21:17") - MOSCOW_TIME_OFFSET));

        UNIT_ASSERT(! TCronEntry("3 * * * *")(TInstant::ParseIso8601("2013-01-01T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! TCronEntry("3 * * * *")(TInstant::ParseIso8601("2013-01-01T00:01") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(TCronEntry("3 * * * *")(TInstant::ParseIso8601("2013-01-01T00:03") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! TCronEntry("*/7 * * * *")(TInstant::ParseIso8601("2013-01-01T00:01") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(TCronEntry("*/7 * * * *")(TInstant::ParseIso8601("2013-01-01T00:21") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! TCronEntry("12-23 * * * *")(TInstant::ParseIso8601("2013-01-01T00:11") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(TCronEntry("12-23 * * * *")(TInstant::ParseIso8601("2013-01-01T00:12") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(TCronEntry("12-23 * * * *")(TInstant::ParseIso8601("2013-01-01T00:17") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(TCronEntry("12-23 * * * *")(TInstant::ParseIso8601("2013-01-01T00:23") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! TCronEntry("12-23 * * * *")(TInstant::ParseIso8601("2013-01-01T00:24") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! TCronEntry("12-23/3 * * * *")(TInstant::ParseIso8601("2013-01-01T00:11") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(TCronEntry("12-23/3 * * * *")(TInstant::ParseIso8601("2013-01-01T00:12") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! TCronEntry("12-23/3 * * * *")(TInstant::ParseIso8601("2013-01-01T00:13") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(TCronEntry("12-23/3 * * * *")(TInstant::ParseIso8601("2013-01-01T00:18") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! TCronEntry("12-23/3 * * * *")(TInstant::ParseIso8601("2013-01-01T00:23") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! TCronEntry("12-23/3 * * * *")(TInstant::ParseIso8601("2013-01-01T00:24") - MOSCOW_TIME_OFFSET));

        UNIT_ASSERT(! TCronEntry("3 * * * * *")(TInstant::ParseIso8601("2013-01-01T00:00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! TCronEntry("3 * * * * *")(TInstant::ParseIso8601("2013-01-01T00:00:01") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(TCronEntry("3 * * * * *")(TInstant::ParseIso8601("2013-01-01T00:00:03") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! TCronEntry("*/7 * * * * *")(TInstant::ParseIso8601("2013-01-01T00:00:01") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(TCronEntry("*/7 * * * * *")(TInstant::ParseIso8601("2013-01-01T00:00:21") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! TCronEntry("12-23 * * * * *")(TInstant::ParseIso8601("2013-01-01T00:00:11") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(TCronEntry("12-23 * * * * *")(TInstant::ParseIso8601("2013-01-01T00:00:12") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(TCronEntry("12-23 * * * * *")(TInstant::ParseIso8601("2013-01-01T00:00:17") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(TCronEntry("12-23 * * * * *")(TInstant::ParseIso8601("2013-01-01T00:00:23") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! TCronEntry("12-23 * * * * *")(TInstant::ParseIso8601("2013-01-01T00:00:24") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! TCronEntry("12-23/3 * * * * *")(TInstant::ParseIso8601("2013-01-01T00:00:11") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(TCronEntry("12-23/3 * * * * *")(TInstant::ParseIso8601("2013-01-01T00:00:12") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! TCronEntry("12-23/3 * * * * *")(TInstant::ParseIso8601("2013-01-01T00:00:13") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(TCronEntry("12-23/3 * * * * *")(TInstant::ParseIso8601("2013-01-01T00:00:18") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! TCronEntry("12-23/3 * * * * *")(TInstant::ParseIso8601("2013-01-01T00:00:23") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! TCronEntry("12-23/3 * * * * *")(TInstant::ParseIso8601("2013-01-01T00:00:24") - MOSCOW_TIME_OFFSET));
    }

    // $ cal 01 2013
    //     January 2013
    // Su Mo Tu We Th Fr Sa
    //        1  2  3  4  5
    //  6  7  8  9 10 11 12
    // 13 14 15 16 17 18 19
    // 20 21 22 23 24 25 26
    // 27 28 29 30 31

    Y_UNIT_TEST(DayOfMonthAndDayOfWeek) {
        TMoscowZoneGuard guard;
        // Every second day of month OR Thursday
        TCronEntry cron("* * */2 * 4");
        UNIT_ASSERT(! cron(TInstant::ParseIso8601("2013-01-01T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-02T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-03T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-04T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron(TInstant::ParseIso8601("2013-01-05T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-06T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron(TInstant::ParseIso8601("2013-01-07T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-08T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron(TInstant::ParseIso8601("2013-01-09T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-10T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron(TInstant::ParseIso8601("2013-01-11T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-12T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron(TInstant::ParseIso8601("2013-01-13T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-14T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron(TInstant::ParseIso8601("2013-01-15T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-16T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-17T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-18T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron(TInstant::ParseIso8601("2013-01-19T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-20T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron(TInstant::ParseIso8601("2013-01-21T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-22T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron(TInstant::ParseIso8601("2013-01-23T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-24T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron(TInstant::ParseIso8601("2013-01-25T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-26T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron(TInstant::ParseIso8601("2013-01-27T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-28T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron(TInstant::ParseIso8601("2013-01-29T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-30T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-31T00:00") - MOSCOW_TIME_OFFSET));
    }

    Y_UNIT_TEST(DayOfMonth2) {
        TMoscowZoneGuard guard;
        // Every 1st, 11th, and 21st day in 12:00:00
        TCronEntry cron("0 0 12 1,11,21 * *");

        UNIT_ASSERT(! cron(TInstant::ParseIso8601("2013-01-01T11:00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-01T12:00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron(TInstant::ParseIso8601("2013-01-01T13:00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron(TInstant::ParseIso8601("2013-01-02T12:00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron(TInstant::ParseIso8601("2013-01-03T12:00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron(TInstant::ParseIso8601("2013-01-10T12:00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron(TInstant::ParseIso8601("2013-01-11T12:00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron(TInstant::ParseIso8601("2013-01-12T12:00:00") - MOSCOW_TIME_OFFSET));
    }

    Y_UNIT_TEST(DayOfWeek) {
        TMoscowZoneGuard guard;
        TCronEntry cron0("* * * 1 * 0");
        TCronEntry cron7("* * * 1 * 7");

        UNIT_ASSERT(cron0(TInstant::ParseIso8601("2013-01-06T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron0(TInstant::ParseIso8601("2013-01-07T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron0(TInstant::ParseIso8601("2013-01-08T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron0(TInstant::ParseIso8601("2013-01-09T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron0(TInstant::ParseIso8601("2013-01-10T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron0(TInstant::ParseIso8601("2013-01-11T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron0(TInstant::ParseIso8601("2013-01-12T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron0(TInstant::ParseIso8601("2013-01-13T00:00") - MOSCOW_TIME_OFFSET));

        UNIT_ASSERT(cron7(TInstant::ParseIso8601("2013-01-06T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron7(TInstant::ParseIso8601("2013-01-07T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron7(TInstant::ParseIso8601("2013-01-08T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron7(TInstant::ParseIso8601("2013-01-09T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron7(TInstant::ParseIso8601("2013-01-10T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron7(TInstant::ParseIso8601("2013-01-11T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron7(TInstant::ParseIso8601("2013-01-12T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron7(TInstant::ParseIso8601("2013-01-13T00:00") - MOSCOW_TIME_OFFSET));

        TCronEntry cron01r("* * * 1 * 0-1");
        TCronEntry cron67r("* * * 1 * 6-7");
        TCronEntry cron07r("* * * 1 * 0-7");

        UNIT_ASSERT(cron01r(TInstant::ParseIso8601("2013-01-06T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron01r(TInstant::ParseIso8601("2013-01-07T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron01r(TInstant::ParseIso8601("2013-01-08T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron01r(TInstant::ParseIso8601("2013-01-09T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron01r(TInstant::ParseIso8601("2013-01-10T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron01r(TInstant::ParseIso8601("2013-01-11T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron01r(TInstant::ParseIso8601("2013-01-12T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron01r(TInstant::ParseIso8601("2013-01-13T00:00") - MOSCOW_TIME_OFFSET));

        UNIT_ASSERT(cron67r(TInstant::ParseIso8601("2013-01-06T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron67r(TInstant::ParseIso8601("2013-01-07T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron67r(TInstant::ParseIso8601("2013-01-08T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron67r(TInstant::ParseIso8601("2013-01-09T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron67r(TInstant::ParseIso8601("2013-01-10T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron67r(TInstant::ParseIso8601("2013-01-11T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron67r(TInstant::ParseIso8601("2013-01-12T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron67r(TInstant::ParseIso8601("2013-01-13T00:00") - MOSCOW_TIME_OFFSET));

        UNIT_ASSERT(cron07r(TInstant::ParseIso8601("2013-01-06T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron07r(TInstant::ParseIso8601("2013-01-07T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron07r(TInstant::ParseIso8601("2013-01-08T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron07r(TInstant::ParseIso8601("2013-01-09T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron07r(TInstant::ParseIso8601("2013-01-10T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron07r(TInstant::ParseIso8601("2013-01-11T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron07r(TInstant::ParseIso8601("2013-01-12T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron07r(TInstant::ParseIso8601("2013-01-13T00:00") - MOSCOW_TIME_OFFSET));

        TCronEntry cron01l("* * * 1 * 0,1");
        TCronEntry cron67l("* * * 1 * 6,7");

        UNIT_ASSERT(cron01l(TInstant::ParseIso8601("2013-01-06T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron01l(TInstant::ParseIso8601("2013-01-07T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron01l(TInstant::ParseIso8601("2013-01-08T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron01l(TInstant::ParseIso8601("2013-01-09T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron01l(TInstant::ParseIso8601("2013-01-10T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron01l(TInstant::ParseIso8601("2013-01-11T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron01l(TInstant::ParseIso8601("2013-01-12T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron01l(TInstant::ParseIso8601("2013-01-13T00:00") - MOSCOW_TIME_OFFSET));

        UNIT_ASSERT(cron67l(TInstant::ParseIso8601("2013-01-06T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron67l(TInstant::ParseIso8601("2013-01-07T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron67l(TInstant::ParseIso8601("2013-01-08T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron67l(TInstant::ParseIso8601("2013-01-09T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron67l(TInstant::ParseIso8601("2013-01-10T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron67l(TInstant::ParseIso8601("2013-01-11T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron67l(TInstant::ParseIso8601("2013-01-12T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron67l(TInstant::ParseIso8601("2013-01-13T00:00") - MOSCOW_TIME_OFFSET));

        TCronEntry cron2s("* * * 1 * */2");
        TCronEntry cron3s("* * * 1 * */3");

        UNIT_ASSERT(! cron2s(TInstant::ParseIso8601("2013-01-06T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron2s(TInstant::ParseIso8601("2013-01-07T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron2s(TInstant::ParseIso8601("2013-01-08T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron2s(TInstant::ParseIso8601("2013-01-09T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron2s(TInstant::ParseIso8601("2013-01-10T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron2s(TInstant::ParseIso8601("2013-01-11T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron2s(TInstant::ParseIso8601("2013-01-12T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron2s(TInstant::ParseIso8601("2013-01-13T00:00") - MOSCOW_TIME_OFFSET));

        UNIT_ASSERT(! cron3s(TInstant::ParseIso8601("2013-01-06T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron3s(TInstant::ParseIso8601("2013-01-07T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron3s(TInstant::ParseIso8601("2013-01-08T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron3s(TInstant::ParseIso8601("2013-01-09T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron3s(TInstant::ParseIso8601("2013-01-10T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron3s(TInstant::ParseIso8601("2013-01-11T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(cron3s(TInstant::ParseIso8601("2013-01-12T00:00") - MOSCOW_TIME_OFFSET));
        UNIT_ASSERT(! cron3s(TInstant::ParseIso8601("2013-01-13T00:00") - MOSCOW_TIME_OFFSET));
    }
}
