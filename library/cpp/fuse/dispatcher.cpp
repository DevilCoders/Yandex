#include "dispatcher.h"

#include <errno.h>

namespace NFuse {

void IDispatcher::Init(
    const struct fuse_init_in& initInput,
    IInvalidator* invalidator,
    struct fuse_init_out& initOut
) {
    Y_UNUSED(initInput, invalidator, initOut);
}

int IDispatcher::HandleException(TTypedRequestContextBase ctx, std::exception_ptr ex) {
    Y_UNUSED(ctx);
    Y_UNUSED(ex);
    return EIO;
}

int IDispatcher::HandleTimeout(TTypedRequestContextBase ctx) {
    Y_UNUSED(ctx);
    return ETIMEDOUT;
}

TString IDispatcher::GetDebugInfo(const TTypedRequestContextBase& ctx) {
    Y_UNUSED(ctx);
    return {};
}

void IDispatcher::Lookup(
    TTypedRequestContext<struct fuse_entry_out> ctx,
    TInodeNum,
    TStringBuf
) {
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::Forget(
    TInodeNum,
    unsigned long
) {
}

void IDispatcher::GetAttr(
    TTypedRequestContext<struct fuse_attr_out> ctx,
    TInodeNum
) {
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::ReadLink(
    TTypedRequestContext<TStringBuf> ctx,
    TInodeNum
) {
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::Access(
    TTypedRequestContext<void> ctx,
    TInodeNum,
    int
) {
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::Open(
    TTypedRequestContext<TFuseFileHandle> ctx,
    TInodeNum,
    int
) {
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::Release(
    TTypedRequestContext<void> ctx,
    TInodeNum,
    ui64
) {
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::Read(
    TTypedRequestContext<TStringBuf> ctx,
    TInodeNum,
    ui64,
    size_t,
    off_t
) {
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::OpenDir(
    TTypedRequestContext<TFuseDirHandle> ctx,
    TInodeNum,
    int
) {
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::ReleaseDir(
    TTypedRequestContext<void> ctx,
    TInodeNum,
    ui64
) {
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::ReadDir(
    TTypedRequestContext<TFuseDirList> ctx,
    TInodeNum,
    ui64,
    off_t,
    TFuseDirList&&
) {
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::ReadDirPlus(
    TTypedRequestContext<TFuseDirListPlus> ctx,
    TInodeNum,
    ui64,
    off_t,
    TFuseDirListPlus&&
) {
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::StatFs(
    TTypedRequestContext<struct fuse_statfs_out> ctx
) {
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::SetAttr(
    TTypedRequestContext<struct fuse_attr_out> ctx,
    TInodeNum ino,
    const fuse_setattr_in& args
) {
    Y_UNUSED(ctx, ino, args);
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::Write(
    TTypedRequestContext<TFuseWriteSize> ctx,
    TInodeNum ino,
    ui64 fh,
    TStringBuf data,
    off_t offset
) {
    Y_UNUSED(ctx, ino, fh, data, offset);
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::Flush(
    TTypedRequestContext<void> ctx,
    TInodeNum ino,
    ui64 fh
) {
    Y_UNUSED(ctx, ino, fh);
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::FSync(
    TTypedRequestContext<void> ctx,
    TInodeNum ino,
    ui64 fh,
    bool dataSync
) {
    Y_UNUSED(ctx, ino, fh, dataSync);
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::FAllocate(
    TTypedRequestContext<void> ctx,
    TInodeNum ino,
    off_t offset,
    size_t length,
    int mode
) {
    Y_UNUSED(ctx, ino, offset, length, mode);
    ctx.ReplyError(ENOSYS);
}


void IDispatcher::MkNod(
    TTypedRequestContext<struct fuse_entry_out> ctx,
    TInodeNum ino,
    TStringBuf name,
    int mode,
    int rdev
) {
    Y_UNUSED(ctx, ino, name, mode, rdev);
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::MkDir(
    TTypedRequestContext<struct fuse_entry_out> ctx,
    TInodeNum ino,
    TStringBuf name,
    int mode
) {
    Y_UNUSED(ctx, ino, name, mode);
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::Create(
    TTypedRequestContext<std::pair<const fuse_entry_out, const fuse_open_out>> ctx,
    TInodeNum ino,
    TStringBuf name,
    int mode,
    int flags
) {
    Y_UNUSED(ctx, ino, name, mode, flags);
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::Symlink(
    TTypedRequestContext<struct fuse_entry_out> ctx,
    TInodeNum ino,
    TStringBuf name,
    TStringBuf link
) {
    Y_UNUSED(ctx, ino, name, link);
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::Link(
    TTypedRequestContext<struct fuse_entry_out> ctx,
    TInodeNum ino,
    TInodeNum oldnodeid,
    TStringBuf newName
) {
    Y_UNUSED(ctx, ino, oldnodeid, newName);
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::Unlink(
    TTypedRequestContext<void> ctx,
    TInodeNum ino,
    TStringBuf name
) {
    Y_UNUSED(ctx, ino, name);
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::RmDir(
    TTypedRequestContext<void> ctx,
    TInodeNum ino,
    TStringBuf name
) {
    Y_UNUSED(ctx, ino, name);
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::Rename(
    TTypedRequestContext<void> ctx,
    TInodeNum ino,
    TStringBuf name,
    TInodeNum newParent,
    TStringBuf newName,
    ui32 flags
) {
    Y_UNUSED(ctx, ino, name, newParent, newName, flags);
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::FSyncDir(
    TTypedRequestContext<void> ctx,
    TInodeNum ino,
    ui64 fh,
    bool dataSync
) {
    Y_UNUSED(ctx, ino, fh, dataSync);
    ctx.ReplyError(ENOSYS);
}


void IDispatcher::ListXattr(
    TTypedRequestContext<TFuseXattrList> ctx,
    TInodeNum ino,
    TFuseXattrList&& xattrList
) {
    Y_UNUSED(ctx, ino, xattrList);
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::GetXattr(
    TTypedRequestContext<TFuseXattrBuf> ctx,
    TInodeNum ino,
    TStringBuf attrName,
    TFuseXattrBuf&& xattrBuf
) {
    Y_UNUSED(ctx, ino, attrName, xattrBuf);
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::SetXattr(
    TTypedRequestContext<void> ctx,
    TInodeNum ino,
    int flags,
    TStringBuf attrName,
    TStringBuf value
) {
    Y_UNUSED(ctx, ino, flags, attrName, value);
    ctx.ReplyError(ENOSYS);
}

void IDispatcher::RemoveXattr(
    TTypedRequestContext<void> ctx,
    TInodeNum ino,
    TStringBuf attrName
) {
    Y_UNUSED(ctx, ino, attrName);
    ctx.ReplyError(ENOSYS);
}


void IDispatcher::Bmap(
    TTypedRequestContext<void> ctx
) {
    // Intentionally not implemented
    ctx.ReplyError(ENOSYS);
}

} //namespace NFuse
