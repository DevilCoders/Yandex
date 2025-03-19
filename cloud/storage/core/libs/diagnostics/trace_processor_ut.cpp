#include "trace_processor.h"

#include <cloud/storage/core/protos/media.pb.h>
#include <cloud/storage/core/libs/common/public.h>
#include <cloud/storage/core/libs/common/scheduler_test.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/logger/backend.h>
#include <library/cpp/lwtrace/all.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/split.h>
#include <util/system/event.h>
#include <util/system/spinlock.h>
#include <util/thread/pool.h>

namespace NCloud {

#define LWTRACE_UT_PROVIDER(PROBE, EVENT, GROUPS, TYPES, NAMES)                \
    PROBE(                                                                     \
        InitialProbe,                                                          \
        GROUPS("Group"),                                                       \
        TYPES(TString, ui32, ui64),                                            \
        NAMES("requestType", NProbeParam::MediaKind, "requestId")              \
    )                                                                          \
    PROBE(                                                                     \
        SomeProbe,                                                             \
        GROUPS("Group"),                                                       \
        TYPES(TString, ui64),                                                  \
        NAMES("requestType", "requestId")                                      \
    )                                                                          \
    PROBE(                                                                     \
        SomeOtherProbe,                                                        \
        GROUPS("Group"),                                                       \
        TYPES(TString, ui64),                                                  \
        NAMES("requestType", "requestId")                                      \
    )                                                                          \
    PROBE(                                                                     \
        SomeProbeWithExecutionTime,                                            \
        GROUPS("Group"),                                                       \
        TYPES(TString, ui64, ui64),                                            \
        NAMES("requestType", "requestId", NProbeParam::RequestExecutionTime)   \
    )                                                                          \
// LWTRACE_UT_PROVIDER

LWTRACE_DECLARE_PROVIDER(LWTRACE_UT_PROVIDER)
LWTRACE_DEFINE_PROVIDER(LWTRACE_UT_PROVIDER)
LWTRACE_USING(LWTRACE_UT_PROVIDER)

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TLogBackendImpl
    : TLogBackend
{
    TVector<std::pair<ELogPriority, TString>> Recs;
    TAdaptiveLock Lock;

    void WriteData(const TLogRecord& rec) override
    {
        with_lock (Lock) {
            Recs.emplace_back(rec.Priority, TString(rec.Data, rec.Len));
        }
    }

    void ReopenLog() override
    {
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TEnv
{
    std::unique_ptr<NLWTrace::TManager> LWManager;
    ITimerPtr Timer;
    std::shared_ptr<TTestScheduler> Scheduler;
    std::shared_ptr<TLogBackendImpl> LogBackend;

    TEnv()
        : Timer(CreateWallClockTimer())
        , Scheduler(new TTestScheduler())
        , LogBackend(new TLogBackendImpl())
    {
        RebuildLWManager();
    }

    void RebuildLWManager()
    {
        LWManager = std::make_unique<NLWTrace::TManager>(
            *Singleton<NLWTrace::TProbeRegistry>(), false);
        NLWTrace::TQuery q;
        auto* block = q.AddBlocks();
        auto* pd = block->MutableProbeDesc();
        pd->SetName("InitialProbe");
        pd->SetProvider("LWTRACE_UT_PROVIDER");
        block->AddAction()->MutableRunLogShuttleAction();
        LWManager->New("filter", q);
    }

    ITraceProcessorPtr CreateTraceProcessor(
        TDuration hdd = TDuration::Zero(),
        TDuration ssd = TDuration::Zero(),
        TDuration nonreplSSD = TDuration::Zero())
    {
        auto monitoring = CreateMonitoringServiceStub();
        auto logging = CreateLoggingService(LogBackend);
        TVector<ITraceReaderPtr> readers{
            CreateSlowRequestsFilter(
                "filter",
                logging,
                "STORAGE_TRACE",
                hdd,
                ssd,
                nonreplSSD
        )};
        return NCloud::CreateTraceProcessor(
            Timer,
            Scheduler,
            logging,
            monitoring,
            "STORAGE_TRACE",
            *LWManager,
            std::move(readers)
        );
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TTraceProcessorTest)
{
    const ui32 REQUEST_COUNT = 10;

    void Track(
        NProto::EStorageMediaKind mediaKind = NProto::STORAGE_MEDIA_HYBRID,
        ui64 executionTime = 0)
    {
        TVector<NLWTrace::TOrbit> orbits(REQUEST_COUNT);

        for (ui64 requestId = 0; requestId < orbits.size(); ++requestId) {
            LWTRACK(
                InitialProbe,
                orbits[requestId],
                "s0",
                static_cast<ui32>(mediaKind),
                requestId
            );
        }

        for (ui64 requestId = 0; requestId < orbits.size(); ++requestId) {
            LWTRACK(SomeProbe, orbits[requestId], "s1", requestId);
        }

        for (ui64 requestId = 0; requestId < orbits.size(); ++requestId) {
            if (executionTime) {
                LWTRACK(
                    SomeProbeWithExecutionTime,
                    orbits[requestId],
                    "s2",
                    requestId,
                    executionTime
                );
            } else {
                LWTRACK(SomeOtherProbe, orbits[requestId], "s2", requestId);
            }
        }
    }

    void Check(TEnv& env, ui32 requestCount, ui64 executionTime = 0)
    {
        env.Scheduler->RunAllScheduledTasks();
        UNIT_ASSERT_VALUES_EQUAL(requestCount, env.LogBackend->Recs.size());

        for (const auto& rec : env.LogBackend->Recs) {
            Cdbg << int(rec.first) << "\t" << rec.second;

            TVector<TString> parts;
            StringSplitter(rec.second).SplitByString("WARN: ").Collect(&parts);
            UNIT_ASSERT_VALUES_EQUAL(2, parts.size());

            NJson::TJsonValue v;
            UNIT_ASSERT(NJson::ReadJsonFastTree(parts[1], &v, false));

            const auto& arr = v.GetArray();
            UNIT_ASSERT_VALUES_EQUAL(4, arr.size());
            UNIT_ASSERT_VALUES_EQUAL("InitialProbe", arr[0][0].GetString());
            UNIT_ASSERT_VALUES_EQUAL("SomeProbe", arr[1][0].GetString());
            if (executionTime) {
                UNIT_ASSERT_VALUES_EQUAL(
                    "SomeProbeWithExecutionTime",
                    arr[2][0].GetString()
                );
                UNIT_ASSERT_VALUES_EQUAL(
                    executionTime,
                    FromString<ui64>(arr[2][4].GetString())
                );
            } else {
                UNIT_ASSERT_VALUES_EQUAL("SomeOtherProbe", arr[2][0].GetString());
            }
            UNIT_ASSERT_VALUES_EQUAL("SlowRequests", arr[3][0].GetString());
        }

        env.LogBackend->Recs.clear();

        env.Scheduler->RunAllScheduledTasks();
        UNIT_ASSERT_VALUES_EQUAL(0, env.LogBackend->Recs.size());
    }

    Y_UNIT_TEST(ShouldCollectAllOverlappingTracks)
    {
        TEnv env;
        auto tp = env.CreateTraceProcessor();
        tp->Start();

        Track();
        Check(env, REQUEST_COUNT);
    }

    Y_UNIT_TEST(ShouldProcessLotsOfTracks)
    {
        TEnv env;
        auto tp = env.CreateTraceProcessor();
        tp->Start();

        auto threadPool = CreateThreadPool(20);
        const auto runs = 100;
        // lwtrace depot sizes are limited by 1000
        static_assert(runs <= 1000 / REQUEST_COUNT);

        TAtomic remaining = runs;
        TManualEvent ev;

        for (size_t j = 0; j < runs; ++j) {
            threadPool->SafeAddFunc([&remaining, &ev] () {
                Track();

                AtomicDecrement(remaining);
                ev.Signal();
            });
        }

        while (AtomicGet(remaining)) {
            ev.WaitI();
        }

        auto requestCount = Min<ui32>(runs * REQUEST_COUNT, DumpTracksLimit);
        Check(env, requestCount);
    }

    Y_UNIT_TEST(SlowRequestThresholdByTrackLength)
    {
        // no way to override timestamps in track items
        // => will have to test only max+zero and zero+max thresholds
        TEnv env;
        auto tp = env.CreateTraceProcessor(
            TDuration::Zero(),
            TDuration::Max(),
            TDuration::Max()
        );
        tp->Start();


        Track(NProto::STORAGE_MEDIA_HYBRID);
        Check(env, REQUEST_COUNT);
        Track(NProto::STORAGE_MEDIA_HDD);
        Check(env, REQUEST_COUNT);
        Track(NProto::STORAGE_MEDIA_SSD);
        Check(env, 0);
        Track(NProto::STORAGE_MEDIA_SSD_NONREPLICATED);
        Check(env, 0);

        env.RebuildLWManager();
        tp = env.CreateTraceProcessor(
            TDuration::Max(),
            TDuration::Zero(),
            TDuration::Max()
        );
        tp->Start();

        Track(NProto::STORAGE_MEDIA_HYBRID);
        Check(env, 0);
        Track(NProto::STORAGE_MEDIA_HDD);
        Check(env, 0);
        Track(NProto::STORAGE_MEDIA_SSD);
        Check(env, REQUEST_COUNT);
        Track(NProto::STORAGE_MEDIA_SSD_NONREPLICATED);
        Check(env, 0);

        env.RebuildLWManager();
        tp = env.CreateTraceProcessor(
            TDuration::Max(),
            TDuration::Max(),
            TDuration::Zero()
        );
        tp->Start();

        Track(NProto::STORAGE_MEDIA_HYBRID);
        Check(env, 0);
        Track(NProto::STORAGE_MEDIA_HDD);
        Check(env, 0);
        Track(NProto::STORAGE_MEDIA_SSD);
        Check(env, 0);
        Track(NProto::STORAGE_MEDIA_SSD_NONREPLICATED);
        Check(env, REQUEST_COUNT);
    }

    Y_UNIT_TEST(SlowRequestThresholdByExecutionTimeParam)
    {
        TEnv env;
        auto tp = env.CreateTraceProcessor(
            TDuration::Seconds(20),
            TDuration::Seconds(20),
            TDuration::Seconds(20)
        );
        tp->Start();

        Track(NProto::STORAGE_MEDIA_HYBRID, 15'000'000);
        Check(env, 0);
        Track(NProto::STORAGE_MEDIA_HYBRID, 25'000'000);
        Check(env, REQUEST_COUNT, 25'000'000);
    }
}

}   // namespace NCloud
