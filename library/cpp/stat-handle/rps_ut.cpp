#include <library/cpp/stat-handle/rps.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/datetime/base.h>

using NStat::TRpsCounter;

Y_UNIT_TEST_SUITE(TStatRpsTest){
    static void RunRequests(TRpsCounter & rps, size_t n, size_t totalSeconds){
        TDuration pause = TDuration::Seconds(totalSeconds) / n;
for (size_t i = 0; i < n; ++i) {
    rps.RegisterRequest();
    Sleep(pause);
}
}

bool CheckRps(const TRpsCounter& rps, float value, float seconds) {
    float calcRps = rps.RecentRps(TDuration::Seconds(seconds));
    // Cerr << "Measured RPS: " << calcRps << Endl;
    // Cerr << "Real RPS: " << value << Endl;
    return value < 0.0001 ? fabs(calcRps - value) < 0.01 : fabs(calcRps - value) / value < 0.01;
}

Y_UNIT_TEST(TestRps) {
    TRpsCounter rps(TDuration::Seconds(10), TDuration::Seconds(1));

    UNIT_ASSERT(CheckRps(rps, 0, 0));
    UNIT_ASSERT(CheckRps(rps, 0, 0.5));
    UNIT_ASSERT(CheckRps(rps, 0, 1));
    UNIT_ASSERT(CheckRps(rps, 0, 50));

    RunRequests(rps, 10, 2);

    UNIT_ASSERT(CheckRps(rps, 0, 0));
    UNIT_ASSERT(CheckRps(rps, 5, 0.5));
    UNIT_ASSERT(CheckRps(rps, 5, 1));
    UNIT_ASSERT(CheckRps(rps, 5, 1.5));
    UNIT_ASSERT(CheckRps(rps, 5, 2));
    UNIT_ASSERT(CheckRps(rps, 5, 3));
    UNIT_ASSERT(CheckRps(rps, 5, 20));

    RunRequests(rps, 2, 1);
    RunRequests(rps, 10, 2);

    UNIT_ASSERT(CheckRps(rps, 0, 0));
    UNIT_ASSERT(CheckRps(rps, 5, 1));
    UNIT_ASSERT(CheckRps(rps, 5, 2));
    UNIT_ASSERT(CheckRps(rps, 4.4, 20));

    Sleep(TDuration::Seconds(1));
    UNIT_ASSERT(CheckRps(rps, 0, 1));
    UNIT_ASSERT(CheckRps(rps, 2.5, 2));

    Sleep(TDuration::Seconds(1));
    UNIT_ASSERT(CheckRps(rps, 0, 1));
    UNIT_ASSERT(CheckRps(rps, 0, 2));
    UNIT_ASSERT(CheckRps(rps, 5.0 / 3, 3));

    RunRequests(rps, 60, 2);
    UNIT_ASSERT(CheckRps(rps, 0, 0));
    UNIT_ASSERT(CheckRps(rps, 30, 1));
    UNIT_ASSERT(CheckRps(rps, 30, 2));
    UNIT_ASSERT(CheckRps(rps, 20, 3));
    UNIT_ASSERT(CheckRps(rps, 15, 4));

    Sleep(TDuration::Seconds(0.5));
    UNIT_ASSERT(CheckRps(rps, 0, 0));
    UNIT_ASSERT(CheckRps(rps, 30, 1));
    UNIT_ASSERT(CheckRps(rps, 30, 2));
    UNIT_ASSERT(CheckRps(rps, 20, 3));
    UNIT_ASSERT(CheckRps(rps, 15, 4));

    Sleep(TDuration::Seconds(0.5));
    UNIT_ASSERT(CheckRps(rps, 0, 0));
    UNIT_ASSERT(CheckRps(rps, 0, 1));
    UNIT_ASSERT(CheckRps(rps, 15, 2));
    UNIT_ASSERT(CheckRps(rps, 20, 3));

    // should not have any effect
    rps.RegisterRequest(Now() - TDuration::Minutes(1));
    UNIT_ASSERT(CheckRps(rps, 0, 0));
    UNIT_ASSERT(CheckRps(rps, 0, 1));
    UNIT_ASSERT(CheckRps(rps, 15, 2));
    UNIT_ASSERT(CheckRps(rps, 20, 3));

    // shifts available window to future,
    // current rps becomes 0
    rps.RegisterRequest(Now() + TDuration::Minutes(1));
    UNIT_ASSERT(CheckRps(rps, 0, 0));
    UNIT_ASSERT(CheckRps(rps, 0, 1));
    UNIT_ASSERT(CheckRps(rps, 0, 2));
    UNIT_ASSERT(CheckRps(rps, 0, 3));

    RunRequests(rps, 10, 1);
    UNIT_ASSERT(CheckRps(rps, 0, 0)); // same
    UNIT_ASSERT(CheckRps(rps, 0, 1));
    UNIT_ASSERT(CheckRps(rps, 0, 2));
    UNIT_ASSERT(CheckRps(rps, 0, 3));
}
}
;
