LIBRARY()

OWNER(g:cs_dev)

PEERDIR (
    kernel/common_server/library/persistent_queue/abstract
    kernel/common_server/library/persistent_queue/db
    kernel/common_server/library/persistent_queue/kafka
)

SRCS(
    message.cpp
)

PEERDIR(
)

END()
