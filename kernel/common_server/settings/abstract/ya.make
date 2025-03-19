LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/storage/records
    kernel/common_server/library/storage/reply
    kernel/common_server/library/scheme
)

SRCS(
    abstract.cpp
    object.cpp
    config.cpp
)

END()
