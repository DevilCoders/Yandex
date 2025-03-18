#include "request_context.h"
#include "low_mount.h"

#include <util/system/error.h>

#include <errno.h>

namespace NFuse {

TRequestContext::TRequestContext(
    TLowMountRef mount,
    const fuse_in_header& header
)
    : IFuseRequestContext(mount->GetRequestTimeout())
    , Mount_(mount)
    , Header_(header)
    , Opcode_(header.opcode)
{
}

TRequestContext::~TRequestContext() {
    Y_ASSERT(!IsActive());
}

struct fuse_in_header TRequestContext::ExtractHeader() {
    fuse_in_header ret = Header_;
    memset(&Header_, 0, sizeof(Header_));
    return ret;
}

bool TRequestContext::IsActive() {
    return Header_.opcode != 0;
}

void TRequestContext::Timeout() {
    Mount_->Timeout(this);
}

void TRequestContext::SendNone() {
    with_lock(Lock_) {
        if (IsActive()) {
            ExtractHeader();
            Mount_->FinishRequest(this, Opcode_);
        }
    }
}

EMessageStatus TRequestContext::SendError(int err) {
    with_lock(Lock_) {
        if (IsActive()) {
            auto res = Mount_->SendError(ExtractHeader(), err);
            Mount_->FinishRequest(this, Opcode_);
            return res;
        } else {
            return EMessageStatus::Timeout;
        }
    }
}

EMessageStatus TRequestContext::SendReply(TStringBuf payload) {
    with_lock(Lock_) {
        if (IsActive()) {
            auto res = Mount_->SendReply(ExtractHeader(), payload);
            Mount_->FinishRequest(this, Opcode_);
            return res;
        } else {
            return EMessageStatus::Timeout;
        }
    }
}

EMessageStatus TRequestContext::SendOpenReply(ui64 fh) {
    struct fuse_open_out openOut;
    memset(&openOut, 0, sizeof(openOut));
    openOut.fh = fh;
    openOut.open_flags |= FOPEN_KEEP_CACHE;
    return IFuseRequestContext::SendReply(openOut);
}

EMessageStatus TRequestContext::SendOpenDirReply(ui64 fh) {
    struct fuse_open_out openOut;
    memset(&openOut, 0, sizeof(openOut));
    openOut.fh = fh;
#ifdef _linux_
    openOut.open_flags = FOPEN_KEEP_CACHE | FOPEN_CACHE_DIR;
#endif
    return IFuseRequestContext::SendReply(openOut);
}

EMessageStatus TRequestContext::SendWriteReply(size_t size) {
    struct fuse_write_out writeOut;
    memset(&writeOut, 0, sizeof(writeOut));
    writeOut.size = size;
    return IFuseRequestContext::SendReply(writeOut);
}

EMessageStatus TRequestContext::SendCreateReply(const struct fuse_entry_out& entry, const struct fuse_open_out& open) {
    TBuffer buf;
    buf.Reserve(sizeof(entry) + sizeof(open));
    buf.Append(reinterpret_cast<const char*>(&entry), sizeof(entry));
    buf.Append(reinterpret_cast<const char*>(&open), sizeof(open));
    return SendReply(TStringBuf{buf.Data(), buf.Size()});
}

EMessageStatus TRequestContext::SendGetXattrSizeReply(size_t size) {
    struct fuse_getxattr_out getxattrOut = {};
    getxattrOut.size = size;
    return IFuseRequestContext::SendReply(getxattrOut);
}

} // namespace NFuse
