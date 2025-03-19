LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/yconf
    kernel/common_server/abstract
    kernel/common_server/library/interfaces
)

SRCS(
    abstract.cpp
)

END()
