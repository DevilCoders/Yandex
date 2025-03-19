#include "test.h"

#include "client.h"
#include "context.h"
#include "request.h"

#include <cloud/filestore/libs/client/config.h>
#include <cloud/filestore/libs/client/session.h>
#include <cloud/filestore/libs/service/context.h>
#include <cloud/filestore/libs/service/filestore.h>
#include <cloud/filestore/libs/service/request.h>

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/scheduler.h>
#include <cloud/storage/core/libs/common/thread.h>
#include <cloud/storage/core/libs/diagnostics/histogram.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <library/cpp/protobuf/json/proto2json.h>

#include <util/datetime/base.h>
#include <util/generic/deque.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/random/random.h>
#include <util/string/builder.h>
#include <util/string/printf.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/condvar.h>
#include <util/system/event.h>
#include <util/system/mutex.h>
#include <util/system/thread.h>

namespace NCloud::NFileStore::NLoadTest {

using namespace NThreading;
using namespace NCloud::NFileStore::NClient;

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TTestStats
{
    struct TStats
    {
        ui64 Requests = 0;
        TLatencyHistogram Hist;
    };

    TString Name;
    bool Success = true;
    TMap<NProto::EAction, TStats> ActionStats;
};

////////////////////////////////////////////////////////////////////////////////

template <typename T>
T WaitForCompletion(
    const TString& request,
    const TFuture<T>& future)
{
    const auto& response = future.GetValue(TDuration::Max());
    if (HasError(response)) {
        const auto& error = response.GetError();
        throw yexception()
            << "Failed to execute " << request << " request: "
            << FormatError(error);
    }

    return response;
}

////////////////////////////////////////////////////////////////////////////////

class TRequestsCompletionQueue
{
private:
    TMutex Lock;
    TDeque<std::unique_ptr<TCompletedRequest>> Items;

public:
    void Enqueue(std::unique_ptr<TCompletedRequest> request)
    {
        with_lock (Lock) {
            Items.emplace_back(std::move(request));
        }
    }

    std::unique_ptr<TCompletedRequest> Dequeue()
    {
        with_lock (Lock) {
            std::unique_ptr<TCompletedRequest> ptr;
            if (Items) {
                ptr = std::move(Items.front());
                Items.pop_front();
            }

            return ptr;
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class TLoadTest final
    : public ITest
    , public ISimpleThread
    , public std::enable_shared_from_this<TLoadTest>
{
private:
    static constexpr TDuration ReportInterval = TDuration::Seconds(5);

    const TAppContext& Ctx;
    const NProto::TLoadTest Config;
    const ITimerPtr Timer;
    const ISchedulerPtr Scheduler;
    const ILoggingServicePtr Logging;

    TLog Log;
    IFileStoreServicePtr Client;
    ISessionPtr Session;

    TString FileSystemId;
    TString ClientId;
    TString SessionId;

    ui32 MaxIoDepth = 64;
    ui32 CurrentIoDepth = 0;

    TDuration MaxDuration;
    TInstant StartTs;

    ui64 MaxRequests = 0;
    ui64 RequestsSent = 0;
    ui64 RequestsCompleted = 0;

    TInstant LastReportTs;
    ui64 LastRequestsCompleted = 0;

    IRequestGeneratorPtr RequestGenerator;
    TRequestsCompletionQueue CompletionQueue;
    TAutoEvent Event;

    TTestStats TestStats;
    TPromise<NProto::TTestStats> Result = NewPromise<NProto::TTestStats>();

public:
    TLoadTest(
            const TAppContext& ctx,
            const NProto::TLoadTest& config,
            ITimerPtr timer,
            ISchedulerPtr scheduler,
            ILoggingServicePtr logging,
            IClientFactoryPtr clientFactory)
        : Ctx(ctx)
        , Config(config)
        , Timer(std::move(timer))
        , Scheduler(std::move(scheduler))
        , Logging(std::move(logging))
        , Client(clientFactory->CreateClient())

    {
        Log = Logging->CreateLog("RUNNER");
    }

    TFuture<NProto::TTestStats> Run() override
    {
        ISimpleThread::Start();
        return Result;
    }

    void* ThreadProc() override
    {
        StartTs = TInstant::Now();
        NCloud::SetCurrentThreadName(MakeTestTag());

        try {
            SetupTest();
            LastReportTs = TInstant::Now();

            while (!ShouldStop()) {
                while (SendNextRequest()) {
                    ++RequestsSent;
                }

                if (!ShouldStop()) {
                    Event.WaitD(RequestGenerator->NextRequestAt());
                    ProcessCompletedRequests();
                }

                ReportProgress();
            }

            TeardownTest();

            // prevent race between this thread and main thread
            // destroying test instance right after setvalue()
            auto result = Result;
            result.SetValue(GetStats());
        } catch(...) {
            STORAGE_ERROR("%s test has failed: %s",
                MakeTestTag().c_str(), CurrentExceptionMessage().c_str());

            Result.SetException(std::current_exception());
        }

        return nullptr;
    }

private:
    void SetupTest()
    {
        Client->Start();

        // setup test limits
        if (auto depth = Config.GetIODepth()) {
            MaxIoDepth = depth;
        }

        MaxRequests = Config.GetRequestsCount();
        MaxDuration = TDuration::Seconds(Config.GetTestDuration());

        STORAGE_INFO("%s setting up test",
            MakeTestTag().c_str());

        // setup test filestore
        if (Config.HasFileSystemId()) {
            FileSystemId = Config.GetFileSystemId();
        } else if (Config.HasCreateFileStoreRequest()) {
            FileSystemId = Config.GetCreateFileStoreRequest().GetFileSystemId();

            auto request = std::make_shared<NProto::TCreateFileStoreRequest>(
                Config.GetCreateFileStoreRequest());

            STORAGE_INFO("%s create filestore: %s",
                MakeTestTag().c_str(),
                DumpMessage(*request).c_str());

            TCallContextPtr ctx = MakeIntrusive<TCallContext>();
            auto result = Client->CreateFileStore(ctx, request);
            WaitForCompletion(GetRequestName(*request), result);
        } else {
            ythrow yexception()
                << MakeTestTag()
                << " config should have either existing filesystem id or request to create one";
        }

        CreateSession();

        NProto::THeaders headers;
        headers.SetClientId(Config.GetName());
        headers.SetSessionId(SessionId);

        switch (Config.GetSpecsCase()) {
            case NProto::TLoadTest::kIndexLoadSpec:
                RequestGenerator = CreateIndexRequestGenerator(
                    Config.GetIndexLoadSpec(),
                    Logging,
                    Session,
                    FileSystemId,
                    headers);
                break;
            case NProto::TLoadTest::kDataLoadSpec:
                RequestGenerator = CreateDataRequestGenerator(
                    Config.GetDataLoadSpec(),
                    Logging,
                    Session,
                    FileSystemId,
                    headers);
                break;
            default:
                ythrow yexception()
                    << MakeTestTag()
                    << " config should have test spec";
        }
    }

    void CreateSession()
    {
        NProto::TSessionConfig proto;
        proto.SetFileSystemId(FileSystemId);
        proto.SetClientId(ClientId);
        proto.SetSessionPingTimeout(Config.GetSessionPingTimeout());
        proto.SetSessionRetryTimeout(Config.GetSessionRetryTimeout());

        Session = NClient::CreateSession(
            Logging,
            Timer,
            Scheduler,
            Client,
            std::make_shared<TSessionConfig>(proto));

        STORAGE_INFO("%s establishing session",
            MakeTestTag().c_str());

        // establish session
        auto result = Session->CreateSession();
        WaitForCompletion("create-session", result);

        SessionId = result.GetValue().GetSession().GetSessionId();
        STORAGE_INFO("%s session established: %s",
            MakeTestTag().c_str(), SessionId.c_str());
    }

    bool ShouldStop() const
    {
        return AtomicGet(Ctx.ShouldStop) || !TestStats.Success ||
            (LimitsReached() && RequestsCompleted == RequestsSent);
    }

    bool LimitsReached() const
    {
        return (MaxDuration && TInstant::Now() - StartTs >= MaxDuration) ||
            (MaxRequests && RequestsSent >= MaxRequests);
    }

    bool SendNextRequest()
    {
        if (LimitsReached() || (MaxIoDepth && CurrentIoDepth >= MaxIoDepth) ||
            !RequestGenerator->HasNextRequest() || ShouldStop())
        {
            return false;
        }

        ++CurrentIoDepth;
        auto self = weak_from_this();
        RequestGenerator->ExecuteNextRequest().Apply(
            [=] (const TFuture<TCompletedRequest>& future) {
                if (auto ptr = self.lock()) {
                    ptr->SignalCompletion(future.GetValue());
                }
            });

        return true;
    }

    void SignalCompletion(TCompletedRequest request)
    {
        CompletionQueue.Enqueue(
            std::make_unique<TCompletedRequest>(std::move(request)));

        Event.Signal();
    }

    void ProcessCompletedRequests()
    {
        while (auto request = CompletionQueue.Dequeue()) {
            Y_VERIFY(CurrentIoDepth > 0);
            --CurrentIoDepth;
            ++RequestsCompleted;

            auto code = request->Error.GetCode();
            if (FAILED(code)) {
                STORAGE_ERROR("%s failing test due to: %s",
                    MakeTestTag().c_str(),
                    FormatError(request->Error).c_str());

                TestStats.Success = false;
            }

            auto& stats = TestStats.ActionStats[request->Action];
            ++stats.Requests;
            stats.Hist.RecordValue(request->Elapsed);
        }
    }

    void ReportProgress()
    {
        auto now = TInstant::Now();
        auto elapsed = now - LastReportTs;

        if (elapsed > ReportInterval) {
            const auto requestsCompleted = RequestsCompleted - LastRequestsCompleted;

            auto stats = GetStats();
            STORAGE_INFO("%s current rate: %ld r/s; stats:\n%s",
                MakeTestTag().c_str(),
                (ui64)(requestsCompleted / elapsed.Seconds()),
                NProtobufJson::Proto2Json(stats, {.FormatOutput = true}).c_str());

            LastReportTs = now;
            LastRequestsCompleted = RequestsCompleted;
        }
    }

    NProto::TTestStats GetStats()
    {
        NProto::TTestStats results;
        results.SetName(Config.GetName());
        results.SetSuccess(TestStats.Success);

        auto* stats = results.MutableStats();
        for (const auto& pair: TestStats.ActionStats) {
            auto* action = stats->Add();
            action->SetAction(NProto::EAction_Name(pair.first));
            action->SetCount(pair.second.Requests);
            FillLatency(pair.second.Hist, *action->MutableLatency());
        }

        // TODO report some stats for the files generated during the shooting

        return results;
    }

    void TeardownTest()
    {
        while (CurrentIoDepth > 0) {
            Sleep(TDuration::Seconds(1));
            ProcessCompletedRequests();
        }

        auto ctx = MakeIntrusive<TCallContext>();

        auto result = Session->DestroySession();
        WaitForCompletion("destroy session", result);

        if (Config.HasCreateFileStoreRequest()) {
            auto request = std::make_shared<NProto::TDestroyFileStoreRequest>();
            request->SetFileSystemId(FileSystemId);

            auto result = Client->DestroyFileStore(ctx, request);
            WaitForCompletion(GetRequestName(*request), result);
        }

        if (Client) {
            Client->Stop();
        }
    }

    void FillLatency(
        const TLatencyHistogram& hist,
        NProto::TLatency& latency)
    {
        latency.SetP50(hist.GetValueAtPercentile(50));
        latency.SetP90(hist.GetValueAtPercentile(90));
        latency.SetP95(hist.GetValueAtPercentile(95));
        latency.SetP99(hist.GetValueAtPercentile(99));
        latency.SetP999(hist.GetValueAtPercentile(99.9));
        latency.SetMin(hist.GetMin());
        latency.SetMax(hist.GetMax());
        latency.SetMean(hist.GetMean());
        latency.SetStdDeviation(hist.GetStdDeviation());
    }

    TString MakeTestTag()
    {
        return NLoadTest::MakeTestTag(Config.GetName());
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TString MakeTestTag(const TString& name)
{
    return Sprintf("[%s]", name.c_str());
}

ITestPtr CreateLoadTest(
    const TAppContext& ctx,
    const NProto::TLoadTest& config,
    ITimerPtr timer,
    ISchedulerPtr scheduler,
    ILoggingServicePtr logging,
    IClientFactoryPtr clientFactory)
{
    return std::make_shared<TLoadTest>(
        ctx,
        config,
        std::move(timer),
        std::move(scheduler),
        std::move(logging),
        std::move(clientFactory));
}

}   // namespace NCloud::NFileStore::NLoadTest
