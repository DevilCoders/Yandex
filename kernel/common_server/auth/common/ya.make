LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/abstract
    kernel/common_server/common
    kernel/common_server/user_auth/abstract
    library/cpp/logger/global
    library/cpp/mediator/global_notifications
    library/cpp/object_factory
    library/cpp/tvmauth
    library/cpp/yconf
)

SRCS(
    auth.cpp
    tvm_config.cpp
    processor.cpp
    manager.cpp
)

END()
