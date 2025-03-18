#include "rpsschedule.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/map.h>
#include <util/generic/algorithm.h>

#define GEN_DICTLIKE_OUT( TYPE ) \
    template <> \
    void Out< TYPE >(IOutputStream& os, const TYPE & v) { \
        bool first = true; \
        os << "{"; \
        for (const TYPE ::value_type& el : v ) { \
            if (!first) \
                os << ", "; \
            os << el.first << ": " << el.second; \
            first = false; \
        } \
        os << "}"; \
    }

typedef TMap<time_t, int> TRpsCounter;
GEN_DICTLIKE_OUT(TRpsCounter);

#define GOOD(x) UNIT_ASSERT_NO_EXCEPTION(s = FromString<TRpsSchedule>(x))
#define BAD(x) UNIT_ASSERT_EXCEPTION(s = FromString<TRpsSchedule>(x), yexception)

#define RPS(t, v) UNIT_ASSERT_EQUAL_C(rps[t], (v), "t eq " << t)

Y_UNIT_TEST_SUITE(TestRpsScheduleParser) {
    TRpsSchedule s;

    Y_UNIT_TEST(Tank) {
        GOOD("step(25, 5, 5, 60)");
        UNIT_ASSERT_EQUAL(s.size(), 1);
        UNIT_ASSERT_EQUAL(s[0].Mode, SM_STEP);
        UNIT_ASSERT_EQUAL(s[0].Duration, TDuration::Seconds(60));
        UNIT_ASSERT_EQUAL(s[0].CountLadderSteps(), 5);
        UNIT_ASSERT_DOUBLES_EQUAL(s[0].BeginRps, 25, 1e-6);
        UNIT_ASSERT_DOUBLES_EQUAL(s[0].EndRps, 5, 1e-6);
        UNIT_ASSERT_DOUBLES_EQUAL(s[0].StepRps, 5, 1e-6);

        GOOD("step(5, 25, 5, 60)");
        UNIT_ASSERT_EQUAL(s.size(), 1);
        UNIT_ASSERT_EQUAL(s[0].CountLadderSteps(), 5);
        UNIT_ASSERT_EQUAL(s[0].Duration, TDuration::Seconds(60));
        UNIT_ASSERT_EQUAL(s[0].GetElementDuration(), TDuration::Seconds(60*5));
        UNIT_ASSERT_DOUBLES_EQUAL(s[0].BeginRps, 5, 1e-6);
        UNIT_ASSERT_DOUBLES_EQUAL(s[0].EndRps, 25, 1e-6);
        UNIT_ASSERT_DOUBLES_EQUAL(s[0].StepRps, 5, 1e-6);

        GOOD("line(1, 100, 10m)");
        UNIT_ASSERT_EQUAL(s.size(), 1);
        UNIT_ASSERT_EQUAL(s[0].Duration, TDuration::Minutes(10));
        UNIT_ASSERT_DOUBLES_EQUAL(s[0].BeginRps, 1, 1e-6);
        UNIT_ASSERT_DOUBLES_EQUAL(s[0].EndRps, 100, 1e-6);

        GOOD("line(1,10,10m) const(10,20m)");
        UNIT_ASSERT_EQUAL(s.size(), 2);
        UNIT_ASSERT_EQUAL(s[0].Mode, SM_LINE);
        UNIT_ASSERT_EQUAL(s[0].Duration, TDuration::Minutes(10));
        UNIT_ASSERT_DOUBLES_EQUAL(s[0].BeginRps, 1, 1e-6);
        UNIT_ASSERT_DOUBLES_EQUAL(s[0].EndRps, 10, 1e-6);
        UNIT_ASSERT_EQUAL(s[1].Mode, SM_CONST);
        UNIT_ASSERT_EQUAL(s[1].Duration, TDuration::Minutes(20));
        UNIT_ASSERT_DOUBLES_EQUAL(s[1].BeginRps, 10, 1e-6);
        UNIT_ASSERT_DOUBLES_EQUAL(s[1].EndRps, 10, 1e-6);

#if 0 // not implemented but mentioned in https://yandextank.readthedocs.org/en/latest/tutorial.html#first-steps
        GOOD("const(1,27h103m645)");
#endif
    }

    Y_UNIT_TEST(Ugly) {
        // Is empty schedule a valid schedule? It's implemented so, but that's questionable.
        GOOD("");
        UNIT_ASSERT_EQUAL(s.size(), 0);
    }

    Y_UNIT_TEST(Bad) {
        BAD("foo");
        BAD("at_unix()");
        BAD("const(,)");
        BAD("line(,,)");
        BAD("step(,,,)");
    }

    Y_UNIT_TEST(AtUnix) {
        GOOD("at_unix(1000212360)");
        UNIT_ASSERT_EQUAL(s.at(0).Mode, SM_AT_UNIX);
        UNIT_ASSERT_EQUAL(s.at(0).Duration, TDuration::Seconds(1000212360));

        GOOD("at_unix(1000212360.1234)");
        UNIT_ASSERT_EQUAL(s.at(0).Mode, SM_AT_UNIX);
        UNIT_ASSERT_EQUAL(s.at(0).Duration, TDuration::MicroSeconds(1000212360123400));

        GOOD("at_unix(1000212360.123456)");
        UNIT_ASSERT_EQUAL(s.at(0).Mode, SM_AT_UNIX);
        UNIT_ASSERT_EQUAL(s.at(0).Duration, TDuration::MicroSeconds(1000212360123456));

        // can use at_unix to schedule start
        GOOD("at_unix(0.5) const(0.7, 0.33h)");

        // can't use at_unix to schedule delay
        BAD("at_unix(0.5) const(0.7, 0.33h) at_unix(40d)");
        BAD("const(0.7, 0.33h) at_unix(40d)");
    }
}

Y_UNIT_TEST_SUITE(TestRpsScheduleStepper) {
    const TInstant Start = TInstant::Seconds(1000212000);
    const time_t Firetime = 1000212360;
    TRpsSchedule s;

    TRpsCounter CalcRps(TRpsScheduleIterator* it, TInstant start) {
        TRpsCounter rps;

        while (start != TInstant::Zero()) {
            rps[start.TimeT()] += 1;
            start = it->NextShot(start);
        }

        return rps;
    }

    Y_UNIT_TEST(Const) {
        GOOD("at_unix(1000212360) const(10, 1m)");

        TRpsScheduleIterator it(s);
        TInstant now = it.NextShot(Start);

        UNIT_ASSERT_EQUAL(now, TInstant::Seconds(Firetime));
        UNIT_ASSERT_EQUAL(it.GetFinish(), TInstant::Seconds(Firetime) + TDuration::Seconds(60));

        TRpsCounter rps = CalcRps(&it, now);

        Cdbg << s << " -> " << rps << "\n";

        UNIT_ASSERT_EQUAL(rps.size(), 61); // XXX: last shot is a bit late
        RPS(Firetime + 60, 1); // XXX: last shot...
        for (time_t t = Firetime; t < Firetime + 60; ++t) {
            RPS(t, 10);
        }
    }

    Y_UNIT_TEST(Line) {
        GOOD("at_unix(1000212360) line(0, 15, 1m)");

        TRpsScheduleIterator it(s);
        TInstant now = it.NextShot(Start);

        UNIT_ASSERT_EQUAL(now, TInstant::Seconds(Firetime));
        UNIT_ASSERT_EQUAL(it.GetFinish(), TInstant::Seconds(Firetime) + TDuration::Seconds(60));

        TRpsCounter rps = CalcRps(&it, now);

        Cdbg << s << " -> " << rps << "\n";

        TStringStream rps_s;
        rps_s << rps;
        UNIT_ASSERT_STRINGS_EQUAL(rps_s.Str(), "{"
                "1000212360: 1, 1000212362: 1, 1000212363: 1, 1000212364: 1, " // that's how 0.5 RPS looks like :)
                "1000212365: 1, 1000212366: 2, 1000212367: 2, 1000212368: 2, 1000212369: 2, "
                "1000212370: 3, 1000212371: 3, 1000212372: 3, 1000212373: 3, 1000212374: 4, "
                "1000212375: 4, 1000212376: 4, 1000212377: 4, 1000212378: 5, 1000212379: 5, "
                "1000212380: 5, 1000212381: 5, 1000212382: 6, 1000212383: 6, 1000212384: 6, "
                "1000212385: 6, 1000212386: 7, 1000212387: 7, 1000212388: 7, 1000212389: 7, "
                "1000212390: 8, 1000212391: 8, 1000212392: 8, 1000212393: 8, 1000212394: 9, "
                "1000212395: 9, 1000212396: 9, 1000212397: 9, 1000212398: 10, 1000212399: 10, "
                "1000212400: 10, 1000212401: 10, 1000212402: 11, 1000212403: 11, 1000212404: 11, "
                "1000212405: 11, 1000212406: 12, 1000212407: 12, 1000212408: 12, 1000212409: 12, "
                "1000212410: 13, 1000212411: 13, 1000212412: 13, 1000212413: 13, 1000212414: 14, "
                "1000212415: 14, 1000212416: 14, 1000212417: 14, 1000212418: 15, 1000212419: 15, "
                "1000212420: 1}"); // last shot is a bit late
    }

    Y_UNIT_TEST(LineSmoooth) {
        // For following reason it is quite steppy when tested against real nginx:
        // Shot Now()=2015-08-06T16:28:22.450832Z last= 2015-08-06T16:28:22.449489Z next=2015-08-06T16:28:22.999999Z
        // Shot Now()=2015-08-06T16:28:23.000945Z last= 2015-08-06T16:28:22.999999Z next=2015-08-06T16:28:23.464100Z
        // ^^ SleepD sleeps ~1ms more than needed.
        GOOD("at_unix(1000212360) line(0, 20, 30s)");

        TRpsScheduleIterator it(s);
        TInstant now = it.NextShot(Start);

        UNIT_ASSERT_EQUAL(now, TInstant::Seconds(Firetime));
        UNIT_ASSERT_EQUAL(it.GetFinish(), TInstant::Seconds(Firetime) + TDuration::Seconds(30));

        TRpsCounter rps = CalcRps(&it, now);

        Cdbg << s << " -> " << rps << "\n";

        TStringStream rps_s;
        rps_s << rps;
        UNIT_ASSERT_STRINGS_EQUAL(rps_s.Str(), "{"
                "1000212360: 1, 1000212361: 1, 1000212362: 2, 1000212363: 2, 1000212364: 3, "
                "1000212365: 4, 1000212366: 4, 1000212367: 5, 1000212368: 6, 1000212369: 6, "
                "1000212370: 7, 1000212371: 8, 1000212372: 8, 1000212373: 9, 1000212374: 10, "
                "1000212375: 10, 1000212376: 11, 1000212377: 12, 1000212378: 12, 1000212379: 13, "
                "1000212380: 14, 1000212381: 14, 1000212382: 15, 1000212383: 16, 1000212384: 16, "
                "1000212385: 17, 1000212386: 18, 1000212387: 18, 1000212388: 19, 1000212389: 20, "
                "1000212390: 1}");
    }

    Y_UNIT_TEST(LineReversed) {
        GOOD("at_unix(1000212360) line(15, 0, 1m)");

        TRpsScheduleIterator it(s);
        TInstant now = it.NextShot(Start);

        UNIT_ASSERT_EQUAL(now, TInstant::Seconds(Firetime));
        UNIT_ASSERT_EQUAL(it.GetFinish(), TInstant::Seconds(Firetime) + TDuration::Seconds(60));

        TRpsCounter rps = CalcRps(&it, now);

        Cdbg << s << " -> " << rps << "\n";

        TStringStream rps_s;
        rps_s << rps;

        // does not look very smooth, but seems to be good enough
        UNIT_ASSERT_STRINGS_EQUAL(rps_s.Str(), "{"
                "1000212360: 15, 1000212361: 15, 1000212362: 14, 1000212363: 15, 1000212364: 13, "
                "1000212365: 14, 1000212366: 13, 1000212367: 14, 1000212368: 12, 1000212369: 13, "
                "1000212370: 12, 1000212371: 13, 1000212372: 11, 1000212373: 12, 1000212374: 11, "
                "1000212375: 12, 1000212376: 10, 1000212377: 11, 1000212378: 10, 1000212379: 11, "
                "1000212380: 9, 1000212381: 10, 1000212382: 9, 1000212383: 10, 1000212384: 8, "
                "1000212385: 9, 1000212386: 8, 1000212387: 9, 1000212388: 7, 1000212389: 8, "
                "1000212390: 7, 1000212391: 8, 1000212392: 6, 1000212393: 7, 1000212394: 6, "
                "1000212395: 7, 1000212396: 5, 1000212397: 6, 1000212398: 5, 1000212399: 6, "
                "1000212400: 4, 1000212401: 5, 1000212402: 4, 1000212403: 5, 1000212404: 3, "
                "1000212405: 4, 1000212406: 3, 1000212407: 4, 1000212408: 2, 1000212409: 3, "
                "1000212410: 2, 1000212411: 3, 1000212412: 1, 1000212413: 2, 1000212414: 1, "
                "1000212415: 2, 1000212417: 1, 1000212419: 1, 1000212420: 1}"); // 0.5 RPS @ 1000212416
    }

    Y_UNIT_TEST(Step) {
        GOOD("at_unix(1000212360) step(0, 15, 5, 1m)");

        UNIT_ASSERT_EQUAL(s[1].CountLadderSteps(), 4);

        TRpsScheduleIterator it(s);
        TInstant now = it.NextShot(Start);
        UNIT_ASSERT_EQUAL(it.GetFinish(), TInstant::Seconds(Firetime) + TDuration::Seconds(60*4));

        UNIT_ASSERT_EQUAL(now, TInstant::Seconds(Firetime));

        TRpsCounter rps = CalcRps(&it, now);

        Cdbg << s << " -> " << rps << "\n";

        UNIT_ASSERT_EQUAL_C(rps.size(), 60*3 + 2, "rps.size() eq " << rps.size());
        RPS(Firetime, 1); // first shot is just a garbage
        for (time_t t = Firetime + 60; t < Firetime + 60*2; ++t) {
            RPS(t, 5);
        }
        for (time_t t = Firetime + 60*2; t < Firetime + 60*3; ++t) {
            RPS(t, 10);
        }
        RPS(Firetime + 60*3, 16); // WTF?
        for (time_t t = Firetime + 1 + 60*3; t < Firetime + 60*4; ++t) {
            RPS(t, 15);
        }
        RPS(Firetime + 60*4, 1); // last shot is a bit late
    }

    Y_UNIT_TEST(StepReversed) {
        GOOD("at_unix(1000212360) step(15, 0, 5, 1m)");

        UNIT_ASSERT_EQUAL(s[1].CountLadderSteps(), 4);


        TRpsScheduleIterator it(s);
        TInstant now = it.NextShot(Start);
        UNIT_ASSERT_EQUAL(it.GetFinish(), TInstant::Seconds(Firetime) + TDuration::Seconds(60*4));

        UNIT_ASSERT_EQUAL(now, TInstant::Seconds(Firetime));

        TRpsCounter rps = CalcRps(&it, now);

        Cdbg << s << " -> " << rps << "\n";

        UNIT_ASSERT_EQUAL_C(rps.size(), 60*3 + 2, "rps.size() eq " << rps.size());

        RPS(Firetime, 16); // first shot is just a garbage
        for (time_t t = Firetime + 1; t < Firetime + 60; ++t) {
            RPS(t, 15);
        }
        for (time_t t = Firetime + 60; t < Firetime + 60*2; ++t) {
            RPS(t, 10);
        }
        for (time_t t = Firetime + 60*2; t < Firetime + 60*3; ++t) {
            RPS(t, 5);
        }
        RPS(Firetime + 60*3, 1);
        RPS(Firetime + 60*4, 1); // last shot is a bit late
    }

    Y_UNIT_TEST(NextLeap) {
        GOOD("at_unix(1000212360) step(15, 0, 5, 1m)");

        TRpsScheduleIterator it(s);
        TInstant now = it.NextShot(Start, 0);
        UNIT_ASSERT_EQUAL(now, Start);
        UNIT_ASSERT_EQUAL(it.GetFinish(), TInstant::Seconds(Firetime) + TDuration::Seconds(60*4));

        TRpsScheduleIterator it2(it);
        UNIT_ASSERT_EQUAL(it.NextShot(it.NextShot(now)), it2.NextShot(now, 2));
    }
}
