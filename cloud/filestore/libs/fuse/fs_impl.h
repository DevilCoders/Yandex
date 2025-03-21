#pragma once

#include "public.h"

#include "cache.h"
#include "config.h"
#include "convert.h"
#include "fs.h"

#include <cloud/filestore/libs/diagnostics/request_stats.h>
#include <cloud/filestore/libs/service/context.h>
#include <cloud/filestore/libs/service/filestore.h>
#include <cloud/filestore/libs/service/request.h>

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/scheduler.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/system/align.h>
#include <util/system/mutex.h>

namespace NCloud::NFileStore::NFuse {

////////////////////////////////////////////////////////////////////////////////

constexpr ui64 MissingNodeId = -1;

////////////////////////////////////////////////////////////////////////////////

class TFileSystem final
    : public IFileSystem
    , public std::enable_shared_from_this<TFileSystem>
{
private:
    static constexpr size_t PageSize = 4*1024;

private:
    const ILoggingServicePtr Logging;
    const ITimerPtr Timer;
    const ISchedulerPtr Scheduler;
    const IFileStorePtr Session;
    const TFileSystemConfigPtr Config;

    TLog Log;
    IRequestStatsPtr RequestStats;
    ICompletionQueuePtr CompletionQueue;

    TCache Cache;
    TMutex CacheLock;
    THashMap<ui64, uintptr_t> DirectoryHandles;

    TXAttrCache XAttrCache;
    TMutex XAttrLock;

public:
    TFileSystem(
        ILoggingServicePtr logging,
        ISchedulerPtr scheduler,
        ITimerPtr timer,
        TFileSystemConfigPtr config,
        IFileStorePtr session,
        IRequestStatsPtr stats,
        ICompletionQueuePtr queue);

    ~TFileSystem();

    void Reset() override;

    // filesystem information
    void StatFs(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino) override;

    // nodes
    void Lookup(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t parent,
        TString name) override;
    void Forget(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        unsigned long nlookup) override;
    void ForgetMulti(
        TCallContextPtr callContext,
        fuse_req_t req,
        size_t count,
        fuse_forget_data* forgets) override;

    void MkDir(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t parent,
        TString name,
        mode_t mode) override;
    void RmDir(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t parent,
        TString name) override;
    void MkNode(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t parent,
        TString name,
        mode_t mode,
        dev_t rdev) override;
    void Unlink(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t parent,
        TString name) override;
    void Rename(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t parent,
        TString name,
        fuse_ino_t newparent,
        TString newname,
        int flags) override;
    void SymLink(
        TCallContextPtr callContext,
        fuse_req_t req,
        TString link,
        fuse_ino_t parent,
        TString name) override;
    void Link(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        fuse_ino_t newparent,
        TString newname) override;
    void ReadLink(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino) override;
    void Access(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        int mask) override;

    // node attributes
    void SetAttr(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        struct stat* attr,
        int to_set,
        fuse_file_info* fi) override;
    void GetAttr(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        fuse_file_info* fi) override;

    // extended node attributes
    void SetXAttr(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        TString name,
        TString value,
        int flags) override;
    void GetXAttr(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        TString name,
        size_t size) override;
    void ListXAttr(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        size_t size) override;
    void RemoveXAttr(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        TString name) override;

    // directory listing
    void OpenDir(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        fuse_file_info* fi) override;
    void ReadDir(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        size_t size,
        off_t offset,
        fuse_file_info* fi) override;
    void ReleaseDir(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        fuse_file_info* fi) override;

    // read & write files
    void Create(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t parent,
        TString name,
        mode_t mode,
        fuse_file_info* fi) override;
    void Open(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        fuse_file_info* fi) override;
    void Read(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        size_t size,
        off_t offset,
        fuse_file_info* fi) override;
    void Write(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        TStringBuf buffer,
        off_t offset,
        fuse_file_info* fi) override;
    void WriteBuf(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        fuse_bufvec* bufv,
        off_t offset,
        fuse_file_info* fi) override;
    void FAllocate(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        int mode,
        off_t offset,
        off_t length,
        fuse_file_info* fi) override;
    void Flush(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        fuse_file_info* fi) override;
    void FSync(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        int datasync,
        fuse_file_info* fi) override;
    void FSyncDir(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        int datasync,
        fuse_file_info* fi) override;
    void Release(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        fuse_file_info* fi) override;

    // locking
    void GetLock(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        fuse_file_info* fi,
        struct flock* lock) override;
    void SetLock(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        fuse_file_info* fi,
        struct flock* lock,
        bool sleep) override;
    void FLock(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        fuse_file_info* fi,
        int op) override;

private:
    template <typename T>
    static std::shared_ptr<T> StartRequest()
    {
        return std::make_shared<T>();
    }

    template <typename T>
    static std::shared_ptr<T> StartRequest(ui64 node)
    {
        auto request = StartRequest<T>();
        request->SetNodeId(node);

        return request;
    }

    template <typename T>
    bool CheckResponse(
        TCallContext& callContext,
        fuse_req_t req,
        const T& response)
    {
        return CheckError(callContext, req, response.GetError());
    }

    bool CheckError(
        TCallContext& callContext,
        fuse_req_t req,
        const NProto::TError& error);

    bool UpdateNodesCache(
        const NProto::TNodeAttr& attrs,
        fuse_entry_param& entry);

    void UpdateXAttrCache(
        ui64 ino,
        const TString& name,
        const TString& value,
        ui64 version,
        const NProto::TError& error);

    void ReplyCreate(
        TCallContext& callContext,
        fuse_req_t req,
        ui64 handle,
        const NProto::TNodeAttr& attrs);
    void ReplyEntry(
        TCallContext& callContext,
        fuse_req_t req,
        const NProto::TNodeAttr& attrs);
    void ReplyXAttrInt(
        TCallContext& callContext,
        fuse_req_t req,
        const TString& value,
        size_t size);
    void ReplyAttr(
        TCallContext& callContext,
        fuse_req_t req,
        const NProto::TNodeAttr& attrs);

    void ClearDirectoryCache();

#define FILESYSTEM_REPLY_IMPL(name, ...)                                                              \
    template<typename... TArgs>                                                                       \
    int Reply##name(                                                                                  \
        TCallContext& callContext,                                                                    \
        fuse_req_t req,                                                                               \
        TArgs&&... args)                                                                              \
    {                                                                                                 \
        Y_VERIFY(RequestStats);                                                                       \
        Y_VERIFY(CompletionQueue);                                                                    \
        return CompletionQueue->Complete(req, [&] (fuse_req_t r) {                                    \
            return NFuse::Reply##name(                                                                \
                Log,                                                                                  \
                *RequestStats,                                                                        \
                callContext,                                                                          \
                r,                                                                                    \
                std::forward<TArgs>(args)...);                                                        \
        });                                                                                           \
    }                                                                                                 \

    FILESYSTEM_REPLY_METHOD(FILESYSTEM_REPLY_IMPL)

#undef FILESYSTEM_REPLY_IMPL

    struct TRangeLock
    {
        ui64 Handle;
        ui64 Owner;
        off_t Offset;
        off_t Length;
    };

    void AcquireLock(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        const TRangeLock& range,
        NProto::ELockType type,
        bool sleep);
    void ReleaseLock(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        const TRangeLock& range);
    void TestLock(
        TCallContextPtr callContext,
        fuse_req_t req,
        fuse_ino_t ino,
        const TRangeLock& range,
        NProto::ELockType type);
};

}   // namespace NCloud::NFileStore::NFuse
