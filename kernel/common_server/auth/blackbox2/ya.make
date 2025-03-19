LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/auth_client_parser
    library/cpp/http/cookies
    kernel/common_server/auth/common
    kernel/common_server/library/json
    kernel/common_server/library/network/data
    kernel/common_server/library/unistat
    kernel/common_server/auth/blackbox
    kernel/common_server/util/algorithm
)

GENERATE_ENUM_SERIALIZATION(blackbox.h)

SRCS(
    auth.cpp
    GLOBAL blackbox.cpp
)

END()
