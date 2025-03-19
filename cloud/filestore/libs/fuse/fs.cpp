#include "fs.h"

#include "fs_impl.h"
#include "probes.h"

namespace NCloud::NFileStore::NFuse {

LWTRACE_USING(FILESTORE_FUSE_PROVIDER);

////////////////////////////////////////////////////////////////////////////////

namespace {

struct TFuseFileSystemFactory
    : public IFileSystemFactory
{
    ILoggingServicePtr Logging;
    ISchedulerPtr Scheduler;
    ITimerPtr Timer;

    TFuseFileSystemFactory(
            ILoggingServicePtr logging,
            ISchedulerPtr scheduler,
            ITimerPtr timer)
        : Logging(std::move(logging))
        , Scheduler(std::move(scheduler))
        , Timer(std::move(timer))
    {}

    IFileSystemPtr CreateFileSystem(
        TFileSystemConfigPtr config,
        IFileStorePtr session,
        IRequestStatsPtr stats,
        ICompletionQueuePtr queue) override
    {
        return NCloud::NFileStore::NFuse::CreateFileSystem(
            Logging,
            Scheduler,
            Timer,
            std::move(config),
            std::move(session),
            std::move(stats),
            std::move(queue));
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

// TODO: it would be nice to get rid of this garbage...

int ReplyNone(
    TLog& log,
    IRequestStats& requestStats,
    TCallContext& callContext,
    fuse_req_t req)
{
    FILESTORE_TRACK(RequestCompleted, (&callContext), "None");
    requestStats.ResponseSent(callContext);
    fuse_reply_none(req);
    requestStats.RequestCompleted(log, callContext);
    return 0;
}

int ReplyError(
    TLog& log,
    IRequestStats& requestStats,
    TCallContext& callContext,
    fuse_req_t req,
    int error)
{
    FILESTORE_TRACK(RequestCompleted, (&callContext), "Error");
    requestStats.ResponseSent(callContext);
    int res = fuse_reply_err(req, error);
    requestStats.RequestCompleted(log, callContext);
    return res;
}

int ReplyEntry(
    TLog& log,
    IRequestStats& requestStats,
    TCallContext& callContext,
    fuse_req_t req,
    const fuse_entry_param *e)
{
    FILESTORE_TRACK(RequestCompleted, (&callContext), "Entry");
    requestStats.ResponseSent(callContext);
    int res = fuse_reply_entry(req, e);
    requestStats.RequestCompleted(log, callContext);
    return res;
}

int ReplyCreate(
    TLog& log,
    IRequestStats& requestStats,
    TCallContext& callContext,
    fuse_req_t req,
    const fuse_entry_param *e,
    const fuse_file_info *fi)
{
    FILESTORE_TRACK(RequestCompleted, (&callContext), "Create");
    requestStats.ResponseSent(callContext);
    int res = fuse_reply_create(req, e, fi);
    requestStats.RequestCompleted(log, callContext);
    return res;
}

int ReplyAttr(
    TLog& log,
    IRequestStats& requestStats,
    TCallContext& callContext,
    fuse_req_t req,
    const struct stat *attr,
    double attr_timeout)
{
    FILESTORE_TRACK(RequestCompleted, (&callContext), "Attr");
    requestStats.ResponseSent(callContext);
    int res = fuse_reply_attr(req, attr, attr_timeout);
    requestStats.RequestCompleted(log, callContext);
    return res;
}

int ReplyReadLink(
    TLog& log,
    IRequestStats& requestStats,
    TCallContext& callContext,
    fuse_req_t req,
    const char *link)
{
    FILESTORE_TRACK(RequestCompleted, (&callContext), "ReadLink");
    requestStats.ResponseSent(callContext);
    int res = fuse_reply_readlink(req, link);
    requestStats.RequestCompleted(log, callContext);
    return res;
}

int ReplyOpen(
    TLog& log,
    IRequestStats& requestStats,
    TCallContext& callContext,
    fuse_req_t req,
    const fuse_file_info *fi)
{
    FILESTORE_TRACK(RequestCompleted, (&callContext), "Open");
    requestStats.ResponseSent(callContext);
    int res = fuse_reply_open(req, fi);
    requestStats.RequestCompleted(log, callContext);
    return res;
}

int ReplyWrite(
    TLog& log,
    IRequestStats& requestStats,
    TCallContext& callContext,
    fuse_req_t req,
    size_t count)
{
    FILESTORE_TRACK(RequestCompleted, (&callContext), "Write");
    requestStats.ResponseSent(callContext);
    int res = fuse_reply_write(req, count);
    requestStats.RequestCompleted(log, callContext);
    return res;
}

int ReplyBuf(
    TLog& log,
    IRequestStats& requestStats,
    TCallContext& callContext,
    fuse_req_t req,
    const char *buf,
    size_t size)
{
    FILESTORE_TRACK(RequestCompleted, (&callContext), "Buf");
    requestStats.ResponseSent(callContext);
    int res = fuse_reply_buf(req, buf, size);
    requestStats.RequestCompleted(log, callContext);
    return res;
}

int ReplyStatFs(
    TLog& log,
    IRequestStats& requestStats,
    TCallContext& callContext,
    fuse_req_t req,
    const struct statvfs *stbuf)
{
    FILESTORE_TRACK(RequestCompleted, (&callContext), "StatFs");
    requestStats.ResponseSent(callContext);
    int res = fuse_reply_statfs(req, stbuf);
    requestStats.RequestCompleted(log, callContext);
    return res;
}

int ReplyXAttr(
    TLog& log,
    IRequestStats& requestStats,
    TCallContext& callContext,
    fuse_req_t req,
    size_t count)
{
    FILESTORE_TRACK(RequestCompleted, (&callContext), "XAttr");
    requestStats.ResponseSent(callContext);
    int res = fuse_reply_xattr(req, count);
    requestStats.RequestCompleted(log, callContext);
    return res;
}

int ReplyLock(
    TLog& log,
    IRequestStats& requestStats,
    TCallContext& callContext,
    fuse_req_t req,
    const struct flock *lock)
{
    FILESTORE_TRACK(RequestCompleted, (&callContext), "Lock");
    requestStats.ResponseSent(callContext);
    int res = fuse_reply_lock(req, lock);
    requestStats.RequestCompleted(log, callContext);
    return res;
}

////////////////////////////////////////////////////////////////////////////////

IFileSystemFactoryPtr CreateFuseFileSystemFactory(
        ILoggingServicePtr logging,
        ISchedulerPtr scheduler,
        ITimerPtr timer)
{
    return std::make_shared<TFuseFileSystemFactory>(
        std::move(logging),
        std::move(scheduler),
        std::move(timer));
}

////////////////////////////////////////////////////////////////////////////////

IFileSystemPtr CreateFileSystem(
    ILoggingServicePtr logging,
    ISchedulerPtr scheduler,
    ITimerPtr timer,
    TFileSystemConfigPtr config,
    IFileStorePtr session,
    IRequestStatsPtr stats,
    ICompletionQueuePtr queue)
{
    return std::make_shared<TFileSystem>(
        std::move(logging),
        std::move(scheduler),
        std::move(timer),
        std::move(config),
        std::move(session),
        std::move(stats),
        std::move(queue));
}

}   // namespace NCloud::NFileStore::NFuse
