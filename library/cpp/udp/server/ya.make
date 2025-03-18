LIBRARY(udp)

OWNER(
    gritukan
)

SRCS(
    callbacks.cpp
    options.cpp
    packet.cpp
    server.cpp
    worker.cpp
)

PEERDIR(
    library/cpp/threading/future
)

END()

