#pragma once

#include <util/generic/vector.h>
#include <util/network/socket.h>

namespace NUdp {
    //! Sends UDP packet. Throws if transmission fails.
    /*!
     *  \param address Destination of the packet.
     *  \param data Body of the packet.
     */
    void SendUdpPacket(const TNetworkAddress& address, const TStringBuf data);

}
