#pragma once

#include "arg_parser.h"
#include "channel.h"
#include "dispatcher.h"
#include "fuse_kernel.h"
#include "pid_ring.h"
#include "raw_channel.h"
#include "request_context.h"

#include <library/cpp/fuse/deadline/deadline.h>

#include <library/cpp/logger/log.h>
#include <library/cpp/monlib/dynamic_counters/counters.h>
#include <library/cpp/threading/future/future.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/getpid.h>
#include <util/thread/pool.h>

#include <sys/uio.h>

namespace NFuse {

static constexpr size_t MAX_FUSE_HANDLERS = 64;

class TLowMount final : public TThrRefBase, public IInvalidator {
public:
    struct TOptions {
        IChannelRef Channel;
        IDispatcherRef Dipatcher;
        TString MountPath; // Used only for logs
        TLog Log;
        NMonitoring::TDynamicCounterPtr Counters = MakeIntrusive<NMonitoring::TDynamicCounters>();
        TDuration RequestTimeout = TDuration::Seconds(20);
        NThreading::TDeadlineScheduler* DeadlineScheduler = nullptr;
        bool Debug = false;
        bool AllowOurself = false;
        bool NoInvalidation = false;
    };

    TLowMount(const TOptions& options);

    TString GetMountPath() const {
        return MountPath_;
    }

    void SetIOThreadCount(int count);
    void Process(TStringBuf data, int threadIdx);

    TDuration GetRequestTimeout() const {
        return RequestTimeout_;
    }

    // These three are done asynchronously. Use FlushInvalidations to detect when
    // these operations are complete
    void InvalidateInode(TInodeNum ino, i64 off, i64 len) override;
    void InvalidateEntry(TInodeNum parent, const TString& name) override;
    // Falls back to InvalidateEntry if FUSE protocol version is lower than 7.18
    void NotifyEntryDelete(TInodeNum parent, TInodeNum child, const TString& name) override;

    NThreading::TFuture<void> FlushInvalidations() override;

    EMessageStatus InvalidateInodeSync(TInodeNum ino, i64 off, i64 len) override;
    EMessageStatus InvalidateEntrySync(TInodeNum parent, const TString& name) override;
    // Falls back to InvalidateEntry if FUSE protocol version is lower than 7.18
    EMessageStatus NotifyEntryDeleteSync(TInodeNum parent, TInodeNum child, const TString& name) override;

    TVector<pid_t> GetLastPids() const override;

    bool WaitInitD(TInstant deadline);

    // Called must ensure that all io worker threads are stopped and no
    // Receive or Process will be called at the time it calls Shutdown
    void Shutdown(const TString& reason);

private:
    friend TRequestContext;
    friend constexpr auto InitHandlers();

    static constexpr size_t HandlerTableSize_ = 64;
    void InitCounters(NMonitoring::TDynamicCounters& counters);

    void FuseInit(
        TRequestContextRef ctx,
        TArgParser arg);

    void FuseLookup(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseForget(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseBatchForget(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseGetAttr(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseReadLink(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseAccess(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseOpen(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseRelease(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseRead(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseOpenDir(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseReleaseDir(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseReadDir(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseReadDirPlus(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseStatFs(
        TRequestContextRef ctx,
        TArgParser arg);

    void FuseSetAttr(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseWrite(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseFlush(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseFSync(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseFAllocate(
        TRequestContextRef ctx,
        TArgParser arg);


    void FuseMkNod(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseMkDir(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseCreate(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseSymlink(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseLink(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseUnlink(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseRmDir(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseRename(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseFSyncDir(
        TRequestContextRef ctx,
        TArgParser arg);


    void FuseListXattr(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseGetXattr(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseSetXattr(
        TRequestContextRef ctx,
        TArgParser arg);
    void FuseRemoveXattr(
        TRequestContextRef ctx,
        TArgParser arg);


    void FuseBmap(
        TRequestContextRef ctx,
        TArgParser arg);
private:

    void InitFuse();

    EMessageStatus SendError(const struct fuse_in_header& req, int err);

    EMessageStatus SendReply(const struct fuse_in_header& req, TStringBuf payload);

    template<typename T>
    EMessageStatus SendReply(const struct fuse_in_header& req, const T& payload) {
        static_assert(std::is_standard_layout_v<T>);
        static_assert(std::is_trivial_v<T>);
        return SendReply(req, TStringBuf{reinterpret_cast<const char*>(&payload), sizeof(T)});
    }

    EMessageStatus SendNotify(int notifyCode, TArrayRef<iovec> iov);

    void Timeout(TRequestContextRef ctx);

    void FinishRequest(TRequestContextRef ctx, ui32 opcode);

private:
    template<typename... Args>
    TRequestContextRef MakeRequest(Args&&... args) {
        return MakeIntrusive<TRequestContext>(this, std::forward<Args>(args)...);
    }

private:
    TLog Log_;
    IChannelRef Channel_;
    IDispatcherRef Dispatcher_;
    const bool TraceFuseMsg_;
    const TDuration RequestTimeout_;

    NThreading::TDeadlineScheduler* Deadlines_;
    THolder<NThreading::TDeadlineScheduler> OwnDeadlines_;

    const TString MountPath_;
    const TProcessId MyPid_;

    std::array<NMonitoring::THistogramPtr, HandlerTableSize_> Histograms_;
    NMonitoring::TDynamicCounters::TCounterPtr ActiveRequests_;
    NMonitoring::TDynamicCounters::TCounterPtr Timeouts_;
    NMonitoring::TDynamicCounters::TCounterPtr ResponsesSuccess_;
    NMonitoring::TDynamicCounters::TCounterPtr ResponsesError_;
    NMonitoring::TDynamicCounters::TCounterPtr UnhandledOps_;
    NMonitoring::TDynamicCounters::TCounterPtr QueuedInvalidations_;
    NMonitoring::TDynamicCounters::TCounterPtr InvalOk_;
    NMonitoring::TDynamicCounters::TCounterPtr InvalNoent_;
    NMonitoring::TDynamicCounters::TCounterPtr InvalTimeout_;
    NMonitoring::TDynamicCounters::TCounterPtr InvalOther_;

    std::unique_ptr<TPidRing[]> PidRings;
    int PidRingsSize;

    struct fuse_init_in InConnInfo_;
    struct fuse_init_out ConnInfo_;
    TManualEvent Initialized_;
    TAtomic Stop_;

    TAtomic SkipLoggingUnknownOpcodes_[MAX_FUSE_HANDLERS];

    TThreadPool InvalidationThread_;
};

} //namespace NFuse
