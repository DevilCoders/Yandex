LIBRARY()

OWNER(g:cs_dev)

SRCS(
    config.h
    config.cpp
    server.h
    server.cpp
)

PEERDIR(
    kernel/common_server/server
    kernel/common_server/service_monitor/pods
)

END()
