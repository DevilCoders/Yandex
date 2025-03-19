LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/settings/abstract
)

SRCS(
    segments.cpp
    GLOBAL config.cpp
)

END()
