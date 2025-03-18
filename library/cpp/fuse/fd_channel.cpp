#include "fd_channel.h"
#include "fuse_kernel.h"

#include <util/generic/yexception.h>
#include <util/stream/format.h>
#include <util/string/builder.h>
#include <util/system/error.h>

#include <errno.h>
#include <unistd.h>

namespace NFuse {

    TFdChannel::TFdChannel(int fd,
                           TLog log,
                           const TString& name)
        : Fd_(fd)
        , BufSize_(std::max<size_t>(getpagesize() + 0x1000, MIN_BUFSIZE))
        , Log_(log)
        , Name_(name)
    { }

    size_t TFdChannel::GetMaxWrite() const {
        return BufSize_ - 4096;
    }

    EMessageStatus TFdChannel::Receive(TBuffer& buf) {
        buf.Resize(BufSize_);

        auto res = read(Fd_, buf.Data(), buf.Capacity());
        int err = LastSystemError();
        if (res == -1) {
            // Not sure why ENOENT is here, but libfuse retries it
            // EdenFS guys don't understand it either
            if (err == ENOENT || err == EINTR || err == EAGAIN) {
                return EMessageStatus::Again;
            }

            if (err == ENODEV) {
                Log_ << TLOG_INFO << "received ENODEV on channel "
                     << Name_ << Endl;
                return EMessageStatus::Stop;
            }

            TStringBuilder msg;
            msg << "error reading FUSE channel " << Name_ << ": "
                << LastSystemErrorText(err);
            Log_ << TLOG_CRIT << msg << Endl;

            ythrow TSystemError(err) << "read error on fuse channel " << Name_;
        }

        buf.Resize(res);

        if (buf.Size() < sizeof(struct fuse_in_header)) {
            TStringBuf data(buf.Data(), buf.Size());

            TStringBuilder msg;
            msg << "short read on fuse device: " << HexText(data);
            Log_ << TLOG_CRIT << msg << Endl;

            ythrow yexception() << msg;
        }

        return EMessageStatus::Success;
    }

    EMessageStatus TFdChannel::Send(TArrayRef<struct iovec> iov) {
        i64 totalSize = 0;
        for (const auto& buf : iov) {
            totalSize += buf.iov_len;
        }

        auto res = writev(Fd_, iov.data(), iov.size());
        if (res < 0) {
            auto err = LastSystemError();
            if (err == ENOENT) {
                return EMessageStatus::NoEntry;
            } else {
                ythrow TSystemError(err)
                    << "error writing to FUSE channel " << Name_ << ": "
                    << LastSystemErrorText(err);
            }
        } else if (res != totalSize) {
            ythrow yexception() << "short write to FUSE channel "
                                << LabeledOutput(Name_, totalSize, res);
        }

        return EMessageStatus::Success;
    }


} // namespace NFuse
