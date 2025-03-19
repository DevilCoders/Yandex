LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/storage
)

SRCS(
    GLOBAL fake_storage.cpp
    GLOBAL config.cpp
)

END()
