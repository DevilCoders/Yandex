LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/api/history
    kernel/common_server/user_auth/abstract
    library/cpp/xml/document
)

SRCS(
    GLOBAL transparent.cpp
)

END()
