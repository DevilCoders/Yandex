LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/daemon
    kernel/daemon/common
    kernel/daemon/protos
    library/cpp/balloc/optional
    library/cpp/build_info
    library/cpp/cpuload
    library/cpp/digest/md5
    library/cpp/http/misc
    library/cpp/http/server
    library/cpp/json
    library/cpp/logger/global
    library/cpp/mediator
    library/cpp/mediator/global_notifications
    library/cpp/object_factory
    library/cpp/regex/pcre
    library/cpp/string_utils/base64
    library/cpp/string_utils/tskv_format
    library/cpp/svnversion
    library/cpp/unistat
    library/cpp/yconf/patcher
    kernel/common_server/library/daemon_base/actions_engine
    kernel/common_server/library/daemon_base/actions_engine/get_conf
    kernel/common_server/library/daemon_base/actions_engine/list_conf
    kernel/common_server/library/daemon_base/controller_actions # factories!
    kernel/common_server/library/daemon_base/metrics
    kernel/common_server/library/daemon_base/unistat_signals
    kernel/common_server/library/sharding
    kernel/common_server/util/network
    kernel/common_server/util/system
)

SRCS(
    context.cpp
    controller.cpp
    GLOBAL extended_commands.cpp
)

END()
