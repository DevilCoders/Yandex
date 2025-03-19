LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    contrib/libs/jwt-cpp
    kernel/common_server/abstract
)

SRCS(
    GLOBAL cipher.cpp
)

END()
