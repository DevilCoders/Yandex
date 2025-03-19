LIBRARY()

OWNER(g:cs_dev)

SRCS(
    GLOBAL startrek.cpp
)

PEERDIR(
    kernel/daemon/config
    kernel/common_server/startrek
    kernel/common_server/abstract
    kernel/common_server/library/json
    kernel/common_server/library/tvm_services
    kernel/common_server/notifications/abstract
)

END()
