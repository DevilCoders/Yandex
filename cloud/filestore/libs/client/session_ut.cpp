#include "session.h"

#include "config.h"

#include <cloud/filestore/libs/service/context.h>
#include <cloud/filestore/libs/service/filestore_test.h>
#include <cloud/filestore/libs/service/request.h>

#include <cloud/storage/core/libs/common/scheduler_test.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>

namespace NCloud::NFileStore::NClient {

using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr TDuration WaitTimeout = TDuration::Seconds(5);

static const TString FileSystemId = "fs1";
static const TString ClientId = "client1";
static const TString SessionId = "session1";

constexpr TDuration RetryTimeout = TDuration::MilliSeconds(100);
constexpr TDuration PingTimeout = TDuration::Seconds(5);

////////////////////////////////////////////////////////////////////////////////

struct TBootstrap
{
    ILoggingServicePtr Logging;
    ITimerPtr Timer;
    std::shared_ptr<TTestScheduler> Scheduler;
    std::shared_ptr<TFileStoreTest> FileStore;
    ISessionPtr Session;

    TBootstrap()
    {
        Logging = CreateLoggingService("console", { TLOG_RESOURCES });
        Timer = CreateWallClockTimer();
        Scheduler = std::make_shared<TTestScheduler>();

        FileStore = std::make_shared<TFileStoreTest>();

        FileStore->CreateSessionHandler = [] (auto request) {
            UNIT_ASSERT_VALUES_EQUAL(GetFileSystemId(*request), FileSystemId);
            UNIT_ASSERT_VALUES_EQUAL(GetClientId(*request), ClientId);

            NProto::TCreateSessionResponse response;
            auto* session = response.MutableSession();
            session->SetSessionId(SessionId);

            return MakeFuture(response);
        };

        FileStore->DestroySessionHandler = [] (auto request) {
            UNIT_ASSERT_VALUES_EQUAL(GetSessionId(*request), SessionId);
            return MakeFuture(NProto::TDestroySessionResponse());
        };

        FileStore->PingSessionHandler = [] (auto request) {
            UNIT_ASSERT_VALUES_EQUAL(GetSessionId(*request), SessionId);
            return MakeFuture(NProto::TPingSessionResponse());
        };

        Session = CreateSession(
            Logging,
            Timer,
            Scheduler,
            FileStore,
            CreateSessionConfig());
    }

    ~TBootstrap()
    {
        Stop();
    }

    void Start()
    {
        if (Scheduler) {
            Scheduler->Start();
        }

        if (Logging) {
            Logging->Start();
        }
    }

    void Stop()
    {
        if (Logging) {
            Logging->Stop();
        }

        if (Scheduler) {
            Scheduler->Stop();
        }
    }

    static TSessionConfigPtr CreateSessionConfig()
    {
        NProto::TSessionConfig proto;
        proto.SetFileSystemId(FileSystemId);
        proto.SetClientId(ClientId);
        proto.SetSessionRetryTimeout(RetryTimeout.MilliSeconds());
        proto.SetSessionPingTimeout(PingTimeout.MilliSeconds());

        return std::make_shared<TSessionConfig>(proto);
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TSessionTest)
{
    Y_UNIT_TEST(ShouldCreateSession)
    {
        TBootstrap bootstrap;
        bootstrap.Start();

        {
            auto future = bootstrap.Session->CreateSession();
            const auto& response = future.GetValue(WaitTimeout);
            UNIT_ASSERT(!HasError(response));
        }

        {
            auto future = bootstrap.Session->DestroySession();
            const auto& response = future.GetValue(WaitTimeout);
            UNIT_ASSERT(!HasError(response));
        }
    }

    Y_UNIT_TEST(ShouldRestoreSession)
    {
        TBootstrap bootstrap;
        bootstrap.Start();

        ui32 createCalled = 0;
        bootstrap.FileStore->CreateSessionHandler = [&] (auto request) {
            UNIT_ASSERT_VALUES_EQUAL(GetFileSystemId(*request), FileSystemId);
            UNIT_ASSERT_VALUES_EQUAL(GetClientId(*request), ClientId);

            ++createCalled;
            NProto::TCreateSessionResponse response;
            auto* session = response.MutableSession();
            session->SetSessionId(SessionId);

            return MakeFuture(response);
        };

        ui32 pingCalled = 0;
        bootstrap.FileStore->PingSessionHandler = [&] (auto request) {
            ++pingCalled;
            UNIT_ASSERT_VALUES_EQUAL(GetSessionId(*request), SessionId);
            return MakeFuture(NProto::TPingSessionResponse());
        };

        TVector<TPromise<NProto::TCreateNodeResponse>> nodes;
        bootstrap.FileStore->CreateNodeHandler = [&] (auto request) {
            UNIT_ASSERT_VALUES_EQUAL(GetFileSystemId(*request), FileSystemId);
            UNIT_ASSERT_VALUES_EQUAL(GetClientId(*request), ClientId);
            UNIT_ASSERT_VALUES_EQUAL(GetSessionId(*request), SessionId);

            nodes.push_back(NewPromise<NProto::TCreateNodeResponse>());
            return nodes.back();
        };

        {
            auto future = bootstrap.Session->CreateSession();
            UNIT_ASSERT_VALUES_EQUAL(createCalled, 1);
            UNIT_ASSERT(!HasError(future.GetValue()));
        }

        bootstrap.Scheduler->RunAllScheduledTasks();
        UNIT_ASSERT_VALUES_EQUAL(pingCalled, 1);

        auto future = bootstrap.Session->CreateNode(
            MakeIntrusive<TCallContext>(),
            std::make_shared<NProto::TCreateNodeRequest>());
        UNIT_ASSERT_VALUES_EQUAL(nodes.size(), 1);

        // should recreate session
        nodes.back().SetValue(TErrorResponse(E_FS_INVALID_SESSION, ""));
        bootstrap.Scheduler->RunAllScheduledTasks();
        UNIT_ASSERT_VALUES_EQUAL(pingCalled, 2);

        UNIT_ASSERT(!future.HasValue());
        UNIT_ASSERT_VALUES_EQUAL(createCalled, 2);
        UNIT_ASSERT_VALUES_EQUAL(nodes.size(), 2);

        // no extra ping scheduled
        bootstrap.Scheduler->RunAllScheduledTasks();
        UNIT_ASSERT_VALUES_EQUAL(pingCalled, 3);
    }

    Y_UNIT_TEST(ShouldRestoreSessionOnlyOnce)
    {
        TBootstrap bootstrap;
        bootstrap.Start();

        ui32 createCalled = 0;
        bootstrap.FileStore->CreateSessionHandler = [&] (auto request) {
            UNIT_ASSERT_VALUES_EQUAL(GetFileSystemId(*request), FileSystemId);
            UNIT_ASSERT_VALUES_EQUAL(GetClientId(*request), ClientId);

            ++createCalled;
            NProto::TCreateSessionResponse response;
            auto* session = response.MutableSession();
            session->SetSessionId(SessionId);

            return MakeFuture(response);
        };

        TVector<TPromise<NProto::TCreateNodeResponse>> nodes;
        bootstrap.FileStore->CreateNodeHandler = [&] (auto request) {
            UNIT_ASSERT_VALUES_EQUAL(GetFileSystemId(*request), FileSystemId);
            UNIT_ASSERT_VALUES_EQUAL(GetClientId(*request), ClientId);
            UNIT_ASSERT_VALUES_EQUAL(GetSessionId(*request), SessionId);

            nodes.push_back(NewPromise<NProto::TCreateNodeResponse>());
            return nodes.back();
        };

        UNIT_ASSERT(!HasError(bootstrap.Session->CreateSession().GetValue()));
        UNIT_ASSERT_VALUES_EQUAL(createCalled, 1);

        constexpr ui32 count= 5;
        TVector<TFuture<NProto::TCreateNodeResponse>> futures(count);
        for (ui32 i = 0; i < 5; ++i) {
            futures[i] = bootstrap.Session->CreateNode(
                MakeIntrusive<TCallContext>(),
                std::make_shared<NProto::TCreateNodeRequest>());
        }

        auto promise = NewPromise<NProto::TCreateSessionResponse>();
        bootstrap.FileStore->CreateSessionHandler = [&] (auto request) {
            UNIT_ASSERT_VALUES_EQUAL(GetFileSystemId(*request), FileSystemId);
            UNIT_ASSERT_VALUES_EQUAL(GetClientId(*request), ClientId);

            ++createCalled;
            return promise;
        };

        // fail each request with the same error
        UNIT_ASSERT_VALUES_EQUAL(nodes.size(), count);
        for (auto& future: nodes) {
            future.SetValue(TErrorResponse(E_FS_INVALID_SESSION, ""));
        }
        bootstrap.Scheduler->RunAllScheduledTasks();

        // assert it was retried
        for (auto& future: futures) {
            UNIT_ASSERT(!future.HasValue());
        }

        // but just once for create session
        UNIT_ASSERT_VALUES_EQUAL(createCalled, 2);
        UNIT_ASSERT_VALUES_EQUAL(nodes.size(), count);

        {
            NProto::TCreateSessionResponse response;
            auto* session = response.MutableSession();
            session->SetSessionId(SessionId);

            promise.SetValue(response);
        }

        // requests were retried
        UNIT_ASSERT_VALUES_EQUAL(nodes.size(), 2 * count);
        for (auto i = count; i < nodes.size(); ++i) {
            UNIT_ASSERT(!nodes[i].HasValue());
        }
    }

    Y_UNIT_TEST(ShouldFailInFlightOnSessionFail)
    {
        TBootstrap bootstrap;
        bootstrap.Start();

        bootstrap.FileStore->CreateSessionHandler = [&] (auto request) {
            UNIT_ASSERT_VALUES_EQUAL(GetFileSystemId(*request), FileSystemId);
            UNIT_ASSERT_VALUES_EQUAL(GetClientId(*request), ClientId);

            NProto::TCreateSessionResponse response;
            auto* session = response.MutableSession();
            session->SetSessionId(SessionId);

            return MakeFuture(response);
        };

        TVector<TPromise<NProto::TCreateNodeResponse>> nodes;
        bootstrap.FileStore->CreateNodeHandler = [&] (auto request) {
            UNIT_ASSERT_VALUES_EQUAL(GetFileSystemId(*request), FileSystemId);
            UNIT_ASSERT_VALUES_EQUAL(GetClientId(*request), ClientId);
            UNIT_ASSERT_VALUES_EQUAL(GetSessionId(*request), SessionId);

            nodes.push_back(NewPromise<NProto::TCreateNodeResponse>());
            return nodes.back();
        };

        UNIT_ASSERT(!HasError(bootstrap.Session->CreateSession().GetValue()));

        constexpr ui32 count= 5;
        TVector<TFuture<NProto::TCreateNodeResponse>> futures(count);
        for (ui32 i = 0; i < 5; ++i) {
            futures[i] = bootstrap.Session->CreateNode(
                MakeIntrusive<TCallContext>(),
                std::make_shared<NProto::TCreateNodeRequest>());
        }

        TVector<TPromise<NProto::TCreateSessionResponse>> creates;
        bootstrap.FileStore->CreateSessionHandler = [&] (auto request) {
            UNIT_ASSERT_VALUES_EQUAL(GetFileSystemId(*request), FileSystemId);
            UNIT_ASSERT_VALUES_EQUAL(GetClientId(*request), ClientId);

            creates.push_back(NewPromise<NProto::TCreateSessionResponse>());
            return creates.back();
        };

        // fail each request with the same error
        UNIT_ASSERT_VALUES_EQUAL(nodes.size(), count);
        for (auto& future: nodes) {
            future.SetValue(TErrorResponse(E_FS_INVALID_SESSION, ""));
        }

        bootstrap.Scheduler->RunAllScheduledTasks();

        // assert it was retried
        for (auto& future: futures) {
            UNIT_ASSERT(!future.HasValue());
        }

        // but just once for create session
        UNIT_ASSERT_VALUES_EQUAL(creates.size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(nodes.size(), count);

        // fail session recreation
        creates.back().SetValue(TErrorResponse(E_FS_INVALID_SESSION, ""));

        // assert new session was requested
        UNIT_ASSERT_VALUES_EQUAL(creates.size(), 2);

        // but inflight requests were failed
        UNIT_ASSERT_VALUES_EQUAL(nodes.size(), count);
        for (auto& future: futures) {
            UNIT_ASSERT(future.HasValue());
            UNIT_ASSERT(HasError(future.GetValue()));
        }
    }
}

}   // namespace NCloud::NFileStore::NClient
