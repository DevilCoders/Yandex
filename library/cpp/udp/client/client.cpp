#include "client.h"

#include <util/network/sock.h>
#include <util/string/printf.h>
#include <util/system/error.h>

namespace NUdp {

    void SendUdpPacket(const TNetworkAddress& address, const TStringBuf data) {
        if (address.Begin() == address.End()) {
            ythrow yexception() << "address should not be empty";
        }

        TInet6DgramSocket socket;
        TSockAddrInet6 addr;
        *(struct sockaddr_in6*)addr.SockAddr() = *(struct sockaddr_in6*)address.Begin()->ai_addr;
        const ssize_t result = socket.SendTo(data.data(), data.size(), &addr);

        if (result < 0) {
            ythrow yexception() << Sprintf("send() failed with %s", LastSystemErrorText());
        }

        const ssize_t bytesSent = result;
        if (bytesSent != static_cast<ssize_t>(data.size())) {
            ythrow yexception() << Sprintf("size of the packet is %zu, but %zd bytes were sent", data.size(), bytesSent);
        }
    }

}
