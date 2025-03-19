LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/json
    library/cpp/regex/pcre
    kernel/common_server/library/searchserver/simple/context
    kernel/common_server/library/logging
)

SRCS(
    json.cpp
    proto.cpp
    abstract.cpp
)

END()
