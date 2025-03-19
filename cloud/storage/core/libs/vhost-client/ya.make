LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    vhost-user-protocol/message.cpp
    vhost-client.cpp
    vhost-queue.cpp
)

PEERDIR(
    library/cpp/logger
)

END()
