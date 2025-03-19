LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/blackbox2
    kernel/common_server/library/tvm_services/abstract
    kernel/common_server/util
    kernel/common_server/auth/common
)

SRCS(
    client.cpp
)

END()
