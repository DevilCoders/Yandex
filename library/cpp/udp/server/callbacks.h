#pragma once

#include "packet.h"

#include <functional>

namespace NUdp {
    //! Callback for UDP packet handling.
    using TCallback = std::function<void(TUdpPacket)>;

    //! Callback for server errors. Argument is a string representation of an error.
    using TErrorCallback = std::function<void(TString)>;

    //! Callback for queue overflows. Argument is a packet which was not put to the
    //! requests queue because of queue overflow.
    using TQueueOverflowCallback = std::function<void(TUdpPacket)>;

}
