LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/logger/global
    library/cpp/neh
    kernel/common_server/util
)

SRCS(
    neh_server.cpp
)

END()
