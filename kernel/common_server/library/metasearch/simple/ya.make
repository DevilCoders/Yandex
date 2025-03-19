LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/daemon/config
    library/cpp/cgiparam
    library/cpp/mediator/global_notifications
    library/cpp/yconf
    search/meta/scatter
    search/meta/scatter/options
    search/meta/waitinfo
)

SRCS(
    config.cpp
    policy.cpp
)

END()
