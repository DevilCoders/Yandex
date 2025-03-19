LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/processors/common
    kernel/common_server/tags
)

SRCS(
    handler.cpp
    GLOBAL description.cpp
    GLOBAL direct.cpp
    GLOBAL object.cpp
    GLOBAL perform.cpp
)

GENERATE_ENUM_SERIALIZATION(description.h)
GENERATE_ENUM_SERIALIZATION(direct.h)

END()
