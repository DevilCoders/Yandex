LIBRARY()

OWNER(ivanmorozov)

PEERDIR(
    kernel/common_server/ut/scripts
    kernel/common_server/library/tvm_services/abstract
)

SRCS(
    check_queue.cpp
)

END()
