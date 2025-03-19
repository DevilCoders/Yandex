LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/json
    kernel/common_server/util/types
)

SRCS(
    adapters.cpp
    builder.cpp
    cast.cpp
)

END()
