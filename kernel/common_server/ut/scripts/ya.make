LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/ut
    library/cpp/protobuf/json
    library/cpp/string_utils/base64
)

SRCS(
    abstract.cpp
    default_actions.cpp
)

END()
