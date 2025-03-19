LIBRARY()

OWNER(g:cs_dev)

SRCS(
    GLOBAL process.cpp
)

PEERDIR(
    kernel/common_server/solutions/fallback_proxy/src/rt_background/queue_reader
    kernel/common_server/library/tvm_services/abstract/request
    kernel/common_server/rt_background
    kernel/common_server/rt_background/processes/common
)

END()
