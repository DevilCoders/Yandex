#include <kernel/common_server/library/time_restriction/time_restriction.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/logger/global/global.h>

Y_UNIT_TEST_SUITE(TimeRestrictionSuite) {

    void CheckRestriction(const TTimeRestriction& r, const ui32 secondsFrom, const ui32 secondsTo) {
        TVector<TInstant> switches;
        i32 state = 0;
        for (ui32 i = secondsFrom / 60; i < secondsTo / 60; ++i) {
            TInstant t = TInstant::Minutes(i);
            bool isActual1 = r.IsActualNow(t);
            bool isActual2 = r.IsActualNowTrivial(t);
            if (isActual1 != isActual2) {
                isActual1 = r.IsActualNow(t);
                isActual2 = r.IsActualNowTrivial(t);
                S_FAIL_LOG << "FAIL";
            }
            if (state == 0 || (state == 1) != isActual1) {
                state = isActual1 ? 1 : -1;
                switches.push_back(t);
            }
        }
        TVector<TInstant> switchesSimple;
        switchesSimple.push_back(TInstant::Minutes(secondsFrom / 60));
        while (switchesSimple.back() < TInstant::Minutes(secondsTo / 60)) {
            switchesSimple.push_back(r.GetNextSwitching(switchesSimple.back()));
        }
        for (ui32 i = 0; i < switches.size(); ++i) {
            if (switchesSimple[i] != switches[i]) {
                r.GetNextSwitching(switchesSimple[i - 1]);
                S_FAIL_LOG << "FAIL";
            }
        }
    }

    Y_UNIT_TEST(TimeRestrictionsPools) {
        {
            TTimeRestrictionsPool restrictions;
            TTimeRestriction restriction1;
            restriction1.SetTimeRestriction(700, 1100);
            restriction1.SetDayRestriction(85);
            CheckRestriction(restriction1, 210000, 410000);
            TTimeRestriction restriction2;
            restriction2.SetDateRestriction(101, 212);
            CheckRestriction(restriction2, 210000, 410000);
            restrictions.Add(restriction1);
            restrictions.Add(restriction2);
            CHECK_WITH_LOG(restrictions.GetNextCorrectionTime(TInstant::Seconds(284400), TDuration::Days(1000)) == TInstant::Seconds(3628800));
            for (ui32 i = 210000; i < 910000; ++i) {
                CHECK_WITH_LOG(restrictions.IsActualNow(TInstant::Seconds(i)));
                CHECK_WITH_LOG(restrictions.GetNextCorrectionTime(TInstant::Seconds(i), TDuration::Days(1000)) == TInstant::Seconds(3628800)) << i;
            }
        }
        {
            TTimeRestrictionsPool restrictions;
            TTimeRestriction restriction1;
            restriction1.SetTimeRestriction(0, 700);
            restriction1.SetDayRestriction(127);
            CheckRestriction(restriction1, 210000, 410000);
            TTimeRestriction restriction2;
            restriction2.SetTimeRestriction(1800, 0);
            restriction2.SetDayRestriction(127);
            CheckRestriction(restriction2, 210000, 410000);
            restrictions.Add(restriction1);
            restrictions.Add(restriction2);
            for (ui32 i = 210000; i < 410000; ++i) {
                restrictions.GetNextCorrectionTime(TInstant::Seconds(i));
            }
        }
    }

    Y_UNIT_TEST(TimeRestrictionsMain) {
        {
            TTimeRestriction restriction1;
            restriction1.SetTimeRestriction(700, 1100);
            restriction1.SetDayRestriction(85);
            CheckRestriction(restriction1, 210000, 410000);
            TTimeRestriction restriction2;
            restriction2.SetDateRestriction(101, 212);
            CheckRestriction(restriction2, 210000, 410000);
        }
        {
            TTimeRestriction restriction1;
            restriction1.SetTimeRestriction(0, 700);
            restriction1.SetDayRestriction(127);
            CheckRestriction(restriction1, 210000, 410000);
            TTimeRestriction restriction2;
            restriction2.SetTimeRestriction(1800, 0);
            restriction2.SetDayRestriction(127);
            CheckRestriction(restriction2, 210000, 410000);
        }
        {
            TTimeRestriction restriction;
            restriction.SetDateRestriction(401, 1002);
            restriction.SetTimeRestriction(0, 2359);
            restriction.SetDayRestriction(112);
            CheckRestriction(restriction, 210000, 410000);
        }
        {
            TTimeRestriction restriction;
            restriction.SetDateRestriction(101, 1231);
            restriction.SetTimeRestriction(1600, 900);
            restriction.SetDayRestriction(32);
            CheckRestriction(restriction, 210000, 410000);
        }
        TVector<ui8> days = { 85, 255, 112 };
        for (auto&& dayFilter : days) {
            {
                TTimeRestriction restriction;
                restriction.SetDayRestriction(dayFilter);
                CheckRestriction(restriction, 210000, 410000);
            }
            {
                TTimeRestriction restriction;
                restriction.SetTimeRestriction(0, 0);
                restriction.SetDayRestriction(dayFilter);
                CheckRestriction(restriction, 210000, 410000);
            }
            {
                TTimeRestriction restriction;
                restriction.SetTimeRestriction(1010, 2020);
                restriction.SetDayRestriction(dayFilter);
                CheckRestriction(restriction, 210000, 410000);
            }
            {
                TTimeRestriction restriction;
                restriction.SetTimeRestriction(2020, 1010);
                restriction.SetDayRestriction(dayFilter);
                CheckRestriction(restriction, 210000, 410000);
            }
            {
                TTimeRestriction restriction;
                restriction.SetDateRestriction(404, 606);
                restriction.SetTimeRestriction(1010, 2020);
                restriction.SetDayRestriction(dayFilter);
                CheckRestriction(restriction, 951806400, 956806400);
            }
            {
                TTimeRestriction restriction;
                restriction.SetDateRestriction(606, 404);
                restriction.SetDayRestriction(dayFilter);
                CheckRestriction(restriction, 951806400, 956806400);
            }
            {
                TTimeRestriction restriction;
                restriction.SetDateRestriction(404, 606);
                restriction.SetDayRestriction(dayFilter);
                CheckRestriction(restriction, 951806400, 956806400);
            }
            {
                TTimeRestriction restriction;
                restriction.SetDateRestriction(606, 404);
                restriction.SetTimeRestriction(2020, 1010);
                restriction.SetDayRestriction(dayFilter);
                CheckRestriction(restriction, 951806400, 956806400);
            }
            {
                TTimeRestriction restriction;
                restriction.SetDateRestriction(606, 404);
                restriction.SetTimeRestriction(1010, 2020);
                restriction.SetDayRestriction(dayFilter);
                CheckRestriction(restriction, 951806400, 956806400);
            }
            {
                TTimeRestriction restriction;
                restriction.SetDateRestriction(606, 404);
                restriction.SetTimeRestriction(2020, 1010);
                restriction.SetDayRestriction(dayFilter);
                CheckRestriction(restriction, 951806400, 956806400);
            }
            {
                TTimeRestriction restriction;
                restriction.SetDateRestriction(606, 404);
                restriction.SetTimeRestriction(1010, 2020);
                restriction.SetDayRestriction(dayFilter);
                CheckRestriction(restriction, 951806400, 956806400);
            }
            {
                TTimeRestriction restriction;
                restriction.SetDateRestriction(404, 606);
                restriction.SetTimeRestriction(1010, 2020);
                restriction.SetDayRestriction(dayFilter);
                CheckRestriction(restriction, 951806400, 956806400);
            }
        }
    }

    Y_UNIT_TEST(CrossSize) {
        {
            TTimeRestriction rr;
            rr.SetTimeRestriction(200, 300);
            rr.SetDayRestriction(TTimeRestriction::wdMonday);

            TDuration result = rr.GetCrossSize(TInstant::Days(100), TInstant::Days(114));
            CHECK_WITH_LOG(result.Hours() == 2);
        }
        {
            TTimeRestriction rr;
            rr.SetTimeRestriction(2200, 300);
            rr.SetDayRestriction(TTimeRestriction::wdMonday | TTimeRestriction::wdTuesday);

            TDuration result = rr.GetCrossSize(TInstant::Days(100), TInstant::Days(114));
            CHECK_WITH_LOG(result.Hours() == 20);
        }
        {
            TTimeRestriction rr;
            rr.SetTimeRestriction(2200, 300);
            rr.SetDayRestriction(TTimeRestriction::wdFriday);

            TDuration result = rr.GetCrossSize(TInstant::Seconds(165600), TInstant::Seconds(165600 + 3600));
            CHECK_WITH_LOG(result.Hours() == 1);
        }
        {
            TTimeRestriction rr;
            rr.SetDateRestriction(107, 1231);
            rr.SetTimeRestriction(2200, 300);
            rr.SetDayRestriction(TTimeRestriction::wdMonday | TTimeRestriction::wdTuesday);

            TDuration result = rr.GetCrossSize(TInstant::Seconds(165600), TInstant::Seconds(165600) + TDuration::Days(14));
            CHECK_WITH_LOG(result.Hours() == 10 + 3);
        }
    }

    void CheckSerialization(const TTimeRestriction& r) {
        const TString ss = r.SerializeAsString();
        TTimeRestriction rNew;
        CHECK_WITH_LOG(rNew.DeserializeFromString(ss));
        CHECK_WITH_LOG(rNew == r);
    }

    Y_UNIT_TEST(Deserialization) {
        TTimeRestriction rr;
        rr.SetDateRestriction(110, 210);
        rr.SetTimeRestriction(100, 200);
        rr.SetDayRestriction(TTimeRestriction::wdMonday | TTimeRestriction::wdFriday);

        {
            TTimeRestriction rNew;
            CHECK_WITH_LOG(rNew.DeserializeFromString(":2000-:900-"));
            CHECK_WITH_LOG(rNew.IsActualNow(TInstant::Seconds(1492947038)) == rNew.IsActualNowTrivial(TInstant::Seconds(1492947038)));
        }
        {
            TTimeRestriction rNew;
            CHECK_WITH_LOG(rNew.DeserializeFromString("  110: 100 - 210  : 200  - 17"));
            CHECK_WITH_LOG(rNew == rr);
        }
        {
            TTimeRestriction rNew;
            CHECK_WITH_LOG(rNew.DeserializeFromString("  110: 100 - 210  : 200  -   monday   |  friday"));
            CHECK_WITH_LOG(rNew == rr);
        }
    }

    Y_UNIT_TEST(Serialization) {
        {
            TTimeRestriction rr;
            CheckSerialization(rr);
        }
        {
            TTimeRestriction rr;
            rr.SetTimeRestriction(100, 200);
            CheckSerialization(rr);
        }
        {
            TTimeRestriction rr;
            rr.SetDateRestriction(110, 210);
            CheckSerialization(rr);
        }
        {
            TTimeRestriction rr;
            rr.SetDayRestriction(TTimeRestriction::wdMonday | TTimeRestriction::wdFriday);
            CheckSerialization(rr);
        }
        {
            TTimeRestriction rr;
            rr.SetDateRestriction(110, 210);
            rr.SetTimeRestriction(100, 200);
            rr.SetDayRestriction(TTimeRestriction::wdMonday | TTimeRestriction::wdFriday);
            CheckSerialization(rr);
        }
    }

    Y_UNIT_TEST(Year1970) {
        TTimeRestriction rr;
        rr.SetDateRestriction(1001, 430);
        rr.SetTimeRestriction(1700, 900);
        rr.SetDayRestriction(127);
        rr.SetTimezoneShift(3);
        rr.Compile();

        TInstant timestamp = TInstant::Seconds(3);
        TInstant nextSwitching = rr.GetNextSwitching(timestamp);
        UNIT_ASSERT_VALUES_EQUAL(nextSwitching, TInstant::Hours(6));
    }

    Y_UNIT_TEST(ScrewUp2) {
        TTimeRestriction rr;
        rr.SetDateRestriction(101, 1231);
        rr.SetTimeRestriction(900, 2000);
        rr.SetDayRestriction(112);
        rr.SetTimezoneShift(3);
        rr.Compile();

        TInstant timestamp = TInstant::Seconds(7956915720);
        TInstant nextSwitching = rr.GetNextSwitching(timestamp);
        UNIT_ASSERT_VALUES_EQUAL(nextSwitching, TInstant::Seconds(7956943200));
    }

    Y_UNIT_TEST(Weekend) {
        i64 tzs = 3;
        TTimeRestrictionsPool restrictions;
        TTimeRestriction weekend;
        weekend.SetDayRestriction(TTimeRestriction::wdSaturday | TTimeRestriction::wdSunday);
        weekend.SetTimezoneShift(tzs);
        restrictions.Add(weekend);

        TTimeRestriction friday;
        friday.SetDayRestriction(TTimeRestriction::wdFriday);
        friday.SetTimeRestriction(1800, 2400);
        friday.SetTimezoneShift(tzs);
        restrictions.Add(friday);
        Cout << restrictions.SerializeToJson().GetStringRobust() << Endl;
        {
            TInstant timestamp = TInstant::Seconds(1560297600);
            UNIT_ASSERT(!restrictions.IsActualNow(timestamp));
        }
        {
            TInstant timestamp = TInstant::Seconds(1560595094);
            UNIT_ASSERT(restrictions.IsActualNow(timestamp));
            TInstant finish = restrictions.GetNextCorrectionTime(timestamp);
            UNIT_ASSERT_VALUES_EQUAL(finish, TInstant::Seconds(1560718800));
        }
        {
            TInstant timestamp = TInstant::Seconds(1560542400);
            UNIT_ASSERT(restrictions.IsActualNow(timestamp));
            TInstant finish = restrictions.GetNextCorrectionTime(timestamp);
            UNIT_ASSERT_VALUES_EQUAL(finish, TInstant::Seconds(1560718800));
        }
    }
}
