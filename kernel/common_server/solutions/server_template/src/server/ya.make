LIBRARY()

OWNER(ivanmorozov)

PEERDIR(
    kernel/common_server/server
    kernel/common_server/solutions/server_template/src/abstract
    kernel/common_server/solutions/server_template/src/permissions
)

SRCS(
    server.cpp
    config.cpp
)

END()
