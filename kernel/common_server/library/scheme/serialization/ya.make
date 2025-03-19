LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/util/types
    contrib/libs/protobuf
)

SRCS(
    abstract.cpp
    default.cpp
    opeanapi.cpp
)

END()
