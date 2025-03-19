LIBRARY()

OWNER(
    svshevtsov
    g:geosaas
)

PEERDIR(
    kernel/daemon/async
    kernel/daemon/common
    kernel/daemon/config
    kernel/daemon/module
    kernel/daemon/common_modules
    kernel/daemon/protos
    library/cpp/unistat
    library/cpp/balloc/optional
    library/cpp/build_info
    library/cpp/cpuload
    library/cpp/digest/md5
    library/cpp/json
    library/cpp/logger/global
    library/cpp/mediator/global_notifications
    library/cpp/object_factory
    library/cpp/string_utils/tskv_format
    library/cpp/svnversion
    library/cpp/threading/named_lock
    library/cpp/yconf/patcher
)

SRCS(
    GLOBAL controller_commands.cpp
    base_controller.cpp
    base_http_client.cpp
    bomb.cpp
    daemon.cpp
    info.cpp
    messages.cpp
    environment_manager.cpp
    server.cpp
    server_description.cpp
)

END()

RECURSE(
    ut
)
