#include "datetime.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/system/event.h>

Y_UNIT_TEST_SUITE(Datetime) {
    Y_UNIT_TEST(ShiftCalendarMonths) {
        {
            const TDuration subsec = TDuration::MicroSeconds(123456);
            const TInstant start = TInstant::ParseIso8601("1999-01-15T18:35:51Z") + subsec;
            UNIT_ASSERT_VALUES_EQUAL(start.MicroSeconds(), 916425351123456ul);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  0), TInstant::ParseIso8601("1999-01-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  1), TInstant::ParseIso8601("1999-02-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  2), TInstant::ParseIso8601("1999-03-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  3), TInstant::ParseIso8601("1999-04-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  4), TInstant::ParseIso8601("1999-05-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  5), TInstant::ParseIso8601("1999-06-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  6), TInstant::ParseIso8601("1999-07-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  7), TInstant::ParseIso8601("1999-08-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  8), TInstant::ParseIso8601("1999-09-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  9), TInstant::ParseIso8601("1999-10-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 10), TInstant::ParseIso8601("1999-11-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 11), TInstant::ParseIso8601("1999-12-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 12), TInstant::ParseIso8601("2000-01-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 13), TInstant::ParseIso8601("2000-02-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 14), TInstant::ParseIso8601("2000-03-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 15), TInstant::ParseIso8601("2000-04-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 16), TInstant::ParseIso8601("2000-05-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 17), TInstant::ParseIso8601("2000-06-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 18), TInstant::ParseIso8601("2000-07-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 19), TInstant::ParseIso8601("2000-08-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 20), TInstant::ParseIso8601("2000-09-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 21), TInstant::ParseIso8601("2000-10-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 22), TInstant::ParseIso8601("2000-11-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 23), TInstant::ParseIso8601("2000-12-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 24), TInstant::ParseIso8601("2001-01-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 25), TInstant::ParseIso8601("2001-02-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_UNEQUAL(start, NUtil::ShiftCalendarMonths(start, 25));
            UNIT_ASSERT_VALUES_UNEQUAL(NUtil::ShiftCalendarMonths(start, 25), NUtil::ShiftCalendarMonths(start, 24));
        }
        {
            const TDuration subsec = TDuration::MicroSeconds(123456);
            const TInstant start = TInstant::ParseIso8601("2003-01-15T18:35:51Z") + subsec;
            UNIT_ASSERT_VALUES_EQUAL(start.MicroSeconds(), 1042655751123456ul);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  0), TInstant::ParseIso8601("2003-01-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  1), TInstant::ParseIso8601("2003-02-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  2), TInstant::ParseIso8601("2003-03-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  3), TInstant::ParseIso8601("2003-04-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  4), TInstant::ParseIso8601("2003-05-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  5), TInstant::ParseIso8601("2003-06-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  6), TInstant::ParseIso8601("2003-07-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  7), TInstant::ParseIso8601("2003-08-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  8), TInstant::ParseIso8601("2003-09-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  9), TInstant::ParseIso8601("2003-10-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 10), TInstant::ParseIso8601("2003-11-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 11), TInstant::ParseIso8601("2003-12-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 12), TInstant::ParseIso8601("2004-01-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 13), TInstant::ParseIso8601("2004-02-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 14), TInstant::ParseIso8601("2004-03-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 15), TInstant::ParseIso8601("2004-04-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 16), TInstant::ParseIso8601("2004-05-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 17), TInstant::ParseIso8601("2004-06-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 18), TInstant::ParseIso8601("2004-07-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 19), TInstant::ParseIso8601("2004-08-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 20), TInstant::ParseIso8601("2004-09-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 21), TInstant::ParseIso8601("2004-10-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 22), TInstant::ParseIso8601("2004-11-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 23), TInstant::ParseIso8601("2004-12-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 24), TInstant::ParseIso8601("2005-01-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 25), TInstant::ParseIso8601("2005-02-15T18:35:51Z") + subsec);
        }
        {
            const TDuration subsec = TDuration::MicroSeconds(123456);
            const TInstant start = TInstant::ParseIso8601("2004-01-15T18:35:51Z") + subsec;
            UNIT_ASSERT_VALUES_EQUAL(start.MicroSeconds(), 1074191751123456ul);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  0), TInstant::ParseIso8601("2004-01-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  1), TInstant::ParseIso8601("2004-02-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  2), TInstant::ParseIso8601("2004-03-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  3), TInstant::ParseIso8601("2004-04-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  4), TInstant::ParseIso8601("2004-05-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  5), TInstant::ParseIso8601("2004-06-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  6), TInstant::ParseIso8601("2004-07-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  7), TInstant::ParseIso8601("2004-08-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  8), TInstant::ParseIso8601("2004-09-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  9), TInstant::ParseIso8601("2004-10-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 10), TInstant::ParseIso8601("2004-11-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 11), TInstant::ParseIso8601("2004-12-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 12), TInstant::ParseIso8601("2005-01-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 13), TInstant::ParseIso8601("2005-02-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 14), TInstant::ParseIso8601("2005-03-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 15), TInstant::ParseIso8601("2005-04-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 16), TInstant::ParseIso8601("2005-05-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 17), TInstant::ParseIso8601("2005-06-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 18), TInstant::ParseIso8601("2005-07-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 19), TInstant::ParseIso8601("2005-08-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 20), TInstant::ParseIso8601("2005-09-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 21), TInstant::ParseIso8601("2005-10-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 22), TInstant::ParseIso8601("2005-11-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 23), TInstant::ParseIso8601("2005-12-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 24), TInstant::ParseIso8601("2006-01-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 25), TInstant::ParseIso8601("2006-02-15T18:35:51Z") + subsec);
        }
        {
            const TDuration subsec = TDuration::MicroSeconds(999999);
            const TInstant start = TInstant::ParseIso8601("2009-06-30T23:59:59Z") + subsec;
            UNIT_ASSERT_VALUES_EQUAL(start.MicroSeconds(), 1246406399999999ul);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  0), TInstant::ParseIso8601("2009-06-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  1), TInstant::ParseIso8601("2009-07-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  2), TInstant::ParseIso8601("2009-08-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  3), TInstant::ParseIso8601("2009-09-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  4), TInstant::ParseIso8601("2009-10-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  5), TInstant::ParseIso8601("2009-11-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  6), TInstant::ParseIso8601("2009-12-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  7), TInstant::ParseIso8601("2010-01-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  8), TInstant::ParseIso8601("2010-02-28T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  9), TInstant::ParseIso8601("2010-03-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 10), TInstant::ParseIso8601("2010-04-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 11), TInstant::ParseIso8601("2010-05-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 12), TInstant::ParseIso8601("2010-06-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 13), TInstant::ParseIso8601("2010-07-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 14), TInstant::ParseIso8601("2010-08-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 15), TInstant::ParseIso8601("2010-09-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 16), TInstant::ParseIso8601("2010-10-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 17), TInstant::ParseIso8601("2010-11-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 18), TInstant::ParseIso8601("2010-12-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 19), TInstant::ParseIso8601("2011-01-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 20), TInstant::ParseIso8601("2011-02-28T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 21), TInstant::ParseIso8601("2011-03-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 22), TInstant::ParseIso8601("2011-04-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 23), TInstant::ParseIso8601("2011-05-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 24), TInstant::ParseIso8601("2011-06-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 25), TInstant::ParseIso8601("2011-07-30T23:59:59Z") + subsec);
        }
        {
            const TDuration subsec = TDuration::MicroSeconds(123456);
            const TInstant start = TInstant::ParseIso8601("2005-01-15T18:35:51Z") + subsec;
            UNIT_ASSERT_VALUES_EQUAL(start.MicroSeconds(), 1105814151123456ul);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  0), TInstant::ParseIso8601("2005-01-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  1), TInstant::ParseIso8601("2005-02-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  2), TInstant::ParseIso8601("2005-03-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  3), TInstant::ParseIso8601("2005-04-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  4), TInstant::ParseIso8601("2005-05-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  5), TInstant::ParseIso8601("2005-06-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  6), TInstant::ParseIso8601("2005-07-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  7), TInstant::ParseIso8601("2005-08-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  8), TInstant::ParseIso8601("2005-09-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  9), TInstant::ParseIso8601("2005-10-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 10), TInstant::ParseIso8601("2005-11-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 11), TInstant::ParseIso8601("2005-12-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 12), TInstant::ParseIso8601("2006-01-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 13), TInstant::ParseIso8601("2006-02-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 14), TInstant::ParseIso8601("2006-03-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 15), TInstant::ParseIso8601("2006-04-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 16), TInstant::ParseIso8601("2006-05-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 17), TInstant::ParseIso8601("2006-06-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 18), TInstant::ParseIso8601("2006-07-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 19), TInstant::ParseIso8601("2006-08-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 20), TInstant::ParseIso8601("2006-09-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 21), TInstant::ParseIso8601("2006-10-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 22), TInstant::ParseIso8601("2006-11-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 23), TInstant::ParseIso8601("2006-12-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 24), TInstant::ParseIso8601("2007-01-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 25), TInstant::ParseIso8601("2007-02-15T18:35:51Z") + subsec);
        }
        {
            const TDuration subsec = TDuration::MicroSeconds(123456);
            const TInstant start = TInstant::ParseIso8601("2100-01-15T18:35:51Z") + subsec;
            UNIT_ASSERT_VALUES_EQUAL(start.MicroSeconds(), 4103721351123456ul);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  0), TInstant::ParseIso8601("2100-01-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  1), TInstant::ParseIso8601("2100-02-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  2), TInstant::ParseIso8601("2100-03-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  3), TInstant::ParseIso8601("2100-04-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  4), TInstant::ParseIso8601("2100-05-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  5), TInstant::ParseIso8601("2100-06-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  6), TInstant::ParseIso8601("2100-07-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  7), TInstant::ParseIso8601("2100-08-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  8), TInstant::ParseIso8601("2100-09-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  9), TInstant::ParseIso8601("2100-10-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 10), TInstant::ParseIso8601("2100-11-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 11), TInstant::ParseIso8601("2100-12-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 12), TInstant::ParseIso8601("2101-01-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 13), TInstant::ParseIso8601("2101-02-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 14), TInstant::ParseIso8601("2101-03-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 15), TInstant::ParseIso8601("2101-04-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 16), TInstant::ParseIso8601("2101-05-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 17), TInstant::ParseIso8601("2101-06-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 18), TInstant::ParseIso8601("2101-07-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 19), TInstant::ParseIso8601("2101-08-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 20), TInstant::ParseIso8601("2101-09-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 21), TInstant::ParseIso8601("2101-10-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 22), TInstant::ParseIso8601("2101-11-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 23), TInstant::ParseIso8601("2101-12-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 24), TInstant::ParseIso8601("2102-01-15T18:35:51Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 25), TInstant::ParseIso8601("2102-02-15T18:35:51Z") + subsec);
        }
        {
            const TDuration subsec = TDuration::MicroSeconds(0);
            const TInstant start = TInstant::ParseIso8601("2022-01-31T00:00:00Z") + subsec;
            UNIT_ASSERT_VALUES_EQUAL(start.MicroSeconds(), 1643587200000000ul);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  0), TInstant::ParseIso8601("2022-01-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  1), TInstant::ParseIso8601("2022-02-28T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  2), TInstant::ParseIso8601("2022-03-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  3), TInstant::ParseIso8601("2022-04-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  4), TInstant::ParseIso8601("2022-05-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  5), TInstant::ParseIso8601("2022-06-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  6), TInstant::ParseIso8601("2022-07-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  7), TInstant::ParseIso8601("2022-08-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  8), TInstant::ParseIso8601("2022-09-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  9), TInstant::ParseIso8601("2022-10-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 10), TInstant::ParseIso8601("2022-11-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 11), TInstant::ParseIso8601("2022-12-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 12), TInstant::ParseIso8601("2023-01-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 13), TInstant::ParseIso8601("2023-02-28T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 14), TInstant::ParseIso8601("2023-03-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 15), TInstant::ParseIso8601("2023-04-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 16), TInstant::ParseIso8601("2023-05-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 17), TInstant::ParseIso8601("2023-06-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 18), TInstant::ParseIso8601("2023-07-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 19), TInstant::ParseIso8601("2023-08-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 20), TInstant::ParseIso8601("2023-09-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 21), TInstant::ParseIso8601("2023-10-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 22), TInstant::ParseIso8601("2023-11-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 23), TInstant::ParseIso8601("2023-12-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 24), TInstant::ParseIso8601("2024-01-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 25), TInstant::ParseIso8601("2024-02-29T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 26), TInstant::ParseIso8601("2024-03-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 27), TInstant::ParseIso8601("2024-04-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 28), TInstant::ParseIso8601("2024-05-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 29), TInstant::ParseIso8601("2024-06-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 30), TInstant::ParseIso8601("2024-07-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 31), TInstant::ParseIso8601("2024-08-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 32), TInstant::ParseIso8601("2024-09-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 33), TInstant::ParseIso8601("2024-10-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 34), TInstant::ParseIso8601("2024-11-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 35), TInstant::ParseIso8601("2024-12-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 36), TInstant::ParseIso8601("2025-01-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 37), TInstant::ParseIso8601("2025-02-28T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 38), TInstant::ParseIso8601("2025-03-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 39), TInstant::ParseIso8601("2025-04-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 40), TInstant::ParseIso8601("2025-05-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 41), TInstant::ParseIso8601("2025-06-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 42), TInstant::ParseIso8601("2025-07-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 43), TInstant::ParseIso8601("2025-08-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 44), TInstant::ParseIso8601("2025-09-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 45), TInstant::ParseIso8601("2025-10-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 46), TInstant::ParseIso8601("2025-11-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 47), TInstant::ParseIso8601("2025-12-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 48), TInstant::ParseIso8601("2026-01-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 49), TInstant::ParseIso8601("2026-02-28T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 50), TInstant::ParseIso8601("2026-03-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 51), TInstant::ParseIso8601("2026-04-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 52), TInstant::ParseIso8601("2026-05-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 53), TInstant::ParseIso8601("2026-06-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 54), TInstant::ParseIso8601("2026-07-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 55), TInstant::ParseIso8601("2026-08-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 56), TInstant::ParseIso8601("2026-09-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 57), TInstant::ParseIso8601("2026-10-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 58), TInstant::ParseIso8601("2026-11-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 59), TInstant::ParseIso8601("2026-12-31T00:00:00Z") + subsec);
        }
        {
            const TDuration subsec = TDuration::MicroSeconds(0);
            const TInstant start = TInstant::ParseIso8601("2022-01-31T00:00:00Z") + subsec;
            UNIT_ASSERT_VALUES_EQUAL(start.MicroSeconds(), 1643587200000000ul);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,   0), TInstant::ParseIso8601("2022-01-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  -1), TInstant::ParseIso8601("2021-12-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  -2), TInstant::ParseIso8601("2021-11-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  -3), TInstant::ParseIso8601("2021-10-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  -4), TInstant::ParseIso8601("2021-09-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  -5), TInstant::ParseIso8601("2021-08-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  -6), TInstant::ParseIso8601("2021-07-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  -7), TInstant::ParseIso8601("2021-06-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  -8), TInstant::ParseIso8601("2021-05-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  -9), TInstant::ParseIso8601("2021-04-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -10), TInstant::ParseIso8601("2021-03-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -11), TInstant::ParseIso8601("2021-02-28T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -12), TInstant::ParseIso8601("2021-01-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -13), TInstant::ParseIso8601("2020-12-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -14), TInstant::ParseIso8601("2020-11-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -15), TInstant::ParseIso8601("2020-10-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -16), TInstant::ParseIso8601("2020-09-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -17), TInstant::ParseIso8601("2020-08-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -18), TInstant::ParseIso8601("2020-07-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -19), TInstant::ParseIso8601("2020-06-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -20), TInstant::ParseIso8601("2020-05-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -21), TInstant::ParseIso8601("2020-04-30T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -22), TInstant::ParseIso8601("2020-03-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -23), TInstant::ParseIso8601("2020-02-29T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -24), TInstant::ParseIso8601("2020-01-31T00:00:00Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -25), TInstant::ParseIso8601("2019-12-31T00:00:00Z") + subsec);
        }
        {
            const TDuration subsec = TDuration::MicroSeconds(999999);
            const TInstant start = TInstant::ParseIso8601("2022-01-31T23:59:59Z") + subsec;
            UNIT_ASSERT_VALUES_EQUAL(start.MicroSeconds(), 1643673599999999ul);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  0), TInstant::ParseIso8601("2022-01-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  1), TInstant::ParseIso8601("2022-02-28T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  2), TInstant::ParseIso8601("2022-03-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  3), TInstant::ParseIso8601("2022-04-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  4), TInstant::ParseIso8601("2022-05-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  5), TInstant::ParseIso8601("2022-06-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  6), TInstant::ParseIso8601("2022-07-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  7), TInstant::ParseIso8601("2022-08-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  8), TInstant::ParseIso8601("2022-09-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  9), TInstant::ParseIso8601("2022-10-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 10), TInstant::ParseIso8601("2022-11-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 11), TInstant::ParseIso8601("2022-12-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 12), TInstant::ParseIso8601("2023-01-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 13), TInstant::ParseIso8601("2023-02-28T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 14), TInstant::ParseIso8601("2023-03-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 15), TInstant::ParseIso8601("2023-04-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 16), TInstant::ParseIso8601("2023-05-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 17), TInstant::ParseIso8601("2023-06-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 18), TInstant::ParseIso8601("2023-07-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 19), TInstant::ParseIso8601("2023-08-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 20), TInstant::ParseIso8601("2023-09-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 21), TInstant::ParseIso8601("2023-10-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 22), TInstant::ParseIso8601("2023-11-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 23), TInstant::ParseIso8601("2023-12-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 24), TInstant::ParseIso8601("2024-01-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 25), TInstant::ParseIso8601("2024-02-29T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 26), TInstant::ParseIso8601("2024-03-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 27), TInstant::ParseIso8601("2024-04-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 28), TInstant::ParseIso8601("2024-05-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 29), TInstant::ParseIso8601("2024-06-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 30), TInstant::ParseIso8601("2024-07-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 31), TInstant::ParseIso8601("2024-08-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 32), TInstant::ParseIso8601("2024-09-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 33), TInstant::ParseIso8601("2024-10-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 34), TInstant::ParseIso8601("2024-11-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 35), TInstant::ParseIso8601("2024-12-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 36), TInstant::ParseIso8601("2025-01-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 37), TInstant::ParseIso8601("2025-02-28T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 38), TInstant::ParseIso8601("2025-03-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 39), TInstant::ParseIso8601("2025-04-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 40), TInstant::ParseIso8601("2025-05-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 41), TInstant::ParseIso8601("2025-06-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 42), TInstant::ParseIso8601("2025-07-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 43), TInstant::ParseIso8601("2025-08-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 44), TInstant::ParseIso8601("2025-09-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 45), TInstant::ParseIso8601("2025-10-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 46), TInstant::ParseIso8601("2025-11-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 47), TInstant::ParseIso8601("2025-12-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 48), TInstant::ParseIso8601("2026-01-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 49), TInstant::ParseIso8601("2026-02-28T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 50), TInstant::ParseIso8601("2026-03-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 51), TInstant::ParseIso8601("2026-04-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 52), TInstant::ParseIso8601("2026-05-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 53), TInstant::ParseIso8601("2026-06-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 54), TInstant::ParseIso8601("2026-07-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 55), TInstant::ParseIso8601("2026-08-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 56), TInstant::ParseIso8601("2026-09-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 57), TInstant::ParseIso8601("2026-10-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 58), TInstant::ParseIso8601("2026-11-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 59), TInstant::ParseIso8601("2026-12-31T23:59:59Z") + subsec);
        }
        {
            const TDuration subsec = TDuration::MicroSeconds(999999);
            const TInstant start = TInstant::ParseIso8601("2026-12-31T23:59:59Z") + subsec;
            UNIT_ASSERT_VALUES_EQUAL(start.MicroSeconds(), 1798761599999999ul);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,   0), TInstant::ParseIso8601("2026-12-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  -1), TInstant::ParseIso8601("2026-11-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  -2), TInstant::ParseIso8601("2026-10-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  -3), TInstant::ParseIso8601("2026-09-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  -4), TInstant::ParseIso8601("2026-08-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  -5), TInstant::ParseIso8601("2026-07-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  -6), TInstant::ParseIso8601("2026-06-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  -7), TInstant::ParseIso8601("2026-05-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  -8), TInstant::ParseIso8601("2026-04-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start,  -9), TInstant::ParseIso8601("2026-03-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -10), TInstant::ParseIso8601("2026-02-28T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -11), TInstant::ParseIso8601("2026-01-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -12), TInstant::ParseIso8601("2025-12-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -13), TInstant::ParseIso8601("2025-11-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -14), TInstant::ParseIso8601("2025-10-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -15), TInstant::ParseIso8601("2025-09-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -16), TInstant::ParseIso8601("2025-08-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -17), TInstant::ParseIso8601("2025-07-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -18), TInstant::ParseIso8601("2025-06-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -19), TInstant::ParseIso8601("2025-05-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -20), TInstant::ParseIso8601("2025-04-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -21), TInstant::ParseIso8601("2025-03-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -22), TInstant::ParseIso8601("2025-02-28T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -23), TInstant::ParseIso8601("2025-01-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -24), TInstant::ParseIso8601("2024-12-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -25), TInstant::ParseIso8601("2024-11-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -26), TInstant::ParseIso8601("2024-10-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -27), TInstant::ParseIso8601("2024-09-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -28), TInstant::ParseIso8601("2024-08-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -29), TInstant::ParseIso8601("2024-07-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -30), TInstant::ParseIso8601("2024-06-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -31), TInstant::ParseIso8601("2024-05-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -32), TInstant::ParseIso8601("2024-04-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -33), TInstant::ParseIso8601("2024-03-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -34), TInstant::ParseIso8601("2024-02-29T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -35), TInstant::ParseIso8601("2024-01-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -36), TInstant::ParseIso8601("2023-12-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -37), TInstant::ParseIso8601("2023-11-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -38), TInstant::ParseIso8601("2023-10-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -39), TInstant::ParseIso8601("2023-09-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -40), TInstant::ParseIso8601("2023-08-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -41), TInstant::ParseIso8601("2023-07-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -42), TInstant::ParseIso8601("2023-06-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -43), TInstant::ParseIso8601("2023-05-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -44), TInstant::ParseIso8601("2023-04-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -45), TInstant::ParseIso8601("2023-03-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -46), TInstant::ParseIso8601("2023-02-28T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -47), TInstant::ParseIso8601("2023-01-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -48), TInstant::ParseIso8601("2022-12-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -49), TInstant::ParseIso8601("2022-11-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -50), TInstant::ParseIso8601("2022-10-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -51), TInstant::ParseIso8601("2022-09-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -52), TInstant::ParseIso8601("2022-08-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -53), TInstant::ParseIso8601("2022-07-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -54), TInstant::ParseIso8601("2022-06-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -55), TInstant::ParseIso8601("2022-05-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -56), TInstant::ParseIso8601("2022-04-30T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -57), TInstant::ParseIso8601("2022-03-31T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -58), TInstant::ParseIso8601("2022-02-28T23:59:59Z") + subsec);
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, -59), TInstant::ParseIso8601("2022-01-31T23:59:59Z") + subsec);
        }
    }

    Y_UNIT_TEST(ShiftCalendarMonthsStepBounds) {
        const TDuration subsec = TDuration::MicroSeconds(999999);

        const TInstant start = TInstant::ParseIso8601("1970-01-01T23:59:59Z") + subsec;
        UNIT_ASSERT_VALUES_EQUAL(start.MicroSeconds(), 86399999999ul);

        for (i32 i = 0; i < 1200; ++i) {
            const auto curr = NUtil::ShiftCalendarMonths(start, i);
            const auto next = NUtil::ShiftCalendarMonths(start, i + 1);
            const TDuration distance = next - curr;
            UNIT_ASSERT_LE_C(distance, TDuration::Days(31), "distance between " << start << " + " << i << " months (" << curr << ") and + " << i + 1 << " months (" << next << ") is too big: " << distance << ", expected to be less or equal to " << TDuration::Days(31) << ".");
            UNIT_ASSERT_GE_C(distance, TDuration::Days(28), "distance between " << start << " + " << i << " months (" << curr << ") and + " << i + 1 << " months (" << next << ") is too small: " << distance << ", expected to be greater or equal to " << TDuration::Days(28) << ".");
        }
    }

    Y_UNIT_TEST(ShiftCalendarMonthsSelfCheck) {
        const TDuration subsec = TDuration::MicroSeconds(999999);

        const TInstant first = TInstant::ParseIso8601("1970-01-01T23:59:59Z") + subsec;
        UNIT_ASSERT_VALUES_EQUAL(first.MicroSeconds(), 86399999999ul);

        const TInstant last = TInstant::ParseIso8601("2070-01-01T23:59:59Z") + subsec;
        UNIT_ASSERT_VALUES_EQUAL(last.MicroSeconds(), 3155846399999999ul);

        for (i32 i = 0; i <= 1200; ++i) {
            UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(first, i), NUtil::ShiftCalendarMonths(last, i - 1200));
        }
    }

    Y_UNIT_TEST(ShiftCalendarMonthsUTC3) {
        const TDuration subsec = TDuration::MicroSeconds(0);

        const TInstant start = TInstant::ParseIso8601("2022-02-28T23:00:00Z") + subsec;
        UNIT_ASSERT_VALUES_EQUAL(start.MicroSeconds(), 1646089200000000ul);

        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 1), TInstant::ParseIso8601("2022-03-28T23:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 2), TInstant::ParseIso8601("2022-04-28T23:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 3), TInstant::ParseIso8601("2022-05-28T23:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 4), TInstant::ParseIso8601("2022-06-28T23:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 5), TInstant::ParseIso8601("2022-07-28T23:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 6), TInstant::ParseIso8601("2022-08-28T23:00:00Z") + subsec);

        constexpr auto UTC3 = std::chrono::hours{+3};

        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 0, UTC3), TInstant::ParseIso8601("2022-02-28T23:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 1, UTC3), TInstant::ParseIso8601("2022-03-31T23:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 2, UTC3), TInstant::ParseIso8601("2022-04-30T23:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 3, UTC3), TInstant::ParseIso8601("2022-05-31T23:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 4, UTC3), TInstant::ParseIso8601("2022-06-30T23:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 5, UTC3), TInstant::ParseIso8601("2022-07-31T23:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 6, UTC3), TInstant::ParseIso8601("2022-08-31T23:00:00Z") + subsec);

        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 0, UTC3) + UTC3, TInstant::ParseIso8601("2022-03-01T02:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 1, UTC3) + UTC3, TInstant::ParseIso8601("2022-04-01T02:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 2, UTC3) + UTC3, TInstant::ParseIso8601("2022-05-01T02:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 3, UTC3) + UTC3, TInstant::ParseIso8601("2022-06-01T02:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 4, UTC3) + UTC3, TInstant::ParseIso8601("2022-07-01T02:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 5, UTC3) + UTC3, TInstant::ParseIso8601("2022-08-01T02:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 6, UTC3) + UTC3, TInstant::ParseIso8601("2022-09-01T02:00:00Z") + subsec);
    }

    Y_UNIT_TEST(ShiftCalendarMonthsUTC_5) {
        const TDuration subsec = TDuration::MicroSeconds(0);

        const TInstant start = TInstant::ParseIso8601("2022-02-01T01:00:00Z") + subsec;
        UNIT_ASSERT_VALUES_EQUAL(start.MicroSeconds(), 1643677200000000ul);

        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 0), TInstant::ParseIso8601("2022-02-01T01:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 1), TInstant::ParseIso8601("2022-03-01T01:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 2), TInstant::ParseIso8601("2022-04-01T01:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 3), TInstant::ParseIso8601("2022-05-01T01:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 4), TInstant::ParseIso8601("2022-06-01T01:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 5), TInstant::ParseIso8601("2022-07-01T01:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 6), TInstant::ParseIso8601("2022-08-01T01:00:00Z") + subsec);

        constexpr auto UTC_5 = std::chrono::hours{-5};

        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 0, UTC_5), TInstant::ParseIso8601("2022-02-01T01:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 1, UTC_5), TInstant::ParseIso8601("2022-03-01T01:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 2, UTC_5), TInstant::ParseIso8601("2022-04-01T01:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 3, UTC_5), TInstant::ParseIso8601("2022-05-01T01:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 4, UTC_5), TInstant::ParseIso8601("2022-06-01T01:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 5, UTC_5), TInstant::ParseIso8601("2022-07-01T01:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 6, UTC_5), TInstant::ParseIso8601("2022-08-01T01:00:00Z") + subsec);

        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 0, UTC_5) + UTC_5, TInstant::ParseIso8601("2022-01-31T20:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 1, UTC_5) + UTC_5, TInstant::ParseIso8601("2022-02-28T20:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 2, UTC_5) + UTC_5, TInstant::ParseIso8601("2022-03-31T20:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 3, UTC_5) + UTC_5, TInstant::ParseIso8601("2022-04-30T20:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 4, UTC_5) + UTC_5, TInstant::ParseIso8601("2022-05-31T20:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 5, UTC_5) + UTC_5, TInstant::ParseIso8601("2022-06-30T20:00:00Z") + subsec);
        UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(start, 6, UTC_5) + UTC_5, TInstant::ParseIso8601("2022-07-31T20:00:00Z") + subsec);
    }

    Y_UNIT_TEST(ShiftCalendarMonthsSelfCheckUTCOffsets) {
        const TDuration subsec = TDuration::MicroSeconds(999999);

        const TInstant first = TInstant::ParseIso8601("1970-01-01T23:59:59Z") + subsec;
        UNIT_ASSERT_VALUES_EQUAL(first.MicroSeconds(), 86399999999ul);

        const TInstant last = TInstant::ParseIso8601("2070-01-01T23:59:59Z") + subsec;
        UNIT_ASSERT_VALUES_EQUAL(last.MicroSeconds(), 3155846399999999ul);

        for (i32 utcOffsetHours = -12; utcOffsetHours <= 12; ++utcOffsetHours) {
            const auto offset = std::chrono::hours(utcOffsetHours);
            for (i32 i = 0; i <= 1200; ++i) {
                UNIT_ASSERT_VALUES_EQUAL(NUtil::ShiftCalendarMonths(first, i, offset), NUtil::ShiftCalendarMonths(last, i - 1200, offset));
            }
        }
    }

    Y_UNIT_TEST(LeapSecond) {
        const TDuration subsec = TDuration::MicroSeconds(999999);

        const TInstant lastLeapSecond = TInstant::ParseIso8601("2016-12-31T23:59:60Z") + subsec;
        UNIT_ASSERT_VALUES_EQUAL(lastLeapSecond.MicroSeconds(), 1483228800999999);
        UNIT_ASSERT_VALUES_EQUAL(lastLeapSecond.ToStringUpToSeconds(), "2017-01-01T00:00:00Z");

        const TInstant lastLeapSecondAlt = TInstant::ParseIso8601("2017-01-01T00:00:00Z") + subsec;
        UNIT_ASSERT_VALUES_EQUAL(lastLeapSecondAlt.MicroSeconds(), 1483228800999999);

        UNIT_ASSERT_VALUES_EQUAL(lastLeapSecond, lastLeapSecondAlt);
    }

    Y_UNIT_TEST(FormatDuration) {
        UNIT_ASSERT_VALUES_EQUAL(NUtil::FormatDuration(TDuration::Zero()), "0ms");
        UNIT_ASSERT_VALUES_EQUAL(NUtil::FormatDuration(TDuration::MicroSeconds(999)), "0ms");
        UNIT_ASSERT_VALUES_EQUAL(NUtil::FormatDuration(TDuration::MicroSeconds(123999)), "123ms");
        UNIT_ASSERT_VALUES_EQUAL(NUtil::FormatDuration(TDuration::MilliSeconds(123)), "123ms");
        UNIT_ASSERT_VALUES_EQUAL(NUtil::FormatDuration(TDuration::Seconds(123123) + TDuration::MicroSeconds(505050)), "123123s");
        UNIT_ASSERT_VALUES_EQUAL(NUtil::FormatDuration(TDuration::Minutes(654321)), "654321m");
        UNIT_ASSERT_VALUES_EQUAL(NUtil::FormatDuration(TDuration::Hours(555)), "555h");
        UNIT_ASSERT_VALUES_EQUAL(NUtil::FormatDuration(TDuration::Days(42)), "42d");
        UNIT_ASSERT_VALUES_EQUAL(NUtil::FormatDuration(TDuration::Days(42) + TDuration::Seconds(1)), "3628801s");
    }
}
