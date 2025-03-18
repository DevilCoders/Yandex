#include "socket_pair.h"

#include <util/network/pair.h>

std::pair<TSocketHolder, TSocketHolder>
MakeSocketPair() {
    SOCKET sockets[2];
    Y_ENSURE_EX(SocketPair(sockets) == 0,
                TSystemError() << "SocketPair() failed");

    return {sockets[0], sockets[1]};
}
