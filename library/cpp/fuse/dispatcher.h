#pragma once

#include "base.h"
#include "fuse_dir_list.h"
#include "fuse_kernel.h"
#include "fuse_request_context.h"
#include "fuse_xattr_buf.h"
#include "fuse_xattr_list.h"

namespace NFuse {

class IInvalidator {
public:
    virtual ~IInvalidator() = default;

    // These three are done asynchronously. Use FlushInvalidations to detect when
    // these operations are complete
    virtual void InvalidateInode(TInodeNum ino, i64 off, i64 len) = 0;
    virtual void InvalidateEntry(TInodeNum parent, const TString& name) = 0;
    // Falls back to InvalidateEntry if FUSE protocol version is lower than 7.18
    virtual void NotifyEntryDelete(TInodeNum parent, TInodeNum child, const TString& name) = 0;

    virtual NThreading::TFuture<void> FlushInvalidations() = 0;

    virtual EMessageStatus InvalidateInodeSync(TInodeNum ino, i64 off, i64 len) = 0;
    virtual EMessageStatus InvalidateEntrySync(TInodeNum parent, const TString& name) = 0;
    // Falls back to InvalidateEntry if FUSE protocol version is lower than 7.18
    virtual EMessageStatus NotifyEntryDeleteSync(TInodeNum parent, TInodeNum child, const TString& name) = 0;

    virtual TVector<pid_t> GetLastPids() const {
        return {};
    }
};

class IDispatcher : public TThrRefBase {
public:
    virtual ~IDispatcher() = default;

    virtual void Init(
        const struct fuse_init_in& initInput,
        IInvalidator* invalidator,
        struct fuse_init_out& initOut
    );

    // Returns error code that request should be finished with
    virtual int HandleTimeout(TTypedRequestContextBase ctx);

    // Returns error code that request should be finished with
    virtual int HandleException(TTypedRequestContextBase ctx, std::exception_ptr ex);

    virtual TString GetDebugInfo(const TTypedRequestContextBase& ctx);

    virtual void Lookup(
        TTypedRequestContext<struct fuse_entry_out> ctx,
        TInodeNum parent,
        TStringBuf name
    );

    virtual void Forget(
        TInodeNum ino,
        unsigned long nlookup
    );

    virtual void GetAttr(
        TTypedRequestContext<struct fuse_attr_out> ctx,
        TInodeNum ino
    );

    virtual void ReadLink(
        TTypedRequestContext<TStringBuf> ctx,
        TInodeNum ino
    );

    virtual void Access(
        TTypedRequestContext<void> ctx,
        TInodeNum ino,
        int mask
    );

    // The returned fh (file handle) value will be passed to other operations,
    // but note that osxfuse can reuse same fh for different file descriptors
    virtual void Open(
        TTypedRequestContext<TFuseFileHandle> ctx,
        TInodeNum ino,
        int flags
    );

    virtual void Release(
        TTypedRequestContext<void> ctx,
        TInodeNum ino,
        ui64 fh
    );

    virtual void Read(
        TTypedRequestContext<TStringBuf> ctx,
        TInodeNum ino,
        ui64 fh,
        size_t size,
        off_t offset
    );

    virtual void OpenDir(
        TTypedRequestContext<TFuseDirHandle> ctx,
        TInodeNum ino,
        int flags
    );

    virtual void ReleaseDir(
        TTypedRequestContext<void> ctx,
        TInodeNum ino,
        ui64 fh
    );

    virtual void ReadDir(
        TTypedRequestContext<TFuseDirList> ctx,
        TInodeNum ino,
        ui64 fh,
        off_t off,
        TFuseDirList&& dirList
    );

    virtual void ReadDirPlus(
        TTypedRequestContext<TFuseDirListPlus> ctx,
        TInodeNum ino,
        ui64 fh,
        off_t off,
        TFuseDirListPlus&& dirList
    );

    virtual void StatFs(
        TTypedRequestContext<struct fuse_statfs_out> ctx
    );

    virtual void SetAttr(
        TTypedRequestContext<struct fuse_attr_out> ctx,
        TInodeNum ino,
        const struct fuse_setattr_in& args
    );

    virtual void Write(
        TTypedRequestContext<TFuseWriteSize> ctx,
        TInodeNum ino,
        ui64 fh,
        TStringBuf data,
        off_t offset
    );

    virtual void Flush(
        TTypedRequestContext<void> ctx,
        TInodeNum ino,
        ui64 fh
    );

    virtual void FSync(
        TTypedRequestContext<void> ctx,
        TInodeNum ino,
        ui64 fh,
        bool dataSync
    );

    virtual void FAllocate(
        TTypedRequestContext<void> ctx,
        TInodeNum ino,
        off_t offset,
        size_t length,
        int mode
    );


    virtual void MkNod(
        TTypedRequestContext<struct fuse_entry_out> ctx,
        TInodeNum ino,
        TStringBuf name,
        int mode,
        int rdev
    );

    virtual void MkDir(
        TTypedRequestContext<struct fuse_entry_out> ctx,
        TInodeNum ino,
        TStringBuf name,
        int mode
    );

    virtual void Create(
        TTypedRequestContext<std::pair<const fuse_entry_out, const fuse_open_out>> ctx,
        TInodeNum ino,
        TStringBuf name,
        int mode,
        int flags
    );

    virtual void Symlink(
        TTypedRequestContext<struct fuse_entry_out> ctx,
        TInodeNum ino,
        TStringBuf name,
        TStringBuf link
    );

    virtual void Link(
        TTypedRequestContext<struct fuse_entry_out> ctx,
        TInodeNum ino,
        TInodeNum oldnodeid,
        TStringBuf newName
    );

    virtual void Unlink(
        TTypedRequestContext<void> ctx,
        TInodeNum ino,
        TStringBuf name
    );

    virtual void RmDir(
        TTypedRequestContext<void> ctx,
        TInodeNum ino,
        TStringBuf name
    );

    virtual void Rename(
        TTypedRequestContext<void> ctx,
        TInodeNum ino,
        TStringBuf name,
        TInodeNum newParent,
        TStringBuf newName,
        ui32 flags
    );

    virtual void FSyncDir(
        TTypedRequestContext<void> ctx,
        TInodeNum ino,
        ui64 fh,
        bool dataSync
    );


    virtual void ListXattr(
        TTypedRequestContext<TFuseXattrList> ctx,
        TInodeNum ino,
        TFuseXattrList&& xattrList
    );

    // Note: TFuseXattrBuf does not hold the data.
    // It expects that data owner will keep data until ctx.SendReply(xattrBuf) is invoked
    virtual void GetXattr(
        TTypedRequestContext<TFuseXattrBuf> ctx,
        TInodeNum ino,
        TStringBuf attrName,
        TFuseXattrBuf&& xattrBuf
    );

    virtual void SetXattr(
        TTypedRequestContext<void> ctx,
        TInodeNum ino,
        int flags,
        TStringBuf attrName,
        TStringBuf value
    );

    virtual void RemoveXattr(
        TTypedRequestContext<void> ctx,
        TInodeNum ino,
        TStringBuf attrName
    );


    virtual void Bmap(
        TTypedRequestContext<void> ctx
    );
};

using IDispatcherRef = TIntrusivePtr<IDispatcher>;

} // namespace NFuse
