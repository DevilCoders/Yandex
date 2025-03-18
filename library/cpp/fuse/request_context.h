#pragma once

#include "dispatcher.h"
#include "fuse_kernel.h"

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/system/spinlock.h>

namespace NFuse {

using TLowMountRef = TIntrusivePtr<class TLowMount>;

class TRequestContext final : public IFuseRequestContext
{
public:
    using TThisRef = TIntrusivePtr<TRequestContext>;

    TRequestContext(TLowMountRef mount, const fuse_in_header& header);
    ~TRequestContext();

    const fuse_in_header& Header() const {
        return Header_;
    }

    TInodeNum INode() const {
        return Header().nodeid;
    }

    NThreading::TDeadlineOwner GetOwner() const override {
        return reinterpret_cast<NThreading::TDeadlineOwner>(Mount_.Get());
    }

    fuse_in_header GetHeader() const override {
        return Header();
    }

    void SendNone() override;

    EMessageStatus SendError(int err) override;

    EMessageStatus SendReply(TStringBuf payload) override;

    EMessageStatus SendOpenReply(ui64 fh) override;
    EMessageStatus SendOpenDirReply(ui64 fh) override;

    EMessageStatus SendWriteReply(size_t size) override;

    EMessageStatus SendCreateReply(const struct fuse_entry_out& entry, const struct fuse_open_out& open) override;

    EMessageStatus SendGetXattrSizeReply(size_t size) override;

    void Timeout() override;

private:
    struct fuse_in_header ExtractHeader();
    bool IsActive();

private:
    TLowMountRef Mount_;
    fuse_in_header Header_;
    TAdaptiveLock Lock_;
    const ui32 Opcode_;
};

using TRequestContextRef = TIntrusivePtr<TRequestContext>;

} // namespace NFuse
