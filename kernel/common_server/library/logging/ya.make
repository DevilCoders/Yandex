LIBRARY()

OWNER(g:cs_dev)

GENERATE_ENUM_SERIALIZATION(events.h)

PEERDIR(
    library/cpp/logger
    library/cpp/logger/global
    kernel/common_server/library/logging/record
    kernel/common_server/util/object_counter
)

SRCS(
    events.cpp
    GLOBAL backend.cpp
    accumulator.cpp
)

END()

RECURSE_FOR_TESTS(ut)
