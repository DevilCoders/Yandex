LIBRARY()

OWNER(ivanmorozov)

PEERDIR(
    library/cpp/json/writer
    kernel/common_server/library/logging
    library/cpp/logger/global
)

SRCS(
    scanner.cpp
    value_scanner.cpp
    stack.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
