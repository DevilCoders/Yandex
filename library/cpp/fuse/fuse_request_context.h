#pragma once

#include "base.h"
#include "channel.h"
#include "fuse_dir_list.h"
#include "fuse_kernel.h"
#include "fuse_xattr_buf.h"
#include "fuse_xattr_list.h"

#include <library/cpp/fuse/deadline/deadline.h>
#include <library/cpp/fuse/mrw_lock/mrw_lock.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>

#include <errno.h>

namespace NFuse {

struct TFuseFileHandle {
    TFuseFileHandle(ui64 val)
        : Val(val)
    { }

    ui64 Val;
};

struct TFuseDirHandle {
    TFuseDirHandle(ui64 val)
        : Val(val)
    { }

    ui64 Val;
};

struct TFuseWriteSize {
    TFuseWriteSize(size_t val)
        : Val(val)
    { }

    size_t Val;
};

class IFuseRequestContext : public NThreading::IDeadlineAction {
public:
    IFuseRequestContext(TDuration timeout, TInstant now = TInstant::Now())
        : IDeadlineAction(timeout.ToDeadLine(now))
        , Start_(now)
    {
    }

    TInstant GetStart() const {
        return Start_;
    }

    void SetTreeGuard(NThreading::TReadWriteGuard&& globalGuard) {
        GlobalGuard_ = std::move(globalGuard);
    }

    bool HoldsWriteLock() const {
        return GlobalGuard_.HoldsWriteLock();
    }

    bool HoldsReadLock() const {
        return GlobalGuard_.HoldsReadLock();
    }

public:
    virtual struct fuse_in_header GetHeader() const = 0;

    virtual void SendNone() = 0;
    virtual EMessageStatus SendError(int err) = 0;
    virtual EMessageStatus SendReply(TStringBuf payload) = 0;
    virtual EMessageStatus SendOpenReply(ui64 fh) = 0;
    virtual EMessageStatus SendOpenDirReply(ui64 fh) = 0;
    virtual EMessageStatus SendWriteReply(size_t size) = 0;
    virtual EMessageStatus SendCreateReply(const struct fuse_entry_out& entry, const struct fuse_open_out& open) = 0;
    virtual EMessageStatus SendGetXattrSizeReply(size_t size) = 0;

    template<typename T>
    EMessageStatus SendReply(const T& payload) {
        static_assert(std::is_standard_layout_v<T>);
        static_assert(std::is_trivial_v<T>);
        return SendReply(TStringBuf{reinterpret_cast<const char*>(&payload), sizeof(T)});
    }

    EMessageStatus SendReply(const TString& payload) {
        return SendReply(TStringBuf{payload});
    }

    EMessageStatus SendReply(const TFuseFileHandle& fh) {
        return SendOpenReply(fh.Val);
    }

    EMessageStatus SendReply(const TFuseDirHandle& fh) {
        return SendOpenDirReply(fh.Val);
    }

    EMessageStatus SendReply(const TFuseDirList& list) {
        return SendReply(list.AsStrBuf());
    }

    EMessageStatus SendReply(const TFuseDirListPlus& list) {
        return SendReply(list.AsStrBuf());
    }

    EMessageStatus SendReply(const TFuseWriteSize& size) {
        return SendWriteReply(size.Val);
    }

    EMessageStatus SendReply(const std::pair<const struct fuse_entry_out, const struct fuse_open_out>& create) {
        return SendCreateReply(create.first, create.second);
    }

    EMessageStatus SendReply(const TFuseXattrList& list) {
        if (list.SizeRequested()) {
            struct fuse_getxattr_out getxattrOut = {};
            getxattrOut.size = list.GetRequiredSize();
            return SendReply(getxattrOut);
        } else if (list.ResponseFitsSize()) {
            return SendReply(list.AsStrBuf());
        } else {
            return SendError(ERANGE);
        }
    }

    EMessageStatus SendReply(const TFuseXattrBuf& buf) {
        if (buf.SizeRequested()) {
            struct fuse_getxattr_out getxattrOut = {};
            getxattrOut.size = buf.GetRequiredSize();
            return SendReply(getxattrOut);
        } else if (buf.ResponseFitsSize()) {
            return SendReply(buf.AsStrBuf());
        } else {
            return SendError(ERANGE);
        }
    }

private:
    TInstant Start_;
    NThreading::TReadWriteGuard GlobalGuard_;
};

using IFuseRequestContextRef = TIntrusivePtr<IFuseRequestContext>;

class TTypedRequestContextBase {
public:
    TTypedRequestContextBase(IFuseRequestContextRef ctx)
        : Ctx_(ctx)
    {
    }

    TInstant GetDeadline() const {
        return Ctx_->GetDeadline();
    }

    TInstant GetStart() const {
        return Ctx_->GetStart();
    }

    struct fuse_in_header GetHeader() const {
        return Ctx_->GetHeader();
    }

    ui32 GetUid() const {
        return GetHeader().uid;
    }
    ui32 GetGid() const {
        return GetHeader().gid;
    }

    EMessageStatus ReplyError(int code) {
        return Ctx_->SendError(code);
    }

    void SetTreeGuard(NThreading::TReadWriteGuard&& globalGuard) {
        Ctx_->SetTreeGuard(std::move(globalGuard));
    }

    bool HoldsWriteLock() const {
        return Ctx_->HoldsWriteLock();
    }

    bool HoldsReadLock() const {
        return Ctx_->HoldsReadLock();
    }

protected:
    IFuseRequestContextRef Ctx_;
};

template<class T>
class TTypedRequestContext : public TTypedRequestContextBase {
public:
    TTypedRequestContext(IFuseRequestContextRef ctx)
        : TTypedRequestContextBase(ctx)
    {
    }

    template<class U>
    TTypedRequestContext(U ctx)
        : TTypedRequestContextBase(ctx)
    {
    }

    EMessageStatus Reply(const T& resp) {
        return Ctx_->SendReply(resp);
    }
};

template<>
class TTypedRequestContext<void> : public TTypedRequestContextBase {
public:
    TTypedRequestContext(IFuseRequestContextRef ctx)
        : TTypedRequestContextBase(ctx)
    {
    }

    template<class U>
    TTypedRequestContext(U ctx)
        : TTypedRequestContextBase(ctx)
    {
    }
};

} // namespace NFuse
