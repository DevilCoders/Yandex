LIBRARY()

OWNER(
    ivanmorozov
    g:geosaas
)

PEERDIR(
    kernel/daemon/common
    library/cpp/getopt/small
    library/cpp/getopt
    library/cpp/http/server
    library/cpp/json
    library/cpp/logger/global
    library/cpp/watchdog
    library/cpp/yaml/scheme
    library/cpp/yconf
    library/cpp/yconf/patcher
    search/config/preprocessor
)

SRCS(
    daemon_config.cpp
    options.cpp
    patcher.cpp
    config_constructor.cpp
    its_config.cpp
)

END()
