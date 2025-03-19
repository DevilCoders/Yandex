#include "fs.h"

#include "config.h"
#include "driver.h"
#include "log.h"

#include <cloud/filestore/libs/client/config.h>
#include <cloud/filestore/libs/client/session.h>
#include <cloud/filestore/libs/diagnostics/request_stats.h>
#include <cloud/filestore/libs/fuse/cache.h>
#include <cloud/filestore/libs/fuse/protos/session.pb.h>
#include <cloud/filestore/libs/fuse/test/context.h>
#include <cloud/filestore/libs/fuse/test/request.h>

#include <cloud/filestore/libs/service/context.h>
#include <cloud/filestore/libs/service/filestore.h>
#include <cloud/filestore/libs/service/filestore_test.h>
#include <cloud/filestore/libs/service/request.h>

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/scheduler.h>
#include <cloud/storage/core/libs/common/scheduler_test.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/common/timer_test.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <library/cpp/monlib/dynamic_counters/counters.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/threading/atomic/bool.h>

#include <util/datetime/base.h>
#include <util/generic/guid.h>
#include <util/generic/string.h>

namespace NCloud::NFileStore::NFuse {

using namespace NThreading;
using namespace NCloud::NFileStore::NClient;

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr TDuration WaitTimeout = TDuration::Seconds(5);

TString CreateBuffer(size_t len, char fill = 0)
{
    return TString(len, fill);
}

////////////////////////////////////////////////////////////////////////////////

struct TBootstrap
{
    TFuseContext Fuse;

    ILoggingServicePtr Logging;
    ITimerPtr Timer;
    ISchedulerPtr Scheduler;
    NMonitoring::TDynamicCountersPtr Counters;

    std::shared_ptr<TFileStoreTest> Service;

    IFileSystemDriverPtr Driver;

    TBootstrap(
            ITimerPtr timer = CreateWallClockTimer(),
            ISchedulerPtr scheduler = CreateScheduler())
        : Timer{std::move(timer)}
        , Scheduler{std::move(scheduler)}
        , Counters{MakeIntrusive<NMonitoring::TDynamicCounters>()}
    {
        signal(SIGUSR1, SIG_IGN);   // see fuse/driver for details

        Logging = CreateLoggingService("console", { TLOG_RESOURCES });
        InitLog(Logging);

        Service = std::make_shared<TFileStoreTest>();
        Service->CreateSessionHandler = [] (auto request) {
            UNIT_ASSERT(request->GetRestoreClientSession());
            NProto::TCreateSessionResponse result;
            result.MutableSession()->SetSessionId(CreateGuidAsString());
            return MakeFuture(result);
        };

        Service->ResetSessionHandler = [] (auto request) {
            Y_UNUSED(request);
            return MakeFuture(NProto::TResetSessionResponse{});
        };

        Service->DestroySessionHandler = [] (auto request) {
            Y_UNUSED(request);
            return MakeFuture(NProto::TDestroySessionResponse());
        };

        Service->PingSessionHandler = [] (auto request) {
            Y_UNUSED(request);
            return MakeFuture(NProto::TPingSessionResponse());
        };

        auto sessionConfig = std::make_shared<TSessionConfig>(NProto::TSessionConfig{});
        auto session = CreateSession(
            Logging,
            Timer,
            Scheduler,
            Service,
            std::move(sessionConfig));

        auto factory = CreateFuseFileSystemFactory(
            Logging,
            Scheduler,
            Timer);

        NProto::TFuseConfig proto;
        proto.SetDebug(true);
        proto.SetSocketPath("/tmp/vhost.socket");

        auto config = std::make_shared<TFuseConfig>(proto);
        Driver = CreateFileSystemDriver(
            config,
            Logging,
            CreateRequestStatsRegistry(
                "fs_ut",
                {},
                Counters,
                Timer),
            session,
            factory);
    }

    ~TBootstrap()
    {
        Stop();
    }

    void Start()
    {
        StartAsync().Wait();
    }

    TFuture<NProto::TError> StartAsync()
    {
        auto fs = MakeFuture<NProto::TError>();
        auto driver = MakeFuture<NProto::TError>();

        if (Scheduler) {
            Scheduler->Start();
        }

        return Driver->StartAsync();
    }

    void Stop()
    {
        StopAsync().Wait();
    }

    TFuture<void> StopAsync()
    {
        TFuture<void> f = Driver->StopAsync();

        if (Scheduler) {
            f.Apply([&] (auto) {
                Scheduler->Stop();
            });
        }
        return f;
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TFileSystemTest)
{
    Y_UNIT_TEST(ShouldHandleInitRequest)
    {
        TBootstrap bootstrap;
        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));
    }

    Y_UNIT_TEST(ShouldHandleWriteRequest)
    {
        TBootstrap bootstrap;

        const ui64 nodeId = 123;
        const ui64 handleId = 456;
        bootstrap.Service->CreateHandleHandler = [&] (auto request) {
            Y_UNUSED(request);
            NProto::TCreateHandleResponse result;
            result.SetHandle(handleId);
            result.MutableNodeAttr()->SetId(nodeId);
            return MakeFuture(result);
        };

        bootstrap.Service->WriteDataHandler = [&] (auto request) {
            Y_UNUSED(request);
            NProto::TWriteDataResponse result;
            return MakeFuture(result);
        };

        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        auto handle = bootstrap.Fuse.SendRequest<TCreateHandleRequest>("/file1");
        UNIT_ASSERT_VALUES_EQUAL(handle.GetValue(WaitTimeout), handleId);

        auto write = bootstrap.Fuse.SendRequest<TWriteRequest>(
            nodeId, handleId, 0, CreateBuffer(4096, 'a'));
        UNIT_ASSERT_NO_EXCEPTION(write.GetValue(WaitTimeout));
    }

    Y_UNIT_TEST(ShouldPassSessionId)
    {
        TBootstrap bootstrap;

        const TString sessionId = CreateGuidAsString();
        bootstrap.Service->CreateSessionHandler = [&] (auto) {
            NProto::TCreateSessionResponse result;
            result.MutableSession()->SetSessionId(sessionId);
            return MakeFuture(result);
        };

        const ui64 nodeId = 123;
        const ui64 handleId = 456;
        bootstrap.Service->CreateHandleHandler = [&] (auto request) {
            NProto::TCreateHandleResponse result;
            if (GetSessionId(*request) != sessionId) {
                result = TErrorResponse(E_ARGUMENT, "invalid session id passed");
            } else {
                result.SetHandle(handleId);
                result.MutableNodeAttr()->SetId(nodeId);
            }
            return MakeFuture(result);
        };

        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        auto handle = bootstrap.Fuse.SendRequest<TCreateHandleRequest>("/file1");
        UNIT_ASSERT_VALUES_EQUAL(handle.GetValue(WaitTimeout), handleId);
    }

    Y_UNIT_TEST(ShouldRecoverSession)
    {
        TBootstrap bootstrap;

        const TSet<int> expected = {100500, 100501};
        int handle = 100500;

        const TString sessionId = CreateGuidAsString();

        bool recovered = false;
        bootstrap.Service->CreateSessionHandler = [&] (auto request) {
            NProto::TCreateSessionResponse result;
            if (auto session = GetSessionId(*request)) {
                if (session != sessionId) {
                    NProto::TCreateSessionResponse result =
                        TErrorResponse(E_ARGUMENT, "invalid session");
                    return MakeFuture(result);
                }

                recovered = true;
            }

            result.MutableSession()->SetSessionId(sessionId);
            return MakeFuture(result);
        };

        bool called = false;
        auto promise = NewPromise<NProto::TCreateHandleResponse>();
        bootstrap.Service->CreateHandleHandler = [&] (auto request) {
            NProto::TCreateHandleResponse result;
            if (!called) {
                called = true;
                return promise.GetFuture();
            } else if (GetSessionId(*request) != sessionId) {
                result = TErrorResponse(E_ARGUMENT, "");
            } else {
                result.SetHandle(handle++);
                result.MutableNodeAttr()->SetId(100500);
            }

            return MakeFuture(result);
        };

        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        auto future1 = bootstrap.Fuse.SendRequest<TCreateHandleRequest>("/file1");
        UNIT_ASSERT(!future1.HasValue());

        auto future2 = bootstrap.Fuse.SendRequest<TCreateHandleRequest>("/file2");
        UNIT_ASSERT(!future2.HasValue());

        NProto::TCreateHandleResponse result;
        result = TErrorResponse(E_FS_INVALID_SESSION, "invalid session");
        promise.SetValue(result);

        UNIT_ASSERT(IsIn(expected, future1.GetValue(WaitTimeout)));
        UNIT_ASSERT(IsIn(expected, future2.GetValue(WaitTimeout)));
        UNIT_ASSERT(recovered);
    }

    Y_UNIT_TEST(ShouldWaitForSessionToAppear)
    {
        TBootstrap bootstrap;

        const int handle = 100500;
        const TString sessionId = CreateGuidAsString();

        auto promise = NewPromise<NProto::TCreateSessionResponse>();
        bootstrap.Service->CreateSessionHandler = [&] (auto) {
            return promise;
        };

        bootstrap.Service->CreateHandleHandler = [&] (auto request) {
            NProto::TCreateHandleResponse result;
            if (GetSessionId(*request) != sessionId) {
                result = TErrorResponse(E_ARGUMENT, "");
            } else {
                result.SetHandle(handle);
                result.MutableNodeAttr()->SetId(100500);
            }

            return MakeFuture(result);
        };

        auto startFuture = bootstrap.StartAsync();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT(!init.HasValue());

        auto future = bootstrap.Fuse.SendRequest<TCreateHandleRequest>("/file1");
        UNIT_ASSERT(!future.HasValue());

        NProto::TCreateSessionResponse session;
        session.MutableSession()->SetSessionId(sessionId);
        promise.SetValue(session);

        UNIT_ASSERT(startFuture.Wait(WaitTimeout));
        const auto& error = startFuture.GetValue();
        UNIT_ASSERT(!FAILED(error.GetCode()));

        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));
        UNIT_ASSERT_VALUES_EQUAL(future.GetValue(WaitTimeout), handle);
    }

    Y_UNIT_TEST(ShouldFailRequestIfFailedToRecoverSession)
    {
        TBootstrap bootstrap;

        const int handle = 100500;
        const TString sessionId1 = CreateGuidAsString();
        const TString sessionId2 = CreateGuidAsString();

        TString sessionId = sessionId1;
        bootstrap.Service->CreateSessionHandler = [&] (auto request) {
            NProto::TCreateSessionResponse result;
            if (auto session = GetSessionId(*request)) {
                if (session != sessionId) {
                    result = TErrorResponse(E_FS_INVALID_SESSION, "invalid session");
                }
            }

            result.MutableSession()->SetSessionId(sessionId);
            sessionId = sessionId2;

            return MakeFuture(result);
        };

        bootstrap.Service->CreateHandleHandler = [&] (auto request) {
            NProto::TCreateHandleResponse result;
            if (GetSessionId(*request) != sessionId) {
                result = TErrorResponse(E_FS_INVALID_SESSION, "");
            } else {
                result.SetHandle(handle);
                result.MutableNodeAttr()->SetId(100500);
            }

            return MakeFuture(result);
        };

        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        auto future = bootstrap.Fuse.SendRequest<TCreateHandleRequest>("/file1");
        UNIT_ASSERT_EXCEPTION(future.GetValue(WaitTimeout), yexception);

        future = bootstrap.Fuse.SendRequest<TCreateHandleRequest>("/file1");
        UNIT_ASSERT_NO_EXCEPTION(future.GetValue(WaitTimeout));
        UNIT_ASSERT_VALUES_EQUAL(future.GetValue(WaitTimeout), handle);
    }

    Y_UNIT_TEST(ShouldPingSession)
    {
        auto scheduler = std::make_shared<TTestScheduler>();
        TBootstrap bootstrap(CreateWallClockTimer(), scheduler);

        const TString sessionId = CreateGuidAsString();
        bootstrap.Service->CreateSessionHandler = [&] (auto) {
            NProto::TCreateSessionResponse result;
            result.MutableSession()->SetSessionId(sessionId);

            return MakeFuture(result);
        };

        bool called = false;
        bootstrap.Service->PingSessionHandler = [&] (auto request) {
            if (GetSessionId(*request) == sessionId) {
                called = true;
            }

            return MakeFuture(NProto::TPingSessionResponse());
        };

        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        scheduler->RunAllScheduledTasks();

        UNIT_ASSERT(called);
    }

    Y_UNIT_TEST(ShouldHandleReadDir)
    {
        TBootstrap bootstrap;
        bootstrap.Service->ListNodesHandler = [&] (auto request) {
            Y_UNUSED(request);
            NProto::TListNodesResponse result;
            result.AddNames()->assign("1.txt");

            auto* node = result.AddNodes();
            node->SetId(10);
            node->SetType(NProto::E_REGULAR_NODE);

            return MakeFuture(result);
        };

        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        const ui64 nodeId = 123;

        auto handle = bootstrap.Fuse.SendRequest<TOpenDirRequest>(nodeId);
        UNIT_ASSERT(handle.Wait(WaitTimeout));
        auto handleId = handle.GetValue();

        // read dir consists of sequantial reading until empty resposne
        auto read = bootstrap.Fuse.SendRequest<TReadDirRequest>(nodeId, handleId);
        UNIT_ASSERT(read.Wait(WaitTimeout));
        auto size = read.GetValue();
        UNIT_ASSERT(size > 0);

        read = bootstrap.Fuse.SendRequest<TReadDirRequest>(nodeId, handleId, size);
        UNIT_ASSERT(read.Wait(WaitTimeout));
        size = read.GetValue();
        UNIT_ASSERT_VALUES_EQUAL(size, 0);

        auto close = bootstrap.Fuse.SendRequest<TReleaseDirRequest>(nodeId, handleId);
        UNIT_ASSERT_NO_EXCEPTION(close.GetValue(WaitTimeout));
    }

    Y_UNIT_TEST(ShouldHandleReadDirInvalidHandles)
    {
        TBootstrap bootstrap;
        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        const ui64 nodeId = 123;

        auto read = bootstrap.Fuse.SendRequest<TReadDirRequest>(nodeId, 100500);
        UNIT_ASSERT(read.Wait(WaitTimeout));
        UNIT_ASSERT(read.HasException());

        auto close = bootstrap.Fuse.SendRequest<TReleaseDirRequest>(nodeId, 100500);
        UNIT_ASSERT_NO_EXCEPTION(close.GetValue(WaitTimeout));
    }

    Y_UNIT_TEST(ShouldHandleReadDirPaging)
    {
        TBootstrap bootstrap;

        ui32 numCalls = 0;
        bootstrap.Service->ListNodesHandler = [&] (auto request) {
            static ui64 id = 1;

            NProto::TListNodesResponse result;
            result.AddNames()->assign(ToString(id) + ".txt");

            auto* node = result.AddNodes();
            node->SetId(id++);
            node->SetType(NProto::E_REGULAR_NODE);

            if (!numCalls) {
                result.SetCookie("cookie");
            } else {
                Y_VERIFY(request->GetCookie());
            }

            ++numCalls;
            return MakeFuture(result);
        };

        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        const ui64 nodeId = 123;

        auto handle = bootstrap.Fuse.SendRequest<TOpenDirRequest>(nodeId);
        UNIT_ASSERT(handle.Wait(WaitTimeout));
        auto handleId = handle.GetValue();

        // read dir consists of sequantial reading until empty resposne
        auto read = bootstrap.Fuse.SendRequest<TReadDirRequest>(nodeId, handleId);
        UNIT_ASSERT(read.Wait(WaitTimeout));
        UNIT_ASSERT_VALUES_EQUAL(numCalls, 1);

        auto size1 = read.GetValue();
        UNIT_ASSERT(size1 > 0);

        read = bootstrap.Fuse.SendRequest<TReadDirRequest>(nodeId, handleId, size1);
        UNIT_ASSERT(read.Wait(WaitTimeout));
        UNIT_ASSERT_VALUES_EQUAL(numCalls, 2);

        auto size2 = read.GetValue();
        UNIT_ASSERT(size2 > 0);

        read = bootstrap.Fuse.SendRequest<TReadDirRequest>(nodeId, handleId, size1 + size2);
        UNIT_ASSERT(read.Wait(WaitTimeout));
        UNIT_ASSERT_VALUES_EQUAL(numCalls, 2);

        auto size3 = read.GetValue();
        UNIT_ASSERT_VALUES_EQUAL(size3, 0);

        auto close = bootstrap.Fuse.SendRequest<TReleaseDirRequest>(nodeId, handleId);
        UNIT_ASSERT_NO_EXCEPTION(close.GetValue(WaitTimeout));
    }

    Y_UNIT_TEST(ShouldHandleForgetRequestsForUnknownNodes)
    {
        TBootstrap bootstrap;

        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        const ui64 nodeId = 123;
        const ui64 refCount = 10;

        auto forget = bootstrap.Fuse.SendRequest<TForgetRequest>(nodeId, refCount);
        UNIT_ASSERT_NO_EXCEPTION(forget.GetValue(WaitTimeout));
    }

    Y_UNIT_TEST(ShouldResetSessionStateUponInitAndDestroy)
    {
        TBootstrap bootstrap;

        TString state;
        ui32 resets = 0;
        bootstrap.Service->ResetSessionHandler = [&] (auto request) {
            ++resets;
            state = request->GetSessionState();

            return MakeFuture(NProto::TResetSessionResponse{});
        };

        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        UNIT_ASSERT_VALUES_EQUAL(resets, 1);
        UNIT_ASSERT(state);

        auto destroy = bootstrap.Fuse.SendRequest<TDestroyRequest>();
        UNIT_ASSERT_NO_EXCEPTION(destroy.GetValue(WaitTimeout));

        UNIT_ASSERT_VALUES_EQUAL(resets, 2);
        UNIT_ASSERT(!state);
    }

    Y_UNIT_TEST(ShouldReinitSessionWithoutInitRequest)
    {
        TBootstrap bootstrap;

        const ui64 nodeId = 123;
        const ui64 handleId = 456;
        bootstrap.Service->CreateHandleHandler = [&] (auto request) {
            Y_VERIFY(request->GetNodeId() != nodeId);

            NProto::TCreateHandleResponse response;
            response.SetHandle(handleId);
            response.MutableNodeAttr()->SetId(nodeId);

            return MakeFuture(response);
        };

        bootstrap.Service->CreateSessionHandler = [&] (auto request) {
            Y_UNUSED(request);
            NProto::TFuseSessionState state;
            state.SetProtoMajor(7);
            state.SetProtoMinor(33);
            state.SetBufferSize(256 * 1024);

            NProto::TCreateSessionResponse response;
            response.MutableSession()->SetSessionId(CreateGuidAsString());
            Y_VERIFY(state.SerializeToString(response.MutableSession()->MutableSessionState()));

            return MakeFuture(response);
        };

        bootstrap.Start();

        auto handle = bootstrap.Fuse.SendRequest<TCreateHandleRequest>("/file1");
        UNIT_ASSERT_VALUES_EQUAL(handle.GetValue(WaitTimeout), handleId);
    }

    Y_UNIT_TEST(ShouldWaitToAcquireLock)
    {
        TBootstrap bootstrap;
        bootstrap.Start();

        ui32 called = 0;
        bootstrap.Service->AcquireLockHandler = [&] (auto) {
            if (++called < 3) {
                NProto::TAcquireLockResponse response = TErrorResponse(E_FS_WOULDBLOCK, "xxx");
                return MakeFuture(response);
            }

            NProto::TAcquireLockResponse response;
            return MakeFuture(response);
        };

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        auto result = bootstrap.Fuse.SendRequest<TAcquireLockRequest>(0, F_RDLCK);
        UNIT_ASSERT_NO_EXCEPTION(result.GetValue(WaitTimeout));
        UNIT_ASSERT_VALUES_EQUAL(called, 3);
    }

    Y_UNIT_TEST(ShouldNotWaitToAcquireLock)
    {
        TBootstrap bootstrap;
        bootstrap.Start();

        ui32 called = 0;
        bootstrap.Service->AcquireLockHandler = [&] (auto) {
            ++called;
            NProto::TAcquireLockResponse response = TErrorResponse(E_FS_WOULDBLOCK, "xxx");
            return MakeFuture(response);
        };

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        auto result = bootstrap.Fuse.SendRequest<TAcquireLockRequest>(0, F_RDLCK, false);
        UNIT_ASSERT(result.Wait(WaitTimeout));
        UNIT_ASSERT_VALUES_EQUAL(called, 1);
        UNIT_ASSERT(result.HasException());

        bootstrap.Service->AcquireLockHandler = [&] (auto) {
            ++called;
            NProto::TAcquireLockResponse response = TErrorResponse(E_FS_BADHANDLE, "xxx");
            return MakeFuture(response);
        };

        result = bootstrap.Fuse.SendRequest<TAcquireLockRequest>(0, F_RDLCK);
        UNIT_ASSERT(result.Wait(WaitTimeout));
        UNIT_ASSERT_VALUES_EQUAL(called, 2);
        UNIT_ASSERT(result.HasException());
    }

    Y_UNIT_TEST(ShouldTestLock)
    {
        TBootstrap bootstrap;
        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        bootstrap.Service->TestLockHandler = [&] (auto) {
            NProto::TTestLockResponse response;
            return MakeFuture(response);
        };

        auto result = bootstrap.Fuse.SendRequest<TTestLockRequest>(0, F_RDLCK, 0, 100);
        UNIT_ASSERT(result.Wait(WaitTimeout));
        auto lk = result.GetValue();
        UNIT_ASSERT_VALUES_EQUAL(lk.type, F_UNLCK);

        bootstrap.Service->TestLockHandler = [&] (auto) {
            NProto::TTestLockResponse response;
            return MakeFuture(response);
        };

        bootstrap.Service->TestLockHandler = [&] (auto) {
            NProto::TTestLockResponse response = TErrorResponse(E_FS_WOULDBLOCK, "");
            response.SetOwner(100);
            response.SetOffset(100500);
            response.SetLength(500100);

            return MakeFuture(response);
        };

        result = bootstrap.Fuse.SendRequest<TTestLockRequest>(0, F_RDLCK, 0, 100);
        UNIT_ASSERT(result.Wait(WaitTimeout));
        lk = result.GetValue();

        UNIT_ASSERT_VALUES_EQUAL(lk.type, F_WRLCK);
        UNIT_ASSERT_VALUES_EQUAL(lk.start, 100500);
        UNIT_ASSERT_VALUES_EQUAL(lk.end, 100500 + 500100 - 1);

        bootstrap.Service->TestLockHandler = [&] (auto) {
            NProto::TTestLockResponse response = TErrorResponse(E_FS_BADHANDLE, "");
            return MakeFuture(response);
        };

        result = bootstrap.Fuse.SendRequest<TTestLockRequest>(0, F_RDLCK, 0, 100);
        UNIT_ASSERT(result.Wait(WaitTimeout));
        UNIT_ASSERT_EXCEPTION(result.GetValue(), yexception);
    }

    Y_UNIT_TEST(ShouldNotFailOnStopWithRequestsInFlight)
    {
        NAtomic::TBool sessionDestroyed = false;

        TBootstrap bootstrap;
        bootstrap.Service->DestroySessionHandler = [&sessionDestroyed] (auto request) {
            Y_UNUSED(request);
            sessionDestroyed = true;
            return MakeFuture(NProto::TDestroySessionResponse());
        };

        NThreading::TPromise<NProto::TListNodesResponse> response = NewPromise<NProto::TListNodesResponse>();
        bootstrap.Service->ListNodesHandler = [&] (auto request) {
            Y_UNUSED(request);
            return response.GetFuture();
        };

        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        const ui64 nodeId = 123;

        auto handle = bootstrap.Fuse.SendRequest<TOpenDirRequest>(nodeId);
        UNIT_ASSERT(handle.Wait(WaitTimeout));
        auto handleId = handle.GetValue();

        auto read = bootstrap.Fuse.SendRequest<TReadDirRequest>(nodeId, handleId);
        UNIT_ASSERT(!read.Wait(WaitTimeout));

        auto stop = bootstrap.StopAsync();
        UNIT_ASSERT(stop.Wait(WaitTimeout));
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            read.GetValueSync(),
            yexception,
            "Interrupted system call");
    }

    Y_UNIT_TEST(ShouldNotAbortOnInvalidServerLookup)
    {
        TBootstrap bootstrap;
        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        bootstrap.Service->GetNodeAttrHandler = [] (auto) {
            NProto::TGetNodeAttrResponse response;
            // response.MutableNode()->SetId(123);
            return MakeFuture(response);
        };

        auto lookup = bootstrap.Fuse.SendRequest<TLookupRequest>("test");
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            lookup.GetValue(WaitTimeout),
            yexception,
            "Unknown error -5");
    }

    Y_UNIT_TEST(ShouldCacheXAttrValueOnGet)
    {
        TBootstrap bootstrap;
        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        int callCount = 0;
        bootstrap.Service->GetNodeXAttrHandler = [&callCount] (auto) {
            ++callCount;
            NProto::TGetNodeXAttrResponse response;
            response.SetValue("value");
            response.SetVersion(1);
            return MakeFuture(response);
        };

        {
            auto xattr = bootstrap.Fuse.SendRequest<TGetXAttrValueRequest>("name", 6);
            UNIT_ASSERT(xattr.Wait(WaitTimeout));
            UNIT_ASSERT_STRINGS_EQUAL("value", xattr.GetValue());
        }
        {
            auto xattr = bootstrap.Fuse.SendRequest<TGetXAttrValueRequest>("name", 6);
            UNIT_ASSERT(xattr.Wait(WaitTimeout));
            UNIT_ASSERT_STRINGS_EQUAL("value", xattr.GetValue());
            UNIT_ASSERT_EQUAL(1, callCount);
        }
    }

    Y_UNIT_TEST(ShouldCacheXAttrValueOnSet)
    {
        TBootstrap bootstrap;
        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        int callCount = 0;
        bootstrap.Service->GetNodeXAttrHandler = [&callCount] (auto) {
            ++callCount;
            NProto::TGetNodeXAttrResponse response;
            return MakeFuture(response);
        };
        bootstrap.Service->SetNodeXAttrHandler = [] (auto) {
            NProto::TSetNodeXAttrResponse response;
            response.SetVersion(1);
            return MakeFuture(response);
        };

        {
            auto set = bootstrap.Fuse.SendRequest<TSetXAttrValueRequest>("name", "value");
            UNIT_ASSERT(set.Wait(WaitTimeout));
        }
        {
            auto xattr = bootstrap.Fuse.SendRequest<TGetXAttrValueRequest>("name", 6);
            UNIT_ASSERT(xattr.Wait(WaitTimeout));
            UNIT_ASSERT_STRINGS_EQUAL("value", xattr.GetValue());
            UNIT_ASSERT_EQUAL(0, callCount);
        }
    }

    Y_UNIT_TEST(ShouldSkipCacheValueAfterTimeout)
    {
        std::shared_ptr<TTestTimer> timer = std::make_shared<TTestTimer>();
        TBootstrap bootstrap{timer};
        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        int callCount = 0;
        bootstrap.Service->GetNodeXAttrHandler = [&callCount] (auto) {
            ++callCount;
            NProto::TGetNodeXAttrResponse response;
            response.SetValue("value");
            response.SetVersion(callCount);
            return MakeFuture(response);
        };

        {
            auto xattr = bootstrap.Fuse.SendRequest<TGetXAttrValueRequest>("name", 6);
            UNIT_ASSERT(xattr.Wait(WaitTimeout));
            UNIT_ASSERT_STRINGS_EQUAL("value", xattr.GetValue());
        }

        timer->AdvanceTime(TDuration::Hours(1));

        {
            auto xattr = bootstrap.Fuse.SendRequest<TGetXAttrValueRequest>("name", 6);
            UNIT_ASSERT(xattr.Wait(WaitTimeout));
            UNIT_ASSERT_STRINGS_EQUAL("value", xattr.GetValue());
            UNIT_ASSERT_EQUAL(2, callCount);
        }
    }

    Y_UNIT_TEST(ShouldNotCacheXAttrWhenErrorHappens)
    {
        TBootstrap bootstrap;
        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        int callCount = 0;
        bootstrap.Service->GetNodeXAttrHandler = [&callCount] (auto) {
            NProto::TGetNodeXAttrResponse response;
            if (callCount == 0) {
                response.MutableError()->SetCode(MAKE_FILESTORE_ERROR(NProto::E_FS_XATTR2BIG));
            } else {
                response.SetValue("value");
                response.SetVersion(callCount);
            }
            ++callCount;
            return MakeFuture(response);
        };

        {
            auto xattr = bootstrap.Fuse.SendRequest<TGetXAttrValueRequest>("name", 6);
            UNIT_ASSERT(xattr.Wait(WaitTimeout));
            UNIT_ASSERT_EXCEPTION(xattr.GetValue(), yexception);
        }
        {
            auto xattr = bootstrap.Fuse.SendRequest<TGetXAttrValueRequest>("name", 6);
            UNIT_ASSERT(xattr.Wait(WaitTimeout));
            UNIT_ASSERT_STRINGS_EQUAL("value", xattr.GetValue());
            UNIT_ASSERT_EQUAL(2, callCount);
        }
    }

    Y_UNIT_TEST(ShouldCacheXAttrAbsence)
    {
        TBootstrap bootstrap;
        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        int callCount = 0;
        bootstrap.Service->GetNodeXAttrHandler = [&callCount] (auto) {
            NProto::TGetNodeXAttrResponse response;
            response.MutableError()->SetCode(MAKE_FILESTORE_ERROR(NProto::E_FS_NOXATTR));
            ++callCount;
            return MakeFuture(response);
        };

        {
            auto xattr = bootstrap.Fuse.SendRequest<TGetXAttrValueRequest>("name", 6);
            UNIT_ASSERT(xattr.Wait(WaitTimeout));
            UNIT_ASSERT_EXCEPTION_CONTAINS(
                xattr.GetValue(),
                yexception,
                "-61"); // NODATA error code
        }
        {
            auto xattr = bootstrap.Fuse.SendRequest<TGetXAttrValueRequest>("name", 6);
            UNIT_ASSERT(xattr.Wait(WaitTimeout));
            UNIT_ASSERT_EXCEPTION_CONTAINS(
                xattr.GetValue(),
                yexception,
                "-61"); // NODATA error code
            UNIT_ASSERT_EQUAL(1, callCount);
        }
    }

    Y_UNIT_TEST(SuspendShouldNotDestroySession)
    {
        TBootstrap bootstrap;
        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        bool sessionDestroyed = false;
        bootstrap.Service->DestroySessionHandler = [&] (auto) {
            sessionDestroyed = true;
            NProto::TDestroySessionResponse response;
            return MakeFuture(response);
        };

        bootstrap.Service->ResetSessionHandler = [&] (auto) {
            sessionDestroyed = true;
            NProto::TResetSessionResponse response;
            return MakeFuture(response);
        };

        auto future = bootstrap.Driver->SuspendAsync();
        UNIT_ASSERT_NO_EXCEPTION(future.GetValue(WaitTimeout));
        UNIT_ASSERT(!sessionDestroyed);

        // prevent stack use after scope on shutdown
        bootstrap.Service->ResetSessionHandler = [] (auto) {
            return MakeFuture(NProto::TResetSessionResponse{});
        };

        bootstrap.Service->DestroySessionHandler = [] (auto) {
            return MakeFuture(NProto::TDestroySessionResponse());
        };
    }

    Y_UNIT_TEST(StopShouldDestroySession)
    {
        bool sessionReset = false;
        bool sessionDestroyed = false;

        TBootstrap bootstrap;
        bootstrap.Service->ResetSessionHandler = [&] (auto) {
            sessionReset = true;
            NProto::TResetSessionResponse response;
            return MakeFuture(response);
        };
        bootstrap.Service->DestroySessionHandler = [&] (auto) {
            sessionDestroyed = true;
            NProto::TDestroySessionResponse response;
            return MakeFuture(response);
        };

        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        auto future = bootstrap.Driver->StopAsync();
        UNIT_ASSERT_NO_EXCEPTION(future.GetValue(WaitTimeout));
        UNIT_ASSERT(sessionReset);
        UNIT_ASSERT(sessionDestroyed);

        // prevent stack use after scope on shutdown
        bootstrap.Service->ResetSessionHandler = [] (auto) {
            return MakeFuture(NProto::TResetSessionResponse{});
        };

        bootstrap.Service->DestroySessionHandler = [] (auto) {
            return MakeFuture(NProto::TDestroySessionResponse());
        };
    }

    Y_UNIT_TEST(ShouldHitErrorMetricOnFailure)
    {
        TBootstrap bootstrap;

        bootstrap.Service->CreateSessionHandler = [&] (auto) {
            return MakeFuture(NProto::TCreateSessionResponse{});
        };
        bootstrap.Service->CreateHandleHandler = [&] (auto) {
            NProto::TCreateHandleResponse result = TErrorResponse(E_FS_EXIST, "");
            return MakeFuture(result);
        };

        bootstrap.Start();

        auto init = bootstrap.Fuse.SendRequest<TInitRequest>();
        UNIT_ASSERT_NO_EXCEPTION(init.GetValue(WaitTimeout));

        auto future = bootstrap.Fuse.SendRequest<TCreateHandleRequest>("/file1");
        UNIT_ASSERT_EXCEPTION(future.GetValueSync(), yexception);
        bootstrap.Stop(); // wait till all requests are done writing their stats

        auto counters = bootstrap.Counters
            ->FindSubgroup("component", "fs_ut")
            ->FindSubgroup("request", "CreateHandle");
        UNIT_ASSERT_EQUAL(1, counters->GetCounter("Errors")->GetAtomic());
        UNIT_ASSERT_EQUAL(0, counters->GetCounter("Errors/Fatal")->GetAtomic());
    }
}

}   // namespace NCloud::NFileStore::NFuse
