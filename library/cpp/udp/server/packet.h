#pragma once

#include <util/generic/vector.h>
#include <util/network/sock.h>

namespace NUdp {
    //! Size of the UDP packet. It includes reserved bytes for metadata
    //! so maximum allowed length of the message is less.
    constexpr size_t UdpPacketSize = 65535;

    //! Struct representing UDP packet.
    struct TUdpPacket {
        //! Body of the packet.
        TString Data;

        //! Address of the sender.
        TSockAddrInet6 Address;
    };

}
