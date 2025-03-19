#include "service.h"

#include "config.h"

#include <cloud/filestore/libs/client/config.h>
#include <cloud/filestore/libs/client/session.h>
#include <cloud/filestore/libs/service/filestore_test.h>
#include <cloud/filestore/libs/service/request.h>

#include <cloud/storage/core/libs/common/scheduler_test.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/datetime/base.h>
#include <util/generic/scope.h>
#include <util/generic/string.h>

#include <sys/stat.h>

namespace NCloud::NFileStore::NGateway {

using namespace NClient;
using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

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
    std::shared_ptr<TFileStoreService> Service;

    TBootstrap()
    {
        Logging = CreateLoggingService("console", { TLOG_RESOURCES });
        Timer = CreateWallClockTimer();
        Scheduler = std::make_shared<TTestScheduler>();

        FileStore = std::make_shared<TFileStoreTest>();

        FileStore->CreateSessionHandler = [] (
            std::shared_ptr<NProto::TCreateSessionRequest> request)
        {
            UNIT_ASSERT_EQUAL(GetFileSystemId(*request), FileSystemId);
            UNIT_ASSERT_EQUAL(GetClientId(*request), ClientId);

            NProto::TCreateSessionResponse response;
            auto* session = response.MutableSession();
            session->SetSessionId(SessionId);

            return MakeFuture(response);
        };

        FileStore->DestroySessionHandler = [] (
            std::shared_ptr<NProto::TDestroySessionRequest> request)
        {
            UNIT_ASSERT_EQUAL(GetSessionId(*request), SessionId);
            return MakeFuture(NProto::TDestroySessionResponse());
        };

        FileStore->PingSessionHandler = [] (auto request) {
            UNIT_ASSERT_EQUAL(GetSessionId(*request), SessionId);
            return MakeFuture(NProto::TPingSessionResponse());
        };

        Session = CreateSession(
            Logging,
            Timer,
            Scheduler,
            FileStore,
            CreateSessionConfig());

        Service = std::make_shared<TFileStoreService>(
            Logging,
            Session,
            CreateServiceConfig());
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

        if (FileStore) {
            FileStore->Start();
        }

        if (Service) {
            Service->Start();
        }
    }

    void Stop()
    {
        if (Service) {
            Service->Stop();
        }

        if (FileStore) {
            FileStore->Stop();
        }

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

    static TFileStoreServiceConfigPtr CreateServiceConfig()
    {
        NProto::TNfsGatewayConfig config;
        // TODO
        return std::make_shared<TFileStoreServiceConfig>(config);
    }
};

////////////////////////////////////////////////////////////////////////////////

class TReadDirCallback
    : public yfs_readdir_cb
{
    using TCallbackFn = std::function<int (
        const char *name,
        const struct stat *attr,
        uint64_t offset)>;

private:
    TCallbackFn Fn;

public:
    TReadDirCallback(TCallbackFn fn)
        : Fn(std::move(fn))
    {
        invoke = [] (
            struct yfs_readdir_cb *cb,
            const char *name,
            const struct stat *attr,
            uint64_t offset)
        {
            return static_cast<TReadDirCallback*>(cb)->Fn(name, attr, offset);
        };
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TDirEntry
{
    TString Name;
    NProto::TNodeAttr Attrs;
};

std::pair<ui32, ui32> ParseMode(ui32 mode)
{
    ui32 type = NProto::E_INVALID_NODE;
    if (S_ISREG(mode)) {
        type = NProto::E_REGULAR_NODE;
    } else if (S_ISDIR(mode)) {
        type = NProto::E_DIRECTORY_NODE;
    } else if (S_ISLNK(mode)) {
        type = NProto::E_LINK_NODE;
    }
    return { type, mode & ~S_IFMT };
}

TDirEntry DirEntry(TString name, ui64 ino, ui32 type, ui32 mode)
{
    TDirEntry entry;
    entry.Name = std::move(name);
    entry.Attrs.SetId(ino);
    entry.Attrs.SetType(type);
    entry.Attrs.SetMode(mode);
    return entry;
}

NProto::TListNodesResponse ListNodeResponse(
    const TVector<TDirEntry>& entries,
    const TString& offset = {})
{
    NProto::TListNodesResponse response;
    for (const auto& entry: entries) {
        *response.AddNames() = entry.Name;
        response.AddNodes()->CopyFrom(entry.Attrs);
    }

    response.SetCookie(offset);
    return response;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

Y_UNIT_TEST_SUITE(TFileStoreServiceTest)
{
    void TestReadDir(const TVector<NProto::TListNodesResponse>& responses)
    {
        TBootstrap bootstrap;
        bootstrap.Start();

        static constexpr ui64 DirNodeId = 1;

        size_t entriesCount = 0;
        for (const auto& response: responses) {
            entriesCount += response.NodesSize();
        }

        bootstrap.FileStore->ListNodesHandler =
            [&] (std::shared_ptr<NProto::TListNodesRequest> request) {
                UNIT_ASSERT_EQUAL(request->GetNodeId(), DirNodeId);

                size_t index = FromStringWithDefault<ui32>(request->GetCookie(), 0);
                if (index < responses.size()) {
                    return MakeFuture(responses[index]);
                }

                return MakeFuture(ErrorResponse<NProto::TListNodesResponse>(
                    S_FALSE, "no more entries"));
            };

        void* handle = nullptr;
        int retval = bootstrap.Service->OpenDir(DirNodeId, &handle);
        UNIT_ASSERT(retval == 0);

        Y_DEFER {
            int retval = bootstrap.Service->ReleaseDir(handle);
            UNIT_ASSERT(retval == 0);
        };

        TVector<TDirEntry> entries;
        TReadDirCallback callback([&] (
            const char *name,
            const struct stat *attr,
            uint64_t offset)
        {
            Y_UNUSED(offset);

            auto [type, mode] = ParseMode(attr->st_mode);
            entries.push_back(DirEntry(name, attr->st_ino, type, mode));

            return 0;
        });

        retval = bootstrap.Service->ReadDir(handle, &callback, 0);
        UNIT_ASSERT(retval > 0);
        UNIT_ASSERT_EQUAL(size_t(retval), entriesCount);

        UNIT_ASSERT_EQUAL(entries.size(), entriesCount);
    }

    Y_UNIT_TEST(ShouldReadDirSinglePage)
    {
        TestReadDir({
            ListNodeResponse({
                DirEntry("file1", 100, NProto::E_REGULAR_NODE, 0777),
                DirEntry("dir1", 101, NProto::E_DIRECTORY_NODE, 0777),
                DirEntry("link1", 102, NProto::E_LINK_NODE, 0777),
            }),
        });
    }

    Y_UNIT_TEST(ShouldReadDirMultiPage)
    {
        TestReadDir({
            ListNodeResponse({
                DirEntry("file1", 100, NProto::E_REGULAR_NODE, 0777),
                DirEntry("dir1", 101, NProto::E_DIRECTORY_NODE, 0777),
                DirEntry("link1", 102, NProto::E_LINK_NODE, 0777),
            }, "1"),
            ListNodeResponse({
                DirEntry("file2", 103, NProto::E_REGULAR_NODE, 0777),
                DirEntry("dir2", 104, NProto::E_DIRECTORY_NODE, 0777),
                DirEntry("link2", 105, NProto::E_LINK_NODE, 0777),
            }, "2"),
            ListNodeResponse({
                DirEntry("file3", 106, NProto::E_REGULAR_NODE, 0777),
                DirEntry("dir3", 107, NProto::E_DIRECTORY_NODE, 0777),
                DirEntry("link3", 108, NProto::E_LINK_NODE, 0777),
            }, "3"),
        });
    }
}

}   // namespace NCloud::NFileStore::NGateway
