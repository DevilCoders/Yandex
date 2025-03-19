LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/abstract
    kernel/common_server/api/history
)

SRCS(
    cipher.cpp
    object.cpp
    GLOBAL config.cpp
)

END()
