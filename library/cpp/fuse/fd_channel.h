#pragma once

#include "channel.h"

#include <library/cpp/logger/log.h>

namespace NFuse {
    class TFdChannel : public IChannel {
    public:
        static constexpr size_t MIN_BUFSIZE = 0x21000;

        TFdChannel(int fd,
                   TLog log = TLog(),
                   const TString& name = TString());

        size_t GetMaxWrite() const override;
        EMessageStatus Receive(TBuffer& buf) override;
        EMessageStatus Send(TArrayRef<struct iovec> iov) override;

    private:
        const int Fd_;
        const size_t BufSize_;
        TLog Log_;
        const TString Name_;
    };

}
