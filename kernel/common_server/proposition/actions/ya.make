LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/api/history
    kernel/common_server/api/links
    kernel/common_server/library/storage
    kernel/common_server/util
)

SRCS(
    abstract.cpp
    GLOBAL db_entity.cpp
    GLOBAL role.cpp
    GLOBAL execute_query.cpp
    GLOBAL send_request.cpp
    GLOBAL settings.cpp
)

END()
