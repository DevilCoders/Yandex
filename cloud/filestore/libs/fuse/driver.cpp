#include "driver.h"

#include "config.h"
#include "convert.h"
#include "fs.h"
#include "fuse.h"
#include "probes.h"

#include <cloud/filestore/libs/client/session.h>
#include <cloud/filestore/libs/diagnostics/request_stats.h>
#include <cloud/filestore/libs/fuse/protos/session.pb.h>
#include <cloud/filestore/libs/service/request.h>

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/media.h>
#include <cloud/storage/core/libs/common/thread.h>
#include <cloud/storage/core/libs/diagnostics/incomplete_requests.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <library/cpp/threading/atomic/bool.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/system/info.h>
#include <util/system/rwlock.h>
#include <util/system/thread.h>

#include <errno.h>
#include <pthread.h>

namespace NCloud::NFileStore::NFuse {

LWTRACE_USING(FILESTORE_FUSE_PROVIDER);

using namespace NThreading;
using namespace NCloud::NFileStore::NClient;

namespace {

////////////////////////////////////////////////////////////////////////////////

void CancelRequest(
    TLog& log,
    IRequestStats& requestStats,
    TCallContext& callContext,
    fuse_req_t req,
    enum fuse_cancelation_code code)
{
    FILESTORE_TRACK(RequestCompleted, (&callContext), "Cancel");
    requestStats.ResponseSent(callContext);
    fuse_cancel_request(req, code);
    requestStats.RequestCompleted(log, callContext);
}

////////////////////////////////////////////////////////////////////////////////

class TCompletionQueue final
    : public ICompletionQueue
    , public IIncompleteRequestProvider
{
private:
    const IRequestStatsPtr RequestStats;
    TLog Log;
    const NProto::EStorageMediaKind RequestMediaKind;

    enum fuse_cancelation_code CancelCode;
    NAtomic::TBool ShouldStop = false;
    TAtomicCounter CompletingCount = {0};
    TPromise<void> StopPromise = NewPromise();

    TMutex RequestsLock;
    THashMap<fuse_req_t, TCallContextPtr> Requests;

public:
    TCompletionQueue(
            IRequestStatsPtr stats,
            TLog& log,
            NProto::EStorageMediaKind requestMediaKind)
        : RequestStats(std::move(stats))
        , Log(log)
        , RequestMediaKind(requestMediaKind)
    {
        Y_UNUSED(Log);
    }

    TMaybe<enum fuse_cancelation_code> Enqueue(fuse_req_t req, TCallContextPtr context)
    {
        TGuard g{RequestsLock};

        if (ShouldStop) {
            return CancelCode;
        }

        Requests[req] = context;
        return Nothing();
    }

    int Complete(fuse_req_t req, TCompletionCallback cb) noexcept override
    {
        with_lock (RequestsLock) {
            if (!Requests.erase(req)) {
                return 0;
            }
            CompletingCount.Add(1);
        }

        int ret = cb(req);
        if (CompletingCount.Dec() == 0 && ShouldStop) {
            StopPromise.TrySetValue();
        }
        return ret;
    }

    TFuture<void> Stop(enum fuse_cancelation_code code) {
        CancelCode = code;
        ShouldStop = true;

        TGuard g{RequestsLock};
        for (auto&& [req, context] : Requests) {
            CancelRequest(Log, *RequestStats, *context, req, CancelCode);
        }
        Requests.clear();

        if (CompletingCount.Val() == 0) {
            StopPromise.TrySetValue();
        }

        return StopPromise.GetFuture();
    }

    TVector<TIncompleteRequest> GetIncompleteRequests() override
    {
        auto now = GetCycleCount();

        TGuard g{RequestsLock};
        TVector<TIncompleteRequest> requests(Reserve(Requests.size()));
        for (auto&& [_, context]: Requests) {
            ui64 startedCycles = context->GetStartedCycles();
            if (startedCycles > 0 && now > startedCycles) {
                requests.push_back(TIncompleteRequest{
                    .MediaKind = RequestMediaKind,
                    .RequestType = static_cast<ui32>(context->RequestType),
                    .RequestTime = CyclesToDurationSafe(now - startedCycles)
                });
            }
        }

        return requests;
    }

};

////////////////////////////////////////////////////////////////////////////////

class TArgs
{
private:
    fuse_args Args = FUSE_ARGS_INIT(0, nullptr);

public:
    TArgs(const TFuseConfig& config)
    {
        AddArg(""); // fuse_opt_parse starts with 1

        if (config.GetDebug()) {
            AddArg("-odebug");
        }

#if defined(FUSE_VIRTIO)
        if (auto path = config.GetSocketPath()) {
            AddArg("--socket-path=" + path);
        }

        // HIPRIO + number of requests queues
        ui32 queues = Max(2u, config.GetVhostQueuesCount());
        AddArg("--thread-pool-size=" + ToString(queues));
#else
        if (config.GetReadOnly()) {
            AddArg("-oro");
        }
#endif
    }

    ~TArgs()
    {
        fuse_opt_free_args(&Args);
    }

    void AddArg(const TString& arg)
    {
        fuse_opt_add_arg(&Args, arg.c_str());
    }

    operator fuse_args* ()
    {
        return &Args;
    }
};

#if defined(FUSE_VIRTIO)

////////////////////////////////////////////////////////////////////////////////
// virtio-fs version

class TSession
{
private:
    TArgs Args;

    fuse_session* Session = nullptr;

public:
    TSession(
            const TFuseConfig& config,
            const fuse_lowlevel_ops& ops,
            const TString& state,
            void* context)
        : Args(config)
    {
        Session = fuse_session_new(Args, &ops, sizeof(ops), context);
        Y_ENSURE(Session, "fuse_session_new() failed");

        TryLoadSessionState(state);

        int error = fuse_session_mount(Session);
        Y_ENSURE(!error, "fuse_session_mount() failed");
    }

    void TryLoadSessionState(const TString& state)
    {
        NProto::TFuseSessionState proto;
        if (!state || !proto.ParseFromString(state)) {
            return;
        }

        // FIXME: sanity check
        Y_VERIFY(proto.GetProtoMajor());
        Y_VERIFY(proto.GetBufferSize());

        fuse_session_params params = {
            proto.GetProtoMajor(),
            proto.GetProtoMinor(),
            proto.GetCapable(),
            proto.GetWant(),
            proto.GetBufferSize(),
        };

        fuse_session_setparams(Session, &params);
    }

    void Exit()
    {
        if (Session) {
            fuse_session_exit(Session);
        }
    }

    void Unmount()
    {
        if (Session) {
            fuse_session_unmount(Session);
            fuse_session_destroy(Session);
            Session = nullptr;
        }
    }

    operator fuse_session* ()
    {
        return Session;
    }

    TString Dump() const
    {
        Y_VERIFY(Session);

        fuse_session_params params;
        fuse_session_getparams(Session, &params);

        NProto::TFuseSessionState session;
        session.SetProtoMajor(params.proto_major);
        session.SetProtoMinor(params.proto_minor);
        session.SetCapable(params.capable);
        session.SetWant(params.want);
        session.SetBufferSize(params.bufsize);

        TString result;
        Y_PROTOBUF_SUPPRESS_NODISCARD session.SerializeToString(&result);

        return result;
    }
};

#else

////////////////////////////////////////////////////////////////////////////////
// regular FUSE version

class TSession
{
private:
    TArgs Args;
    TString MountPath;

    fuse_chan* Channel = nullptr;
    fuse_session* Session = nullptr;

public:
    TSession(
            const TFuseConfig& config,
            const fuse_lowlevel_ops& ops,
            const TString& state,
            void* context)
        : Args(config)
        , MountPath(config.GetMountPath())
    {
        Y_UNUSED(state);

        Channel = fuse_mount(MountPath.c_str(), Args);
        Y_ENSURE(Channel, "fuse_mount() failed");

        Session = fuse_lowlevel_new(Args, &ops, sizeof(ops), context);
        Y_ENSURE(Session, "fuse_lowlevel_new() failed");

        fuse_session_add_chan(Session, Channel);
    }

    void Exit()
    {
        fuse_session_exit(*this);
    }

    void Unmount()
    {
        if (Session) {
            fuse_session_remove_chan(Channel);
            fuse_session_destroy(Session);
            fuse_unmount(MountPath.c_str(), Channel);
            Session = nullptr;
        }
    }

    operator fuse_session* ()
    {
        return Session;
    }

    TString Dump() const
    {
        return {};
    }
};

#endif

////////////////////////////////////////////////////////////////////////////////

class TSessionThread final
    : public ISimpleThread
{
private:
    TLog Log;
    TSession Session;

    pthread_t ThreadId = 0;

public:
    TSessionThread(
            TLog log,
            const TFuseConfig& config,
            const fuse_lowlevel_ops& ops,
            const TString& state,
            void* context)
        : Log(std::move(log))
        , Session(config, ops, state, context)
    {}

    void Start()
    {
        ISimpleThread::Start();
    }

    void StopThread()
    {
        STORAGE_INFO("stopping FUSE loop");

        Session.Exit();
        if (auto threadId = AtomicGet(ThreadId)) {
            // session loop may get stuck on sem_wait/read.
            // Interrupt it by sending the thread a signal.
            pthread_kill(threadId, SIGUSR1);
        }

        Join();
    }

    void Unmount()
    {
        StopThread();

        STORAGE_INFO("unmounting FUSE session");
        Session.Unmount();
    }

    const TSession& GetSession() const
    {
        return Session;
    }

private:
    void* ThreadProc() override
    {
        STORAGE_INFO("starting FUSE loop");

        ::NCloud::SetCurrentThreadName("FUSE");

        AtomicSet(ThreadId, pthread_self());
        fuse_session_loop(Session);

        return nullptr;
    }
};

////////////////////////////////////////////////////////////////////////////////

class TFileSystemDriver final
    : public IFileSystemDriver
{
private:
    const TFuseConfigPtr Config;
    const IRequestStatsRegistryPtr StatsRegistry;
    const ISessionPtr Session;
    const IFileSystemFactoryPtr FileSystemFactory;

    TLog Log;

    TString SessionState;
    std::unique_ptr<TSessionThread> SessionThread;

    std::shared_ptr<TCompletionQueue> CompletionQueue;
    IRequestStatsPtr RequestStats;
    IFileSystemPtr FileSystem;

public:
    TFileSystemDriver(
            TFuseConfigPtr config,
            ILoggingServicePtr logging,
            IRequestStatsRegistryPtr statsRegistry,
            ISessionPtr session,
            IFileSystemFactoryPtr fileSystemFactory)
        : Config(std::move(config))
        , StatsRegistry(std::move(statsRegistry))
        , Session(std::move(session))
        , FileSystemFactory(std::move(fileSystemFactory))
    {
        Log = logging->CreateLog("FUSE");
    }

    TFuture<NProto::TError> StartAsync() override
    {
        return Session->CreateSession().Apply(
            [=] (const TFuture<NProto::TCreateSessionResponse>& future) {
                return StartWithSessionState(future);
            });
    }

    TFuture<void> StopAsync() override
    {
        StatsRegistry->Unregister(
            Config->GetFileSystemId(),
            Config->GetClientId());

        if (!CompletionQueue) {
            return MakeFuture();
        }

        auto queue = CompletionQueue->Stop(FUSE_ERROR);
        auto thread = queue.Apply(
            [=] (const TFuture<void>&) {
                if (SessionThread) {
                    SessionThread->Unmount();
                }
        });

        return thread.Apply(
            [=] (const TFuture<void>&) {
                return Session->DestroySession().IgnoreResult();
            });
    }

    TFuture<void> SuspendAsync() override
    {
        if (!CompletionQueue) {
            return MakeFuture();
        }

        auto queue = CompletionQueue->Stop(FUSE_SUSPEND);
        return queue.Apply(
            [=] (const TFuture<void>&) {
                if (SessionThread) {
                    // just stop loop, leave connection
                    SessionThread->StopThread();
                }
        });
    }

    TVector<TIncompleteRequest> GetIncompleteRequests() override
    {
        return CompletionQueue->GetIncompleteRequests();
    }

private:
    NProto::TError StartWithSessionState(
        const TFuture<NProto::TCreateSessionResponse>& future)
    {
        NProto::TError error;

        try {
            auto response = future.GetValue();
            if (HasError(response)) {
                STORAGE_ERROR("[f:%s][c:%s] failed to create session: %s",
                    Config->GetFileSystemId().Quote().c_str(),
                    Config->GetClientId().Quote().c_str(),
                    FormatError(response.GetError()).c_str());

                return response.GetError();
            }

            const auto& filestore = response.GetFileStore();
            ui64 rawMedia = filestore.GetStorageMediaKind();
            if (!NProto::EStorageMediaKind_IsValid(rawMedia)) {
                STORAGE_WARN("[f:%s][c:%s] got unsupported media kind %lu",
                    Config->GetFileSystemId().Quote().c_str(),
                    Config->GetClientId().Quote().c_str(),
                    rawMedia);
            }

            NProto::EStorageMediaKind media = NProto::STORAGE_MEDIA_SSD;
            switch (rawMedia) {
                case NProto::STORAGE_MEDIA_SSD:
                    media = NProto::STORAGE_MEDIA_SSD;
                    break;
                default:
                    media = NProto::STORAGE_MEDIA_HDD;
                    break;
            }

            RequestStats = StatsRegistry->GetFileSystemStats(
                Config->GetFileSystemId(),
                Config->GetClientId(),
                media);
            CompletionQueue = std::make_shared<TCompletionQueue>(
                RequestStats,
                Log,
                media);
            FileSystem = FileSystemFactory->CreateFileSystem(
                MakeFileSystemConfig(filestore),
                Session,
                RequestStats,
                CompletionQueue);

            SessionState = response.GetSession().GetSessionState();
            fuse_lowlevel_ops ops = {};
            InitOps(ops);

            STORAGE_INFO("[f:%s][c:%s] starting %s session",
                Config->GetFileSystemId().Quote().c_str(),
                Config->GetClientId().Quote().c_str(),
                SessionState.empty() ? "new" : "existing");

            SessionThread = std::make_unique<TSessionThread>(
                Log,
                *Config,
                ops,
                SessionState,
                this);

            SessionThread->Start();
        } catch (const TServiceError& e) {
            error = MakeError(e.GetCode(), TString(e.GetMessage()));

            STORAGE_ERROR("[f:%s][c:%s] failed to start: %s",
                Config->GetFileSystemId().Quote().c_str(),
                Config->GetClientId().Quote().c_str(),
                FormatError(error).c_str());
        } catch (...) {
            error = MakeError(E_FAIL, CurrentExceptionMessage());
            STORAGE_ERROR("[f:%s][c:%s] failed to start: %s",
                Config->GetFileSystemId().Quote().c_str(),
                Config->GetClientId().Quote().c_str(),
                FormatError(error).c_str());
        }

        return error;
    }

    TFileSystemConfigPtr MakeFileSystemConfig(const NProto::TFileStore& filestore)
    {
        NProto::TFileSystemConfig config;
        config.SetFileSystemId(filestore.GetFileSystemId());
        config.SetBlockSize(filestore.GetBlockSize());

        return std::make_shared<TFileSystemConfig>(config);
    }

    void Init(fuse_conn_info* conn)
    {
        // Cap the number of scatter-gather segments per request to virtqueue
        // size, taking FUSE own headers into account (see FUSE_HEADER_OVERHEAD
        // in linux:fs/fuse/virtio_fs.c)
        const size_t maxPages = Config->GetFuseMaxWritePages();

        // libfuse doesn't allow to configure max_pages directly but infers it
        // from fuse_conn_info.max_write, dividing it by page size.
        // see CLOUD-75329 for details
        const size_t maxWrite = NSystemInfo::GetPageSize() * maxPages;
        if (maxWrite < conn->max_write) {
            STORAGE_DEBUG("[f:%s][c:%s] setting max write pages %u -> %lu",
                Config->GetFileSystemId().Quote().c_str(),
                Config->GetClientId().Quote().c_str(),
                conn->max_write,
                maxWrite);
            conn->max_write = maxWrite;
        }

        // in case of newly mount we should drop any prev state
        // e.g. left from a crash or smth, paranoid mode
        ResetSessionState(SessionThread->GetSession().Dump());
    }

    void Destroy()
    {
        STORAGE_INFO("[f:%s][c:%s] got destroy request",
            Config->GetFileSystemId().Quote().c_str(),
            Config->GetClientId().Quote().c_str());
        // in case of unmount we should cleanup everything
        ResetSessionState({});
    }

    void ResetSessionState(const TString& state)
    {
        STORAGE_INFO("[f:%s][c:%s] resetting session state",
            Config->GetFileSystemId().Quote().c_str(),
            Config->GetClientId().Quote().c_str());

        SessionState = state;

        auto callContext = MakeIntrusive<TCallContext>(CreateRequestId());
        auto request = std::make_shared<NProto::TResetSessionRequest>();
        request->SetSessionState(state);

        auto response = Session->ResetSession(
            std::move(callContext),
            std::move(request));

        // TODO: CRIT EVENT, though no way to interrupt mount
        auto result = response.GetValueSync();
        STORAGE_INFO("[f:%s][c:%s] session reset completed: %s",
            Config->GetFileSystemId().Quote().c_str(),
            Config->GetClientId().Quote().c_str(),
            FormatError(result.GetError()).c_str());

        FileSystem->Reset();
    }

    template <typename Method, typename... Args>
    static void Call(
        Method&& m,
        const char* name,
        EFileStoreRequest requestType,
        ui32 requestSize,
        fuse_req_t req,
        Args&&... args) noexcept
    {
        auto* pThis = static_cast<TFileSystemDriver*>(fuse_req_userdata(req));
        auto& Log = pThis->Log;

        auto callContext = MakeIntrusive<TCallContext>(fuse_req_unique(req));
        callContext->RequestType = requestType;
        callContext->RequestSize = requestSize;

        if (auto cancelCode = pThis->CompletionQueue->Enqueue(req, callContext)) {
            STORAGE_DEBUG("driver is stopping, cancel request");
            CancelRequest(
                pThis->Log,
                *pThis->RequestStats,
                *callContext,
                req,
                *cancelCode);
            return;
        }

        FILESTORE_TRACK(RequestReceived, callContext, name);
        pThis->RequestStats->RequestStarted(Log, *callContext);

        try {
            auto* fs = pThis->FileSystem.get();
            (fs->*m)(callContext, req, std::forward<Args>(args)...);
        } catch (const TServiceError& e) {
            STORAGE_ERROR("unexpected error: "
                << FormatResultCode(e.GetCode()) << " " << e.GetMessage());
            ReplyError(
                pThis->Log,
                *pThis->RequestStats,
                *callContext,
                req,
                ErrnoFromError(e.GetCode()));
        } catch (...) {
            STORAGE_ERROR("unexpected error: " << CurrentExceptionMessage());
            ReplyError(
                pThis->Log,
                *pThis->RequestStats,
                *callContext,
                req,
                EIO);
        }
    }

    static void InitOps(fuse_lowlevel_ops& ops)
    {
#define CALL(m, requestType, requestSize, req, ...)                            \
    TFileSystemDriver::Call(                                                   \
        &IFileSystem::m,                                                       \
        #m,                                                                    \
        requestType,                                                           \
        requestSize,                                                           \
        req,                                                                   \
        __VA_ARGS__)                                                           \
// CALL

        //
        // Initialization
        //

        ops.init = [] (void* userdata, fuse_conn_info* conn) {
            static_cast<TFileSystemDriver*>(userdata)->Init(conn);
        };
        ops.destroy = [] (void* userdata) {
            static_cast<TFileSystemDriver*>(userdata)->Destroy();
        };

        //
        // Filesystem information
        //

        ops.statfs = [] (fuse_req_t req, fuse_ino_t ino) {
            CALL(StatFs, EFileStoreRequest::GetFileStoreInfo, 0, req, ino);
        };

        //
        // Nodes
        //

        ops.lookup = [] (fuse_req_t req, fuse_ino_t parent, const char* name) {
            CALL(Lookup, EFileStoreRequest::GetNodeAttr, 0, req, parent, name);
        };
        ops.forget = [] (fuse_req_t req, fuse_ino_t ino, unsigned long nlookup) {
            CALL(Forget, EFileStoreRequest::MAX, 0, req, ino, nlookup);
        };
        ops.forget_multi = [] (fuse_req_t req, size_t count, fuse_forget_data* forgets) {
            CALL(ForgetMulti, EFileStoreRequest::MAX, 0, req, count, forgets);
        };
        ops.mkdir = [] (fuse_req_t req, fuse_ino_t parent, const char* name, mode_t mode) {
            CALL(MkDir, EFileStoreRequest::CreateNode, 0, req, parent, name, mode);
        };
        ops.rmdir = [] (fuse_req_t req, fuse_ino_t parent, const char* name) {
            CALL(RmDir, EFileStoreRequest::UnlinkNode, 0, req, parent, name);
        };
        ops.mknod = [] (fuse_req_t req, fuse_ino_t parent, const char* name, mode_t mode, dev_t rdev) {
            CALL(MkNode, EFileStoreRequest::CreateNode, 0, req, parent, name, mode, rdev);
        };
        ops.unlink = [] (fuse_req_t req, fuse_ino_t parent, const char* name) {
            CALL(Unlink, EFileStoreRequest::UnlinkNode, 0, req, parent, name);
        };
#if defined(FUSE_VIRTIO)
        ops.rename = [] (fuse_req_t req, fuse_ino_t parent, const char* name, fuse_ino_t newparent, const char* newname, uint32_t flags) {
            CALL(Rename, EFileStoreRequest::RenameNode, 0, req, parent, name, newparent, newname, flags);
        };
#else
        ops.rename = [] (fuse_req_t req, fuse_ino_t parent, const char* name, fuse_ino_t newparent, const char* newname) {
            CALL(Rename, EFileStoreRequest::RenameNode, 0, req, parent, name, newparent, newname, 0);
        };
#endif
        ops.symlink = [] (fuse_req_t req, const char* link, fuse_ino_t parent, const char* name) {
            CALL(SymLink, EFileStoreRequest::CreateNode, 0, req, link, parent, name);
        };
        ops.link = [] (fuse_req_t req, fuse_ino_t ino, fuse_ino_t newparent, const char* newname) {
            CALL(Link, EFileStoreRequest::CreateNode, 0, req, ino, newparent, newname);
        };
        ops.readlink = [] (fuse_req_t req, fuse_ino_t ino) {
            CALL(ReadLink, EFileStoreRequest::ReadLink, 0, req, ino);
        };

        //
        // Node attributes
        //

        ops.setattr = [] (fuse_req_t req, fuse_ino_t ino, struct stat* attr, int to_set, fuse_file_info* fi) {
            CALL(SetAttr, EFileStoreRequest::SetNodeAttr, 0, req, ino, attr, to_set, fi);
        };
        ops.getattr = [] (fuse_req_t req, fuse_ino_t ino, fuse_file_info* fi) {
            CALL(GetAttr, EFileStoreRequest::GetNodeAttr, 0, req, ino, fi);
        };
        ops.access = [] (fuse_req_t req, fuse_ino_t ino, int mask) {
            CALL(Access, EFileStoreRequest::AccessNode, 0, req, ino, mask);
        };

        //
        // Extended node attributes
        //

        ops.setxattr = [] (fuse_req_t req, fuse_ino_t ino, const char* name, const char* value, size_t size, int flags) {
            CALL(SetXAttr, EFileStoreRequest::SetNodeXAttr, 0, req, ino, name, TString{value, size}, flags);
        };
        ops.getxattr = [] (fuse_req_t req, fuse_ino_t ino, const char* name, size_t size) {
            CALL(GetXAttr, EFileStoreRequest::GetNodeXAttr, 0, req, ino, name, size);
        };
        ops.listxattr = [] (fuse_req_t req, fuse_ino_t ino, size_t size) {
            CALL(ListXAttr, EFileStoreRequest::ListNodeXAttr, 0, req, ino, size);
        };
        ops.removexattr = [] (fuse_req_t req, fuse_ino_t ino, const char* name) {
            CALL(RemoveXAttr, EFileStoreRequest::RemoveNodeXAttr, 0, req, ino, name);
        };

        //
        // Directory listing
        //

        ops.opendir = [] (fuse_req_t req, fuse_ino_t ino, fuse_file_info* fi) {
            CALL(OpenDir, EFileStoreRequest::MAX, 0, req, ino, fi);
        };
#if defined(FUSE_VIRTIO)
        ops.readdirplus = [] (fuse_req_t req, fuse_ino_t ino, size_t size, off_t offset, fuse_file_info* fi) {
            CALL(ReadDir, EFileStoreRequest::ListNodes, 0, req, ino, size, offset, fi);
        };
#else
        ops.readdir = [] (fuse_req_t req, fuse_ino_t ino, size_t size, off_t offset, fuse_file_info* fi) {
            CALL(ReadDir, EFileStoreRequest::ListNodes, 0, req, ino, size, offset, fi);
        };
#endif
        ops.releasedir = [] (fuse_req_t req, fuse_ino_t ino, fuse_file_info* fi) {
            CALL(ReleaseDir, EFileStoreRequest::MAX, 0, req, ino, fi);
        };

        //
        // Read  write files
        //

        ops.create = [] (fuse_req_t req, fuse_ino_t parent, const char* name, mode_t mode, fuse_file_info* fi) {
            CALL(Create, EFileStoreRequest::CreateHandle, 0, req, parent, name, mode, fi);
        };
        ops.open = [] (fuse_req_t req, fuse_ino_t ino, fuse_file_info* fi) {
            CALL(Open, EFileStoreRequest::CreateHandle, 0, req, ino, fi);
        };
        ops.read = [] (fuse_req_t req, fuse_ino_t ino, size_t size, off_t offset, fuse_file_info* fi) {
            CALL(Read, EFileStoreRequest::ReadData, size, req, ino, size, offset, fi);
        };
        ops.write = [] (fuse_req_t req, fuse_ino_t ino, const char* buf, size_t size, off_t offset, fuse_file_info* fi) {
            CALL(Write, EFileStoreRequest::WriteData, size, req, ino, TStringBuf{buf, size}, offset, fi);
        };
        ops.write_buf = [] (fuse_req_t req, fuse_ino_t ino, fuse_bufvec* bufv, off_t offset, fuse_file_info* fi) {
            CALL(WriteBuf, EFileStoreRequest::WriteData, fuse_buf_size(bufv), req, ino, bufv, offset, fi);
        };
        ops.fallocate = [] (fuse_req_t req, fuse_ino_t ino, int mode, off_t offset, off_t length, fuse_file_info* fi) {
            CALL(FAllocate, EFileStoreRequest::AllocateData, length, req, ino, mode, offset, length, fi);
        };
        ops.flush = [] (fuse_req_t req, fuse_ino_t ino, fuse_file_info* fi) {
            CALL(Flush, EFileStoreRequest::MAX, 0, req, ino, fi);
        };
        ops.fsync = [] (fuse_req_t req, fuse_ino_t ino, int datasync, fuse_file_info* fi) {
            CALL(FSync, EFileStoreRequest::MAX, 0, req, ino, datasync, fi);
        };
        ops.fsyncdir = [] (fuse_req_t req, fuse_ino_t ino, int datasync, fuse_file_info* fi) {
            CALL(FSyncDir, EFileStoreRequest::MAX, 0, req, ino, datasync, fi);
        };
        ops.release = [] (fuse_req_t req, fuse_ino_t ino, fuse_file_info* fi) {
            CALL(Release, EFileStoreRequest::DestroyHandle, 0, req, ino, fi);
        };

        //
        // Locking
        //

        ops.getlk = [] (fuse_req_t req, fuse_ino_t ino, fuse_file_info* fi, struct flock* lock) {
            CALL(GetLock, EFileStoreRequest::TestLock, lock->l_len, req, ino, fi, lock);
        };
        ops.setlk = [] (fuse_req_t req, fuse_ino_t ino, fuse_file_info* fi, struct flock* lock, int sleep) {
            CALL(SetLock, GetLockRequestType(lock), lock->l_len, req, ino, fi, lock, sleep != 0);
        };
        ops.flock = [] (fuse_req_t req, fuse_ino_t ino, fuse_file_info* fi, int op) {
            CALL(FLock, GetLockRequestType(op), 0, req, ino, fi, op);
        };
#undef CALL
    }

private:
    static EFileStoreRequest GetLockRequestType(int op)
    {
        return (op & LOCK_UN) ? EFileStoreRequest::ReleaseLock : EFileStoreRequest::AcquireLock;
    }

    static EFileStoreRequest GetLockRequestType(struct flock* lock)
    {
        return (lock->l_type == F_UNLCK) ? EFileStoreRequest::ReleaseLock : EFileStoreRequest::AcquireLock;
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IFileSystemDriverPtr CreateFileSystemDriver(
    TFuseConfigPtr config,
    ILoggingServicePtr logging,
    IRequestStatsRegistryPtr requestStats,
    ISessionPtr session,
    IFileSystemFactoryPtr fileSystemFactory)
{
    return std::make_shared<TFileSystemDriver>(
        std::move(config),
        std::move(logging),
        std::move(requestStats),
        std::move(session),
        std::move(fileSystemFactory));
}

}   // namespace NCloud::NFileStore::NFuse
