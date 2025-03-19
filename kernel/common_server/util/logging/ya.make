LIBRARY()

OWNER(g:cs_dev)

SRCS(
    backend_creator_helpers.cpp
    replies.cpp
    trace.cpp
    number_format.cpp
)

PEERDIR(
    library/cpp/logger/global
    library/cpp/logger/init_context
    library/cpp/string_utils/scan
    library/cpp/yconf
    library/cpp/yconf/patcher
)

END()
