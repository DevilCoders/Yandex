#include "raw_channel.h"
#include "fuse_get_version.h"
#include "fuse_mount.h"

#include <util/generic/yexception.h>
#include <util/string/strip.h>
#include <util/system/shellcommand.h>

namespace NFuse {

    TRawChannel::TRawChannel(const TString& mountPath, const TRawChannelOptions& opts, TLog log)
        : MountPath_(mountPath)
        , Fd_(-1)
        , Log_(log)
    {
        Fd_ = ArcFuseMount(mountPath, opts, Log_);
    }

    TRawChannel::~TRawChannel() {
        try {
            Unmount();
        } catch (...) {
            Log_ << TLOG_ERR << CurrentExceptionMessage() << Endl;
        }
    }

    void TRawChannel::Unmount() {
        if (Fd_ >= 0) {
            auto fd = Fd_;
            Fd_ = -1;
            ArcFuseUnmount(MountPath_, fd, Log_);
        }
    }

} // namespace NFuse
