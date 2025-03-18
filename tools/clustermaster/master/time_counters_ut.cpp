#include "time_counters.h"

#include <library/cpp/testing/unittest/registar.h>

enum EAbc {
    ABC_A,
    ABC_B,
    ABC_C,
};

Y_UNIT_TEST_SUITE(TCountersTest) {
    Y_UNIT_TEST(Test1) {
        TTimeCounters<EAbc>::TNamesVector names;
        names.push_back(TCounterName<EAbc>(ABC_A, "A"));
        names.push_back(TCounterName<EAbc>(ABC_B, "B"));
        names.push_back(TCounterName<EAbc>(ABC_C, "C"));

        TTimeCounters<EAbc> counters(names);

        int till = 1800;
        for (int second = 0; second < 1800; second++) {
            TInstant time = TInstant::Seconds(second);
            counters.Maintain(time);
            counters.Add(ABC_A, 1); // 1 per second
            counters.Add(ABC_B, 2); // 2 per second
            counters.Add(ABC_C, 4); // 4 per second
        }

        TInstant end = TInstant::Seconds(till - 1);

        int a5 = counters.Get5Seconds(ABC_A, end);
        int b5 = counters.Get5Seconds(ABC_B, end);
        int c5 = counters.Get5Seconds(ABC_C, end);
        UNIT_ASSERT_VALUES_EQUAL(5, a5);
        UNIT_ASSERT_VALUES_EQUAL(10, b5); // x2 (2 per second)
        UNIT_ASSERT_VALUES_EQUAL(20, c5); // x4

        int a60 = counters.GetOneMinute(ABC_A, end);
        int a180 = counters.Get3Minutes(ABC_A, end);
        int c60 = counters.GetOneMinute(ABC_C, end);
        int c180 = counters.Get3Minutes(ABC_C, end);
        UNIT_ASSERT_VALUES_EQUAL(60, a60);
        UNIT_ASSERT_VALUES_EQUAL(180, a180);
        UNIT_ASSERT_VALUES_EQUAL(240, c60); // x4
        UNIT_ASSERT_VALUES_EQUAL(720, c180); // x4

        int a1800 = counters.GetHalfAnHour(ABC_A, end);
        int c1800 = counters.GetHalfAnHour(ABC_C, end);
        UNIT_ASSERT_VALUES_EQUAL(1200, a1800); // one cell is zero
        UNIT_ASSERT_VALUES_EQUAL(4800, c1800);
    }
}
