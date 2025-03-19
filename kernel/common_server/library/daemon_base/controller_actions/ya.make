LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/daemon/async
    kernel/daemon/common
    kernel/daemon/config
    library/cpp/json
    library/cpp/json/writer
    library/cpp/logger/global
    library/cpp/string_utils/base64
    kernel/common_server/library/sharding
)

SRCS(
    async_controller_action.cpp
    GLOBAL restart.cpp
    GLOBAL get_status.cpp
    GLOBAL put_file.cpp
    GLOBAL take_file.cpp
    GLOBAL delete_file.cpp
    GLOBAL shutdown.cpp
    GLOBAL download_configs_from_dm.cpp
)

END()
