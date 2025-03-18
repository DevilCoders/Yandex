#include <library/cpp/testing/unittest/registar.h>

#include "rps_calc.h"
#include "timedelta_checker.h"

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/string/printf.h>

namespace NAntiRobot {

Y_UNIT_TEST_SUITE(RpsCounters) {

const float DEFAULT_EPS = 1e-4;

float MSec(float seconds) {
    return 1e6 * seconds;
}


class TConstRpsGen {
public:
    explicit TConstRpsGen(float rps, TInstant startTime = TInstant::Zero())
        : NextTime(startTime)
        , Step(TDuration::MicroSeconds(static_cast<ui64>(MSec(1.0) / rps)))
    {
    }

    TInstant Next() {
        TInstant result = NextTime;
        NextTime += Step;
        return result;
    }

private:
    TInstant NextTime;
    TDuration Step;
};

void CheckNothing(float) {
}

class TRpsEqualsTo {
public:
    explicit TRpsEqualsTo(float value, float precision = DEFAULT_EPS)
        : Value(value)
        , Precision(precision)
    {}

    void operator() (float rps) {
        UNIT_ASSERT_DOUBLES_EQUAL(rps, Value, Precision);
    }

private:
    float Value;
    float Precision;
};

class TRpsConvergesTo {
public:
    explicit TRpsConvergesTo(float value, float precision = DEFAULT_EPS)
        : Value(value)
        , Precision(precision)
        , Previous(-1.0f)
    {}

    void operator() (float rps) {
        if (fabs(Previous + 1.0f) < Precision) {
            Previous = rps;
        } else {
            UNIT_ASSERT_C(fabs(Previous - Value) >= fabs(rps - Value) - Precision,
                          Sprintf("Prev = %.5f, Cur = %.5f, Value = %.5f", Previous, rps, Value));
            Previous = rps;
        }
    }

private:
    float Value;
    float Precision;
    float Previous;
};

struct TSender {
    float RPS;
    TDuration Duration;
    std::function<void(float)> EachTimeAsserter;
    std::function<void(float)> AfterAllAsserter;

    TSender(float rps, TDuration duration)
        : RPS(rps)
        , Duration(duration)
        , EachTimeAsserter(CheckNothing)
        , AfterAllAsserter(CheckNothing)
    {
    }

    TSender& AssertEachTime(const std::function<void(float)>& check) {
        EachTimeAsserter = check;
        return *this;
    }

    TSender& AssertAfter(const std::function<void(float)>& check) {
        AfterAllAsserter = check;
        return *this;
    }
};

struct TRpsSenderProxy {
    float RPS;

    explicit TRpsSenderProxy(float rps)
        : RPS(rps)
    {}

    TSender During(TDuration duration) {
        return TSender(RPS, duration);
    }

    TSender Count(size_t requestCount) {
        TDuration duration = TDuration::MicroSeconds(static_cast<ui64>(MSec(1.0) * requestCount / RPS));
        return TSender(RPS, duration);
    }
};

TRpsSenderProxy RPS(float rps) {
    UNIT_ASSERT(rps > DEFAULT_EPS);
    return TRpsSenderProxy(rps);
}

struct TReqsCountSenderProxy {
    size_t RequestCount;

    explicit TReqsCountSenderProxy(size_t requestCount)
        : RequestCount(requestCount)
    {
    }

    TSender During(TDuration duration) {
        float rps = RequestCount * MSec(1.0) / duration.MicroSeconds();
        return TSender(rps, duration);
    }
};

TReqsCountSenderProxy Requests(size_t requestCount) {
    UNIT_ASSERT(requestCount > 0);
    return TReqsCountSenderProxy(requestCount);
}

class TTester {
public:
    TTester(TDiscreteRpsCounter& rpsCounter)
        : RpsCounter(rpsCounter)
        , CurrentTime(TInstant::Zero())
    {
    }

    void Send(const TSender& sender) {
        TConstRpsGen gen(sender.RPS, CurrentTime);
        CurrentTime += sender.Duration;
        for (TInstant i = gen.Next(); i < CurrentTime; i = gen.Next()) {
            RpsCounter.ProcessRequest(i);
            sender.EachTimeAsserter(RpsCounter.GetRps());
        }
        sender.AfterAllAsserter(RpsCounter.GetRps());
    }

    void Wait(TDuration duration) {
        CurrentTime += duration;
    }

private:
    TDiscreteRpsCounter& RpsCounter;
    TInstant CurrentTime;
};

void TestConstantRps(TDiscreteRpsCounter& rpsCounter) {
    const float RPS_DATA[] = {0.01f, 0.25f, 1.0f, 5.0f, 100.0f};

    for (float rpsValue : RPS_DATA) {
        rpsCounter.Clear();
        TTester t(rpsCounter);

        t.Send(RPS(rpsValue).During(TDuration::Hours(1))
                            .AssertEachTime(TRpsConvergesTo(rpsValue))
                            .AssertAfter(TRpsEqualsTo(rpsValue)));
    }
}

Y_UNIT_TEST(ConstantRps) {
    TDiscreteRpsCounter rpsCounter(0.6f);
    TestConstantRps(rpsCounter);
}

void TestVaryingRps(TDiscreteRpsCounter& rpsCounter) {
    const std::pair<float, TDuration> testData[] = {
        {0.25f, TDuration::Minutes(1)},
        {12.0f, TDuration::MilliSeconds(3250)},
        { 2.0f, TDuration::MilliSeconds(5200)},
        {0.25f, TDuration::Minutes(1)},
        {120.0f, TDuration::MilliSeconds(1431)},
    };

    TTester tester(rpsCounter);

    for (const auto& cur: testData) {
        tester.Send(RPS(cur.first).During(cur.second)
                    .AssertEachTime(TRpsConvergesTo(cur.first))
                    /*.AssertAfter([](float rps){ Cerr << rps << Endl; })*/);
    }
}

Y_UNIT_TEST(VaryingRps) {
    TDiscreteRpsCounter rpsCounter(0.6f);
    TestVaryingRps(rpsCounter);
}

void TestClear(TDiscreteRpsCounter& rpsCounter) {
    TTester tester(rpsCounter);

    tester.Send(RPS(100.0f).During(TDuration::Minutes(5)).AssertAfter(TRpsEqualsTo(100.0f)));

    rpsCounter.Clear();
    UNIT_ASSERT_DOUBLES_EQUAL(rpsCounter.GetRps(), 0.0f, DEFAULT_EPS);
}

Y_UNIT_TEST(Clear) {
    TDiscreteRpsCounter rpsCounter(0.6f);
    TestClear(rpsCounter);
}

void TestNoMatterWhenToStart(TDiscreteRpsCounter& rpsCounter) {
    const TDuration INITIAL_WAIT[] = {
        TDuration::Zero(),
        TDuration::MilliSeconds(200),
        TDuration::Minutes(5),
        TDuration::Hours(1),
        TDuration::Hours(1) + TDuration::MilliSeconds(323),
    };

    TVector<float> resultRps;
    for (TDuration duration : INITIAL_WAIT) {
        rpsCounter.Clear();

        TTester tester(rpsCounter);
        tester.Wait(duration);
        tester.Send(RPS(8.0f).Count(100));

        resultRps.push_back(rpsCounter.GetRps());
    }

    for (size_t i = 0; i + 1 < resultRps.size(); ++i) {
        UNIT_ASSERT_DOUBLES_EQUAL(resultRps[i], resultRps[i + 1], DEFAULT_EPS);
    }
}

Y_UNIT_TEST(NoMatterWhenToStart) {
    TDiscreteRpsCounter rpsCounter(0.6f);
    TestNoMatterWhenToStart(rpsCounter);
}

void TestFewFrequentRequestsDuringSecond(TDiscreteRpsCounter& rpsCounter) {
    /*
     * Send requests non-uniformly during a second
     * ||||....||||....||||....||||....||||....||||....||||....||||....||||....||||....
     * 0s                              1s                              2s
     */
    TTester tester(rpsCounter);

    const size_t longTermRps = 20;
    // we split second in a number of equal slots
    const size_t activitySlots = 4;
    const size_t totalSlots = 2 * activitySlots;
    const size_t requestPerSlot = longTermRps / activitySlots;
    const TDuration slot = TDuration::Seconds(1) / totalSlots;
    const size_t totalDurationInSeconds = 5;

    for (size_t curSecond = 0; curSecond < totalDurationInSeconds; ++curSecond) {
        for (size_t j = 0; j < activitySlots; ++j) {
            tester.Send(Requests(requestPerSlot)
                        .During(slot)
                        .AssertEachTime(TRpsConvergesTo(longTermRps)));
            tester.Wait(slot);
        }
    }
}

Y_UNIT_TEST(FewFrequentRequestsDuringSecond) {
    TDiscreteRpsCounter rpsCounter(0.6f);
    TestFewFrequentRequestsDuringSecond(rpsCounter);
}

}

}
