LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/roles/abstract
    kernel/common_server/util/algorithm
    library/cpp/yconf
)

SRCS(
    GLOBAL configured.cpp
)

END()
