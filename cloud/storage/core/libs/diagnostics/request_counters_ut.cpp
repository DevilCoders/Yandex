#include "request_counters.h"

#include "monitoring.h"

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/timer.h>

#include <library/cpp/monlib/dynamic_counters/counters.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/datetime/cputimer.h>
#include <util/generic/size_literals.h>

namespace NCloud {

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
    TRequestCounters& requestCounters,
    TRequestCounters::TRequestType requestType,
    std::initializer_list<TRequest> requests)
{
    for (const auto& request: requests) {
        auto requestStarted = requestCounters.RequestStarted(
            requestType,
            request.RequestBytes);

        requestCounters.RequestCompleted(
            requestType,
            requestStarted - DurationToCyclesSafe(request.RequestTime),
            request.PostponedTime,
            request.RequestBytes,
            EErrorKind::Success);
    }
}

void AddIncompleteStats(
    TRequestCounters& requestCounters,
    TRequestCounters::TRequestType requestType,
    std::initializer_list<TDuration> requests)
{
    for (auto requestTime: requests) {
        requestCounters.AddIncompleteStats(requestType, requestTime);
    }
}

////////////////////////////////////////////////////////////////////////////////

const ui32 WriteRequestType = 0;
const ui32 ReadRequestType = 1;

const TString RequestNames[] {
    "WriteBlocks",
    "ReadBlocks",
};

auto RequestType2Name(TRequestCounters::TRequestType t) {
    UNIT_ASSERT(t == 0 || t == 1);
    return RequestNames[t];
}

auto IsReadWriteRequest(TRequestCounters::TRequestType t)
{
    UNIT_ASSERT(t == 0 || t == 1);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

auto MakeRequestCounters()
{
    return TRequestCounters(
        CreateWallClockTimer(),
        2,
        RequestType2Name,
        IsReadWriteRequest
    );
}

auto MakeRequestCountersPtr()
{
    return std::make_shared<TRequestCounters>(
        CreateWallClockTimer(),
        2,
        RequestType2Name,
        IsReadWriteRequest
    );
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TRequestCountersTest)
{
    Y_UNIT_TEST(ShouldTrackRequestsInProgress)
    {
        auto monitoring = CreateMonitoringServiceStub();

        auto requestCounters = MakeRequestCounters();
        requestCounters.Register(*monitoring->GetCounters());

        auto counters = monitoring
            ->GetCounters()
            ->GetSubgroup("request", "WriteBlocks");

        auto inProgress = counters->GetCounter("InProgress");
        auto inProgressBytes = counters->GetCounter("InProgressBytes");

        UNIT_ASSERT_EQUAL(inProgress->Val(), 0);
        UNIT_ASSERT_EQUAL(inProgressBytes->Val(), 0);

        auto started = requestCounters.RequestStarted(
            WriteRequestType,
            1_MB);

        UNIT_ASSERT_EQUAL(inProgress->Val(), 1);
        UNIT_ASSERT_EQUAL(inProgressBytes->Val(), 1_MB);

        requestCounters.RequestCompleted(
            WriteRequestType,
            started,
            TDuration::Zero(),
            1_MB,
            EErrorKind::Success);

        UNIT_ASSERT_EQUAL(inProgress->Val(), 0);
        UNIT_ASSERT_EQUAL(inProgressBytes->Val(), 0);
    }

    Y_UNIT_TEST(ShouldTrackIncompleteRequests)
    {
        auto monitoring = CreateMonitoringServiceStub();

        auto requestCounters = MakeRequestCounters();
        requestCounters.Register(*monitoring->GetCounters());

        auto counter = monitoring->GetCounters()
            ->GetSubgroup("request", "WriteBlocks")
            ->GetCounter("MaxTime");
        UNIT_ASSERT_EQUAL(counter->Val(), 0);

        AddIncompleteStats(requestCounters, WriteRequestType, {
            TDuration::MilliSeconds(100),
            TDuration::MilliSeconds(150),
            TDuration::MilliSeconds(50),
            TDuration::MilliSeconds(200),
        });

        requestCounters.UpdateStats();
        UNIT_ASSERT_EQUAL(counter->Val(), 200'000);

        AddIncompleteStats(requestCounters, WriteRequestType, {
            TDuration::MilliSeconds(30),
            TDuration::MilliSeconds(170),
            TDuration::MilliSeconds(150),
            TDuration::MilliSeconds(90),
        });

        requestCounters.UpdateStats();
        UNIT_ASSERT_EQUAL(counter->Val(), 200'000);

        AddIncompleteStats(requestCounters, WriteRequestType, {
            TDuration::MilliSeconds(130),
            TDuration::MilliSeconds(70),
            TDuration::MilliSeconds(250),
            TDuration::MilliSeconds(190),
        });

        requestCounters.UpdateStats();
        UNIT_ASSERT_EQUAL(counter->Val(), 250'000);
    }

    Y_UNIT_TEST(ShouldTrackPostponedRequests)
    {
        auto monitoring = CreateMonitoringServiceStub();

        auto requestCounters = MakeRequestCounters();
        requestCounters.Register(*monitoring->GetCounters());

        auto counters = monitoring
            ->GetCounters()
            ->GetSubgroup("request", "WriteBlocks");

        auto postponedQueueSize = counters->GetCounter("PostponedQueueSize");
        auto maxPostponedQueueSize = counters->GetCounter("MaxPostponedQueueSize");

        UNIT_ASSERT_EQUAL(postponedQueueSize->Val(), 0);
        UNIT_ASSERT_EQUAL(maxPostponedQueueSize->Val(), 0);

        requestCounters.RequestPostponed(WriteRequestType);
        UNIT_ASSERT_EQUAL(postponedQueueSize->Val(), 1);
        UNIT_ASSERT_EQUAL(maxPostponedQueueSize->Val(), 0);

        requestCounters.RequestPostponed(WriteRequestType);
        UNIT_ASSERT_EQUAL(postponedQueueSize->Val(), 2);
        UNIT_ASSERT_EQUAL(maxPostponedQueueSize->Val(), 0);

        requestCounters.RequestAdvanced(WriteRequestType);
        UNIT_ASSERT_EQUAL(postponedQueueSize->Val(), 1);
        UNIT_ASSERT_EQUAL(maxPostponedQueueSize->Val(), 0);

        requestCounters.UpdateStats();
        UNIT_ASSERT_EQUAL(maxPostponedQueueSize->Val(), 2);
    }

    Y_UNIT_TEST(ShouldTrackFastPathHits)
    {
        auto monitoring = CreateMonitoringServiceStub();

        auto requestCounters = MakeRequestCounters();
        requestCounters.Register(*monitoring->GetCounters());

        auto counters = monitoring
            ->GetCounters()
            ->GetSubgroup("request", "WriteBlocks");

        auto fastPathHits = counters->GetCounter("FastPathHits");

        UNIT_ASSERT_EQUAL(fastPathHits->Val(), 0);

        requestCounters.RequestFastPathHit(WriteRequestType);
        requestCounters.RequestFastPathHit(WriteRequestType);
        requestCounters.RequestFastPathHit(WriteRequestType);

        UNIT_ASSERT_EQUAL(fastPathHits->Val(), 3);
    }

    Y_UNIT_TEST(ShouldFillTimePercentiles)
    {
        auto monitoring = CreateMonitoringServiceStub();

        auto requestCounters = MakeRequestCounters();
        requestCounters.Register(*monitoring->GetCounters());

        auto writeBlocks = monitoring
            ->GetCounters()
            ->GetSubgroup("request", "WriteBlocks");

        auto readBlocks = monitoring
            ->GetCounters()
            ->GetSubgroup("request", "ReadBlocks");

        AddRequestStats(requestCounters, ReadRequestType, {
            { 1_MB, TDuration::MilliSeconds(101), TDuration::MilliSeconds(50) },
        });

        requestCounters.UpdateStats(true);

        {
            auto percentiles = writeBlocks->GetSubgroup("percentiles", "Time");

            auto p100 = percentiles->GetCounter("100");
            auto p50 = percentiles->GetCounter("50");

            UNIT_ASSERT_VALUES_EQUAL(0, p100->Val());
            UNIT_ASSERT_VALUES_EQUAL(0, p50->Val());
        }

        {
            auto percentiles = writeBlocks->GetSubgroup("percentiles", "ExecutionTime");

            auto p100 = percentiles->GetCounter("100");
            auto p50 = percentiles->GetCounter("50");

            UNIT_ASSERT_VALUES_EQUAL(0, p100->Val());
            UNIT_ASSERT_VALUES_EQUAL(0, p50->Val());
        }

        {
            auto percentiles = readBlocks->GetSubgroup("percentiles", "Time");
            auto p100 = percentiles->GetCounter("100");
            auto p50 = percentiles->GetCounter("50");

            UNIT_ASSERT_VALUES_EQUAL(200, p100->Val());
            UNIT_ASSERT_VALUES_EQUAL(150, p50->Val());
        }

        {
            auto percentiles = readBlocks->GetSubgroup("percentiles", "ExecutionTime");
            auto p100 = percentiles->GetCounter("100");
            auto p50 = percentiles->GetCounter("50");

            UNIT_ASSERT_VALUES_EQUAL(100, p100->Val());
            UNIT_ASSERT_VALUES_EQUAL(75, p50->Val());
        }
    }

    Y_UNIT_TEST(ShouldFillSizePercentiles)
    {
        auto monitoring = CreateMonitoringServiceStub();

        auto requestCounters = MakeRequestCounters();
        requestCounters.Register(*monitoring->GetCounters());

        auto counters = monitoring
            ->GetCounters()
            ->GetSubgroup("request", "WriteBlocks");

        AddRequestStats(requestCounters, WriteRequestType, {
            { 1_MB, TDuration::MilliSeconds(100), TDuration::Zero() },
            { 2_MB, TDuration::MilliSeconds(100), TDuration::Zero() },
            { 3_MB, TDuration::MilliSeconds(100), TDuration::Zero() },
        });

        requestCounters.UpdateStats(true);

        auto percentiles = counters->GetSubgroup("percentiles", "Size");
        auto p100 = percentiles->GetCounter("100");
        auto p50 = percentiles->GetCounter("50");

        UNIT_ASSERT_VALUES_EQUAL(4*1024, p100->Val());
        UNIT_ASSERT_VALUES_EQUAL(1.5*1024, p50->Val());
    }

    Y_UNIT_TEST(ShouldCountSilentErrors)
    {
        auto monitoring = CreateMonitoringServiceStub();

        auto requestCounters = MakeRequestCounters();
        requestCounters.Register(*monitoring->GetCounters());

        auto counters = monitoring
            ->GetCounters()
            ->GetSubgroup("request", "WriteBlocks");

        auto shoot = [&] (auto errorKind) {
            auto requestStarted = requestCounters.RequestStarted(
                WriteRequestType,
                1_MB);

            requestCounters.RequestCompleted(
                WriteRequestType,
                requestStarted - DurationToCyclesSafe(TDuration::MilliSeconds(201)),
                TDuration::MilliSeconds(100),
                1_MB,
                errorKind);
        };

        shoot(EErrorKind::ErrorFatal);
        shoot(EErrorKind::ErrorRetriable);
        shoot(EErrorKind::ErrorThrottling);
        shoot(EErrorKind::ErrorSession);
        shoot(EErrorKind::ErrorSilent);

        requestCounters.UpdateStats(true);

        auto errors = counters->GetCounter("Errors");
        UNIT_ASSERT_VALUES_EQUAL(4, errors->Val());

        auto fatal = counters->GetCounter("Errors/Fatal");
        UNIT_ASSERT_VALUES_EQUAL(1, fatal->Val());

        auto retriable = counters->GetCounter("Errors/Retriable");
        UNIT_ASSERT_VALUES_EQUAL(1, retriable->Val());

        auto throttling = counters->GetCounter("Errors/Throttling");
        UNIT_ASSERT_VALUES_EQUAL(1, throttling->Val());

        auto session = counters->GetCounter("Errors/Session");
        UNIT_ASSERT_VALUES_EQUAL(1, session->Val());

        auto silent = counters->GetCounter("Errors/Silent");
        UNIT_ASSERT_VALUES_EQUAL(1, silent->Val());
    }

    Y_UNIT_TEST(ShouldNotUpdateSubscribers)
    {
        auto monitoring = CreateMonitoringServiceStub();
        auto counters = MakeRequestCountersPtr();
        counters->Register(*monitoring->GetCounters());

        auto subscriber = MakeRequestCountersPtr();
        subscriber->Register(*monitoring->GetCounters()->GetSubgroup("subscribers", "s"));
        counters->Subscribe(subscriber);

        AddRequestStats(*counters, WriteRequestType, {
            { 1_MB, TDuration::MilliSeconds(100), TDuration::Zero() },
            { 2_MB, TDuration::MilliSeconds(100), TDuration::Zero() },
            { 3_MB, TDuration::MilliSeconds(100), TDuration::Zero() },
        });

        counters->UpdateStats();

        {
            auto maxTime = monitoring
                ->GetCounters()
                ->GetSubgroup("request", "WriteBlocks")
                ->GetCounter("MaxTime");

            UNIT_ASSERT(maxTime->Val() > 0);
        }

        {
            auto maxTime = monitoring
                ->GetCounters()
                ->GetSubgroup("subscribers", "s")
                ->GetSubgroup("request", "WriteBlocks")
                ->GetCounter("maxTime");

            UNIT_ASSERT_EQUAL(0, maxTime->Val());
        }
    }

    Y_UNIT_TEST(ShouldNotifySubscribers)
    {
        auto monitoring = CreateMonitoringServiceStub();
        auto counters = MakeRequestCountersPtr();
        counters->Register(*monitoring->GetCounters());

        auto outerSubscriber = MakeRequestCountersPtr();
        outerSubscriber->Register(*monitoring->GetCounters()->GetSubgroup("subscribers", "outer"));
        counters->Subscribe(outerSubscriber);

        auto innerSubscriber = MakeRequestCountersPtr();
        innerSubscriber->Register(*monitoring->GetCounters()->GetSubgroup("subscribers", "inner"));
        outerSubscriber->Subscribe(innerSubscriber);

        AddRequestStats(*counters, WriteRequestType, {
            { 1_MB, TDuration::MilliSeconds(100), TDuration::Zero() },
            { 2_MB, TDuration::MilliSeconds(100), TDuration::Zero() },
            { 3_MB, TDuration::MilliSeconds(100), TDuration::Zero() },
        });

        {
            counters->UpdateStats();
            auto maxTime = monitoring
                ->GetCounters()
                ->GetSubgroup("request", "WriteBlocks")
                ->GetCounter("MaxTime");

            UNIT_ASSERT(maxTime->Val() > 0);
        }

        {
            outerSubscriber->UpdateStats();
            auto maxTime = monitoring
                ->GetCounters()
                ->GetSubgroup("subscribers", "outer")
                ->GetSubgroup("request", "WriteBlocks")
                ->GetCounter("MaxTime");

            UNIT_ASSERT(maxTime->Val() > 0);
        }

        {
            innerSubscriber->UpdateStats();
            auto maxTime = monitoring
                ->GetCounters()
                ->GetSubgroup("subscribers", "inner")
                ->GetSubgroup("request", "WriteBlocks")
                ->GetCounter("MaxTime");

            UNIT_ASSERT(maxTime->Val() > 0);
        }
    }
}

}   // namespace NCloud
