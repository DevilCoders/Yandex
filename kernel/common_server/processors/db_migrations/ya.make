LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/processors/common
    kernel/common_server/migrations
)

SRCS(
    GLOBAL migrations.cpp
)

END()
