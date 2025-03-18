#pragma once

#include <util/generic/array_ref.h>
#include <util/generic/buffer.h>
#include <util/generic/ptr.h>

#include <sys/uio.h>

namespace NFuse {

    enum class EMessageStatus {
        // Message was successfully sent or received
        Success,
        // Message was not processed. You should try again later (e.g. EAGAIN)
        Again,
        // Message was sent, but ended with ENOENT or something similar
        NoEntry,
        // Message was not send because of timeout
        Timeout,
        // Message was not processed and channel is stopped (e.g. ENODEV)
        Stop
    };

    class IChannel : public TThrRefBase {
    public:
        virtual size_t GetMaxWrite() const = 0;

        // Reads one FUSE message from channel.
        // Throws yexception on unexpected errors and short read.
        virtual EMessageStatus Receive(TBuffer& buf) = 0;

        // Writes one FUSE message to channel.
        // Throws yexception on unexpected errors and short write.
        virtual EMessageStatus Send(TArrayRef<struct iovec> iov) = 0;
    };

    using IChannelRef = TIntrusivePtr<IChannel>;

} // namespace NFuse
