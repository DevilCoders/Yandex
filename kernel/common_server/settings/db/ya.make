LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/yconf
    library/cpp/json
    kernel/common_server/api/history
    kernel/common_server/settings/abstract
    kernel/common_server/library/storage
)


SRCS(
    settings.cpp
    GLOBAL config.cpp
)

END()
