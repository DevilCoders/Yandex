LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/storage
    kernel/common_server/library/async_impl
)

SRCS(
    GLOBAL storage.cpp
    GLOBAL config.cpp
)

END()
