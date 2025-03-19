LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/api/history
    kernel/common_server/library/time_restriction
)

SRCS(
    abstract.cpp
    GLOBAL db.cpp
)

END()
