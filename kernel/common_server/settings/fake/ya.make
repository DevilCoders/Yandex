LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/settings/abstract
)

SRCS(
    GLOBAL config.cpp
    settings.cpp
)

END()
