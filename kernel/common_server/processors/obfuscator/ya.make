LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/processors/db_entity
    kernel/common_server/processors/common
)

SRCS(
    GLOBAL handler.cpp
)

END()
