LIBRARY()

OWNER(g:cs_dev)

SRCS(
    pq.cpp
    config.cpp
    request.cpp
)

PEERDIR(
    library/cpp/logger/global
    library/cpp/mediator/global_notifications
    library/cpp/object_factory
    library/cpp/tvmauth/client
    library/cpp/yconf
    kernel/common_server/util
    kernel/common_server/library/tvm_services/abstract
)

END()
