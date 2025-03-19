LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/api/history
    kernel/common_server/util
    library/cpp/deprecated/atomic
)

SRCS(
    storage.cpp
    index.cpp
    structure.cpp
)

END()
