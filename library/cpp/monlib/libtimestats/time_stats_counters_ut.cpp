#include "time_stats_counters.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TTimeStatsCountersTest) {
    using namespace NTimeStats;
    Y_UNIT_TEST(TestCreation_OK) {
        NMonitoring::TDynamicCounterPtr dynCounters(new NMonitoring::TDynamicCounters());
        TTimeStatsCounters counters(dynCounters, ETimeAccuracy::SECONDS);
        UNIT_ASSERT_NO_EXCEPTION(
            counters.AddBucket(TDuration::Seconds(1))
                .AddBucket(TDuration::Seconds(2))
                .AddBucket(TDuration::Seconds(3)));
    }
    Y_UNIT_TEST(TestCreation_WrongOrder) {
        NMonitoring::TDynamicCounterPtr dynCounters(new NMonitoring::TDynamicCounters());
        TTimeStatsCounters counters(dynCounters, ETimeAccuracy::SECONDS);
        UNIT_ASSERT_EXCEPTION(
            counters.AddBucket(TDuration::Seconds(2)).AddBucket(TDuration::Seconds(1)), yexception);
    }
    Y_UNIT_TEST(TestCreation_WrongOrderAccuracy) {
        NMonitoring::TDynamicCounterPtr dynCounters(new NMonitoring::TDynamicCounters());
        TTimeStatsCounters counters(dynCounters, ETimeAccuracy::MINUTES);
        UNIT_ASSERT_EXCEPTION(
            counters.AddBucket(TDuration::Seconds(1)).AddBucket(TDuration::Seconds(2)), yexception);
    }
    Y_UNIT_TEST(TestReport_OK) {
        NMonitoring::TDynamicCounterPtr dynCounters(new NMonitoring::TDynamicCounters());
        TTimeStatsCounters counters(dynCounters, ETimeAccuracy::SECONDS);
        UNIT_ASSERT_NO_EXCEPTION(
            counters.AddBucket(TDuration::Seconds(1))
                .AddBucket(TDuration::Seconds(2))
                .AddBucket(TDuration::Seconds(3)));

        counters.ReportEvent(TDuration::Seconds(0));
        counters.ReportEvent(TDuration::Seconds(1));
        counters.ReportEvent(TDuration::Seconds(2));
        counters.ReportEvent(TDuration::Seconds(3));
        counters.ReportEvent(TDuration::Seconds(4));

        UNIT_ASSERT_VALUES_EQUAL(dynCounters->GetCounter("1s", true)->Val(), 2);
        UNIT_ASSERT_VALUES_EQUAL(dynCounters->GetCounter("2s", true)->Val(), 1);
        UNIT_ASSERT_VALUES_EQUAL(dynCounters->GetCounter("3s", true)->Val(), 2);
    }
} // TTimeStatsCountersTest
