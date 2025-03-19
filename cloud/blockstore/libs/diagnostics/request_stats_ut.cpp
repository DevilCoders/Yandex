#include "request_stats.h"

#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/common/timer_test.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>

#include <library/cpp/monlib/dynamic_counters/counters.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/datetime/cputimer.h>
#include <util/generic/size_literals.h>
#include <util/system/sanitizers.h>

namespace NCloud::NBlockStore {

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TRequest
{
    size_t RequestBytes;
    TDuration RequestTime;
    TDuration PostponedTime;
};

////////////////////////////////////////////////////////////////////////////////

void AddRequestStats(
    IRequestStats& requestStats,
    NCloud::NProto::EStorageMediaKind mediaKind,
    EBlockStoreRequest requestType,
    std::initializer_list<TRequest> requests)
{
    for (const auto& request: requests) {
        auto requestStarted = requestStats.RequestStarted(
            mediaKind,
            requestType,
            request.RequestBytes);

        requestStats.RequestCompleted(
            mediaKind,
            requestType,
            requestStarted - DurationToCyclesSafe(request.RequestTime),
            request.PostponedTime,
            request.RequestBytes,
            EErrorKind::Success);
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TRequestStatsTest)
{
    Y_UNIT_TEST(ShouldTrackRequestsPerMediaKind)
    {
        auto monitoring = CreateMonitoringServiceStub();
        auto requestStats = CreateServerRequestStats(
            monitoring->GetCounters(),
            CreateWallClockTimer());

        auto totalCounters = monitoring
            ->GetCounters()
            ->GetSubgroup("request", "WriteBlocks");
        auto totalCount = totalCounters->GetCounter("Count");

        auto ssdCounters = monitoring
            ->GetCounters()
            ->GetSubgroup("type", "ssd")
            ->GetSubgroup("request", "WriteBlocks");
        auto ssdCount = ssdCounters->GetCounter("Count");

        auto hddCounters = monitoring
            ->GetCounters()
            ->GetSubgroup("type", "hdd")
            ->GetSubgroup("request", "WriteBlocks");
        auto hddCount = hddCounters->GetCounter("Count");

        auto ssdNonreplCounters = monitoring
            ->GetCounters()
            ->GetSubgroup("type", "ssd_nonrepl")
            ->GetSubgroup("request", "WriteBlocks");
        auto ssdNonreplCount = ssdNonreplCounters->GetCounter("Count");

        auto ssdMirror2Counters = monitoring
            ->GetCounters()
            ->GetSubgroup("type", "ssd_mirror2")
            ->GetSubgroup("request", "WriteBlocks");
        auto ssdMirror2Count = ssdMirror2Counters->GetCounter("Count");

        auto ssdMirror3Counters = monitoring
            ->GetCounters()
            ->GetSubgroup("type", "ssd_mirror3")
            ->GetSubgroup("request", "WriteBlocks");
        auto ssdMirror3Count = ssdMirror3Counters->GetCounter("Count");

        UNIT_ASSERT_EQUAL(totalCount->Val(), 0);
        UNIT_ASSERT_EQUAL(ssdCount->Val(), 0);
        UNIT_ASSERT_EQUAL(hddCount->Val(), 0);
        UNIT_ASSERT_EQUAL(ssdNonreplCount->Val(), 0);
        UNIT_ASSERT_EQUAL(ssdMirror2Count->Val(), 0);
        UNIT_ASSERT_EQUAL(ssdMirror3Count->Val(), 0);

        {
            AddRequestStats(
                *requestStats,
                NCloud::NProto::STORAGE_MEDIA_SSD,
                EBlockStoreRequest::WriteBlocks,
            {
                { 1_MB, TDuration::MilliSeconds(100), TDuration::Zero() }
            });

            UNIT_ASSERT_EQUAL(totalCount->Val(), 1);
            UNIT_ASSERT_EQUAL(ssdCount->Val(), 1);
            UNIT_ASSERT_EQUAL(hddCount->Val(), 0);
            UNIT_ASSERT_EQUAL(ssdNonreplCount->Val(), 0);
            UNIT_ASSERT_EQUAL(ssdMirror2Count->Val(), 0);
            UNIT_ASSERT_EQUAL(ssdMirror3Count->Val(), 0);
        }

        {
            AddRequestStats(
                *requestStats,
                NCloud::NProto::STORAGE_MEDIA_HDD,
                EBlockStoreRequest::WriteBlocks,
            {
                { 1_MB, TDuration::MilliSeconds(100), TDuration::Zero() }
            });

            UNIT_ASSERT_EQUAL(totalCount->Val(), 2);
            UNIT_ASSERT_EQUAL(ssdCount->Val(), 1);
            UNIT_ASSERT_EQUAL(hddCount->Val(), 1);
            UNIT_ASSERT_EQUAL(ssdNonreplCount->Val(), 0);
            UNIT_ASSERT_EQUAL(ssdMirror2Count->Val(), 0);
            UNIT_ASSERT_EQUAL(ssdMirror3Count->Val(), 0);
        }

        {
            AddRequestStats(
                *requestStats,
                NCloud::NProto::STORAGE_MEDIA_SSD_NONREPLICATED,
                EBlockStoreRequest::WriteBlocks,
            {
                { 1_MB, TDuration::MilliSeconds(100), TDuration::Zero() }
            });

            UNIT_ASSERT_EQUAL(totalCount->Val(), 3);
            UNIT_ASSERT_EQUAL(ssdCount->Val(), 1);
            UNIT_ASSERT_EQUAL(hddCount->Val(), 1);
            UNIT_ASSERT_EQUAL(ssdNonreplCount->Val(), 1);
            UNIT_ASSERT_EQUAL(ssdMirror2Count->Val(), 0);
            UNIT_ASSERT_EQUAL(ssdMirror3Count->Val(), 0);
        }

        {
            AddRequestStats(
                *requestStats,
                NCloud::NProto::STORAGE_MEDIA_SSD_MIRROR2,
                EBlockStoreRequest::WriteBlocks,
            {
                { 1_MB, TDuration::MilliSeconds(100), TDuration::Zero() }
            });

            UNIT_ASSERT_EQUAL(totalCount->Val(), 4);
            UNIT_ASSERT_EQUAL(ssdCount->Val(), 1);
            UNIT_ASSERT_EQUAL(hddCount->Val(), 1);
            UNIT_ASSERT_EQUAL(ssdNonreplCount->Val(), 1);
            UNIT_ASSERT_EQUAL(ssdMirror2Count->Val(), 1);
            UNIT_ASSERT_EQUAL(ssdMirror3Count->Val(), 0);
        }

        {
            AddRequestStats(
                *requestStats,
                NCloud::NProto::STORAGE_MEDIA_SSD_MIRROR3,
                EBlockStoreRequest::WriteBlocks,
            {
                { 1_MB, TDuration::MilliSeconds(100), TDuration::Zero() }
            });

            UNIT_ASSERT_EQUAL(totalCount->Val(), 5);
            UNIT_ASSERT_EQUAL(ssdCount->Val(), 1);
            UNIT_ASSERT_EQUAL(hddCount->Val(), 1);
            UNIT_ASSERT_EQUAL(ssdNonreplCount->Val(), 1);
            UNIT_ASSERT_EQUAL(ssdMirror2Count->Val(), 1);
            UNIT_ASSERT_EQUAL(ssdMirror3Count->Val(), 1);
        }
    }

    Y_UNIT_TEST(ShouldFillTimePercentiles)
    {
        // Hdr histogram is no-op under Tsan, so just finish this test
        if (NSan::TSanIsOn()) {
            return;
        }

        auto monitoring = CreateMonitoringServiceStub();
        auto requestStats = CreateServerRequestStats(
            monitoring->GetCounters(),
            CreateWallClockTimer());

        auto totalCounters = monitoring
            ->GetCounters()
            ->GetSubgroup("request", "WriteBlocks");

        auto ssdCounters = monitoring
            ->GetCounters()
            ->GetSubgroup("type", "ssd")
            ->GetSubgroup("request", "WriteBlocks");

        auto hddCounters = monitoring
            ->GetCounters()
            ->GetSubgroup("type", "hdd")
            ->GetSubgroup("request", "WriteBlocks");

        auto ssdNonreplCounters = monitoring
            ->GetCounters()
            ->GetSubgroup("type", "ssd_nonrepl")
            ->GetSubgroup("request", "WriteBlocks");

        AddRequestStats(
            *requestStats,
            NCloud::NProto::STORAGE_MEDIA_SSD,
            EBlockStoreRequest::WriteBlocks,
        {
            { 1_MB, TDuration::MilliSeconds(100), TDuration::Zero() },
            { 1_MB, TDuration::MilliSeconds(200), TDuration::Zero() },
            { 1_MB, TDuration::MilliSeconds(300), TDuration::Zero() },
        });

        AddRequestStats(
            *requestStats,
            NCloud::NProto::STORAGE_MEDIA_HDD,
            EBlockStoreRequest::WriteBlocks,
        {
            { 1_MB, TDuration::MilliSeconds(400), TDuration::Zero() },
            { 1_MB, TDuration::MilliSeconds(500), TDuration::Zero() },
            { 1_MB, TDuration::MilliSeconds(600), TDuration::Zero() },
        });

        AddRequestStats(
            *requestStats,
            NCloud::NProto::STORAGE_MEDIA_SSD_NONREPLICATED,
            EBlockStoreRequest::WriteBlocks,
        {
            { 1_MB, TDuration::MilliSeconds(10), TDuration::Zero() },
            { 1_MB, TDuration::MilliSeconds(20), TDuration::Zero() },
            { 1_MB, TDuration::MilliSeconds(30), TDuration::Zero() },
        });

        requestStats->UpdateStats(true);

        {
            auto percentiles = totalCounters->GetSubgroup("percentiles", "Time");
            auto p100 = percentiles->GetCounter("100");
            auto p50 = percentiles->GetCounter("50");

            UNIT_ASSERT_VALUES_EQUAL(600063, p100->Val());
            UNIT_ASSERT_VALUES_EQUAL(200063, p50->Val());
        }

        {
            auto percentiles = ssdCounters->GetSubgroup("percentiles", "Time");
            auto p100 = percentiles->GetCounter("100");
            auto p50 = percentiles->GetCounter("50");

            UNIT_ASSERT_VALUES_EQUAL(300031, p100->Val());
            UNIT_ASSERT_VALUES_EQUAL(200063, p50->Val());
        }

        {
            auto percentiles = hddCounters->GetSubgroup("percentiles", "Time");
            auto p100 = percentiles->GetCounter("100");
            auto p50 = percentiles->GetCounter("50");

            UNIT_ASSERT_VALUES_EQUAL(600063, p100->Val());
            UNIT_ASSERT_VALUES_EQUAL(500223, p50->Val());
        }

        {
            auto percentiles =
                ssdNonreplCounters->GetSubgroup("percentiles", "Time");
            auto p100 = percentiles->GetCounter("100");
            auto p50 = percentiles->GetCounter("50");

            UNIT_ASSERT_VALUES_EQUAL(30015, p100->Val());
            UNIT_ASSERT_VALUES_EQUAL(20015, p50->Val());
        }
    }

    Y_UNIT_TEST(ShouldFillSizePercentiles)
    {
        // Hdr histogram is no-op under Tsan, so just finish this test
       if (NSan::TSanIsOn()) {
            return;
       }

        auto monitoring = CreateMonitoringServiceStub();
        auto requestStats = CreateServerRequestStats(
            monitoring->GetCounters(),
            CreateWallClockTimer());

        auto totalCounters = monitoring
            ->GetCounters()
            ->GetSubgroup("request", "WriteBlocks");

        auto ssdCounters = monitoring
            ->GetCounters()
            ->GetSubgroup("type", "ssd")
            ->GetSubgroup("request", "WriteBlocks");

        auto hddCounters = monitoring
            ->GetCounters()
            ->GetSubgroup("type", "hdd")
            ->GetSubgroup("request", "WriteBlocks");

        auto ssdNonreplCounters = monitoring
            ->GetCounters()
            ->GetSubgroup("type", "ssd_nonrepl")
            ->GetSubgroup("request", "WriteBlocks");

        AddRequestStats(
            *requestStats,
            NCloud::NProto::STORAGE_MEDIA_SSD,
            EBlockStoreRequest::WriteBlocks,
        {
            { 1_MB, TDuration::MilliSeconds(100), TDuration::Zero() },
            { 2_MB, TDuration::MilliSeconds(100), TDuration::Zero() },
            { 3_MB, TDuration::MilliSeconds(100), TDuration::Zero() },
        });

        AddRequestStats(
            *requestStats,
            NCloud::NProto::STORAGE_MEDIA_HDD,
            EBlockStoreRequest::WriteBlocks,
        {
            { 4_MB, TDuration::MilliSeconds(100), TDuration::Zero() },
            { 5_MB, TDuration::MilliSeconds(100), TDuration::Zero() },
            { 6_MB, TDuration::MilliSeconds(100), TDuration::Zero() },
        });

        AddRequestStats(
            *requestStats,
            NCloud::NProto::STORAGE_MEDIA_SSD_NONREPLICATED,
            EBlockStoreRequest::WriteBlocks,
        {
            { 7_MB, TDuration::MilliSeconds(100), TDuration::Zero() },
            { 8_MB, TDuration::MilliSeconds(100), TDuration::Zero() },
            { 9_MB, TDuration::MilliSeconds(100), TDuration::Zero() },
        });

        requestStats->UpdateStats(true);

        {
            auto percentiles = totalCounters->GetSubgroup("percentiles", "Size");
            auto p100 = percentiles->GetCounter("100");
            auto p50 = percentiles->GetCounter("50");

            UNIT_ASSERT_VALUES_EQUAL(9445375, p100->Val());
            UNIT_ASSERT_VALUES_EQUAL(5246975, p50->Val());
        }

        {
            auto percentiles = ssdCounters->GetSubgroup("percentiles", "Size");
            auto p100 = percentiles->GetCounter("100");
            auto p50 = percentiles->GetCounter("50");

            UNIT_ASSERT_VALUES_EQUAL(3147775, p100->Val());
            UNIT_ASSERT_VALUES_EQUAL(2099199, p50->Val());
        }

        {
            auto percentiles = hddCounters->GetSubgroup("percentiles", "Size");
            auto p100 = percentiles->GetCounter("100");
            auto p50 = percentiles->GetCounter("50");

            UNIT_ASSERT_VALUES_EQUAL(6295551, p100->Val());
            UNIT_ASSERT_VALUES_EQUAL(5246975, p50->Val());
        }

        {
            auto percentiles =
                ssdNonreplCounters->GetSubgroup("percentiles", "Size");
            auto p100 = percentiles->GetCounter("100");
            auto p50 = percentiles->GetCounter("50");

            UNIT_ASSERT_VALUES_EQUAL(9445375, p100->Val());
            UNIT_ASSERT_VALUES_EQUAL(8396799, p50->Val());
        }
    }

    Y_UNIT_TEST(ShouldTrackSilentErrors)
    {
        auto monitoring = CreateMonitoringServiceStub();
        auto requestStats = CreateServerRequestStats(
            monitoring->GetCounters(),
            CreateWallClockTimer());

        auto totalCounters = monitoring
            ->GetCounters()
            ->GetSubgroup("request", "WriteBlocks");

        auto ssdCounters = monitoring
            ->GetCounters()
            ->GetSubgroup("type", "ssd")
            ->GetSubgroup("request", "WriteBlocks");

        auto hddCounters = monitoring
            ->GetCounters()
            ->GetSubgroup("type", "hdd")
            ->GetSubgroup("request", "WriteBlocks");

        auto ssdNonreplCounters = monitoring
            ->GetCounters()
            ->GetSubgroup("type", "ssd_nonrepl")
            ->GetSubgroup("request", "WriteBlocks");

        auto shoot = [&] (auto mediaKind) {
            auto requestStarted = requestStats->RequestStarted(
                mediaKind,
                EBlockStoreRequest::WriteBlocks,
                1_MB);

            requestStats->RequestCompleted(
                mediaKind,
                EBlockStoreRequest::WriteBlocks,
                requestStarted - DurationToCyclesSafe(TDuration::MilliSeconds(100)),
                TDuration::Zero(),
                1_MB,
                EErrorKind::ErrorSilent);
        };

        shoot(NCloud::NProto::STORAGE_MEDIA_SSD);
        shoot(NCloud::NProto::STORAGE_MEDIA_HDD);
        shoot(NCloud::NProto::STORAGE_MEDIA_SSD_NONREPLICATED);

        auto totalErrors = totalCounters->GetCounter("Errors/Silent");
        auto ssdErrors = ssdCounters->GetCounter("Errors/Silent");
        auto hddErrors = hddCounters->GetCounter("Errors/Silent");
        auto ssdNonreplErrors = ssdNonreplCounters->GetCounter("Errors/Silent");

        UNIT_ASSERT_VALUES_EQUAL(3, totalErrors->Val());
        UNIT_ASSERT_VALUES_EQUAL(1, ssdErrors->Val());
        UNIT_ASSERT_VALUES_EQUAL(1, hddErrors->Val());
        UNIT_ASSERT_VALUES_EQUAL(1, ssdNonreplErrors->Val());
    }
}

}   // namespace NCloud::NBlockStore
