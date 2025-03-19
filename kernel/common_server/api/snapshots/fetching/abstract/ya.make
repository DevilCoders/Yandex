LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/api/history
    kernel/common_server/util
)

SRCS(
    content.cpp
    context.cpp
    snapshot.cpp
)

END()
