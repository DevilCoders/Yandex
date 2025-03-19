LIBRARY()

OWNER(g:cs_dev)

SRCS(
    GLOBAL sms.cpp
    request.cpp
)

PEERDIR(
    kernel/common_server/notifications/abstract
    kernel/common_server/library/tvm_services/abstract
    kernel/common_server/util
)

END()
