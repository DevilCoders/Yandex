LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/tvmauth/client
    kernel/common_server/auth/common
    kernel/common_server/library/logging
    kernel/common_server/abstract
)

SRCS(
    GLOBAL tvm.cpp
)

END()
