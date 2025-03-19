LIBRARY()

OWNER(iddqd)

PEERDIR(
    kernel/common_proxy/common
    kernel/common_proxy/converters
    kernel/common_proxy/senders
    kernel/common_proxy/sources
    kernel/common_proxy/unistat_signals
    kernel/daemon
)

SRCS(
    server.cpp
    config.cpp
)

END()
